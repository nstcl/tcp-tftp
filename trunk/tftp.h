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
// this file contains the data structures needed to construct a tftp message header
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
/************ general defines ****************/
#define SUCCESS 0
#define CRITICAL -1
#define WARNING -2
#define INFO -3
#define DEBUG -4
#define	SEGSIZE		512		/* data segment size */

/*********************************************/
/*
 * Trivial File Transfer Protocol (IEN-133) - based on arpa/tftp.h
 */


/*
 * Packet types.
 */
#define	RRQ	    0x1				/* read request */
#define	WRQ	    0x2				/* write request */
#define	DATA	0x3				/* data packet */
#define	ACK	    0x4				/* acknowledgement */
#define	ERROR	0x5				/* error code */
#define LIST    0x6              /* ls request */
#define CD      0x7              /* cd request */
#define CLOSE	0x8		/* close session request */
struct	tftphdr {
	short	th_opcode;			/* packet type */
	union {
		unsigned short	tu_block;	/* block # */
		short	tu_code;		/* error code */
		char	tu_stuff[1];		/* request packet stuff */
	} __attribute__ ((__packed__)) th_u;
	char	th_data[1];			/* data or error string */
} __attribute__ ((__packed__));

#define	th_block	th_u.tu_block
#define	th_code		th_u.tu_code
#define	th_stuff	th_u.tu_stuff
#define	th_msg		th_data

//#define BUFFER_SIZE 1024
const unsigned short BUFFER_SIZE = SEGSIZE+sizeof(struct tftphdr);

/*
 * Error codes.
 */
#define	EUNDEF		0		/* not defined */
#define	ENOTFOUND	1		/* file not found */
#define	EACCESS		2		/* access violation */
#define	ENOSPACE	3		/* disk full or allocation exceeded */
#define	EBADOP		4		/* illegal TFTP operation */
#define	EBADID		5		/* unknown transfer ID */
#define	EEXISTS		6		/* file already exists */
#define	ENOUSER		7		/* no such user */

