#include "time.h"
#include "iniparserClass.h"

using namespace std;

extern std::queue<Event> eventQueue;      // queue for acessing common objects in event queue
extern pthread_mutex_t eventQueue_mutex; 
extern pthread_cond_t eventQueue_cv;                // cv to wait on

extern pthread_mutex_t print_mutex;
extern int timeToShutdown;
extern pthread_mutex_t shutdown_mutex; 

extern void addNodeInRoutingTable(unsigned char addUOID[UOID_LEN], int sockfd_new); 
extern void deleteNodeInRoutingTable(unsigned char deleteUOID[UOID_LEN]);
extern int findKeyInRoutingTable(unsigned char findUOID[UOID_LEN]);
extern int getValueFromRoutingTable (unsigned char findUOID[UOID_LEN]);
extern void displayRoutingTable();

extern iniparserClass *iniparserInfo;

extern std::map<int, neighbourConn*> connList;
extern pthread_mutex_t connList_mutex; 
extern pthread_cond_t connList_cv;                // cv to wait on


extern std::vector<NeighbourNode> Neighbours;
extern pthread_mutex_t Neighbours_mutex; 


extern void pushSentEventInWriteQueue(FS_Event &event, neighbourConn *myConn);
extern unsigned int determineFwdTTL(unsigned int rcvdTTL, unsigned int myTTL);
extern void performForwarding(unsigned char *fs_data, unsigned int fs_len, int sockfd);
extern void performFlooding(unsigned char *fs_data, unsigned int fs_len, int sockfd);

// status request
extern void determineRecordInfo(unsigned int &rsize, unsigned int &rlen);
extern int determineRecordInfo_Files(unsigned int &rsize, unsigned int &rlen,  unsigned int fileRef[], unsigned int nxtlen[]);

extern unsigned int globalFileNum; // later read it from the external file wenever node restarts - currently initialized in initialize() func
extern pthread_mutex_t globalFileNum_mutex; 


extern void displayConnList();

extern void formMetadataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *storeMsg);

// search-specific functions
extern int determineSearchRecordInfo_fname(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);
extern int determineSearchRecordInfo_sha1hash(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);
extern int determineSearchRecordInfo_kwrd(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);

/********** DEFINE FILEID FUNCTIONS ************************/

extern void addNodeInFileIDMap(unsigned char fileID[UOID_LEN], int fileRef);
extern int findFileIDInFileIDMap(int fileRef);
extern int getFileIDFromFileIDMap(int fileRef, unsigned char fileID[UOID_LEN]);

/** LRU FUNCTIONS **/
extern int updateLRU_SearchResp(unsigned int newfileRef);
extern void displayLRUList();


void* eventQueueDp_thread(void*)  
{	
	while(1)
	{   
		//printf("!!!!!!!!!!****** INSIDE EVENTQUEUE LOOP \n");	
		pthread_mutex_lock(&eventQueue_mutex);		
		Event event;
		while (eventQueue.empty() && (timeToShutdown==FALSE))
		{		//printf("!!!!!!!!!!****** INSIDE EVENTQUEUE WHILE WAITING FOR JOB DP \n");	
				pthread_cond_wait(&eventQueue_cv, &eventQueue_mutex);
				//pthread_mutex_unlock (&queueMutex);
				//printf("!!!!!!!!!!****** EVENTQUEUE JOB BECOME AVAILABLE WOKE UP FROM COND WAIT \n");
		}
		
		pthread_mutex_lock (&shutdown_mutex);
		// time to shut down is true, free the lock and signal all the sleeping threads
		if(timeToShutdown==TRUE)
		{	
			pthread_mutex_unlock (&shutdown_mutex);
			pthread_mutex_unlock(&eventQueue_mutex);
			pthread_cond_broadcast(&eventQueue_cv); //todo required?

			if(PRINT_FLAG)
				printf("eventQueueDp_thread is exiting \n");

			pthread_exit(NULL);
		}
		pthread_mutex_unlock (&shutdown_mutex);
		
		// if its not time to shutdown and eventQueue is not empty, process the event job
		// access the front element in the queue for processing
		event = eventQueue.front();
		
		// dequeue a job
		eventQueue.pop();

		pthread_mutex_unlock(&eventQueue_mutex);   
		
		/* work on the job */
		int sockfd = event.sockfd;
		Message msg = event.eventMsg;                         // grab the buffer pointer that was pushed into the queuetor ob		
		int eventtype =	event.eventType;

		if(eventtype==MSG_RECEIVED)
		{
			pthread_mutex_lock(&print_mutex);
			//printf("***********!!!!!!!!!!!!!!!!!!!!!!dequeue msg from event queue is !!!!!!!!!!!********\n");  
			pthread_mutex_unlock(&print_mutex);

			// obtain the corresponding connection object associated with this event-sockfd
			pthread_mutex_lock(&connList_mutex); 
			neighbourConn *myConn = connList[sockfd];
			//printf("am processing the event belonging to %d .. my connList size is %d \n", myConn->sockfd, connList.size());
			pthread_mutex_unlock(&connList_mutex); 
			
			// received message
			msg.printHeader();
			//got the msg , parse the msg and extract the UOID and see if you had seen the msg before
			if(findKeyInRoutingTable(msg.UOID)==FALSE)
			{
				// add this entry into the routing table
				addNodeInRoutingTable(msg.UOID, sockfd);			

				// have to take necessary action depending on msg type			
				switch(msg.msgType)
				{


					case JNRQ:         //join request	<port> <hostname>		
					{
						//printf("inside JNRQ !!!!!!!!!!!!!!!!!!!!!! \n");

						// compose the response only if i received the request
						// for extracting data from the message in the event object
						unsigned char joinUOID[UOID_LEN];
						//unsigned char *recvBuf;					

						// for composing the response
						unsigned int respTotalLen = 0;
						unsigned int respDataLen = 0;					
						unsigned char respBuf[200];
						unsigned int location = 0, dist = 0;
						//unsigned short port = 0;
						//char hostname[50];									
						
						/**************************** COMPOSE RESPONSE & PUSH IN myConn WRITEQUEUE ************************************/

						//recvBuf = (unsigned char*) malloc(sizeof(msg));	

						
						location = myConn->mylocation;
						
						
						//printf("receiving/reading data from EVQD ****@#($@#$)#@@@@@ *** dataLen %d connlocation %d \n\n", msg.dataLen, location); 						
													

						// decode the received join request
						//msg.recvJoinReq(location, port, hostname, recvBuf);
						memcpy(joinUOID, msg.UOID, UOID_LEN);

						// compose the JOIN response, then compose the flooding msg
						Message respMsg = Message();

						// compose the response header
						respDataLen = respMsg.getDataLenJoinResp(iniparserInfo->myhostname);
						respTotalLen = MAX_HEADER_LENGTH + respDataLen;		
						respMsg.updateMsgHeader(JNRS, iniparserInfo->msgTTL,respDataLen);

						//printf("strlen(iniparserInfo->myhostname) %d, respDataLen %d, respTotalLen %d \n", strlen(iniparserInfo->myhostname), respDataLen, respTotalLen);
						
						respMsg.printHeader();
						
						// compose the response data
						respMsg.formMsgHeaderInBuffer(respBuf);
						dist = diff(iniparserInfo->myinitLocation,location);
						respMsg.formJoinResp(joinUOID, dist, iniparserInfo->myport, iniparserInfo->myhostname, respBuf);
						
						//printf("dist %d, iniparserInfo->myinitLocation %d ,location %d \n", dist, iniparserInfo->myinitLocation,location);
						
						//printf("eventQD_thread %d: created JOINRESP Response fr the incoming connection..\n", sockfd);

						// construct the event object
						FS_Event event = FS_Event(respBuf, respTotalLen, sockfd, MSG_SENT);
						pushSentEventInWriteQueue(event, myConn);

						/**************** COMPOSE FORWARDING MESSAGES & PUSH INTO CORRESPONDING WRITE QUEUES ****************/

						// acquire the lock and push the event object into the writeQueues	
						// update the msg with the new ttl before caling forwarding function

						//unsigned char *fwdBuf;					

						// for composing the fwd response
						//unsigned int fwdTotalLen = 0;
						//unsigned int fwdDataLen = 0;					
						unsigned char fwdBuf[200];
						
						// new TTL to be updated with the msg received
						unsigned int fwdTTL = determineFwdTTL(msg.TTL, iniparserInfo->msgTTL);
						msg.TTL = fwdTTL;
						if(fwdTTL != 0)
						{
							// put the msg into buffer to pass it for forwarding
							// get the datalen
							respDataLen = msg.dataLen;
							respTotalLen = MAX_HEADER_LENGTH + respDataLen;		
							Log l = Log();		
							char s[100];
							sprintf(s , "received JNRQ message now forwarding %d len of data, msg size %d of fwdTTL %d", respDataLen, respTotalLen,msg.TTL);
							l.debug_log(s);
							//msg.updateMsgHeader(JNRQ, fwdTTL, msg.dataLen);
							msg.formMsgHeaderInBuffer(fwdBuf);
							memcpy(&fwdBuf[27], &msg.data[0], respTotalLen);
							performForwarding(fwdBuf, respTotalLen, sockfd); 
						} // if fwdTTL != 0

					} // end JNRQ
					break;
					
					/* Join Response from other nodes in the network is received, then just forward to it to the corresponding
					link in which the joining node's request's uoid 's matches with the routing table lookup */
					case JNRS:        //join response	<uoid> <distance> <port> <hostname> // not working 
					{
						// processed in process.cpp						
					}
						break;
					


					/* only for server conn object, it composes the response otherwise it ignores it and moves on to process
						the next event in the EventQueue Dispatcher */
					case HLLO:       //hello	<port> <hostname>
					{
						//printf(" ******!!!!! HELLO MSG RECEIVED BY EVENT QD ****!!!!!!! \n");

						//for sending data to neighbour node
						unsigned char* sendBuf;			
						//int byte_count = 0;
						unsigned int sendTotalLen = 0;
						unsigned int sendDataLen = 0;					

						/**************************** COMPOSE RESPONSE & PUSH IN myConn WRITEQUEUE ************************************/

						// Create a Hello Message only for the server-conn type
						// Construct the HELLO Message - response to be sent to the other end	
						if(myConn->createdBy == 'S')
						{	
							Message sendMsg = Message();
							
							sendDataLen = sendMsg.getDataLenHello(iniparserInfo->myhostname);
							sendTotalLen = MAX_HEADER_LENGTH + sendDataLen;		
							sendMsg.updateMsgHeader(HLLO, 1 ,sendDataLen);

							//printf("strlen(iniparserInfo->myhostname) %d, sendDataLen %d, sendTotalLen %d \n", strlen(iniparserInfo->myhostname), sendDataLen, sendTotalLen);

							sendMsg.printHeader();

							sendBuf = (unsigned char*) malloc(sendTotalLen+1);	
							memset(&sendBuf[0], 0, sendTotalLen+1);
							//sendBuf[sendTotalLen] = '\0';

							if(sendBuf == NULL)
							{
								//printf("write_thread %d: Failure in Allocating Dynamic Memory \n", newsockfd);

								// free up malloc memory & kill the thread
								/* free(sendBuf);
								*/
								continue; // todo is it correct ??

							}
							
							sendMsg.formMsgHeaderInBuffer(sendBuf);
							sendMsg.formHelloMsg(iniparserInfo->myport, iniparserInfo->myhostname, sendBuf);
							sendBuf[sendTotalLen] = '\0'; 

							//printf("eventQD_thread %d: created HELLO Msg Response fr the incoming connection..\n", sockfd);


							FS_Event event = FS_Event(sendBuf, sendTotalLen, sockfd, MSG_SENT);
							pushSentEventInWriteQueue(event, myConn);

							free(sendBuf);

						} //end if(myConn->createdBy == 'S')

					} //end HLLO
						break;

					case KPAV:       //keepalive	(none)
						// processed in process.cpp
						break;

					case NTFY:       //notify	<errorcode>
						// havent implemented
						break;

					case CKRQ:       //check request	(none)
						// havent implemented
						break;
					
					case CKRS:       //check response	<uoid>
						// havent implemented
						break;

					
					
					case SHRQ:       //search request	<searchtype> <query>
					{
							// if the search query does not exist in any of the indexs, then node only forwards the req and dont sent search response
							/**************************** COMPOSE RESPONSE & PUSH IN myConn WRITEQUEUE ************************************/

							// compute 3 vectors 
							// precompute the length of the total records
							// grab the #record and total record size

							
							unsigned int rsize = 0;
							unsigned int rlen = 0;
							unsigned int fileRef[30];
							unsigned int nxtlen[30];
							
							int retValue = FALSE;

							// based on the search type, call the corresponding search record functions
							if(msg.searchtype == SEARCH_FILENAME)
								retValue = determineSearchRecordInfo_fname((char*)&msg.data[1], rsize, rlen, fileRef, nxtlen);
							else if(msg.searchtype == SEARCH_SHA1HASH)
								retValue = determineSearchRecordInfo_sha1hash((char*)&msg.data[1], rsize, rlen, fileRef, nxtlen);
							else if(msg.searchtype == SEARCH_KEYWORDS)
								retValue = determineSearchRecordInfo_kwrd((char*)&msg.data[1], rsize, rlen, fileRef, nxtlen);

							if(retValue == TRUE)
							{
								Log l = Log();
								char s[100];
								sprintf(s, "The #search records for query: %s - %d, total record size - %d", &msg.data[1], rlen, rsize);
								l.debug_log(s);														

								// for composing the response
								unsigned char *respBuf;					
								unsigned char reqUOID[UOID_LEN];
								unsigned int respTotalLen = 0;
								unsigned int respDataLen = 0;																	
									

								// decode the received join request						
								memcpy(reqUOID, msg.UOID, UOID_LEN);

								// compose the SEARCH response, then compose the flooding msg for forwarding 
								Message respMsg = Message();
							
								// compose the response header
								respDataLen = UOID_LEN + rsize;
								respTotalLen = MAX_HEADER_LENGTH + respDataLen;
								
								if(PRINT_FLAG)
									printf("SHRQ response info: respDataLen %d respTotalLen %d \n", respDataLen, respTotalLen);

								// form the whole message then later copy into data section starting from 27th byte
								respBuf = (unsigned char*) malloc(respTotalLen);
								
								//memset(&respBuf[0], 0, respTotalLen+1);
								respMsg.updateMsgHeader(SHRS, iniparserInfo->msgTTL,respDataLen);
								respMsg.formMsgHeaderInBuffer(respBuf);
								
								respMsg.printHeader();
								
								// compose the response data
								memcpy(&respBuf[27], &reqUOID[0], UOID_LEN);

								unsigned int rstartbyte = 47; //initial byte
								unsigned int count = 0;

								// compose the record section										
								
								for ( count=0; count < rlen; count++ )
								{		
									
									unsigned int metaSize = nxtlen[count];
									unsigned char fileID[FILE_ID_LEN];
									unsigned int startpos = rstartbyte+NEXT_LENGTH_LEN + FILE_ID_LEN;																
									
									if(PRINT_FLAG)
										printf("count %d metaSize %d fileRef[count] %d startpos %d \n", count, metaSize, fileRef[count], startpos);

									/* check whether fileID exists or not
									if it exists, then get fileID from the fileid map
									or not, create new one and push it into the map
									*/
									if(findFileIDInFileIDMap(fileRef[count]) == TRUE)
									{
										getFileIDFromFileIDMap(fileRef[count], fileID);
										if(PRINT_FLAG)
										{
											printf("fileID found, so reading the existing one \n");
											printf("**** FILEID NODE EXISTING - READ FOR SHRS %02x%02x%02x%02x: \n", fileID[16], fileID[17], fileID[18], fileID[19]);
										}
									}
									else
									{
										addNodeInFileIDMap(fileID, fileRef[count]);
										if(PRINT_FLAG)
										{
											printf("fileID not found, so creating new one \n");
											printf("**** FILEID NODE ADDED FOR SHRS %02x%02x%02x%02x: \n", fileID[16], fileID[17], fileID[18], fileID[19]);
										}

									}

									/* If a node generates a search response message for a file in its cache, 
										it should update the LRU order of this file by pushing it in front.
									*/
									updateLRU_SearchResp(fileRef[count]);
									displayLRUList();

									if(count == (rlen-1)) //make the record length to be zero for the last record
									{
										unsigned int len = 0;																	

										memcpy(&respBuf[rstartbyte], &len, NEXT_LENGTH_LEN); 
										
										memcpy(&respBuf[rstartbyte+NEXT_LENGTH_LEN], &fileID[0], FILE_ID_LEN); 
																											
										formMetadataInBuffer(fileRef[count], startpos, metaSize, respBuf);
									}
									else
									{
										memcpy(&respBuf[rstartbyte], &metaSize, NEXT_LENGTH_LEN); 

										memcpy(&respBuf[rstartbyte+NEXT_LENGTH_LEN], &fileID[0], FILE_ID_LEN); 
										
										formMetadataInBuffer(fileRef[count], startpos, metaSize, respBuf);
									}

									rstartbyte = rstartbyte + NEXT_LENGTH_LEN + FILE_ID_LEN + metaSize; //next byte

								} //end for

								//respBuf[rstartbyte] = '\0';							

								// end composing the record section 

								//printf("eventQD_thread %d: created STATUSRESP Response fr the incoming connection..%d \n", sockfd, strlen((char *)respBuf));
							
								// construct the event object
								FS_Event event = FS_Event(respBuf, respTotalLen, sockfd, MSG_SENT);
								pushSentEventInWriteQueue(event, myConn);

							} // end if(retValue == TRUE) - if atleast one record exist 


								/**************** COMPOSE FORWARDING MESSAGES & PUSH INTO CORRESPONDING WRITE QUEUES ****************/

							// for composing the fwd response
							unsigned int totalLen = 0;
							unsigned int dataLen = 0;					
							unsigned char fwdBuf[200];
							
							// new TTL to be updated with the msg received
							unsigned int fwdTTL = determineFwdTTL(msg.TTL, iniparserInfo->msgTTL);
							msg.TTL = fwdTTL;
							
							if(fwdTTL != 0)
							{
								// put the msg into buffer to pass it for forwarding
								// get the datalen
								dataLen = msg.dataLen;
								totalLen = MAX_HEADER_LENGTH + dataLen;		
								
								//msg.updateMsgHeader(JNRQ, fwdTTL, msg.dataLen);
								msg.formMsgHeaderInBuffer(fwdBuf);
								memcpy(&fwdBuf[27], &msg.data[0], totalLen);
								performForwarding(fwdBuf, totalLen, sockfd); 		
							}
					} // end SHRQ

						break;
					case SHRS:       //search response	<uoid>

						break;
					case GTRQ:       //get request	<fileid>
						
						break;
					case GTRS:       //get response	<uoid>

						break;
					case STOR:       //store	(none)

						break;
					case DELT:       //delete	(none)
						break;

					case STRQ:       //status request	<statustype>
					{
						if(msg.statustype == STATUS_NEIGHBOURS)
						{							
							/**************************** COMPOSE RESPONSE & PUSH IN myConn WRITEQUEUE ************************************/

							// compute 3 vectors 
							// precompute the length of the total records
							// grab the #record and total record size

							
							unsigned int rsize = 0;
							unsigned int rlen = 0;

							determineRecordInfo(rsize, rlen);

							Log l = Log();
							char s[100];
							sprintf(s, "The #records - %d, total record size - %d", rlen, rsize);
							l.debug_log(s);
							
							// len1 of the first byte the record begins
							unsigned short int hinfo_len =HOST_PORT_LEN + strlen(iniparserInfo->myhostname);
				
							unsigned int len1 = MAX_HEADER_LENGTH + UOID_LEN + HOST_INFO_LEN + hinfo_len;
							unsigned int totalDatalen = len1 + rsize;
							
							s[0] = '\0';
							sprintf(s, "The len1 - %d, datalen of STRS - %d", len1, totalDatalen);
							l.debug_log(s);

							// for composing the response
							unsigned char *respBuf;					
							unsigned char reqUOID[UOID_LEN];
							unsigned int respTotalLen = 0;
							unsigned int respDataLen = 0;																	
							//unsigned short port = 0;
							//char hostname[50];									

							// decode the received join request						
							memcpy(reqUOID, msg.UOID, UOID_LEN);

							// compose the JOIN response, then compose the flooding msg
							Message respMsg = Message();
						
							// compose the response header
							respDataLen = totalDatalen - 27;
							respTotalLen = totalDatalen;

							// form the whole message then later copy into data section starting from 27th byte
							respBuf = (unsigned char*) malloc(respTotalLen);
							
							//memset(&respBuf[0], 0, respTotalLen+1);
							respMsg.updateMsgHeader(STRS, iniparserInfo->msgTTL,respDataLen);
							respMsg.formMsgHeaderInBuffer(respBuf);

							s[0] = '\0';
							sprintf(s, "strlen(iniparserInfo->myhostname) %d, respDataLen %d, respTotalLen %d ", strlen(iniparserInfo->myhostname), respDataLen, respTotalLen);
							l.debug_log(s);
							
							respMsg.printHeader();
							
							// compose the response data
							memcpy(&respBuf[27], &reqUOID[0], UOID_LEN);
							memcpy(&respBuf[47], &hinfo_len, HOST_INFO_LEN);
							memcpy(&respBuf[49], &iniparserInfo->myport, HOST_PORT_LEN);
							memcpy(&respBuf[51], iniparserInfo->myhostname, strlen(iniparserInfo->myhostname));

							unsigned int rstartbyte = len1; //initial byte
							unsigned int count = 0;

							// compose the record section
							pthread_mutex_lock(&Neighbours_mutex); 
						
							vector<NeighbourNode>::iterator it;
							
							for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
							{		
								
								unsigned int hlen = strlen((*it).myhostname);
								unsigned int rlen_data = hlen + 2;

								NeighbourNode n = *it;								
								
								unsigned short int dport = n.myport;
								char *host = n.myhostname;

								if(count == (rlen-1)) //make the record length to be zero for the last record
								{
									unsigned int len = 0;
									memcpy(&respBuf[rstartbyte], &len, 4); 
									
									memcpy(&respBuf[rstartbyte+4], &dport, HOST_PORT_LEN); 
									
									memcpy(&respBuf[rstartbyte+6], host, hlen); 
								}
								else
								{
									memcpy(&respBuf[rstartbyte], &rlen_data, 4); 
									memcpy(&respBuf[rstartbyte+4], &dport, HOST_PORT_LEN);
									memcpy(&respBuf[rstartbyte+6], host, hlen);				
								}
								rstartbyte = rstartbyte + 4 + 2 + hlen; //next byte
								count++;

							} //end for

							//respBuf[rstartbyte] = '\0';

							pthread_mutex_unlock(&Neighbours_mutex); 																													
							

							// end composing the record section 

							//printf("eventQD_thread %d: created STATUSRESP Response fr the incoming connection..%d \n", sockfd, strlen((char *)respBuf));
						
							// construct the event object
							FS_Event event = FS_Event(respBuf, respTotalLen, sockfd, MSG_SENT);
							pushSentEventInWriteQueue(event, myConn);

								/**************** COMPOSE FORWARDING MESSAGES & PUSH INTO CORRESPONDING WRITE QUEUES ****************/

							// for composing the fwd response
							unsigned int totalLen = 0;
							unsigned int dataLen = 0;					
							unsigned char fwdBuf[200];
							
							// new TTL to be updated with the msg received
							unsigned int fwdTTL = determineFwdTTL(msg.TTL, iniparserInfo->msgTTL);
							msg.TTL = fwdTTL;
							
							if(fwdTTL != 0)
							{
								// put the msg into buffer to pass it for forwarding
								// get the datalen
								dataLen = msg.dataLen;
								totalLen = MAX_HEADER_LENGTH + dataLen;		
								
								//msg.updateMsgHeader(JNRQ, fwdTTL, msg.dataLen);
								msg.formMsgHeaderInBuffer(fwdBuf);
								memcpy(&fwdBuf[27], &msg.data[0], totalLen);
								performForwarding(fwdBuf, totalLen, sockfd); 		
							}
						} // end  for STRQ if(msg.statustype == STATUS_NEIGHBOURS)
						
						else if(msg.statustype == STATUS_FILE)
						{
							//printf("received STRQ of STATUS_FILE type \n");
							/**************************** COMPOSE RESPONSE & PUSH IN myConn WRITEQUEUE ************************************/

							// compute 3 vectors 
							// precompute the length of the total records
							// grab the #record and total record size
				
							unsigned int rsize = 0;
							unsigned int rlen = 0;

							//use globalfilenum counter
							pthread_mutex_lock(&globalFileNum_mutex);
							rlen = globalFileNum;
							pthread_mutex_unlock(&globalFileNum_mutex);	

							unsigned int nxtlen[rlen+1]; // stores the first record len from index 1
							unsigned int fileRef[rlen+1];

							determineRecordInfo_Files(rsize, rlen, fileRef, nxtlen);

							Log l = Log();
							char s[100];
							sprintf(s, "The #records - %d, total record size - %d STRS FILE", rlen, rsize);
							l.debug_log(s);
							
							// len1 of the first byte the record begins
							unsigned short int hinfo_len =HOST_PORT_LEN + strlen(iniparserInfo->myhostname);
				
							unsigned int len1 = MAX_HEADER_LENGTH + UOID_LEN + HOST_INFO_LEN + hinfo_len;
							unsigned int totalDatalen = len1 + rsize;
							
							s[0] = '\0';
							sprintf(s, "The len1 - %d, datalen of STRS_FILE - %d", len1, totalDatalen);
							l.debug_log(s);

							// for composing the response
							unsigned char *respBuf;					
							unsigned char reqUOID[UOID_LEN];
							unsigned int respTotalLen = 0;
							unsigned int respDataLen = 0;																	
							//unsigned short port = 0;
							//char hostname[50];									

							// decode the received join request						
							memcpy(reqUOID, msg.UOID, UOID_LEN);

							// compose the JOIN response, then compose the flooding msg
							Message respMsg = Message();
						
							// compose the response header
							respDataLen = totalDatalen - 27;
							respTotalLen = totalDatalen;

							// form the whole message then later copy into data section starting from 27th byte
							respBuf = (unsigned char*) malloc(respTotalLen);
							
							//memset(&respBuf[0], 0, respTotalLen+1);
							respMsg.updateMsgHeader(STRS, iniparserInfo->msgTTL,respDataLen);
							respMsg.formMsgHeaderInBuffer(respBuf);

							s[0] = '\0';
							sprintf(s, "STRS FILE strlen(iniparserInfo->myhostname) %d, respDataLen %d, respTotalLen %d ", strlen(iniparserInfo->myhostname), respDataLen, respTotalLen);
							l.debug_log(s);
							
							respMsg.printHeader();
							
							// compose the response data
							memcpy(&respBuf[27], &reqUOID[0], UOID_LEN);
							memcpy(&respBuf[47], &hinfo_len, HOST_INFO_LEN);
							memcpy(&respBuf[49], &iniparserInfo->myport, HOST_PORT_LEN);
							memcpy(&respBuf[51], iniparserInfo->myhostname, strlen(iniparserInfo->myhostname));
//printf("iniparserInfo->myhostname is %s \n", iniparserInfo->myhostname);
//printf("iniparserInfo->myport is %d \n", iniparserInfo->myport);

							unsigned int rstartbyte = len1; //initial byte
							unsigned int count = 0;
							
							// compose the record section
							for (count=1; count <= rlen; count++ )
							{										
								unsigned int metaSize = nxtlen[count];
								unsigned int startpos = rstartbyte+4;		

								if(count == rlen) //make the record length to be zero for the last record
								{
									unsigned int len = 0;
									
									memcpy(&respBuf[rstartbyte], &len, 4); 
									formMetadataInBuffer(fileRef[count], startpos, metaSize, respBuf);

								}
								else
								{
									memcpy(&respBuf[rstartbyte], &metaSize, 4); 
									formMetadataInBuffer(fileRef[count], startpos, metaSize, respBuf);
								}
								
								rstartbyte = rstartbyte + 4 + metaSize; //next byte			
								
								//printf("finished forming status record for fileRef % d \n", fileRef[count]);

							} //end for

							//respBuf[rstartbyte] = '\0';																																			

							// end composing the record section 

							//printf("eventQD_thread %d: created STATUSRESP of 0x02 type Response fr the incoming connection..%d \n", sockfd, strlen((char *)respBuf));
						
							// construct the event object
							FS_Event event = FS_Event(respBuf, respTotalLen, sockfd, MSG_SENT);
							pushSentEventInWriteQueue(event, myConn);

								/**************** COMPOSE FORWARDING MESSAGES & PUSH INTO CORRESPONDING WRITE QUEUES ****************/

							// for composing the fwd response
							unsigned int totalLen = 0;
							unsigned int dataLen = 0;					
							unsigned char fwdBuf[200];
							
							// new TTL to be updated with the msg received
							unsigned int fwdTTL = determineFwdTTL(msg.TTL, iniparserInfo->msgTTL);
							msg.TTL = fwdTTL;
							
							if(fwdTTL != 0)
							{
								// put the msg into buffer to pass it for forwarding
								// get the datalen
								dataLen = msg.dataLen;
								totalLen = MAX_HEADER_LENGTH + dataLen;		
								
								//msg.updateMsgHeader(JNRQ, fwdTTL, msg.dataLen);
								msg.formMsgHeaderInBuffer(fwdBuf);
								memcpy(&fwdBuf[27], &msg.data[0], totalLen);
								performForwarding(fwdBuf, totalLen, sockfd); 		
							}						
						
						} // end for STRQ if(msg.statustype == STATUS_FILE)

					}
						break;

					case STRS:       //status response	<uoid>
					{

						//check whether its for u or the forwards from the other nodes
						// take corresponding actions 

						// processed in process.cpp

						
					}
						break;
					default:
						break;
				}//close of switch
			} // end if lookup fails
			
			// lookup succeeds. so we received duplicate msg and its dropped
			else
			{
				continue;
			}
		} // end if(eventtype==MSG_RECEIVED)

		else if(eventtype==CMD_THREAD)
		{
			

				switch(msg.msgType)
				{
					case STRQ:         //status request	<statustype>		
					{
						if(msg.statustype == STATUS_NEIGHBOURS)
						{

							//printf("am inside the CMD_THREAD EQD HANDLING !!!!!!!!!!!\n ");					
		
							unsigned char fwdBuf[30];
							msg.formMsgHeaderInBuffer(fwdBuf);
							msg.formStatusReq(STATUS_NEIGHBOURS, fwdBuf);
							//printf("INSIDE QDS MY STRQ DATA statustype status type %02x  !!!!!!!!!!!!!!!\n", msg.statustype);						

							// put the msg into buffer to pass it for forwarding
							// get the datalen
							int fwdDataLen = msg.dataLen;
							int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;		

							// flood to all the neighbours of my node
							performFlooding(fwdBuf, fwdTotalLen, sockfd); 
						} // end if(msg.statustype == STATUS_NEIGHBOURS)


						else if(msg.statustype == STATUS_FILE)
						{
							//printf("reached STATUS_FILE case handling of eventtype==CMD_THREAD\n");

							unsigned char fwdBuf[30];
							msg.formMsgHeaderInBuffer(fwdBuf);
							msg.formStatusReq(STATUS_FILE, fwdBuf);
							//printf("INSIDE QDS MY STRQ DATA statustype status type %02x  !!!!!!!!!!!!!!!\n", msg.statustype);						

							// put the msg into buffer to pass it for forwarding
							// get the datalen
							int fwdDataLen = msg.dataLen;
							int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;		

							// flood to all the neighbours of my node
							performFlooding(fwdBuf, fwdTotalLen, sockfd); 
						}

					}
						break;

					case STOR:         //status request	<statustype>		
					{					
						//printf("reached STOR case handling of eventtype==CMD_THREAD\n");
	
						// processed in cmdThread.cpp

						/*unsigned char fwdBuf[30];
						msg.formMsgHeaderInBuffer(fwdBuf);
						msg.formStatusReq(STATUS_NEIGHBOURS, fwdBuf);
						
						if(PRINT_FLAG)
							printf("INSIDE QDS MY STRQ DATA statustype status type %02x  !!!!!!!!!!!!!!!\n", msg.statustype);						

						// put the msg into buffer to pass it for forwarding
						// get the datalen
						int fwdDataLen = msg.dataLen;
						int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;		

						// flood to all the neighbours of my node
						performFlooding(fwdBuf, fwdTotalLen, sockfd); */

					}
						break;

					default:
						break;
				} //close of switch
		
		} // end else if(eventtype==CMD_THREAD)

		// COMMANDS GENERATED BY USER/IMPL-SPECIFIC	
		else if(eventtype==KPAV_THREAD)
		{
			//if(findKeyInRoutingTable(msg.UOID)==FALSE)
			//{
				// add this entry into the routing table
				addNodeInRoutingTable(msg.UOID, sockfd);			

			
				switch(msg.msgType)
				{
					case KPAV:         //status request	<statustype>		
					{
						//printf("am inside the KPAV_THREAD EQD HANDLING !!!!!!!!!!!\n ");					
	
						// this message has an empty body (data length = 0)
						unsigned char fwdBuf[MAX_HEADER_LENGTH];
						msg.formMsgHeaderInBuffer(fwdBuf);												

						// put the msg into buffer to pass it for forwarding
						// get the datalen
						int fwdDataLen = msg.dataLen; // should be zero
						int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;		

						// flood to all the neighbours of my node
						performFlooding(fwdBuf, fwdTotalLen, sockfd); 

					}
						break;

					default:
						break;
				} //close of switch
			//} // end if not found in the routing table 
		
		} // end else if(eventtype==KPAV_THREAD)


	
	} // end main while loop

	pthread_exit(NULL);

} // end eventQueueDp_thread




		

