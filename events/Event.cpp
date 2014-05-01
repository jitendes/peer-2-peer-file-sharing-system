#include "iniparserClass.h"

/************************************************** DEFINE EVENT CLASS **********************************************************/

///// TO BE SHIFTED TO EVENT.H FILE ONCE MULTIPLE REDEFINTION PROBLEM IS FIXED 

extern std::queue<Event> eventQueue;      // queue for acessing common objects in event queue
extern pthread_mutex_t eventQueue_mutex; 
extern pthread_cond_t eventQueue_cv;                // cv to wait on

extern std::queue<Event> cmdLineQueue;
extern pthread_mutex_t cmdLineQueue_mutex;
extern pthread_cond_t cmdLineQueue_cv;



extern std::map<int, neighbourConn*> connList;
extern pthread_mutex_t connList_mutex; 
extern pthread_cond_t connList_cv;                // cv to wait on

extern vector<NeighbourNode> Neighbours;
extern pthread_mutex_t Neighbours_mutex; 

extern iniparserClass *iniparserInfo;

/**************************** LOGGING FUNCTIONS ************************************/
extern void logHLLOMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned short int d_port, char d_hostname[50]);

extern void logJNRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned short int d_port, char d_hostname[50]);

extern void logJNRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned char d_UOID[UOID_LEN], unsigned int d_distance, unsigned short int d_port, char d_hostname[50]);

extern void logSTRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char d_statustype);

extern void logSTRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

extern void logKPAVMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN]);

extern void logSTORMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN]);

extern void logSHRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char d_searchtype, char *d_query);

extern void logSHRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

extern void logGTRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

extern void logGTRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN]);

extern void logDELTMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN]);

/**************************** STORE FUNCTIONS ************************************/
extern int flipCoin(double prob) ;

extern void computeStoreMsgSize(unsigned int filenum, unsigned int &metaSize, unsigned int &dataSize);
extern unsigned int computeFileSize(char type, char *filename);

/**************************** INDEX FUNCTIONS ************************************/

extern int findKeyInNameIndex(string key);
extern void getValueFromNameIndex (string key, unsigned int fileRef[]);
extern int getCountForNameKey (string key);

extern int findKeyInShaIndex(string key);
extern void getValueFromShaIndex (string key, unsigned int fileRef[]);
extern int getCountForShaKey (string key);

extern int findKeyInKwrdIndex(string key);
extern void getValueFromKwrdIndex (string key, unsigned int fileRef[]);
extern int getCountForKwrdKey (string key);

extern void key_wrd_search(char *str, unsigned int &len, unsigned int fileRef[]); 
/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* DEFAULT CONSTRUCTOR */
Event::Event()
{
	// empty body 


}

/* CONSTRUCTOR */
// TODO remember to deleted this msg in the calling function delete(msg)
Event::Event(Message eventMessage, int sockdesc, int eventtype)
{
	//pthread_mutex_lock(&eventQueue_mutex); 

	eventMsg = eventMessage;
	sockfd = sockdesc;
	eventType =	eventtype;

	//pthread_mutex_unlock(&eventQueue_mutex); 

	//printf("****EVENT GENERATED: msgtype %02x, ttl %d, sockfd %d, eventType %d \n", eventMsg.msgType, eventMsg.TTL, sockfd, eventType);
}

/* DESTRUCTOR */
Event::~Event()
{
	/* pthread_mutex_lock(&eventQueue_mutex); 

	delete(this);

	pthread_mutex_unlock(&eventQueue_mutex); 
	*/
}
		

/************************************************** DEFINE FS_EVENT CLASS **********************************************************/



/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* DEFAULT CONSTRUCTOR */
FS_Event::FS_Event()
{
	// empty body 


}

/* CONSTRUCTOR */
// TODO remember to deleted this msg in the calling function delete(msg)
FS_Event::FS_Event(unsigned char *fs_data,unsigned int fs_len, int sockdesc, int eventtype)
{
	//pthread_mutex_lock(&eventQueue_mutex); 

	data = fs_data;
	totalLen = fs_len;
	sockfd = sockdesc;
	eventType =	eventtype;
	filenum = 0;

	//pthread_mutex_unlock(&eventQueue_mutex); 

	//printf("****FS_EVENT GENERATED: msgtype %02x, ttl %d, totalLen %d, sockfd %d, eventType %d \n", (unsigned int)&data[0], (unsigned int)&data[21],totalLen, sockfd, eventType);

}

/* DESTRUCTOR */
FS_Event::~FS_Event()
{
	/* pthread_mutex_lock(&eventQueue_mutex); 

	delete(this);

	pthread_mutex_unlock(&eventQueue_mutex); 
	*/
}
		


		/**************************** USED FOR RECEIVING EVENTS INTO EVENT QUEUE ************************************/

// whenever we push an event, we shud broadcast all the other waiting threads
void pushRecvEventInEventQueue(Event &event)
{
	//printf("*******!!!! acquring lock EVENT JOB INTO EVENT QUEUE - READ THREAD !!!!!!!!!!!! **** \n");
	pthread_mutex_lock(&eventQueue_mutex);					
	//printf("*******!!!! ADDING EVENT JOB INTO EVENT QUEUE - READ THREAD !!!!!!!!!!!! **** \n");
	eventQueue.push(event);
	// wake up sleeping server threads
	//printf("*******!!!! SIGNALING BROADCAST TO EVENT QUEUE - READ THREAD QUEUESIZE %d!!!!!!!!!!!! **** \n", eventQueue.size());
	pthread_cond_broadcast(&eventQueue_cv);

	pthread_mutex_unlock(&eventQueue_mutex);								
} // end pushRecvEventInEventQueue		

		/**************************** USED FOR SENDING EVENTS INTO SAME-CONN WRITE QUEUE ************************************/


/* takes the event and pushes that in the writequeue of myConn in which the request was received */
// SENDING RESPONSES TO THE SENDER
void pushSentEventInWriteQueue(FS_Event &event, neighbourConn *myConn)
{

	/************************************************** FOR LOG ENTRY **********************************************************/

	// for gettimeofday()
	struct timeval logtime;


	gettimeofday(&logtime, NULL);

	char rfsNodeID[MAX_NODE_ID_LENGTH];
	memset(rfsNodeID,0,MAX_NODE_ID_LENGTH);
	sprintf(rfsNodeID,"%s_%d",myConn->myhostname,myConn->myport);
	
	if(PRINT_FLAG)
		printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside SentWriteQueue event.data[0] %02x: rfsNodeID %s \n", event.data[0], rfsNodeID);
	
	Message msg = Message();
	msg.formMsgHeaderFromBuffer(event.data);

	if(event.data[0] == HLLO)
		logHLLOMsg('s', logtime, rfsNodeID, HLLO, event.totalLen, msg.TTL, msg.UOID, iniparserInfo->myport, iniparserInfo->myhostname);

	else if(event.data[0] == JNRS){ // join response generated by me
		unsigned char joinUOID[UOID_LEN];
		unsigned int dist = 0;
		unsigned short int myport =0;
		char myhostname[50];

		msg.recvJoinResp(joinUOID, dist, myport, myhostname, event.data);
		myhostname[strlen(myhostname)] = '\0';

		//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside SentWriteQueue : join resp dist %d \n", dist);
		logJNRSMsg('s', logtime, rfsNodeID, JNRS, event.totalLen, msg.TTL, msg.UOID, joinUOID, dist, iniparserInfo->myport, iniparserInfo->myhostname);
	}

	else if (event.data[0] == STRQ && event.data[27] == STATUS_NEIGHBOURS) // generated status request by the user - status neighbors
	{
		logSTRQMsg('s', logtime, rfsNodeID, STRQ, event.totalLen, msg.TTL, msg.UOID, STATUS_NEIGHBOURS);
	}


	else if (event.data[0] == STRQ && event.data[27] == STATUS_FILE) // generated status request by the user - status files
	{
		logSTRQMsg('s', logtime, rfsNodeID, STRQ, event.totalLen, msg.TTL, msg.UOID, STATUS_FILE);
	}

	else if (event.data[0] == STRS)
	{
		// decode the received join request		
		unsigned char recvUOID[UOID_LEN];
		memcpy(recvUOID, &event.data[27], UOID_LEN);
		logSTRSMsg('s', logtime, rfsNodeID, STRS, event.totalLen, msg.TTL, msg.UOID, recvUOID);
	}
	//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside SentWriteQueue : UOID is %02x%02x%02x%02x \n", msg.UOID[16],msg.UOID[17],msg.UOID[18],msg.UOID[19]);	

	else if (event.data[0] == KPAV)
	{
		logKPAVMsg('s', logtime, rfsNodeID, KPAV, event.totalLen, msg.TTL, msg.UOID); //EMPTY BODY
	}

	else if (event.data[0] == STOR)
	{
		
		/*if(PRINT_FLAG)
			printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside SentWriteQueue for STOR message: rfsNodeID %s, UOID is %02x%02x%02x%02x  \n", rfsNodeID, msg.UOID[16],msg.UOID[17],msg.UOID[18],msg.UOID[19]); */

		logSTORMsg('s', logtime, rfsNodeID, STOR, event.totalLen, msg.TTL, msg.UOID); 
	}

	else if (event.data[0] == SHRQ)
	{
		
		/*if(PRINT_FLAG)
			printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside SentWriteQueue for STOR message: rfsNodeID %s, UOID is %02x%02x%02x%02x  \n", rfsNodeID, msg.UOID[16],msg.UOID[17],msg.UOID[18],msg.UOID[19]); */

		logSHRQMsg('s', logtime, rfsNodeID, SHRQ, event.totalLen, msg.TTL, msg.UOID, event.data[27], (char *)&event.data[28]); 
	}
	else if (event.data[0] == SHRS)
	{
		// decode the received join request		
		unsigned char recvUOID[UOID_LEN];
		memcpy(recvUOID, &event.data[27], UOID_LEN);
		logSHRSMsg('s', logtime, rfsNodeID, SHRS, event.totalLen, msg.TTL, msg.UOID, recvUOID);
	}
	else if (event.data[0] == GTRQ)
	{
		// decode the received join request		
		unsigned char fileID[UOID_LEN];
		memcpy(fileID, &event.data[27], UOID_LEN);
		logGTRQMsg('s', logtime, rfsNodeID, GTRQ, event.totalLen, msg.TTL, msg.UOID, fileID);
	}

	else if (event.data[0] == GTRS)
	{
		// decode the received join request		
		unsigned char recvUOID[UOID_LEN];
		memcpy(recvUOID, &event.data[27], UOID_LEN);
		logGTRSMsg('s', logtime, rfsNodeID, GTRS, event.totalLen, msg.TTL, msg.UOID, recvUOID);
	}
	else if (event.data[0] == DELT)
	{	
		logDELTMsg('s', logtime, rfsNodeID, DELT, event.totalLen, msg.TTL, msg.UOID); 
	}


	/************************************************** PUSH INTO CORRESPONDING QUEUE **********************************************************/

	pthread_mutex_lock(&myConn->writeQueue_mutex);	
	(myConn->writeQueue).push(event);
	pthread_cond_broadcast(&myConn->writeQueue_cv);
	pthread_mutex_unlock(&myConn->writeQueue_mutex);	
} // end pushSentEventInWriteQueue function

		
		/**************************** USED FOR FORWARDING EVENTS INTO SPECIFIC WRITE QUEUE ************************************/


// rcvdTTL - is extracted from the message received for forwarding
// myTTL - belongs to the forwarding node's TTL (that of my node)
unsigned int determineFwdTTL(unsigned int rcvdTTL, unsigned int myTTL)
{
	rcvdTTL--;
	if(rcvdTTL == 0)
		return 0;
	
	int minTTL = min(rcvdTTL, myTTL);
	
	if(minTTL == 0)
		return 0;

	return minTTL;

} //end determineFwdTTL function


/* copy of eventMsg is passed to this function, which then places the updatedTTL in the msg
New event is generated for this eventMsg, along with its eventType and sockfd used for send operation
*/
void composeFwdEvent(unsigned char *fs_data, int sockfd, int eventType, unsigned int fs_len, FS_Event &event)
{
	// place the modified TTL into the eventMsg
	// eventMsg.TTL = updateTTL;

	// Generate new event with its attributes	
	if (eventType == MSG_FORWARD)
		event = FS_Event(fs_data, fs_len, sockfd, MSG_FORWARD);
	else if (eventType == MSG_SENT)
		event = FS_Event(fs_data, fs_len, sockfd, MSG_SENT);

} // end composeFwdMsg function


/* Performs MSG_FORWARD */ 
/* takes the event and pushes that in the writequeue associated with myConn */
// TAKING THE RECEIVED MSG AND FORWARDING TO THE OUTGOING LINKS
void pushFwdEventInWriteQueue(FS_Event &event, neighbourConn *myConn)
{
	/************************************************** FOR LOG ENTRY **********************************************************/

	// for gettimeofday()
	struct timeval logtime;


	gettimeofday(&logtime, NULL);

	char rfsNodeID[MAX_NODE_ID_LENGTH];
	memset(rfsNodeID,0,MAX_NODE_ID_LENGTH);
	sprintf(rfsNodeID,"%s_%d",myConn->myhostname,myConn->myport);
	//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside pushFwdEventInWriteQueue : rfsNodeID %s \n", rfsNodeID);
	
	Message msg = Message();
	msg.formMsgHeaderFromBuffer(event.data);

	if(event.data[0] == JNRQ){ // forwarding other's JNRQ
		unsigned int mylocation = 0;
		unsigned short int myport = 0;
		char myhostname[50];		
		msg.recvJoinReq(mylocation, myport, myhostname, event.data);
		myhostname[strlen(myhostname)] = '\0';
		//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside pushFwdEventInWriteQueue : rfsNodeID %s and myhostname %s \n", rfsNodeID, myhostname);
	
		logJNRQMsg('f', logtime, rfsNodeID, JNRQ, event.totalLen, msg.TTL, msg.UOID, myport, myhostname);
	}

	/*else if(event.data[0] == JNRS){ // forwarding other's JNRS not generated by me 
		unsigned char joinUOID[UOID_LEN];
		unsigned int dist = 0;
		unsigned short int myport =0;
		char myhostname[50];
		msg.recvJoinResp(joinUOID, dist, myport, myhostname, event.data);
		myhostname[strlen(myhostname)] = '\0';

		logJNRSMsg('f', logtime, rfsNodeID, JNRS, event.totalLen, msg.TTL, msg.UOID, joinUOID, dist, myport, myhostname);
	} */


	else if (event.data[0] == STRQ && event.data[27] == STATUS_NEIGHBOURS) // generated status request by the user - status neighbors
	{
		logSTRQMsg('f', logtime, rfsNodeID, STRQ, event.totalLen, msg.TTL, msg.UOID, STATUS_NEIGHBOURS);
	}


	else if (event.data[0] == STRQ && event.data[27] == STATUS_FILE) // generated status request by the user - status files
	{
		logSTRQMsg('f', logtime, rfsNodeID, STRQ, event.totalLen, msg.TTL, msg.UOID, STATUS_FILE);
	}


	else if (event.data[0] == STOR)
	{
		
		/*if(PRINT_FLAG)
			printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside SentWriteQueue for STOR message: rfsNodeID %s, UOID is %02x%02x%02x%02x  \n", rfsNodeID, msg.UOID[16],msg.UOID[17],msg.UOID[18],msg.UOID[19]); */

		logSTORMsg('f', logtime, rfsNodeID, STOR, event.totalLen, msg.TTL, msg.UOID); 
	}

	else if (event.data[0] == SHRQ)
	{

		logSHRQMsg('f', logtime, rfsNodeID, SHRQ, event.totalLen, msg.TTL, msg.UOID, event.data[27],(char *)&event.data[28]); 
	}
	else if (event.data[0] == GTRQ)
	{
		// decode the received join request		
		unsigned char fileID[UOID_LEN];
		memcpy(fileID, &event.data[27], UOID_LEN);
		logGTRQMsg('f', logtime, rfsNodeID, GTRQ, event.totalLen, msg.TTL, msg.UOID, fileID);
	}


	else if (event.data[0] == DELT)
	{	
		logDELTMsg('f', logtime, rfsNodeID, DELT, event.totalLen, msg.TTL, msg.UOID); 
	}

	//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside SentWriteQueue : UOID is %02x%02x%02x%02x \n", msg.UOID[16],msg.UOID[17],msg.UOID[18],msg.UOID[19]);	

	/************************************************** PUSH INTO CORRESPONDING QUEUE **********************************************************/

	pthread_mutex_lock(&myConn->writeQueue_mutex);	
	(myConn->writeQueue).push(event);
	pthread_cond_broadcast(&myConn->writeQueue_cv);
	pthread_mutex_unlock(&myConn->writeQueue_mutex);	

} // end pushFwdEventInWriteQueue function


/* sockfd - message was received
this rcvd msg must be flooded or forwarded to all the other outgoing links excluding the incoming links
*/
void performForwarding(unsigned char *fs_data, unsigned int fs_len, int sockfd)
{
	pthread_mutex_lock(&Neighbours_mutex); 

	vector<NeighbourNode>::iterator it;

	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{
		NeighbourNode Node = *it;

		pthread_mutex_lock(&connList_mutex); 
		neighbourConn *myConn = connList[Node.sockfd];	
		pthread_mutex_unlock(&connList_mutex); 

		if(sockfd != Node.sockfd)
		{
			FS_Event event;
			//Message eventMsg = msg;
			composeFwdEvent(fs_data, Node.sockfd, MSG_FORWARD, fs_len, event);
			pushFwdEventInWriteQueue(event, myConn);
			//printf("!!!!!! %d => %d, %s, %d.... \n !!!!!!!!", sockfd, myConn->sockfd, myConn->myhostname, myConn->myport);
		} // end if

	} //end for

	pthread_mutex_unlock(&Neighbours_mutex); 	
} // end performForwarding function

// for flooding status request to the whole network
void performFlooding(unsigned char *fs_data, unsigned int fs_len, int sockfd)
{
	pthread_mutex_lock(&Neighbours_mutex); 

	vector<NeighbourNode>::iterator it;

	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{
		NeighbourNode Node = *it;

		pthread_mutex_lock(&connList_mutex); 
		neighbourConn *myConn = connList[Node.sockfd];	
		pthread_mutex_unlock(&connList_mutex); 

		if(sockfd != Node.sockfd)
		{
			FS_Event event;
			//Message eventMsg = msg;
			composeFwdEvent(fs_data, Node.sockfd, MSG_SENT, fs_len, event);
			pushSentEventInWriteQueue(event, myConn);
			//printf("!!!!!! %d => %d, %s, %d.... \n !!!!!!!!", sockfd, myConn->sockfd, myConn->myhostname, myConn->myport);
		} // end if

	} //end for

	pthread_mutex_unlock(&Neighbours_mutex); 	
} // end performFlooding function

// for determing the record information - pass the arguments by reference
// rsize = total size of all the records
// rlen = #records
void determineRecordInfo(unsigned int &rsize, unsigned int &rlen)
{
	pthread_mutex_lock(&Neighbours_mutex); 

	vector<NeighbourNode>::iterator it;

	rsize = 0;
	rlen = Neighbours.size();
	
	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{		
		
		int hlen = strlen((*it).myhostname);
		rsize = rsize + 4 + 2 + hlen;

	} //end for

	pthread_mutex_unlock(&Neighbours_mutex); 	
} // end determineRecordInfo function


// for flooding status request to the whole network
// this is used by the initiating node to send STRQ, STOR, SEARCH REQUEST, KPAV MESSAGES
void performFlooding_prob(unsigned char *fs_data, unsigned int fs_len, int sockfd, double prob)
{
	pthread_mutex_lock(&Neighbours_mutex); 

	vector<NeighbourNode>::iterator it;

	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{
		NeighbourNode Node = *it;

		pthread_mutex_lock(&connList_mutex); 
		neighbourConn *myConn = connList[Node.sockfd];	
		pthread_mutex_unlock(&connList_mutex); 

		if(sockfd != Node.sockfd)
		{

			if(flipCoin(prob) == TRUE)
			{
				FS_Event event;
				//event.filenum = sockfd;
				//Message eventMsg = msg;
				composeFwdEvent(fs_data, Node.sockfd, MSG_SENT, fs_len, event);
				pushSentEventInWriteQueue(event, myConn);
				if(PRINT_FLAG)
					printf("!!!!!!PROB %f %d => %d, %s, %d....  !!!!!!!!\n", prob, sockfd, myConn->sockfd, myConn->myhostname, myConn->myport);
			}

		} // end if

	} //end for

	pthread_mutex_unlock(&Neighbours_mutex); 	
} // end performFlooding_prob function


/* sockfd - message was received
this rcvd msg must be flooded or forwarded to all the other outgoing links excluding the incoming links
*/
void performForwarding_prob(unsigned char *fs_data, unsigned int fs_len, int sockfd, double prob)
{
	pthread_mutex_lock(&Neighbours_mutex); 

	vector<NeighbourNode>::iterator it;

	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{
		NeighbourNode Node = *it;

		pthread_mutex_lock(&connList_mutex); 
		neighbourConn *myConn = connList[Node.sockfd];	
		pthread_mutex_unlock(&connList_mutex); 

		if(sockfd != Node.sockfd)
		{
			if(flipCoin(prob) == TRUE)
			{

				FS_Event event;
				//Message eventMsg = msg;
				composeFwdEvent(fs_data, Node.sockfd, MSG_FORWARD, fs_len, event);
				pushFwdEventInWriteQueue(event, myConn);
				if(PRINT_FLAG)
					printf("!!!!!!PROB %f %d => %d, %s, %d....  !!!!!!!!\n", prob, sockfd, myConn->sockfd, myConn->myhostname, myConn->myport);
			}

		} // end if

	} //end for

	pthread_mutex_unlock(&Neighbours_mutex); 	
} // end performForwarding function


/* SEARCH_FILENAME
1. fname - filename query to search
2. rsize - stores the total len of all the records
3. rlen - #records to be composed in the search response
		- #hits matching the search
4. fileRef - fileref hits for the search query
5. nxtlen array stores the length of the metadata field corresponding to all the records
if only one record, the length becomes zero denoting that no records following that
*/
int determineSearchRecordInfo_fname(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]) // search request
{

	// if given search fname exists in the name index, then proceed with the search resp, otherwise discards it
	if(findKeyInNameIndex(fname)==TRUE)
	{
		rsize = 0;
		rlen = getCountForNameKey(fname);
	
		getValueFromNameIndex(fname, fileRef);		

		if(PRINT_FLAG)
			printf("rsize %d rlen %d \n", rsize, rlen);

		// for each file, extract meta size and store it in the nxtlen array
		for (unsigned int i=0; i < rlen; i++ )
		{		
			if(PRINT_FLAG)
				cout<<"FileRef in recordinfo function_fname = "<< fileRef[i] <<endl;
			unsigned int metaSize = 0;
			unsigned int fileSize = 0; // dummy, not used

			// extract the meta size of the filenum and store it in the corresponding index of nxtlen
			computeStoreMsgSize(fileRef[i], metaSize, fileSize);
			nxtlen[i] = metaSize; 

			rsize = rsize + NEXT_LENGTH_LEN + FILE_ID_LEN + metaSize;

			if(PRINT_FLAG)
				printf("loop %d - fileref %d : rsize %d metaSize %d nxtlen[i] %d \n", i, fileRef[i], rsize, metaSize, nxtlen[i]);

		} //end for

		return TRUE;
	
	} // end if(findKeyInNameIndex(fname)==TRUE)

	// discard the search request message by not sending search resp as the filename does not exist
	else
	{
		return FALSE;
	
	}

} // end determineSearchRecordInfo_fname


/* SEARCH_SHA1HASH
1. fname - sha1hash query to search
2. rsize - stores the total len of all the records
3. rlen - #records to be composed in the search response
		- #hits matching the search
4. fileRef - fileref hits for the search query
5. nxtlen array stores the length of the metadata field corresponding to all the records
if only one record, the length becomes zero denoting that no records following that
*/
int determineSearchRecordInfo_sha1hash(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]) // search request
{

	// if given search fname exists in the name index, then proceed with the search resp, otherwise discards it
	if(findKeyInShaIndex(fname)==TRUE)
	{
		rsize = 0;
		rlen = getCountForShaKey (fname);
	
		getValueFromShaIndex (fname, fileRef);		

		if(PRINT_FLAG)
			printf("rsize %d rlen %d \n", rsize, rlen);

		// for each file, extract meta size and store it in the nxtlen array
		for (unsigned int i=0; i < rlen; i++ )
		{		
			if(PRINT_FLAG)
				cout<<"FileRef in recordinfo_sha1 function = "<< fileRef[i] <<endl;
			unsigned int metaSize = 0;
			unsigned int fileSize = 0; // dummy, not used

			// extract the meta size of the filenum and store it in the corresponding index of nxtlen
			computeStoreMsgSize(fileRef[i], metaSize, fileSize);
			nxtlen[i] = metaSize; 

			rsize = rsize + NEXT_LENGTH_LEN + FILE_ID_LEN + metaSize;

			if(PRINT_FLAG)
				printf("loop %d - fileref %d : rsize %d metaSize %d nxtlen[i] %d \n", i, fileRef[i], rsize, metaSize, nxtlen[i]);

		} //end for

		return TRUE;
	
	} // end if(findKeyInNameIndex(fname)==TRUE)

	// discard the search request message by not sending search resp as the filename does not exist
	else
	{
		return FALSE;
	
	}

} // end determineSearchRecordInfo_sha1hash

/* SEARCH_KEYWORDS
1. fname - kwrd query to search
2. rsize - stores the total len of all the records
3. rlen - #records to be composed in the search response
		- #hits matching the search
4. fileRef - fileref hits for the search query
5. nxtlen array stores the length of the metadata field corresponding to all the records
if only one record, the length becomes zero denoting that no records following that
*/
int determineSearchRecordInfo_kwrd(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]) // search request
{

	// convert the keyword query into bit vector, then match it
	rsize = 0;
	rlen = 0;
	
	key_wrd_search(fname, rlen, fileRef);

	// if given search fname exists in the name index, then proceed with the search resp, otherwise discards it
	if(rlen >0)	
	{	
		// for each file, extract meta size and store it in the nxtlen array
		for (unsigned int i=0; i < rlen; i++ )
		{		
			if(PRINT_FLAG)
				cout<<"FileRef in recordinfo_kwrd function = "<< fileRef[i] <<endl;
			unsigned int metaSize = 0;
			unsigned int fileSize = 0; // dummy, not used

			// extract the meta size of the filenum and store it in the corresponding index of nxtlen
			computeStoreMsgSize(fileRef[i], metaSize, fileSize);
			nxtlen[i] = metaSize; 

			rsize = rsize + NEXT_LENGTH_LEN + FILE_ID_LEN + metaSize;

			if(PRINT_FLAG)
				printf("loop %d - fileref %d : rsize %d metaSize %d nxtlen[i] %d \n", i, fileRef[i], rsize, metaSize, nxtlen[i]);

		} //end for

		return TRUE;
	
	} // end if atleast one hit found

	else
	{
		return FALSE;
	}

} // end determineSearchRecordInfo_kwrd

/* STATUS FILES RESPONSE 
takes the #filenum from the globalFileNum before responding for the STRQ of STATUS_FILE
1. rsize - stores the total len of all the records
2. rlen - #records to be composed in the search response
		- #hits matching the search
3. nxtlen array stores the length of the metadata field corresponding to all the records.. [1..n]
if only one record, the length becomes zero denoting that no records following that
*/
int determineRecordInfo_Files(unsigned int &rsize, unsigned int &rlen,  unsigned int fileRef[], unsigned int nxtlen[])
{
	// do for all the cache & perm files located in the mini file directory
	rsize = 0;

	if(PRINT_FLAG)
		printf("in determineRecordInfo_Files function : rsize %d rlen %d \n", rsize, rlen);

	unsigned int currentFileNum = 0;
	
	// for each file, extract meta size and store it in the nxtlen array
	for (unsigned int i=1; i <= rlen; i++ )
	{		
		if(PRINT_FLAG)
			cout<<"FileRef in recordinfo determineRecordInfo_Files = "<< i <<endl;

		unsigned int metaSize = 0;
		unsigned int fileSize = 0; // dummy, not used

		// given the filenum, compute the meta file size and data file size
		char metaPath[256] = "";
		sprintf(metaPath, "%d.meta", i);

		int exist = computeFileSize('m', metaPath);
		
		// if the file exists then populate the status files
		if(exist != -1)
		{
			currentFileNum++;

			// extract the meta size of the filenum and store it in the corresponding index of nxtlen
			computeStoreMsgSize(i, metaSize, fileSize);
			nxtlen[currentFileNum] = metaSize; 
			fileRef[currentFileNum] = i;

			rsize = rsize + 4 + metaSize;

			if(PRINT_FLAG)
				printf("loop %d - fileref %d : rsize %d metaSize %d nxtlen[i] %d \n", i, i, rsize, metaSize, nxtlen[i]);

			
		}

	} //end for

	rlen = currentFileNum;

	return TRUE;

} // end determineRecordInfo_Files

