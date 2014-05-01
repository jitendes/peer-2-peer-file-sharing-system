#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <errno.h>

// C++ Library
#include <cstring>
#include <iostream>
#include <queue>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <sstream>
#include <fstream>

// System Library
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>




/*
#include <queue>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <string>
#include <cstring>
#include <map>
#include <iostream>
#include <openssl/sha.h>
#include <openssl/md5.h>
*/
//#include <openssl/sha.h>
//#include <openssl/md5.h>


//using namespace std;

/*check if the compiler is of C++*/


#define BEACONS_SIZE	(unsigned int)10

#define ERROR			 (int) -1
#define SUCCESS			(unsigned int)0
#define TRUE             (unsigned int) 1
#define FALSE            (unsigned int) 0

#define MAX_CLIENTS			(unsigned int)5 /* BACKLOG - used by server_thread */
#define MAX_THREADS			(unsigned int)30

#define MAX_BUFFER_SIZE		(unsigned int)8000
	
#define uoid_buf_sz 20
/**************************** DEFINING MESSAGE TYPES -  15 KINDS OF MESSAGES ************************************/
#define JNRQ (unsigned char)0xFC  //join request	<port> <hostname>
#define JNRS (unsigned char)0xFB  //join response	<uoid> <distance> <port> <hostname>
#define HLLO (unsigned char)0xFA  //hello	<port> <hostname>
#define KPAV (unsigned char)0xF8  //keepalive	(none)
#define NTFY (unsigned char)0xF7  //notify	<errorcode>
#define CKRQ (unsigned char)0xF6  //check request	(none)
#define CKRS (unsigned char)0xF5  //check response	<uoid>
#define SHRQ (unsigned char)0xEC  //search request	<searchtype> <query>
#define SHRS (unsigned char)0xEB  //search response	<uoid>
#define GTRQ (unsigned char)0xDC  //get request	<fileid>
#define GTRS (unsigned char)0xDB  //get response	<uoid>
#define STOR (unsigned char)0xCC  //store	(none)
#define DELT (unsigned char)0xBC  //delete	(none)
#define STRQ (unsigned char)0xAC  //status request	<statustype>
#define STRS (unsigned char)0xAB  //status response	<uoid>





/**************************** LENGTHS OF DIFFERENT FIELDS ASSOCIATED WITH VARIOUS MESSAGES ************************************/
// ALL ARE DEFINED IN TERMS OF #BYTES 

// MAXIMUM LENGTHS
#define MAX_HEADER_LENGTH					27
#define MAX_NODE_ID_LENGTH					200
#define MAX_NODE_INSTANCE_ID_LENGTH			200
#define SHA_DIGEST_LEN   					20 // todo change the name
#define MD5_DIGEST_LEN						16
#define BIT_VECTOR_LEN						128 // in bytes
#define BIT_VECTOR_NBIT_LEN					512 // in bits - LEN of n bits. bitvector : 2*n

// HEADER FIELDS
#define	MSG_TYPE_LEN			1
#define UOID_LEN				20
#define TTL_LEN					1
#define RESERVED_LEN			1
#define DATA_LENGTH_LEN			4

// DATA FIELDS - PART1 PROJECT
#define HOST_LOCATION_LEN		4
#define HOST_PORT_LEN			2
#define DISTANCE_LEN			4
#define ERROR_CODE_LEN			1
#define STATUS_TYPE_LEN			1
#define HOST_INFO_LEN			2

// DATA FIELDS - PART2 PROJECT
#define SEARCH_TYPE_LEN			1
#define NEXT_LENGTH_LEN			4
#define FILE_ID_LEN				20 // similar to SHA1_LEN
#define SHA1_LEN				20
#define META_DATA_LEN			4


// POSSIBLE VALUES FOR ERROR CODE
#define UNKNOWN_ERROR					0x00
#define USER_SHUTDOWN_ERROR				0x01
#define UNEXPECTED_KILL_SIGNAL_ERROR	0x02
#define SELF_RESTART_ERROR				0x03
#define AUTO_SHUTDOWN_ERROR				0x04


//For log files and some other codes 

// POSSIBLE VALUES FOR STATUS TYPE CODE
#define STATUS_NEIGHBOURS				0x01 
#define STATUS_FILE						0x02 

// POSSIBLE VALUES FOR SEARCH TYPE CODE
#define SEARCH_FILENAME						0x01 
#define SEARCH_SHA1HASH						0x02 
#define SEARCH_KEYWORDS						0x03

// POSSIBLE VALUES FOR MESSAGE CLASS - used for logging purposes, msg & event creation
#define MSG_RECEIVED			0x01 
#define MSG_FORWARD				0x02 
#define MSG_SENT				0x03 
#define CMD_THREAD				0x04
#define STATUS_REPONSE			0x05
#define KPAV_THREAD				0x06 // HANDLED BY EVQD
#define SEARCH_REPONSE			0x07
#define GET_REPONSE				0x08


#define SELECT_INTERVAL			200 //todo change it, specified in ms

#ifndef min
#define min(A,B) (((A)>(B)) ? (B) : (A))
#endif /* ~min */

#ifndef diff
#define diff(A,B) (((A)>(B)) ? (A-B) : (B-A))
#endif /* ~diff */

// turning on/off print stmts
#define PRINT_FLAG FALSE

/*check if the compiler is of C++*/

using namespace std;



