#include "global.h"

/*******************************************  STRUTURES ***********************************************/

typedef struct serverThread
{
	 int myport;
}serverThreadData;


typedef struct clientThread
{
	int hostport;
	char hostname[50];
	int myServerPort;
	int retry;
	int msgttl;
	int index; //stores the array index in clientThreadInfo
}clientThreadData;





//The global values to reset
int restartValid = FALSE;
int initNeighbourFilePresent = FALSE;


unsigned int gMinNeighboursCnt = 0;// = iniparserInfo->minNeighbors;
pthread_mutex_t gMinNeighboursCnt_mutex; 



/*********************************  THREAD FUNCTIONS - DEFINED IN PROCESS.CPP ******************************/
// invoked by server_thread & client thread
void *read_thread(void *sockfd);
void *write_thread(void *sockfd);

// acts as a server side of the node
void *server_thread(void *serverThreadDataInfo);

// acts as a temporary client thread when the node is joining the network and takes care of the JOINING PROCESS
void *tempJoining_thread(void *clientThreadDataInfo);

// acts as a client side of the node
void *client_thread(void *clientThreadDataInfo);

/* acts as a queue dispatcher. dequeues the object from eventQueue and process that event. 
The message contained in the event is processed based on the msgType and DP makes the decision of responding with a appropriate 
response message or/and flooding that message to other outgoing links (excl incoming link in which the msg arrived). */
void* eventQueueDp_thread(void *);			// defined in eventDispatcher.cpp

/*
// is it needed ?
void *softRestart_thread(void *);	

// shutdown thread is it needed?
void *shutdown_thread(void *);
*/

void *KeepAlive_thread(void* th);

/*********************************  THREAD FUNCTIONS - DEFINED IN CMDTHREAD.CPP ******************************/


// This thread will be waiting for any kind of user input. Upon receiving input, does the correspoding actions.
// process events from cmdLineQueue generated by users

void* cmdThread(void *);			// process all the command line commands typed by the user
void *cmdLinTh(void *para);			// thread that waits for all the status responses, search and get responses
void * nam_gen_th(void *);			// process status_neighbors responses
void* status_files_th(void*);		// process status_files responses
void* search_resp_th(void *cnt);	// process search responses

int shortCircuit = FALSE;				// triggered TRUE on ctrl+c - modified by cmdLinTh, cmdThread, sigint_thread
pthread_mutex_t shortCircuit_mutex; 

/* 
This thread sleeps for every 1 sec and acts as a timer for autoshutdown only
*/
void *timer_thread(void *);		

void Tokenize(const string& str,vector<string>& tokens,const string& delimiters = " ");

// for processing events generated by Command Line Interface
std::queue<FS_Event> cmdLineQueue;
pthread_mutex_t cmdLineQueue_mutex;
pthread_cond_t cmdLineQueue_cv;

unsigned int storeFileNum = 0;
pthread_mutex_t storeFileNum_mutex;

/*******************************************  PROCESS.CPP *****************************************************/

// static variables ?? todo fix it
char nodeID[MAX_NODE_ID_LENGTH];
char nodeInstanceID[MAX_NODE_INSTANCE_ID_LENGTH];


// static variable to check the status of regular node whether its able to contact a beacon node during join process
int isConnToBeaconNode = FALSE;

clientThreadData clientThreadInfo[MAX_THREADS];

void *get_in_addr(struct sockaddr *sa);

// function that checks whether the given sock has data to read
int sockdataAvailable(int sock);
void sigint_handler(int signal);

/******************************************* DECLARE RoutingTable Methods ******************************************/

std::vector<RoutingNode> msgRoutingTable;
pthread_mutex_t msgRT_mutex; 

void addNodeInRoutingTable(unsigned char addUOID[UOID_LEN], int addSockNode);
void deleteNodeInRoutingTable(unsigned char deleteUOID[UOID_LEN]);
int findKeyInRoutingTable(unsigned char findUOID[UOID_LEN]);
int getValueFromRoutingTable (unsigned char findUOID[UOID_LEN]);
void displayRoutingTable();

/*******************************************  EXTERN MEMBERS ***********************************************/

extern iniparserClass *iniparserInfo;


/******************************************* EVENT.CPP ********************************************************/

// for processing events generated by incoming & outgoing messages
std::queue<Event> eventQueue;      // queue for acessing common objects in event queue
pthread_mutex_t eventQueue_mutex; 
pthread_cond_t eventQueue_cv;                // cv to wait on


/****** Methods operated by eventQueue and writeQueue of each write threads - DEFINED IN Event.cpp *********/

// routing functions
void pushRecvEventInEventQueue(Event &event);

void pushSentEventInWriteQueue(FS_Event &event, neighbourConn *myConn);

unsigned int determineFwdTTL(unsigned int rcvdTTL, unsigned int myTTL);
void composeFwdEvent(unsigned char *fs_data, int sockfd, int eventType, unsigned int fs_len, FS_Event &event);
void pushFwdEventInWriteQueue(FS_Event &event, neighbourConn *myConn);
void performForwarding(unsigned char *fs_data, unsigned int fs_len, int sockfd);

void performFlooding(unsigned char *fs_data, unsigned int fs_len, int sockfd);

// status request
void determineRecordInfo(unsigned int &rsize, unsigned int &rlen); 
int determineRecordInfo_Files(unsigned int &rsize, unsigned int &rlen,  unsigned int fileRef[], unsigned int nxtlen[]);

// forwarding and flooding based on neighborstoreprob
void performFlooding_prob(unsigned char *fs_data, unsigned int fs_len, int sockfd, double prob);
void performForwarding_prob(unsigned char *fs_data, unsigned int fs_len, int sockfd, double prob);

 // search request

// SEARCH_FILENAME
int determineSearchRecordInfo_fname(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]); 

//SEARCH_SHA1HASH
int determineSearchRecordInfo_sha1hash(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);

// SEARCH_KEYWORDS
int determineSearchRecordInfo_kwrd(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);

/*******************************************  NEIGHBOURCONN.CPP *****************************************************/

std::map<int, neighbourConn*> connList;
pthread_mutex_t connList_mutex; 
pthread_cond_t connList_cv;                // cv to wait on


/****** connList Methods - DEFINED IN neighbourConn.cpp *********/
// displays the contents of the connList
void displayConnList();  // defined in neighbourConn.cpp
int checkConnStatus(char hostname[50], unsigned short int port);



/*******************************************  NEIGHBOURNODE.CPP *****************************************************/

std::vector<NeighbourNode> Neighbours;
pthread_mutex_t Neighbours_mutex; 


/****** Neighbours Methods - DEFINED IN NeighbourNode.cpp *********/

void addNeighbourNode(NeighbourNode node);
void deleteNeighbourNode(char hostname[50], unsigned short int port);
void displayNeighbours();
int getNeighbourSize();


/******************************************* LOG.CPP ******************************************************************/

// for msg logging
pthread_mutex_t  log_mutex;
int fileOpen;
FILE *fp;

int iflog_file_exist();
int create_logfile();
int del_Log();


		/************************* LOG OUTPUT GENERATORS *****************************************/

// logging object created to log about HLLO msg
void logHLLOMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned short int d_port, char d_hostname[50]);

// logging object created to log about join request
void logJNRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned short int d_port, char d_hostname[50]);

// logging object created to log about join response
void logJNRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned char d_UOID[UOID_LEN], unsigned int d_distance, unsigned short int d_port, char d_hostname[50]);

// logging object created to log about status request
void logSTRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char d_statustype);

// logging status responses
void logSTRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

// logging KPAV messages
void logKPAVMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN]);

// logging STOR messages
void logSTORMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN]);

// logging SHRQ - search request messages
void logSHRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char d_searchtype, char *d_query);

// logging SHRS - search response messages
void logSHRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

// logging GTRQ messages
void logGTRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

// logging GTRS messages
void logGTRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

// logging DELT messages
void logDELTMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN]);

/*******************************************  OTHERS *****************************************************/
// to read the value of errno in the case of "Interrupted System Call"
extern int errno;

int timeToShutdown=FALSE;
pthread_mutex_t shutdown_mutex; 

/* pthread_mutex_lock(&shutdown_mutex);
pthread_mutex_unlock(&shutdown_mutex);
*/
pthread_mutex_t print_mutex;

		
/*pthread_mutex_lock(&print_mutex);
pthread_mutex_unlock(&print_mutex);
*/

void deleteFileUponReset();


FILE *fp_nam = FALSE; // delete it after nam file problem resolved


int init_file_exist();
void read_init_file(int &count);
void del_init_neighbour_file();




/******************************************* PROJECT PART2 ******************************************************************/

/******************************************* STORE.CPP ******************************************************************/


// source http://www.cprogramming.com/tips/showTip.php?tip=59&count=30&page=0
void tolowerKeyword(char keywd[]);

unsigned char hexToByte(unsigned char ch);

// copied from the warmup 1 proj
unsigned int computeFileSize(char type, char *filename);

int flipCoin(double prob);

void print_sha1_hex(unsigned char sha1[]); // works on 20 bytes

void print_md5_hex(unsigned char md5[]); // works on 16 bytes

void convert_sha1_to_hex(unsigned char sha1[], unsigned char sha1buf[]); // updates sha1buf - 40bytes

void convert_hex_to_sha1(unsigned char sha1[], unsigned char sha1buf[]); // updates sha1 - 20bytes

int gen_sha1_file(unsigned char sha1[], char *filename);

int gen_sha1_keywd(unsigned char sha1[], char *keywd); //sha1 array is 20bytes

int gen_md5_keywd(unsigned char md5[], char *keywd); //md5 array is 16 byte

unsigned int sha1_keywd_bit(unsigned char sha1[]);	

unsigned int md5_keywd_bit(unsigned char md5[]);

int getuoid(char *node_inst_id, char *obj_type, char *uoid_buf, unsigned int uoid_buf_size); // for generating the random 20-byte OTP

int gen_onetime_pwd(unsigned char  password[]);

int gen_nonce(unsigned char pwdsha1[], unsigned char  pwdUOID[]);

int create_data_file (const char SRC[], const char DEST[]);

int create_pass_file(int filenum, unsigned char pwdUOID[]);

// creating the "meta" file, "data" file, creates pass file, adding into all the indexs - addAllIndexNodes
int create_store_file(int filenum, char *fileName, unsigned int fileSize, char *keywords, unsigned char bitvec[]);

// forming store msg functions - initiating node uses this functions for sending the store request
void computeStoreMsgSize(unsigned int filenum, unsigned int &metaSize, unsigned int &dataSize);
void formMetadataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *storeMsg);
void formFiledataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int dataSize, unsigned char *storeMsg);


// receiving node uses this functions to temporarily create files for extracting the sha1 and compare it against the sha1 computed on received file data
// reading metadata functions
void get_fname_from_metafile(char *filename, char fname_readstr[]);
void get_fsize_from_metafile(char *filename, unsigned int &fileSize);
void get_sha1_from_metafile(char *filename, unsigned char sha1_readstr[]);
void get_nonce_from_metafile(char *filename, unsigned char nonce_readstr[]);
void get_bitvec_from_metafile(char *filename, unsigned char bitvec_readstr[]);

// for reading password function
void get_pwd_from_passfile(int filenum, unsigned char pwdUOID_hex[]);

// reads data from the buffer  -  store
void recvMetadataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *storeMsg, unsigned char sha1_readstr[]);
void recvFiledataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int dataSize, unsigned char *storeMsg, unsigned char sha1_filestr[]);

int create_cache_file(unsigned int filenum, unsigned int m_startpos, unsigned int f_startpos, unsigned int metaSize, unsigned int dataSize, unsigned char *storeMsg);
int create_cache_file(unsigned int filenum, unsigned int m_startpos, unsigned int f_startpos, unsigned int metaSize, unsigned int dataSize, unsigned char *storeMsg, char *srcpath);

// for search responses... filenum - tmp created in the mini directory for info extraction purpose
void recvMetadataFromBuffer_search(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *searchMsg);
void printMetadataFromFile_search(unsigned int filenum);

// for get responses... filenum - tmp created in the mini directory for info extraction purpose
void recvMetadataFromBuffer_get(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *getMsg,  char fname_readstr[], unsigned int &fileSize, unsigned char sha1_readstr[], unsigned char nonce_readstr[]);
int determineFileIdentical(char fname_readstr[], unsigned char sha1_readstr[], unsigned char nonce_readstr[], unsigned int &filenum);

// for delete -send messages
int deletePwdhit(unsigned int fileRef[], unsigned int &fileCnt, unsigned char nonce_sha1hex[], unsigned char pwdUOID_hex[], unsigned int &fileFound);
int create_delete_filespec(unsigned int filenum, char *filename, unsigned char sha1_hex[], unsigned char nonce_sha1hex[], unsigned char pwdUOID_hex[]); // all hex are 40bytes
void formFileSpecInBuffer(unsigned int filenum, unsigned int startpos, unsigned int filespecSize, unsigned char *Msg);

// for delete - recv messages
void recvFileSpecFromBuffer(unsigned int filenum, unsigned int startpos, unsigned int filespecSize, unsigned char *Msg, unsigned char sha1_hex[], unsigned char pwdUOID_hex[]);
int deleteNoncehit(unsigned int fileRef[], unsigned int &fileCnt, unsigned char pwdUOID_hex[], unsigned int &fileFound);

void deleteFiles(int num, unsigned int fileRef);

void deleteAllFiles();

// removing a temporary file
void removeFile(char *filename);

/******************************************* BITVEC.CPP ******************************************************************/

/** BITWISE OPERATIONS on 128 bytes **/
int bitvec_equal(unsigned char b1[], unsigned char b2[]);
void bitvec_and(unsigned char b1[], unsigned char b2[], unsigned char res[]);
int bitvec_hit(unsigned char b1[], unsigned char b2[]);



/*******************************************  INDEX.CPP ********************************************************************/
// index.cpp implements 3 index structures to support 3 types of searches efficiently
// taken from http://advancedcppwithexamples.blogspot.com/2009/04/example-of-c-multimap.html

			/*********************************  MAPS A BIT-VECTOR TO A LIST OF FILE REFERENCES *********************/
			// KEYWORD INDEX LIST STORE THE BIT-VECTOR KEY IN HEXSTRING FORMAT AS A KEY AND LIST OF FILE REFERENCES FOR THAT KEY

std::multimap<string, unsigned int> kwrdIndex; // - EXTERNALIZE
pthread_mutex_t kwrdIndex_mutex; 

void addKwrdIndexNode(string key, unsigned int fileRef );
void deleteKwrdIndexNode(unsigned int fileRef);
int findKeyInKwrdIndex(string key);
void getValueFromKwrdIndex (string key, unsigned int fileRef[]);
int getCountForKwrdKey (string key);
int getKwrdIndexSize();
void displayKwrdIndex();

			/*********************************  MAPS A FILENAME TO A LIST OF FILE REFERENCES *********************/
			// NAME INDEX LIST STORE THE FILE NAME IN STRING FORMAT AS A KEY AND LIST OF FILE REFERENCES FOR THAT KEY 


std::multimap<string, unsigned int> nameIndex; // - EXTERNALIZE
pthread_mutex_t nameIndex_mutex; 

void addNameIndexNode(string key, unsigned int fileRef );
void deleteNameIndexNode(unsigned int fileRef );
int findKeyInNameIndex(string key);
void getValueFromNameIndex (string key, unsigned int fileRef[]);
int getCountForNameKey (string key);
int getNameIndexSize();
void displayNameIndex();

			/*********************************  MAPS A SHA1 VALUE TO A LIST OF FILE REFERENCES *********************/
			// SHA1 INDEX LIST STORE THE SHA1 (COMPUTED BASED ON FILE-CONTENT) IN HEXSTRING FORMAT AS A KEY AND LIST OF FILE REFERENCES FOR THAT KEY 

std::multimap<string, unsigned int> shaIndex; // - EXTERNALIZE
pthread_mutex_t shaIndex_mutex; 

void addShaIndexNode(string key, unsigned int fileRef );
void deleteShaIndexNode(unsigned int fileRef );
int findKeyInShaIndex(string key);
void getValueFromShaIndex (string key, unsigned int fileRef[]);
void getKeyFromShaIndex (string key, unsigned int fileRef);
int getCountForShaKey (string key);
int getShaIndexSize();
void displayShaIndex();

			/*********************************  MAPS A FILE REF NUM TO A FILEID - 20BYTE-UNIQUE UOID *********************/
			// FILE ID MAP THE CORRESPONDING FILEID FOR THE COPY OF FILE X - IDENTIFIED BY FILENUM - ONLY GENERATED UPON THE SEARCH REQUEST 
/*
A FileID distinguishes one copy of X from another copy of X. Since FileID only matters when a user enters a get user command, 
you only need to create a FileID if a corresponding search reply was created. 
*/
std::vector<RoutingNode> fileidMap;
pthread_mutex_t fileidMap_mutex; 

void addNodeInFileIDMap(unsigned char fileID[UOID_LEN], int fileRef);
void deleteNodeInFileIDMap(int fileRef);
int findFileIDInFileIDMap(int fileRef);
int findFileIDInFileIDMap(int &fileRef, unsigned char fileID[]);
int getFileIDFromFileIDMap(int fileRef, unsigned char fileID[UOID_LEN]);
void displayFileIDMap();
int findIDMatchInFileIDMap(unsigned char fileID[UOID_LEN]);
unsigned int getFileRefFromFileIDMap(unsigned char fileID[UOID_LEN]);

		/**************************** COMMON METHODS ************************************/
void addAllIndexNodes(string kwrd_key, string name_key, string sha_key,  unsigned int fileRef);

void deleteAllIndexNodes(unsigned int fileRef);

void displayAllIndexNodes();

void key_wrd_search(char *str, unsigned int &len, unsigned int fileRef[]);   // should pass pointer to all the keywords with some delimiter



			/*********************************  MAPS A SEARCH FILE OPTION REF NUM TO A FILEID - 20BYTE-UNIQUE UOID *********************/
			// STORING THE DISPLAYED OPTIONS MATCHING THE SEARCH REQUEST ISSUED BY THE USER

std::vector<RoutingNode> searchidMap;
pthread_mutex_t searchidMap_mutex; 

void addNodeInSearchIDMap(unsigned char fileID[UOID_LEN], int fileRef, unsigned char sha1_filestr[]);
void deleteNodeInSearchIDMap(int fileRef);
int findFileIDInSearchIDMap(int fileRef);
int getFileIDFromSearchIDMap(int fileRef, unsigned char fileID[UOID_LEN]);
int getSHA1FromSearchIDMap(int fileRef, unsigned char sha1file[]);
void displaySearchIDMap();
void clearSearchIDMap();

/*******************************************  LRU.CPP ********************************************************************/

std::list<LRU_Node> LRUList; // - NOT EXTERNALIZE
pthread_mutex_t LRUList_mutex; 

// EXTERNALIZE ALL 3 CACHE COUNTERS
// initialize it from cacheSize in iniparserInfo, specified in KBytes - multiply it by 1024 to get the size in bytes 
unsigned int cacheSize = 0;  // it is in KB so multiply by 1024 to get total byte size

// in bytes
unsigned int currentCacheSize = 0;

// in bytes
unsigned int remainingCacheSize = cacheSize - currentCacheSize;

void deleteNodeInLRUList(unsigned int newfileRef, unsigned int newfileRef_Size);
void deleteNodeInLRUList(unsigned int newfileRef);
int lru_store_decision(unsigned int fileRef_Size);
int addNodeInLRUList(unsigned int newfileRef, unsigned int newfileRef_Size);
int findFileInLRUList(unsigned int newfileRef);
int updateLRU_SearchResp(unsigned int newfileRef);
void displayLRUList();


/******************************************* EXTERNALIZE.CPP ******************************************************************/

// EXTERNALIZE
unsigned int globalFileNum; // later read it from the external file wenever node restarts - currently initialized in initialize() func

pthread_mutex_t globalFileNum_mutex; 

unsigned int temp_globalFileNum; // this is a temporary file counter for generating temp meta files upon receiving store request

pthread_mutex_t temp_globalFileNum_mutex; 


// GENERAL FUNCTIONS 
int ext_file_exist(char *filename);

// GLOBAL FILE FUNCTIONS 
int gfileCnt_exist();
int gfileCnt_delete();
int gfileCnt_write();
int gfileCnt_read();


// KEYWORD INDEX FUNCTIONS 
int kwrd_index_exist();
int kwrd_index_delete();
int kwrd_index_write();
int kwrd_index_read();


// NAME INDEX FUNCTIONS 
int name_index_exist();
int name_index_delete();
int name_index_write();
int name_index_read();


// SHA1 INDEX FUNCTIONS
int sha1_index_exist();
int sha1_index_delete();
int sha1_index_write();
int sha1_index_read();


// LRU INDEX FUNCTIONS
int lru_index_exist();
int lru_index_delete();
int lru_index_write();
int lru_index_read();

// ALL FUNCTIONS
void deleteAllExtIndexs();
void writeAllExtIndexs();
void readAllExtIndexs();
