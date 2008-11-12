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
// defines:
#define MAX_STRING 255
#define BACKLOG 5
#define DEFAULT_SERVER_PORT 5069
#define NUM_OF_SLAVES 2
// globals:
char gScwd[NUM_OF_SLAVES][MAX_STRING+1] = {{"/tmp"},{"/tmp"}};
int logFile = -1;
// data structures:
typedef enum _Status{IDLE,TRANSFERING,FAIL,DONE}Status;
typedef struct _s_jobDescriptor
{
    char cp_client[16];//max string is '111.111.111.111\0'
    char cp_fileName[MAX_STRING+1];
    Status status;

}s_jobDescriptor;

/*
 * initialize server:
 * 1. prepare worker threads
 * 2. bind to listening socket
 * 3. open log file
 */
int init(ushort *port);
// start to receive connections
int start();
// stop listening to connections
int stop();
//log function
void dlog(s_jobDescriptor *jd);
int main(int argc, char *argv[])
{
  printf("Hello, Server!\n");
  return EXIT_SUCCESS;
}
#endif


