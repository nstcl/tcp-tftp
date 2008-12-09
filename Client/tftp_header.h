

/************ general defines ****************/
#define SUCCESS 0
#define CRITICAL -1
#define WARNING -2
#define INFO -3
#define DEBUG -4
#define SEGSIZE         512             /* data segment size */
#define MAX_CMD 1000 /*msx len of command prompt*/

/*********************************************/
/*
 * Trivial File Transfer Protocol (IEN-133) - based on arpa/tftp.h
 */


/*
 * Packet types.
 */
#define RRQ         01                          /* read request */
#define WRQ         02                          /* write request */
#define DATA    03                              /* data packet */
#define ACK         04                          /* acknowledgement */
#define ERROR   05                              /* error code */
#define LIST    06              /* ls request */
#define CD      07              /* cd request */

struct  tftphdr {
        short   th_opcode;                      /* packet type */
        union {
                unsigned short  tu_block;       /* block # */
                short   tu_code;                /* error code */
                char    tu_stuff[1];            /* request packet stuff */
        } __attribute__ ((__packed__)) th_u;
        char    th_data[1];                     /* data or error string */
} __attribute__ ((__packed__));

#define th_block        th_u.tu_block
#define th_code         th_u.tu_code
#define th_stuff        th_u.tu_stuff
#define th_msg          th_data

//#define BUFFER_SIZE 1024
const unsigned short BUFFER_SIZE = SEGSIZE+sizeof(struct tftphdr);

/*
 * Error codes.
 */
#define EUNDEF          0               /* not defined */
#define ENOTFOUND       1               /* file not found */
#define EACCESS         2               /* access violation */
#define ENOSPACE        3               /* disk full or allocation exceeded */
#define EBADOP          4               /* illegal TFTP operation */
#define EBADID          5               /* unknown transfer ID */
#define EEXISTS         6               /* file already exists */
#define ENOUSER         7               /* no such user */
