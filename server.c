/***************************************************************************
 *   Copyright (C) 2008 by Koby Hershkovitz,04914465,khershko (alpha)      *
 *   khershko@localhost.localdomain                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
// this file is the client side of tcp-tftp application
#ifdef SERVER_BUILD
#include "tftp.h"
#include <pthread.h>
////////////////// defines ////////////////
#define MAX_STRING 255
#define BACKLOG 5
#define DEFAULT_SERVER_PORT 5069
#define NUM_OF_SLAVES 2
#define LOGFILE "./server.log"
#define MAX_JOBS BACKLOG
#define LOGLEVEL DEBUG
////////////////////// data structures ///////////////////////
/* job definition */
typedef enum _Status {IDLE, ASSIGNED, TRANSFERING, FAIL, ABORT, DONE}Status;
typedef struct _s_jobDescriptor
{
    char cp_client[16];//max string is '111.111.111.111\0'
    char cp_fileName[MAX_STRING+1];
    Status status; // transaction status
    unsigned int ui_packets; // number of packets transfered
    double d_time; // transfer time
    int i_socketId; // remote socket id
    struct sockaddr_in sa_address; // remote ip address
    socklen_t i_socketSize; // auxiliry variable to hold the size of the sockaddr structure
    int i_slave;
}s_jobDescriptor;
/* job queue */
typedef struct _s_jobQ
{
    s_jobDescriptor Q[MAX_JOBS];
    int input,output;
}s_jobQ;

/////////////////// globals /////////////////////////
volatile char gScwd[NUM_OF_SLAVES][MAX_STRING+1] = {{"/tmp"},{"/tmp"}};
pthread_mutex_t gScwd_lock;
FILE *logFile = NULL;
ushort us_port = DEFAULT_SERVER_PORT;
volatile s_jobQ jobQ; // job queue structure
pthread_mutex_t jobQ_lock;
int level = LOGLEVEL;
char statusStringMap[6][16]={{"IDLE"}, {"ASSIGNED"}, {"TRANSFERING"}, {"FAIL"}, {"ABORT"}, {"DONE"}};
pthread_t workerThreads[NUM_OF_SLAVES];
int *workerIds;
void *retval; // return code for worker threads
struct sigaction osa; // structure to help handling proper termination when CTRL+C is pressed
volatile int kill_thread_flag = 1;
/////////////////////////function prototypes ////////////////////
/*
 * 1. create and bind to listening socket
 * 2. start listening for incoming connections
 */
int initNetwork(int *listenSocket, struct sockaddr_in *localAddress);

/* open log file */
int initLog(int l);

/*
    1. prepare worker structures
    2. initialize worker threads and locks
 */
int initQ(volatile s_jobQ *jobQ);

// init server
int initServer();

//general log facility
void dlog(int lvl, const char *msg,int jobIndex);

// worker function to handle a transfer session
void *worker(void *id);

// free all allocated resources
void finalize(int sig_no);
/////////////////// funcion implementation /////////////////////////
/*
 initialize log facility
 - l - log level
 */
int initLog(int l)
{
    int err;
    char buf[160];
    level = l;
    if (logFile) fclose(logFile);
    if ((logFile = fopen(LOGFILE,"a")) == NULL)
    {
        err = errno;
        sprintf(buf, "couldn't open %s file(%d), no log will be written to file", LOGFILE, err);
        dlog(WARNING, buf, -1);
        return WARNING;
    }
    return SUCCESS;
}

// initialize the job queue
int initQ(volatile s_jobQ *jq)
{
    /*Q[MAX_JOBS];
    int head,tail;*/
    int i;
    jq->output = -1;
    jq->input = 0;
    for (i = 0 ; i < MAX_JOBS ;i++)
    {
        sprintf((char *)(jq->Q[i].cp_fileName), "%s","##EMPTY_FILE##") ;
        sprintf((char *)(jq->Q[i].cp_client), "%s","0.0.0.0") ;
        jq->Q[i].status = IDLE;
        jq->Q[i].ui_packets = 0;
        jq->Q[i].d_time = 0;
        jq->Q[i].i_socketId = 0;
        jq->Q[i].i_socketSize = 0;
        jq->Q[i].i_slave = -1;

    }

    /*
     *  Initialize workers:
     *  1. initialize locks on shared data
     *  2. create worker threads
     */
    pthread_mutex_init(&gScwd_lock, NULL);
    pthread_mutex_init(&jobQ_lock, NULL);
    workerIds = malloc(NUM_OF_SLAVES*sizeof(int));
    for (i = 0 ; i < NUM_OF_SLAVES ;i++)
    {
        workerIds[i] = i;
        if (pthread_create(workerThreads+i, NULL, worker, (void *)(workerIds+i)))
        {
            dlog(CRITICAL, "cannot create slave/s exiting...", -1);
            exit(1);
        }

    }
    return SUCCESS;

}

/* worker function to handle a transfer session: if the server is started then every second
 * the worker shall poll the job queue for a job. if a job is present then the worker shall serve it
 * and shall update the proper queue pointers and job descriptor values.
 * input: id(integer) - worker id
 * output: result code
 */
 ///TODO when transfer ends initialize job descriptor for reuse
void *worker(void *id)
{

    int wid = *((int *)id);
    char msg[32];
    sigset_t empty_set;
    sigemptyset(&empty_set); // create an empty signal mask to reject all signals
    sigprocmask(SIG_BLOCK, &empty_set, NULL); // apply empty set to thread in order to ignore any signal.
    sprintf(msg,"slave number %d started\n",wid);
    dlog(DEBUG,msg,-1);

    while(kill_thread_flag)
    {
        //printf("%d",wid);
        int currentJob = -10, assignedSlave = -10;
        Status status = IDLE;

        pthread_mutex_lock(&jobQ_lock); // protect queue pointer
        if(jobQ.output == MAX_JOBS-1) jobQ.output = -1; // cycle queue output index
        if((jobQ.Q[jobQ.output+1].status == ASSIGNED) && (jobQ.Q[jobQ.output+1].i_socketId != 0))
        {
            printf("***************** thread(%d) input:%d \t output:%d \tsid=%d \tstatus=%s ******\n",wid,jobQ.input,jobQ.output,jobQ.Q[jobQ.output+1].i_socketId, statusStringMap[jobQ.Q[jobQ.output+1].status]);
            jobQ.Q[++(jobQ.output)].status = TRANSFERING;
            jobQ.Q[jobQ.output].i_slave = wid;
        }
        //also save the status in a non volatile var
        if(jobQ.Q[jobQ.output].status==TRANSFERING)
        {
            currentJob = jobQ.output;
            status = TRANSFERING;
        }
        assignedSlave = jobQ.Q[jobQ.output].i_slave;
        pthread_mutex_unlock(&jobQ_lock); // release queue for other thread to change




        /*
         * handle job
         */

        if((status == TRANSFERING) && (assignedSlave == wid))
        {

            char *txBuf,*rxBuf;
            int rxCount,txCount;




            if(((txBuf=(char *)calloc(BUFFER_SIZE, sizeof(char)))==NULL) || ((rxBuf=(char *)calloc(BUFFER_SIZE,sizeof(char)))==NULL))
            {
                dlog(CRITICAL, "couldn't allocate memory for rx and/or tx buffers", -1);
                exit(1);
            }
            //int recv(int sockfd, void *buf, int len, unsigned int flags);
            //int send(int sockfd, const void *msg, int len, int flags);
            rxCount = recv(jobQ.Q[currentJob].i_socketId, rxBuf, BUFFER_SIZE, 0);
            if((!rxCount) || (rxCount==-1)) // connection was lost: socket is closed and job is initialized
            {
                if(!rxCount)
                {
                    jobQ.Q[currentJob].status = ABORT;
                    dlog(INFO, "connection was lost, no data received", currentJob);
                }
                else
                {
                    jobQ.Q[currentJob].status = ERROR;
                    dlog(WARNING, "no data received(an error occurred). connection will be closed", currentJob);
                }
                jobQ.Q[currentJob].status = IDLE;
                close(jobQ.Q[currentJob].i_socketId);
                jobQ.Q[currentJob].i_socketId = 0;
                fflush(stdout);
                continue;
            }

            ///TODO: here is where the TFTP state machine should be added
            printf("Received from %s:%s\n", jobQ.Q[currentJob].cp_client, rxBuf);
            fflush(stdout);
            txCount = send(jobQ.Q[currentJob].i_socketId, rxBuf,rxCount , 0); //echo buffer
            //if(txCount==-1)

            /// TODO: make the following line a function that will empty job descriptor for new connection
            //close(jobQ.Q[currentJob].i_socketId);
            //jobQ.Q[currentJob].status = IDLE; //finished handling job,

            free(txBuf);
            free(rxBuf);
        }
        fflush(stdout);
        if(logFile) fflush(logFile);
        sleep(1);
    }


    return id;
}

/*
 * general log function
 * lvl: log level
 * msg: message string
 * jobIndex: job index to print, if smaller then 0 then no job will be printed
 */
void dlog(int lvl, const char *msg, int jobIndex)
{
    char logbuf[16];
    char timebuf[80];
    time_t ctime;
    struct tm *ts;
    if (lvl >= level)
    {
        ctime = time(NULL);
        ts = localtime(&ctime);
        asctime_r(ts, timebuf);
        timebuf[strlen(timebuf)-1] = '\0'; // remove new line char from end of time string
        switch (lvl)
        {
        case DEBUG:
            strcpy(logbuf, "DEBUG");
            break;
        case INFO:
            strcpy(logbuf, "INFO");
            break;
        case WARNING:
            strcpy(logbuf, "WARNING");
            break;
        case CRITICAL:
            printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
            strcpy(logbuf, "CRITICAL");
            break;
        default:
            strcpy(logbuf, "UNKNOWN");
            break;
        }
        printf("%s: [%s] %s\n",timebuf, logbuf ,msg);
        if (logFile)
        {
            fprintf(logFile, "%s: [%s] %s\n",timebuf, logbuf ,msg);
        }
        if (jobIndex>=0)
        {
            printf("Transfer Report:\n");
            printf("================\n");
            printf("\tClient IP:%s\n\tFile name:%s\n\tEnd Status:%s\n\tPackets:%d\n\tTime(Sec.):%.2f\n",
                   jobQ.Q[jobIndex].cp_client,
                   jobQ.Q[jobIndex].cp_fileName,
                   statusStringMap[jobQ.Q[jobIndex].status],
                   jobQ.Q[jobIndex].ui_packets,
                   jobQ.Q[jobIndex].d_time);

            if (logFile)
            {
                fprintf(logFile,"Transfer Report:\n");
                fprintf(logFile,"================\n");
                fprintf(logFile, "\tClient IP:%s\n\tFile name:%s\n\tEnd Status:%s\n\tPackets:%d\n\tTime(Sec.):%.2f\n",
                        jobQ.Q[jobIndex].cp_client,
                        jobQ.Q[jobIndex].cp_fileName,
                        statusStringMap[jobQ.Q[jobIndex].status],
                        jobQ.Q[jobIndex].ui_packets,
                        jobQ.Q[jobIndex].d_time);
            }
        }

    }

}

/*
 * 1. create and bind to listening socket
 * 2. start listening for incoming connections
 */
int initNetwork(int *listenSocket, struct sockaddr_in *localAddress)
{
    char *msg;
    static int yes = 1; // socket option value: SO_REUSEADDR = yes
    if ((*listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        dlog(CRITICAL, "Cannot Create Server Socket. Exiting ....", -1);
        exit(1);
    }
    if (setsockopt(*listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {

        dlog(CRITICAL, "Cannot Set Server Socket Options. Exiting ....", -1);
        exit(1);
    }

    localAddress->sin_family = AF_INET;// host byte order
    localAddress->sin_port = htons(us_port);// short, network byte order
    localAddress->sin_addr.s_addr = INADDR_ANY;// automatically fill with my IP
    msg = malloc(800);
    sprintf(msg,"Server socket (%s:%d) created and configured to be reused.\n(0.0.0.0 means all interfaces)\n",inet_ntoa(localAddress->sin_addr), us_port);
    dlog(DEBUG, msg,-1);
    free(msg);
    /* zero the rest of the struct */
    memset(&(localAddress->sin_zero), '\0', 8);

    /* bind to socket */
    if(bind(*listenSocket, (struct sockaddr *)localAddress, sizeof(struct sockaddr)) == -1)
    {
        dlog(CRITICAL, "Server-bind() error", -1);
        exit(1);
    }
    else
        dlog(DEBUG, "Server-bind() is OK...\n", -1);

    if(listen(*listenSocket, BACKLOG) == -1)
    {
        dlog(CRITICAL, "Server-listen() error", -1);
        exit(1);
    }
    dlog(INFO, "Server Listening...\n", -1);

    return SUCCESS;
}

// free all allocated resources
// 1. close log file
void finalize(int sig_no)
{
    int i;
    char msg[80]; // general message buffer
    // close log file

    kill_thread_flag = 0; //signal thread to stop processing loop

    // close all ope connections
    pthread_mutex_lock(&jobQ_lock);
    for(i=0;i<MAX_JOBS;i++)
    {
       if(jobQ.Q[i].i_socketId)  close(jobQ.Q[i].i_socketId);
    }
    pthread_mutex_unlock(&jobQ_lock);

    // collapse worker threads
    for(i = 0 ; i < NUM_OF_SLAVES ; i++)
    {
        if (pthread_join(workerThreads[i], &retval))
            {
                dlog(DEBUG, "slave join failed! Exiting ...\n", -1);
                exit(1);
            }
            else
            {
                sprintf(msg,"slave %d terminated.\n",*(int *)retval);
                dlog(DEBUG, msg, -1);
            }
    }
    free(workerIds);
    if (logFile)
    {
        fclose(logFile);
    }
    sigaction(SIGINT,&osa,NULL); // reset sigaction handler to default
    kill(0,SIGINT); //kill main process with SIGINT signal
}

// entry point
int main(int argc, char *argv[])
{
    // server listening socket information
    int listenSocket;
    struct sockaddr_in localAddress;
    char msg[400]; //save 5 lines worth of general message buffer
    struct sigaction sa,osa;  // structures to handle CTRL+C (SIGINT)
    //////////////////////

     // hook finilize() function to the SIGINT event that is triggered by CTRL+C
     memset(&sa, 0, sizeof(sa));
     sa.sa_handler = &finalize;
     sigaction(SIGINT, &sa,&osa);//save the default sigaction structure

    /* check to see if a valid port was entered */
    if(argc == 2)
        {
            if((us_port = atoi(argv[1])) <= 0)
            {
                printf("Valid ports are an integer in the range of 1-65535\n");
                exit(1);
            }
        }
    else
        if(argc != 1)
        {
            printf("Usage: %s <port number> (Default port is at %d)\n",argv[0], DEFAULT_SERVER_PORT);
            exit(1);
        }


    printf("Intializing Server!\n");
    initLog(DEBUG);
    initQ(&jobQ);
    initNetwork(&listenSocket, &localAddress);
    dlog(DEBUG,"testing dlog...",0);




    /* accept() loop */
    while(1)
    {
        //  only accept connection if queue is not full
        if(((jobQ.input+1 != jobQ.output) && (jobQ.input-jobQ.output != (MAX_JOBS-1))) && (jobQ.Q[jobQ.input].status == IDLE))
        {
            jobQ.Q[jobQ.input].i_socketSize = sizeof(struct sockaddr_in);
            // accept incoming connection
            if((jobQ.Q[jobQ.input].i_socketId = accept(listenSocket, (struct sockaddr *)&(jobQ.Q[jobQ.input].sa_address), (socklen_t *)&(jobQ.Q[jobQ.input].i_socketSize))) == -1)
            {
                dlog(WARNING, "Server-accept() error", -1);
            }
            else
            {
                char *tmp = inet_ntoa(jobQ.Q[jobQ.input].sa_address.sin_addr);
                strncpy((char *)(jobQ.Q[jobQ.input].cp_client) , tmp, strlen(tmp));
                sprintf(msg,"Server: Got connection from %s (socket_id=%d)\n", tmp,jobQ.Q[jobQ.input].i_socketId);
                dlog(DEBUG,msg,-1);
                pthread_mutex_lock(&jobQ_lock);
                    if(jobQ.input == MAX_JOBS) jobQ.input = 0; // cycle queue
                    jobQ.Q[jobQ.input].status =  ASSIGNED; // current job as assigned and move pointer to the next position
                    jobQ.input++;
                pthread_mutex_unlock(&jobQ_lock);

            }
        }
    }





    //finalize();

    return EXIT_SUCCESS;
}
#endif


