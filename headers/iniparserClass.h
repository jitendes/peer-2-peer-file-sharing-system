
#include "iniparser.h"
#include "global.h"

//#include "Message.h"
//#include "Message.cpp"

/*** TODOS:


****/

// to store the type of Node - BEACON or REGULAR Node
enum NODE_TYPE
{
	BEACON,
	NON_BEACON // REGULAR NODE
};

/************************************************** DEFINE INIPARSER CLASS **********************************************************/

class iniparserClass {


	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 
		// The following keys are defined for the [init] section of a start-up configuration file.
		unsigned short int myport;
		unsigned int myinitLocation;
		char * myhomedir;
		char * logFilename;
		unsigned int autoShutDown;
		unsigned int msgTTL;
		unsigned int msgLifetime;
		unsigned int getMsgLifetime;
		unsigned int initNeighbors;
		unsigned int joinTimeout;
		unsigned int keepAliveTimeout;
		unsigned int minNeighbors;
		unsigned int noCheck;
		double cacheProb;
		double storeProb;
		double neighborStoreProb;
		unsigned int cacheSize;


		// following variables belongs to [beacons] section
		unsigned int beaconRetry;
		unsigned int bports[BEACONS_SIZE];
		char* bhostname[BEACONS_SIZE];
		unsigned int noOfBeacons;

		// node-specific attributes
		enum NODE_TYPE myNodeType;
		char myhostname[50];

	
	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		iniparserClass (); // default constructor
		~iniparserClass(); // default destructor
		
		int parseStartupConfigFile(char* ininame);
		enum NODE_TYPE checkNodeType();	
		void printConfigFile();

}; // end of iniparserClass Class



/************************************************** DEFINE MESSAGE CLASS **********************************************************/

class Message {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 
		//msgformat msg;	
		unsigned char msgType; 	//The message type. Please refer to the definitions in global.h
		unsigned char UOID[UOID_LEN]; // Message ID. This field is inserted by a message originating node and copied by all forwarding nodes.
		unsigned char TTL;		// How many hops the message has left before it should be dropped.
		unsigned char resvd;	// (always zero)
		unsigned int dataLen;	// The length of the function-dependent data.
		unsigned char *data; 

		unsigned char d_UOID[UOID_LEN]; // for storing the join uoid that this message is responding to
		
		unsigned char statustype; //0x01 -STATUS_NEIGHBOURS, 0x02 - STATUS_FILE
		unsigned char searchtype; //0x01 - SEARCH_FILENAME, 0x02 - SEARCH_SHA1HASH, 0x03 - SEARCH_KEYWORDS
		
	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		Message(); // default constructor
		void updateMsgHeader(unsigned char msgType,unsigned int msgTTL, unsigned int dataLen);
		~Message(); // default destructor
		

		/**************************** COMMON METHODS ************************************/
		// UOID GENERATOR
        int getuoid(char *node_inst_id, char *obj_type, char *uoid_buf, unsigned int uoid_buf_size);

		// MSG HEADER CONSTRUCTOR
		void formMsgHeaderInBuffer(unsigned char* msg);
		void formMsgHeaderFromBuffer(unsigned char *msg);
		
		void printHeader();


		/**************************** JOIN MSG - SPECIFIC METHODS ************************************/
		void formJoinReq(unsigned int mylocation, unsigned short int myport, char* myhostname, unsigned char *joinReq);		
		void recvJoinReq(unsigned int &mylocation, unsigned short int &myport, char* myhostname, unsigned char *joinReq);

		void formJoinResp(unsigned char joinUOID[UOID_LEN], unsigned int dist, unsigned short int myport, char* myhostname, unsigned char *joinResp);
		void recvJoinResp(unsigned char joinUOID[UOID_LEN], unsigned int &dist, unsigned short int &myport, char* myhostname, unsigned char *joinResp);

		/**************************** HELLO MSG - SPECIFIC METHODS ************************************/
		void formHelloMsg(unsigned short int myport, char* myhostname, unsigned char *helloBuf);		
		void recvHelloMsg(unsigned short int &myport, char* myhostname, unsigned char *helloBuf);

		/**************************** KEEPALIVE MSG - SPECIFIC METHODS ************************************/

		/**************************** NOTIFY MSG - SPECIFIC METHODS ************************************/


		/**************************** CHECK MSG - SPECIFIC METHODS ************************************/


		/**************************** STATUS MSG - SPECIFIC METHODS ************************************/
		void formStatusReq(unsigned char status, unsigned char *joinReq);		
		void recvStatusReq(unsigned char &status, unsigned char *buf);
	
		/**************************** SEARCH MSG - SPECIFIC METHODS ************************************/
		void recvSearchReq(unsigned char &search, unsigned char *buf);
		/**************************** MSG-SPECIFIC DATA LENGTH COMPUTING METHODS ************************************/

		/* Part 1 Project */
		int getDataLenJoinReq(char* myhostname);
		int getDataLenJoinResp(char* myhostname);
		int getDataLenHello(char* myhostname);
		int getDataLenKeepAlive(); // empty body, datalength = 0
		int getDataLenNotify();
		int getDataLenCheckReq(); // empty body, dataLength = 0
		int getDataLenCheckResp();
		int getDataLenStatusReq();
		int getDataLenStatusResp(char* myhostname);

		/* Part 2 Project */

		
}; // end of Message Class



/************************************************** DEFINE EVENT CLASS **********************************************************/

class Event {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 			
		Message eventMsg; //Message associated with this Event
		int sockfd;		   // sockfd which generated/received this Message
		int eventType;		// MSG_RECEIVED, MSG_FORWARD, MSG_SENT, MSG_ERRORLOG, MSG_DEBUGLOG

	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		Event(); // default constructor
		Event(Message eventMessage, int sockdesc, int eventtype);
		~Event(); // default destructor

		/**************************** COMMON METHODS ************************************/
		
}; // end of Event Class


/************************************************** DEFINE FS_EVENT CLASS **********************************************************/

class FS_Event {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 			
		unsigned char *data;  //Message associated with this Event stored in char array
		unsigned int totalLen; 
		int sockfd;		   // sockfd which generated/received this Message
		int eventType;		// MSG_RECEIVED, MSG_FORWARD, MSG_SENT, MSG_ERRORLOG, MSG_DEBUGLOG, STATUS_REPONSE, SEARCH_RESPONSE, GET_REPONSE
		unsigned int filenum;
		//unsigned short int src_port;

	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		FS_Event(); // default constructor
		FS_Event(unsigned char *fs_data,unsigned int fs_len, int sockdesc, int eventtype);
		~FS_Event(); // default destructor

		/**************************** COMMON METHODS ************************************/
		
}; // end of FS_Event Class



/************************************************** DEFINE NEIGHBOUR_CONN CLASS **********************************************************/

class neighbourConn {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 		
		char myhostname[50];
		unsigned short int myport;
		int sockfd;		   // sockfd which generated/received this Message
		pthread_t readThread;
		pthread_t writeThread;
		std::queue<FS_Event> writeQueue;      // queue for acessing write queue for the particular connection
		pthread_mutex_t writeQueue_mutex; 
		pthread_cond_t writeQueue_cv;                // cv to wait on

		unsigned int mylocation;

		unsigned int helloRcvd;
		char createdBy; // 'S' conn created by server, 'C' conn created by Client // wat goes into default?

		// static variables
		static unsigned int currConnections;
		

		/**************************** DEFINE FUNCTION MEMBERS ************************************/

		neighbourConn(); // default constructor
		neighbourConn(char hostname[50], unsigned short int port, int sockfd_new); // constructor for clientThread
		neighbourConn(int sockfd_new); // constructor for writeThread
		~neighbourConn(); // default destructor

		/**************************** COMMON METHODS ************************************/
	
		/*returns true upon duplicate connections detected. checks the existing connections in the connList with the given information, 
		if a matching information is found, then duplicate connection is detected */
		int checkForTieBreak(char hostname[50], unsigned short int port); 


		
		void getConnForSockfd(int sock, neighbourConn **conn);
		void UpdateConnInfoForSockfd(char hostname[50], unsigned short int port);
		int checkHelloRcvd();

}; // end of neighbourConn Class


/************************************************** DEFINE NEIGHBOURNODE CLASS **********************************************************/

class NeighbourNode {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 			
		char myhostname[50];
		unsigned short int myport;
		int sockfd;		   // sockfd which is linked to this neighbournode

	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		NeighbourNode(); // default constructor
		NeighbourNode(char hostname[50], unsigned short int port, int sockfd_new);
		~NeighbourNode(); // default destructor

		/**************************** COMMON METHODS ************************************/

		// MSG HEADER CONSTRUCTOR
		//void enqueueEventJob(unsigned char* msg);
		//void dequeueEventJob(unsigned char *msg);
		
		//void printHeader();


}; // end of NeighbourNode Class


/************************************************** DEFINE NEIGHBOURNODE CLASS **********************************************************/

class RoutingNode {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 			
		unsigned char UOID[UOID_LEN];
		int sockfd;			          // sockfd in which this UOID arrived
		unsigned char sha1_file[SHA_DIGEST_LEN]; // store the sha1 of the file for search resp

	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		RoutingNode(); // default constructor
		RoutingNode(unsigned char addUOID[UOID_LEN], int addsockfd);
		RoutingNode(unsigned char addUOID[UOID_LEN], int addsockfd, unsigned char sha1_filestr[]);
		~RoutingNode(); // default destructor

		/**************************** COMMON METHODS ************************************/


}; // end of RoutingNode Class


/************************************************** DEFINE LOG CLASS **********************************************************/
		
class Log
{
	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 
        

		char type;	// 'r', 'f', 's', 'e', 'd'
		long int sec;// = tv_log.tv_sec;
		long int msec;// = tv_log.tv_usec/1000;
		char from[MAX_NODE_ID_LENGTH];    //to do 
		char to[MAX_NODE_ID_LENGTH];      //to do
		char msgtype;
		long int size;
		int TTL;
		unsigned char UOID[20];
		unsigned short int d_port;
		char d_hostname[50];
		unsigned char d_UOID[UOID_LEN];
		unsigned int d_distance;
		int d_errorcode; // find in global.h UNKNOWN_ERROR	- 0, USER_SHUTDOWN_ERROR - 1, UNEXPECTED_KILL_SIGNAL_ERROR - 2, SELF_RESTART_ERROR - 3
		unsigned char d_statustype; // find the define for possible values in global.h
		unsigned char d_searchtype; // find the search type define in global.h
		char *d_query;

	/**************************** DEFINE FUNCTION MEMBERS ************************************/
		Log(); // default constructor - specify all the default values in this
		//Log(); //todo not sure about overloaded constructor
		~Log(); // default destructor

	/**************************** COMMON METHODS ************************************/
	void write_log();            //return type is 

	void error_log(char *str);    //return type is 
	void debug_log(char *str); 

}; // end Log class


/************************************************** DEFINE NBRLIST CLASS **********************************************************/


struct Node{
	string host_name;
	uint16_t port;
	uint32_t distance;
	Node(string hostname, uint16_t port, uint32_t distance);
};

int compare_distance(Node& n1, Node& n2);

class nbrList {
	public:
	list<Node> nodes;
	void sdistance();
	nbrList();
	//int addNode(Join_res* msg);
	int addNode(Node node);
	void delNodes();
	void write_init_file();

}; // end nbrList Class


/************************************************** PROJECT PART 2 **********************************************************/

/************************************************** DEFINE BITVEC CLASS **********************************************************/


class bitvec {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 			
		unsigned char bitvector[BIT_VECTOR_LEN];


	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		bitvec(); // default constructor
		~bitvec(); // default destructor

		/**************************** COMMON METHODS ************************************/
		int setbitInBV(unsigned int bitpos);
		int setBV_keywd(char *keywd);
		void printBV_hex();
		void storeBV_str(unsigned char bitvec_buf[]);
		void convertFromHex(unsigned char bitvec_buf[]);
		
}; // end of bitvec Class



/************************************************** DEFINE LRU_Node CLASS **********************************************************/


class LRU_Node {

	/**************************** DEFINE DATA MEMBERS ************************************/
	public: 			
		unsigned int fileRef;	// filenumber obtained from globalFileNum
		unsigned int fileRef_Size;		// filesize of fileRef to be added/deleted

	/**************************** DEFINE FUNCTION MEMBERS ************************************/

		LRU_Node();	// default constructor
		LRU_Node(unsigned int newfileRef, unsigned int newfileRef_Size);
		~LRU_Node(); // default destructor

		/**************************** COMMON METHODS ************************************/


}; // end of LRU_Node Class
