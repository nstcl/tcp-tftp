/***************************************************************************
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
#ifdef CLIENT_BUILD

#include "tftp.h"

/**
* CLIENT PROTOTYPES
**/
int changeDirectory(const char *path);
void split(char *s,char* delimiter,char*** parameters);
void mPutFiles(char** parameters);
void mGetFiles(char** parameters);
int putFile(char* strParam,int* totalBytes,char** errorMsg);
int getFile(char* strParam,int* totalBytes,char** errorMsg);
void displayError(char* command,char* errMsg);
void connectToServer(char* destIP,int  port,int displayMsg);
void getCommandAndParam(char *userCmd,char *command,char* strParam);
void trim(char *s);
void displaySystemErr(char* command,int _errno);
void getSystemErr(int _errno,char** errMsg);
void sendAck(short num);
void sendRequest(int type,char* fileName);
int getServerFileList(char** errorMsg);//-1 error 0 -success
void sendDataFile(char* data,int blockNumber,int numOfByteToSend);


int sockfd;//socket num
int connectedToServer=-1;//-1 undefine , 0-NOT connected ,  1-connected
char* port;
char* ip;

int main(int  argc, char** argv) {
 

 if (argc!=3)//check number of  parameters
 {
     printf("This application must recive  2 parameters [serverip] [port]\n");
     printf("For example client 127.0.0.1 69\n");
     printf("try again...\n");
     exit(1);//wrong number parameters
 }
  
   ip=strdup(argv[1]);//get ip paramter
   port=strdup(argv[2]);//get serevr port parameter
   
   connectToServer(ip,atoi(port),1);//connect to server 0-do'nt display message connecnt ;1-display message connect
   
   char userCmd[MAX_CMD];//this buffer get the command that the user enter 
   char command[MAX_CMD];//this buffer contain only the command WITHOUT the parameters
   char strParam[MAX_CMD];//this buffer contain ONLY the parameters
   char **parameters=NULL;//array of pointers.each pointer point to 1 parameter
   int lenCmd=0;//command length
   int lenParams=0;//parameter length
   while(1)
   {
    printf("tftp>");//display prompt
    gets(userCmd);//read command
    trim(userCmd);
    getCommandAndParam(userCmd,command,strParam);//command<-get the command ; strParam<-get command parameter
    lenCmd=strlen(command);
    lenParams=strlen(strParam);

if(lenCmd==0)// no command
     continue;
//CLOSE command 
   if(strcmp(command,"CLOSE")==0 && lenParams==0)
  {      
      puts("CLOSE:started...");
      close(sockfd);
      puts("CLOSE:completed");
      exit(1);
   }
   
//CmyD command 
   if(strcmp(command,"CmyD")==0 && lenParams>0)
 {      
         puts("CmyD:started...");
         if(changeDirectory(strParam)==0)
         puts("CmyD:completed");
         continue;      
 }
 
//CD command     
   if(strcmp(command,"CD")==0 && lenParams>0)
  {      
          char* errorMsg;
          puts("CD:started...");
          if(changeServerDir(strParam,&errorMsg)==-1)//error        
          {
              displayError("CD",errorMsg);
              free(errorMsg);
          }
          else//cd success
          {
              puts("CD:completed");         
          }
          continue;      
  }
//GET command    
  if(strcmp(command,"GET")==0 && lenParams>0)
  {      
          int totalBytes;
          char* errorMsg;
          puts("GET:started...");
          if(getFile(strParam,&totalBytes,&errorMsg)==-1)//error        
          {
              displayError("GET",errorMsg);
              free(errorMsg);
          }
          else//put success
          {
              puts("GET:completed");
              printf("Total number of byte transfered:%d\n",totalBytes);  
          }
          continue;      
  }
//MGET command 
  if(strcmp(command,"MGET")==0 && lenParams>0)
  {      
          int i=0;
          puts("MGET:started...");
          if(getParameters(strParam,&parameters)==-1)//format error (parameters)
          {displayError("MGET","parameters format error");free(parameters);continue;}              
           mGetFiles(parameters);  
           continue;
  }
   
//PUT command    
  if(strcmp(command,"PUT")==0 && lenParams>0)
  {      
          int totalBytes;
          char* errorMsg;
          puts("PUT:started...");
          if(putFile(strParam,&totalBytes,&errorMsg)==-1)//error        
          {
              displayError("PUT",errorMsg);
              free(errorMsg);
          }
          else//put success
          {
              puts("PUT:completed");
              printf("Total number of byte transfered:%d\n",totalBytes);  
          }
          continue;      
  }
//MPUT command    
if(strcmp(command,"MPUT")==0 && lenParams>0)
  {      
          int i=0;
          puts("MPUT:started...");
          if(getParameters(strParam,&parameters)==-1)//format error (parameters)
          {displayError("MPUT","parameters format error");free(parameters);continue;}              
           mPutFiles(parameters);  
           continue;
  }
//LIST command    
   if(strcmp(command,"LIST")==0 && lenParams==0)
  {      
          int totalBytes;
          char* errorMsg;
          puts("LIST:started...");
          if(getServerFileList(&errorMsg)==-1)//error        
          {
              displayError("LIST",errorMsg);
              free(errorMsg);
          }
          else//get success
          {
              puts("LIST:completed");
              //printf("Total number of byte transfered:%d\n",totalBytes);  
          }
          continue;      
  }
   
//INVALID command     
    printf("invalid command\n");
    continue;      
 
   }
 
  return (EXIT_SUCCESS);
  }

/*Change server directory to dir.
On error return error message in errorMsg variable*/

int changeServerDir(char* dir,char** errorMsg)
{
    char*  buffRecv;
    short opcode;
    int numbytes;
    
    if(connectedToServer==0)//connect to server again if connect lost
            connectToServer(ip,atoi(port),0);
    
    sendRequest(CD,dir);//send request to change directory opcode=CD=0x7
    buffRecv=(char*) malloc((516)*sizeof(char));//create buffer for recv data(ack or error)
       if (buffRecv==NULL){displayError("CD","Error mallocating memory"); exit (1); }    
    if((numbytes = recv(sockfd,buffRecv, 516, 0)) == -1)//recv (ack or error)
		{displayError("CD","error recv function");free(buffRecv);exit(1);}
    if(numbytes<4)//if getting wrong header
		{displayError("CD","error header");free(buffRecv);exit(1);}      
     memcpy(&opcode,buffRecv,2);//get opcode
     free(buffRecv);//free buffer
     opcode=ntohs(opcode);//change from network to host
      switch(opcode)
     {
        case ACK:
              return 0;//success
        case ERROR:                    
              (*errorMsg)=strdup(buffRecv+4);//assign error message 
              connectedToServer=0;
			  close(sockfd);//close connection
              return -1;
               
     default :            
             (*errorMsg)=strdup("unknow error");
             return -1;
 }              
}

/*Display server file(s) on his CWD
return -1 on error and assign error message to errorMsg
retrun 0 on success
*/
int getServerFileList(char** errorMsg)//return-1 error 0 -success
{
    char *buffRecv;//buffer for recv data
    char *list;//buffer for server file list
    short opcode;
    short blockNumber;
    char MODE[]= {"octet"};   
    int numbytes;//total byte recived  
          
    if(connectedToServer==0)//connect to server again if connect lost
            connectToServer(ip,atoi(port),0);
    
    buffRecv=(char*) malloc((516)*sizeof(char));//create buffer  header
       if (buffRecv==NULL){displayError("LIST","Error mallocating memory"); exit (1); }    
    sendRequest(LIST,"");//send LIST request opcode=0x6
 
    do
   {
     if((numbytes = recv(sockfd,buffRecv, 516, 0)) == -1)
        {displayError("LIST","error recv function");free(buffRecv);exit(1);}
     if(numbytes<4)//header error 
		{displayError("LIST","error header");free(buffRecv);exit(1);}      
      memcpy(&opcode,buffRecv,2);
      opcode=ntohs(opcode);
      memcpy(&blockNumber,buffRecv+2,2);  
      blockNumber=ntohs(blockNumber);
 switch(opcode)
 {
        case DATA://print list files recv
            list=(char*)malloc((numbytes-4)*sizeof(char));
            if(list==NULL){displayError("LIST","error malloc buffer for list file");free(buffRecv);exit(1);}      
            memcpy(list,buffRecv+4,numbytes-4);  
            printf("%s",list);
			free(list);
            sendAck(blockNumber);//send ack for list files recv          
            break;
        case ERROR:                    
              (*errorMsg)=strdup(buffRecv+4);
              connectedToServer=0; 
			  close(sockfd);//close connection
              return -1;
               
     default :            
             (*errorMsg)=strdup("unknow error");
             return -1;
 }              

    } while(numbytes==516);//until block size == 516     
    free(buffRecv);//free buffer 
    return 0;
}    


/* input fileName to send 
   output totalBytes=number of bytes send,
   errorMsg=error description if occur
   return -1 on error;0 on success
*/

int putFile(char* fileName,int* totalBytes,char** errorMsg)//-1 error 0 -success
{
    
    char *buffRecv;
    short opcode;
    short blockNumber=0;
    char MODE[]= {"octet"};   
    int numbytes;//total byte recived  
    int bytesWrite;
    int ackNum=0;
    (*totalBytes)=0;    
    int lastBlock=0;//last black to send
    int byteReaded=0;
    char fileBuffer[512];
     
    if(connectedToServer==0)
         connectToServer(ip,atoi(port),0);

    //open a file for write;
    FILE * pFile;    
    pFile = fopen (fileName,"r");
    if(!pFile){getSystemErr(errno,(errorMsg)) ;return -1;}//if can not open file 
   
    buffRecv=(char*) malloc((516)*sizeof(char));//create buffer  header
       if (buffRecv==NULL){displayError("PUT","Error mallocating memory"); exit (1); }    
     
    sendRequest(WRQ,fileName);//send  PUT request opcode=0x2 
 
    do
   {
    if((numbytes = recv(sockfd,buffRecv, 516, 0)) == -1){displayError("PUT","error recv function");free(buffRecv);exit(1);} //recv ack or error
    if(numbytes<4){displayError("PUT","error header");free(buffRecv);exit(1);}      
    
    memcpy(&opcode,buffRecv,2);
    opcode=ntohs(opcode);
    memcpy(&blockNumber,buffRecv+2,2);  
    blockNumber=ntohs(blockNumber);   
    //printf("ACK on BLOCK#%d\n",blockNumber);
    switch(opcode)
   {
        case ACK:
              
              if(blockNumber!=(ackNum)){displayError("PUT","error ack number");free(buffRecv);exit(1);}                  
              if (lastBlock==1)//get ack for last block
                   break;//exit switch;
              else//send next chunk
              {                  
                  byteReaded = fread (fileBuffer,1,512,pFile);//read max 512 bytes
                  if(byteReaded==-1){displayError("PUT","error read file");free(buffRecv);exit(1);}                  
                  sendDataFile(fileBuffer,(++ackNum),byteReaded);
                  if(byteReaded<512)//last chunk
                  {  
                      lastBlock=1;                    
                  }
                      (*totalBytes)+=byteReaded;//number of byte send;
              }                                 
              break;
        case ERROR:                    
               (*errorMsg)=strdup(buffRecv+4);
               close(sockfd);//close connection
			   return -1;
               break;
     default :            
             (*errorMsg)=strdup("unknow error");
             return -1;
             break;
 }              

    } while(lastBlock != 1);
    if((numbytes = recv(sockfd,buffRecv, 516, 0)) == -1){displayError("PUT","error recv function");free(buffRecv);exit(1);} //recv ack or error
    
    free(buffRecv);
    close((int)pFile);
    return 0;
     
}    

/*this function send block of data to server */ 

void sendDataFile(char* data,int blockNumber,int numOfByteToSend)
{
     char* buffSend;                  
     int lenSendData;
     short opcode;     
     opcode=htons(DATA);//change from host to network
     blockNumber=htons(blockNumber);//change from host to network
     lenSendData=4+numOfByteToSend;
     
     buffSend=(char*)malloc(lenSendData*sizeof(char));
     if(buffSend==NULL)  { displayError("PUT","Error mallocating memory"); exit (1); }
     memcpy(buffSend,&opcode,2);
     memcpy(buffSend+2,&blockNumber,2);
     memcpy(buffSend+4,data,numOfByteToSend);                  
     if((send(sockfd,buffSend,lenSendData*sizeof(char),0))==-1)//send request put file
             {displayError("PUT","error send data");free(buffSend);exit(1);}                                        
      free(buffSend);           
}


/*this function call in loop to putFile function(PUT request) 
get many files seperated by char (,)*/
void mPutFiles(char** parameters)
{
          int i;
          int totalBytes;
          int totalFileOk=0;
          int totalFileError=0;
          char* errorMsg;
          for(i=0;(parameters[i])!=NULL;i++)
         {  
          printf("PUT:started...(%s)\n",parameters[i]);
          totalBytes=0;
          if(putFile((parameters[i]),&totalBytes,&errorMsg)==-1)//error get file        
          {
              totalFileError++;//count number of failed send file
              displayError("PUT",errorMsg);
              free(errorMsg);
          }
          else//get success
          {
              printf("PUT:completed(%s)\n",parameters[i]);
              printf("Total number of byte transfered:%d\n",totalBytes);  
              totalFileOk++;//count number of success send file
          }        
         
      }  
          printf("MPUT:completed (%d file(s) transfer OK ,%d file(s) FAILED)\n",totalFileOk,totalFileError);
}


/*this function call in loop to getFile function(GET request) 
get many files seperated by char (,)*/

void mGetFiles(char** parameters)
{
          int i;
          int totalBytes;
          int totalFileOk=0;
          int totalFileError=0;
          char* errorMsg;
          for(i=0;parameters[i]!='\0';i++)
         {  
          printf("GET:started...(%s)\n",parameters[i]);
          totalBytes=0;
          if(getFile((parameters[i]),&totalBytes,&errorMsg)==-1)//error get file        
          {
              totalFileError++;//count number of failed recv file
              displayError("GET",errorMsg);
              free(errorMsg);
          }
          else//get success
          {
              printf("GET:completed(%s)\n",parameters[i]);
              printf("Total number of byte transfered:%d\n",totalBytes);  
              totalFileOk++;//count number of success recv file
          }        
         
      }  
          printf("MGET:completed (%d file(s) transfer OK ,%d file(s) FAILED)\n",totalFileOk,totalFileError);
}



/* input fileName to recv
   output totalBytes=number of bytes recv,
   errorMsg=error description if occur
   return -1 on error;0 on success
*/


int getFile(char* fileName,int* totalBytes,char** errorMsg)
{
    char *buffRecv;
    short opcode;
    short blockNumber;
    char MODE[]= {"octet"};   
     int numbytes;//total byte recived  
     int bytesWrite;
     (*totalBytes)=0;
    
     
    if(connectedToServer==0)
         connectToServer(ip,atoi(port),0);

    //open a file for write;
     FILE * pFile;    
    pFile = fopen (fileName,"w");
    if(!pFile){getSystemErr(errno,(errorMsg)) ;return -1;}//if can not open file in directory
   
    buffRecv=(char*) malloc((516)*sizeof(char));//create buffer  header
       if (buffRecv==NULL){displayError("GET","Error mallocating memory"); exit (1); }    
     sendRequest(RRQ,fileName);//send GET request
 
    do
   {
	if((numbytes = recv(sockfd,buffRecv, 516, 0)) == -1)//recv data(error or file data)
    {displayError("GET","error recv function");free(buffRecv);exit(1);}
 
  if(numbytes<4)//header error
    {displayError("GET","error header");free(buffRecv);exit(1);}      
    memcpy(&opcode,buffRecv,2);
    opcode=ntohs(opcode);
    memcpy(&blockNumber,buffRecv+2,2);  
    blockNumber=ntohs(blockNumber);
 switch(opcode)
 {
        case DATA:
            bytesWrite=fwrite((buffRecv+4),1,numbytes-4,pFile);
            if(bytesWrite<0){displayError("GET","error write file");free(buffRecv);exit(1);}      
            (*totalBytes)+=bytesWrite;
              sendAck(blockNumber);//send ack for chunk X          
              break;
        case ERROR:                    
              (*errorMsg)=strdup(buffRecv+4);
              connectedToServer=0; 
              close(sockfd);//close connection
			  return -1;
               
     default :            
             (*errorMsg)=strdup("unknow error");
             return -1;
 }              

    } while(numbytes==516);
    fclose (pFile);//close file    
    free(buffRecv);//free buffer
    return 0;
}    


/*send request 
intput type=RRQ,WRQ,LIST,CD
fileName=the name of the file to send or recv
*/
void sendRequest(int type,char* fileName)
{
    int lenBuffSend;
    char *buffSend;
    char MODE[]= {"octet"};
    short opcode;
    char strCommand[10];

    switch(type)
    {       
        case RRQ://read request
            opcode=htons(RRQ);
            strcpy(strCommand,"GET");
            break;
        case WRQ:
            opcode=htons(WRQ);//write request
            strcpy(strCommand,"PUT");
            break;
        case LIST:
            opcode=htons(LIST);//get list of file request
            strcpy(strCommand,"LIST");
            break;
         case CD:
            opcode=htons(CD);//change server CWD request
            strcpy(strCommand,"CD");
            break;
        default:
            {displayError(strCommand,"error send data");exit(1);}  
    }
    lenBuffSend=2+(strlen(fileName)+1)+strlen(MODE)+1;//calc header lenght      
    buffSend=(char*) malloc((lenBuffSend)*sizeof(char));//create buffer  header
       if (buffSend==NULL){displayError(strCommand,"Error (re)allocating memory"); exit (1); }            
    memcpy(buffSend,&opcode,2);//copy opcode
    strcpy(buffSend+2,fileName);//copy file name
    strcpy(buffSend+2+strlen(fileName)+1,MODE);
    if((send(sockfd,buffSend,lenBuffSend*sizeof(char),0))==-1)       //send request get file
          {displayError(strCommand,"error send data");free(buffSend);exit(1);}  
     free(buffSend);//clear buffer after send
}


/*input 
command=(PUT,GET,LIST,CD,MGET,MPUT)
errMsg=error description
*/
void displayError(char* command,char* errMsg)
{
    printf("%s: failed\n",command);
    printf("Reason: %s\n",errMsg);  
}


/*connect to server using ip and port*/
void connectToServer(char* destIP,int  port,int displayMsg)
{
 if(displayMsg)printf("Connect %s: started...\n",destIP);
 struct sockaddr_in dest_addr;
 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 if(sockfd == -1)//error socket
 {
  displaySystemErr("Connect",errno);
  connectedToServer=0;  
  exit(1);
 }
else

/* host byte order */
dest_addr.sin_family = AF_INET;
/* short, network byte order */
dest_addr.sin_port = htons(port);
dest_addr.sin_addr.s_addr = inet_addr(destIP);
/* zero the rest of the struct */
memset(&(dest_addr.sin_zero), 0, 8);


if(connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1)//connect
{
  displaySystemErr("Connect",errno); 
  exit(1);
}

else
if(displayMsg)puts("Connceted: completed");
connectedToServer=1;//1-connected 0-Not connected
}



/*return user command and parameter command*/

void getCommandAndParam(char *userCmd,char *command,char* strParam)
{
    int i=0;
    int j=0;
    while(userCmd[i]!= ' ' &&  userCmd[i]!='\0')
     {
         command[i]=userCmd[i];
         i++;
     }
   command[i]='\0';
   for( ;userCmd[i]!='\0';i++)
   strParam[j++]=userCmd[i];
   strParam[j]='\0';
   trim(strParam);
}


/*return array of pointers that each pointer point to 1 paramter(=file name) */

int  getParameters(char* strParam,char*** parametres )
 {
     int i=0;
     int j=0;
     int check2Comma=0;
     trim(strParam);
     i=j=0;
     for(j=0;j<strParam[j]!='\0';j++);//run to end of array
     if(strParam[0]==',' || strlen(strParam)==0 || strParam[--j]==',')
   {
        return -1;//error
   }
  i=0;
   while(strParam[i]!='\0')    
   {
       if(strParam[i]==',')//check if there are 2 or more (,)
       {
           check2Comma++;
           if(check2Comma==2)//there are 2 comma 
               return -1;
           i++;
           continue;
        }
       if (strParam[i]!=' ')
           check2Comma=0;
        i++;
   }
   split(strParam, " , ",(parametres));//split to array
   return 0;
 }


/*split string to array accoreding to delimiter*/
void split(char *s,char* delimiter,char*** parameters)
{    
    int lngParam=0;
    int numOfParams=0;
    char *param;
    char * pch;
   
    pch = strtok (s,delimiter);
    while (pch != NULL)
   {
    lngParam=strlen(pch);        
    param = malloc((lngParam+1)*sizeof(char));    
    if(param==NULL)
        { puts ("Error malloc memory"); exit (1); }
    strcpy(param,pch);
    (*parameters) = (char**) realloc ((*parameters), ((++numOfParams)+1) * sizeof(char*));
     if ((*parameters)==NULL)
       { puts ("Error (re)allocating memory"); exit (1); }
     (*parameters)[numOfParams-1]=param;
     (*parameters)[numOfParams]=NULL;
   
    //printf ("%s %d\n",parameters[numOfParams-1],lngParam);
    pch = strtok (NULL,delimiter);

}
}


void trim(char *s)
{
        // Trim spaces and tabs from beginning:
        int i=0,j;
        while((s[i]==' ')||(s[i]=='\t')) {
                i++;
        }
        if(i>0) {
                for(j=0;j<strlen(s);j++) {
                        s[j]=s[j+i];
                }
        s[j]='\0';
        }

        // Trim spaces and tabs from end:
        i=strlen(s)-1;
        while((s[i]==' ')||(s[i]=='\t')) {
                i--;
        }
        if(i<(strlen(s)-1)) {
                s[i+1]='\0';
        }
}

   
/*change local directory*/
  int  changeDirectory(const char *path)
  {
     char errorMsg[80];
   
      if(chdir(path)==0)//success change work directory          
          return 0;
          //printf("CmyD:completed (%s)\n",get_current_dir_name());
     else
  {
    strerror_r(errno,errorMsg,80);
    errorMsg[80-1]='\0';
    displayError("CmyD",errorMsg);
    return -1;
  }
  }

 
  /*display last error according to error number*/
  void displaySystemErr(char* command,int _errno)  
  {
     char errDisplay[80];
     strerror_r(_errno,errDisplay,80);
     errDisplay[80-1]='\0';
     displayError(command,errDisplay);
  }
 
 /*set in errMsg the error description according to error number*/
  
  void getSystemErr(int _errno,char** errMsg)  
  {
    char errDisplay[80];
    strerror_r(_errno,errDisplay,80);
    errDisplay[80-1]='\0';
    (*errMsg)=strdup(errDisplay);    
  }
 
  /*send ack to server num=ack number*/
  void sendAck(short num)
  {
      char buff[4];
      short opcode;
      short blockNum;
      int numbytes;
     
      blockNum=htons(num);
      opcode = htons(ACK);
      memcpy(buff,&opcode,2);//copy opcode
      memcpy(buff+2,&blockNum,2);//copy blockNum
      if((numbytes =send(sockfd,buff,4*sizeof(char),0))==-1)       //send ack;
          {perror("Error send data");exit(1);}           
  }

#endif

