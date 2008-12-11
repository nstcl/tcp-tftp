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
#ifdef CLIENT_BUILD

#include "tftp.h"


int sockfd, numbytes;
int connectedToServer=-1;//-1 undefine 0-NOT connected 1-connected
typedef struct
{
	short   opcode;                      /* packet type */
	short number;//block number or error
	char msg[512];//data or error             
}  TFTPBlock;

/// defines 
#define MAX_CMD 1000 /*msx len of command prompt*/
///

// function prototypes
void changeDirectory(const char *path);
void writeToFile();
void split(char *s,char* delimiter,char*** parameters);
int file_exists (char * fileName);
void mPutFiles(char** parameters);
void mGetFiles(char** parameters);
void displayError(char* command,char* errMsg);
void connectToServer(char* destIP,int  port);
void getCommandAndParam(char *userCmd,char *command,char* strParam);
void trim(char *s);
void displaySystemErr(char* command,int _errno);
void getSystemErr(int _errno,char* errMsg);
void sendAck(short num);
int getFile(char* strParam,int* totalBytes,char* errorMsg);
int  getParameters(char* strParam,char*** parametres );
int putFile(char* strParam,int* totalBytes,char** errorMsg);
//
int main(int  argc, char** argv) {
	//int i;

	if (argc!=3)
	{
		printf("This application must recive  2 parameters [serverip] [port]\n");
		printf("For example client 127.0.0.1 69\n");
		printf("try again...\n");
		exit(1);//wrong parameters
	}
 
	connectToServer(argv[1],atoi(argv[2]));
   
	char userCmd[MAX_CMD];
	char command[MAX_CMD];
	char strParam[MAX_CMD];
	char **parameters=NULL;
	int lenCmd=0;
	int lenParams=0;
	while(1)
	{
		printf("tftp>"); 
		gets(userCmd);
		trim(userCmd);
		getCommandAndParam(userCmd,command,strParam);
		lenCmd=strlen(command);
		lenParams=strlen(strParam);

		if(lenCmd==0)// no command
			continue;
		if(strcmp(command,"CLOSE")==0 && lenParams==0)
		{      
			break;
		}
   
		if(strcmp(command,"CmyD")==0 && lenParams>0)
		{      
			puts("CmyD:started..."); 
			changeDirectory(strParam);      
			continue;      
		}
   
		if(strcmp(command,"GET")==0 && lenParams>0)
		{      
			int totalBytes;
			char* errorMsg;
			puts("GET:started...");
			if(getFile(strParam,&totalBytes,errorMsg)==-1)//error         
			{
				displayError("GET",errorMsg);
				free(errorMsg);
			}
			else//get success
			{
				puts("GET:completed");
				printf("Total number of byte transfered:%d\n",totalBytes);  
			}
			continue;      
		}
    
		if(strcmp(command,"MGET")==0 && lenParams>0)
		{      
			//int i=0;
			puts("MGET:started...");
			if(getParameters(strParam,&parameters)==-1)//format error (parameters)
			{displayError("MGET","parameters format error");free(parameters);continue;}              
			mGetFiles(parameters);  
			continue;
		}
   
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
    
		if(strcmp(command,"MPUT")==0 && lenParams>0)
		{      
			//int i=0;
			puts("MPUT:started...");
			if(getParameters(strParam,&parameters)==-1)//format error (parameters)
			{displayError("MPUT","parameters format error");free(parameters);continue;}              
			mPutFiles(parameters);  
			continue;
		}

    
    
		printf("invalid command\n"); 
		continue;      
  
	}
 
	return (EXIT_SUCCESS);
}
 

int putFile(char* strParam,int* totalBytes,char** errorMsg)//-1 error 0 -success
{
	char *buff; 
	char *recvBuff;
	TFTPBlock buffBlock;
   // int i;
	char systemError[80];
	char MODE[]= {"octet"};
	char opcodeBuff[2];
	int tmp=0;//just for test
  //  int fileBytes=0;//save total byte send (without header);
	int ackNum=0;//last ack num
	char* lastErr;//save last error
	int numBytesRecv;
	int numByteSend;
	int LastBlock=0;
    
	recvBuff=(char*)malloc(516*sizeof(char));//create buffer for recv data 516 
	if (recvBuff==NULL)
	{ displayError("PUT","Error mallocating memory"); exit (1); }          
	int lenHeader=2+(strlen(strParam)+1)+strlen(MODE)+1;//clac header lenght       
	buff=(char*) malloc((lenHeader)*sizeof(char));//create buffer for header
	if (buff==NULL)
	{ displayError("PUT","Error (re)allocating memory"); exit (1); }    
    
	FILE * pFile;    
	pFile = fopen (strParam,"r");
	if(!pFile)//if can not open file in directory
	{getSystemErr(errno,(*errorMsg)) ; return -1;}

	buffBlock.opcode = htons(WRQ);//ntohs(WRQ)
	memcpy(buff,&buffBlock.opcode,2);//copy opcode
	strcpy(buff+2,strParam);//copy file name
	strcpy(buff+2+strlen(strParam)+1,MODE);
	if((numByteSend=send(sockfd,buff,lenHeader*sizeof(char),0))==-1)       //send request put file
	{displayError("PUT","error send data");free(buff);free(recvBuff);exit(1);}  
    
	buffBlock.opcode=-1;
    
	do
	{
		if((numBytesRecv = recv(sockfd,recvBuff, 516, 0)) == -1) 
		{displayError("PUT","error recv function");free(buff);free(recvBuff);exit(1);}
  
		if(numBytesRecv<4)
		{displayError("PUT","error get header file");free(buff);free(recvBuff);exit(1);}      
 
		memcpy(&buffBlock.opcode,recvBuff,2);
		buffBlock.opcode=ntohs(buffBlock.opcode);
		memcpy(&buffBlock.number,recvBuff+2,2);   
		buffBlock.number=ntohs(buffBlock.number);
		switch(buffBlock.opcode)
		{
			case ACK:
              
				if(buffBlock.number!=(ackNum))//check ack number
				{displayError("PUT","error ack number");free(buff);free(recvBuff);exit(1);}                   
				if (LastBlock==1)//get ack for last block
					break;//exit loop;
				else
				{                  
					short opcode=htons(DATA);
					short blockNum=++ackNum;
					blockNum=htons(blockNum);
					char tmpBuffer[512];
					char* sendBuffer;
					int startRead=ackNum-1;
					if(startRead==0)
						startRead=1;
					else
						startRead=startRead*512; 
					int result = fread (tmpBuffer,1,512,pFile);
					if (result == -1) {(*errorMsg)=strdup ("reading error");return -1;}
					int lenSendData=4+result;
					sendBuffer=(char*)malloc(lenSendData*sizeof(char));
					if(sendBuffer==NULL)  { displayError("PUT","Error mallocating memory"); exit (1); }
					memcpy(sendBuffer,&opcode,2);//copy opcode
					memcpy(sendBuffer+2,&blockNum,2);
					memcpy(sendBuffer+4,tmpBuffer,result);//copt data from file                   
					if((numByteSend=send(sockfd,sendBuffer,lenSendData*sizeof(char),0))==-1)       //send request put file
					{displayError("PUT","error send data");free(buff);free(recvBuff);exit(1);}                    
					free(sendBuffer);
					if(result<512)
						LastBlock=1;
					(*totalBytes)+=result;//number of byte send;
				}
              
				break;
			case ERROR:                    
				(*errorMsg)=strdup(recvBuff+4);
				return -1;
				break;
			default :            
				(*errorMsg)=strdup("unknow error");
				return -1;
				break;
		}              
// } while(numbytes<516 && ++tmp<5);
	} while(1);
	free(recvBuff);
	free(buff);
	close(pFile);
	return 0;
     //puts("GET:completed");
     // printf("Total number of byte transfered:%d\n",fileSize);
}     


void mPutFiles(char** parameters)
{
	int i;
	int totalBytes;
	int totalFileOk=0;
	int totalFileError=0;
	char* errorMsg;
	for(i=0;i<parameters[i]!=NULL;i++)
	{  
		printf("PUT:started...(%s)\n",parameters[i]);
		totalBytes=0;
		if(putFile((parameters[i]),&totalBytes,errorMsg)==-1)//error get file        
		{
			totalFileError++;
			displayError("PUT",errorMsg);
			free(errorMsg);
		}
		else//get success
		{
			printf("PUT:completed(%s)\n",parameters[i]);
			printf("Total number of byte transfered:%d\n",totalBytes);  
			totalFileOk++;
		}         
          
	}  
	printf("MPUT:completed (%d file(s) transfer OK ,%d file(s) FAILED)\n",totalFileOk,totalFileError);
}


//get many file seperated by char (,)
void mGetFiles(char** parameters)
{
	int i;
	int totalBytes;
	int totalFileOk=0;
	int totalFileError=0;
	char* errorMsg;
	for(i=0;i<parameters[i]!=NULL;i++)
	{  
		printf("GET:started...(%s)\n",parameters[i]);
		totalBytes=0;
		if(getFile((parameters[i]),&totalBytes,errorMsg)==-1)//error get file        
		{
			totalFileError++;
			displayError("GET",errorMsg);
			free(errorMsg);
		}
		else//get success
		{
			printf("GET:completed(%s)\n",parameters[i]);
			printf("Total number of byte transfered:%d\n",totalBytes);  
			totalFileOk++;
		}         
          
	}  
	printf("MGET:completed (%d file(s) transfer OK ,%d file(s) FAILED)\n",totalFileOk,totalFileError);
}

int getFile(char* strParam,int* totalBytes,char* errorMsg)//-1 error 0 -success
{
	char *buff; 
	char *recvBuff;
	TFTPBlock buffBlock;
	int i;
	char systemError[80];
	char MODE[]= {"octet"};
	char opcodeBuff[2];
	int tmp=0;//just for test
	int fileBytes=0;//save total byte recv (without header);
	int numbytes;    

	recvBuff=(char*)malloc(516*sizeof(char));//create buffer for recv data 516 
	if (recvBuff==NULL)
	{ displayError("GET","Error mallocating memory"); exit (1); }    
	int lenHeader=2+(strlen(strParam)+1)+strlen(MODE)+1;//clac header lenght       
	buff=(char*) malloc((lenHeader)*sizeof(char));//create buffer for header
	if (buff==NULL)
	{ displayError("GET","Error (re)allocating memory"); exit (1); }    
	char* lastErr;//save last error
	FILE * pFile;    
	pFile = fopen (strParam,"w");
	if(!pFile)//if can not open file in directory
	{
          
		getSystemErr(errno,errorMsg) ;
        //displaySystemErr("GET",errno);
		return -1;
	}
   
	buffBlock.opcode = htons(RRQ);//ntohs(RRQ);//form host to network    htons(WRQ);
	memcpy(buff,&buffBlock.opcode,2);//copy opcode
	strcpy(buff+2,strParam);//copy file name
	strcpy(buff+2+strlen(strParam)+1,MODE);
	if((send(sockfd,buff,lenHeader*sizeof(char),0))==-1)       //send request get file
	{displayError("GET","error send data");free(buff);free(recvBuff);exit(1);}  
	buffBlock.opcode=-1;//put other then ilage value;
	do
	{
		if((numbytes = recv(sockfd,recvBuff, 516, 0)) == -1) 
		{displayError("GET","error recv function");free(buff);free(recvBuff);exit(1);}
		printf("===>%d\n",numbytes); 
		if(numbytes<4)
		{displayError("GET","error not get header file");free(buff);free(recvBuff);exit(1);}      
		memcpy(&buffBlock.opcode,recvBuff,2);
		buffBlock.opcode=ntohs(buffBlock.opcode);
		memcpy(&buffBlock.number,recvBuff+2,2);   
		buffBlock.number=ntohs(buffBlock.number);
		switch(buffBlock.opcode)
		{
			case DATA:
				fwrite(recvBuff+4,1,numbytes-4,pFile);
				fileBytes+=(numbytes-4);
				sendAck(buffBlock.number);           
				break;
			case ERROR:                    
				errorMsg=strdup(recvBuff+4);
				return -1;
			default :            
				errorMsg=strdup("unknow error");
				return -1;
		}              
// } while(numbytes<516 && ++tmp<5);
		printf("@@@%d\n",numbytes);
	} while(numbytes==516);
	fclose (pFile);
	(*totalBytes)=fileBytes;
	free(recvBuff);
	free(buff);
	return 0;
     //puts("GET:completed");
     // printf("Total number of byte transfered:%d\n",fileSize);
}     

void displayError(char* command,char* errMsg)
{
	printf("%s: failed\n",command);
	printf("Reason: %s\n",errMsg);   
}

void connectToServer(char* destIP,int  port)
{
	printf("Connect %s: started...\n",destIP);
	struct sockaddr_in dest_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
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
//connect

	if(connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1)
	{
		displaySystemErr("Connect",errno);
		connectedToServer=0;  
	}

	else
		puts("Connceted: completed");
	connectedToServer=1;
}

void getCommandAndParam(char *userCmd,char *command,char* strParam)
{
	int i=0;
	int j=0;
	while(userCmd[i]!= ' ' &&  userCmd[i]!=NULL)
	{
		command[i]=userCmd[i];
		i++;
	}
	command[i]=NULL; 
	for( ;userCmd[i]!=NULL;i++)
		strParam[j++]=userCmd[i];
	strParam[j]=NULL;
	trim(strParam);
}

int  getParameters(char* strParam,char*** parametres )
{
	int i=0;
	int j=0;
	int check2Comma=0;
	trim(strParam);
	i=j=0;
	for(j=0;j<strParam[j]!=NULL;j++);//run to end of array
	if(strParam[0]==',' || strlen(strParam)==0 || strParam[--j]==',')
	{
		return -1;//error
	}
	i=0;
	while(strParam[i]!=NULL)     
	{
		if(strParam[i]==',')//check if there ara 2 or more (,) 
		{
			check2Comma++;
			if(check2Comma==2)
				return -1;
			i++;
			continue;
		}
		if (strParam[i]!=' ')
			check2Comma=0;
		i++;
	}
	split(strParam, " , ",(parametres));
	return 0;
}


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

    

  void changeDirectory(const char *path)
{
	char errorMsg[80];
    
	if(chdir(path)==0)//success chage work directory
		printf("CmyD:completed (%s)\n",get_current_dir_name());
	else
	{
		strerror_r(errno,errorMsg,80);
		errorMsg[80-1]=NULL;
		displayError("CmyD",errorMsg);
	}
}

 void displaySystemErr(char* command,int _errno)  
{
	char errDisplay[80];
	strerror_r(_errno,errDisplay,80);
	errDisplay[80-1]=NULL;
	displayError(command,errDisplay);
}
 
 void getSystemErr(int _errno,char* errMsg)  
{
	char errDisplay[80];
	strerror_r(_errno,errDisplay,80);
	errDisplay[80-1]=NULL;
	errMsg=strdup(errDisplay);    
}
 
 
 
  
  void sendAck(short num)
{
	char buff[4];
	short opcode;
	short blockNum;
      
	blockNum=htons(num);
	printf("SEND ACK %d\n",blockNum);
	opcode = htons(ACK);
	memcpy(buff,&opcode,2);//copy opcode
	memcpy(buff+2,&blockNum,2);//copy blockNum
	if((numbytes =send(sockfd,buff,4*sizeof(char),0))==-1)       //send ack;
	{perror("Error send data");exit(1);}      
      
}

#endif

