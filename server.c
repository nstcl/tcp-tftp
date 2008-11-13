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
typedef enum _Status{IDLE,TRANSFERING,FAIL,DONE}Status;
typedef struct _s_jobDescriptor
{
    char cp_client[16];//max string is '111.111.111.111\0'
    char cp_fileName[MAX_STRING+1];
    Status status;
    unsigned int ui_packets;
    time_t time;

}s_jobDescriptor;
/* job queue */
typedef struct _s_jobQ
{
    s_jobDescriptor jobQ[MAX_JOBS];
    int head,tail;
}s_jobQ;

/////////////////// globals /////////////////////////
char gScwd[NUM_OF_SLAVES][MAX_STRING+1] = {{"/tmp"},{"/tmp"}};
FILE *logFile = NULL;
ushort us_port = DEFAULT_SERVER_PORT;
s_jobQ jobQ;
int level = LOGLEVEL;
/////////////////////////function prototypes ////////////////////
/* bind to listening socket */
int initNetwork();

/* open log file */
int initLog(int l);

/* prepare worker threads */
int initQ();

// init server
int initServer();

// start to receive connections
int start();

// stop listening to connections
int stop();

//general log facility
void dlog(int lvl, const char *msg,int jobIndex);

// free all allocated resources
void finalize();
/////////////////// funcion implementation /////////////////////////

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
        switch(lvl)
        {
            case DEBUG: strcpy(logbuf, "DEBUG");break;
            case INFO: strcpy(logbuf, "INFO");break;
            case WARNING: strcpy(logbuf, "WARNING");break;
            case CRITICAL: printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);strcpy(logbuf, "CRITICAL");break;
            default: strcpy(logbuf, "UNKNOWN");break;
        }
        printf("%s: [%s] %s\n",timebuf, logbuf ,msg);
        if (logFile)
        {
            fprintf(logFile, "%s: [%s] %s\n",timebuf, logbuf ,msg);
        }
        if (jobIndex>=0)
        {
            printf("job print not implemented yet...\n");
            if (logFile) fprintf(logFile,"job print not implemented yet...\n");
        }

    }

}

// free all allocated resources
// 1. close log file
void finalize()
{
    if(logFile)
    {
        fclose(logFile);
    }
}

// entry point
int main(int argc, char *argv[])
{
  printf("Intializing Server!\n");
  initLog(DEBUG);
  dlog(DEBUG,"testing dlog...",-1);

  finalize();
  return EXIT_SUCCESS;
}
#endif


