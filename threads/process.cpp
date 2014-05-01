#include "global.h"
#include "iniparserClass.h"
#include "process.h"


/*

PROJECT:
********
- Neighbours exchange HELLO messages - similar to handshaking procedure (2-way). Until hello messages are exchanged, we are not
processing any kind of messages till then and its dropped. 
- Only JOINREQ is accepted without HELLO
- Add new connection object into connList when new connection is acquired by accept() or connect() - server/client threads
- Read thread extracts the hostname and port info from HELLO and pushes as NeighbourNode into Neighbours List
- This is basically to identify who are permanent neighbours and temporary neighbours (as in the case of JOIN)
Read thread examines the msgType of the incoming msg.. if its HELLO, adds that sender node info into Neighbours list and pushes
into eventQueue. It also updates the helloRcvd field in the connection object to decide whether to process any msgs received 
by this socket. If hello is received, it is pushed into the queue directly. One exception where msgs are accepted w/o receving
HELLO is in the case of JOINREQ which denotes temporary connection by temp non-beacon node.

eventDispatcher thread 
the messages 

TODOs:
******
- handle cleanup part for all the objects created explictly
- initialize & destroy all the mutexs
- how to process the cmdline events?
- reset option - delete all the cached and permanent files, log files and init_neighbor_list
- cleanup the shared data structures - eventQueue, Neighbours, connList in the cleanup.cpp

- check the return code after file write/close operations to check whether successfully performed the store operations in permanent space
-> log the error codes

- during status commands, hit ctrl+c, servand prompt appears as "servant:portno>servant:portno>" - fix it

Neigbours
*********
- Work out when to delete neighbour and corresponding connection
- 

MISTAKES
*******
delete(this) in all the .cpps if we are not using pointers to objects
be careful with printfs.. pay attention to warnings and errors thrown by the compiler
*message -> message object its containing strlen(data) == 0
- got seg fault in the destructor when freeing(data) - fix this
- if((myConn->checkHelloRcvd())==FALSE) - never do it. if ur acquiring lock inside this function and some other operation
is also performed inside the if body acquiring the same lock, deadlock occurs
- never make return statements b4 releasing the owning lock
- whenever we push an event, we shud broadcast all the other waiting threads (producer side)
- whenever we are waiting to process an event in the queue, we shud do cond_signal (consumer side)
- (recvmsg.data != NULL) free(recvmsg.data) is causing probz.
*/


using namespace std;

extern int resetOption; //FALSE - no reset, TRUE - reset reqeuest received from the user

//int timeToShutdown=FALSE;

void catch_alarm(int sig)
{
	if(sig==14)
	{
		//printf("Alram went off...time to quit\n");
		timeToShutdown=true;
	}
	if(sig==2)
	{
		//printf("User typed CTRL C\n");
		timeToShutdown=TRUE;
		//shortCircuit = TRUE;
	}
	if(sig==13)
	{
		//printf("client closed the pipe so need to write to the socket which is closed\n");
		timeToShutdown=true;
	}
}



// copied from http://developerweb.net/viewtopic.php?id=3003 source
int sockdataAvailable(int sock) 
{ 
    /*if(PRINT_FLAG)
		printf("checking the server socket data availability\n"); */

    int             res; 
    fd_set          sready; 
    struct timeval  timeout; 

    FD_ZERO(&sready); 
    FD_SET((unsigned int)sock,&sready); 
    /*bzero((char *)&timeout,sizeof(timeout));*/ 
    memset((char *)&timeout,0,sizeof(timeout)); 

	timeout.tv_sec  = 0;
	timeout.tv_usec = SELECT_INTERVAL;

    res = select(sock+1,&sready,NULL,NULL,&timeout); 

    if( FD_ISSET(sock,&sready) ) 
        res = 1; 
    else 
        res = 0; 

    return(res); 
}



/* Initialize mutex and condition variable objects */
void Initialize()
{
	
	pthread_mutex_init(&gMinNeighboursCnt_mutex, NULL);
	pthread_mutex_init(&shortCircuit_mutex, NULL);

	pthread_mutex_init(&cmdLineQueue_mutex, NULL);
	pthread_cond_init(&cmdLineQueue_cv, NULL);

	pthread_mutex_init(&searchidMap_mutex, NULL);
	
	pthread_mutex_init(&msgRT_mutex, NULL);

	pthread_mutex_init(&eventQueue_mutex, NULL);
	pthread_cond_init (&eventQueue_cv, NULL);


	pthread_mutex_init(&connList_mutex, NULL);
	pthread_cond_init (&connList_cv, NULL);

	pthread_mutex_init(&Neighbours_mutex, NULL);
	
	pthread_mutex_init(&log_mutex, NULL);

	pthread_mutex_init(&shutdown_mutex, NULL);

	pthread_mutex_init(&globalFileNum_mutex, NULL);

	pthread_mutex_init(&temp_globalFileNum_mutex, NULL);

	pthread_mutex_init(&kwrdIndex_mutex, NULL);

	pthread_mutex_init(&nameIndex_mutex, NULL);

	pthread_mutex_init(&shaIndex_mutex, NULL);

	pthread_mutex_init(&LRUList_mutex, NULL);	
	

	// todo change it - for implementing externalize property
	globalFileNum = 0;
	temp_globalFileNum = 0;

}

//todo create delete log file and delete init_neighbour_list file functions
void deleteFileUponReset()
{


}

/************************************************** DEFINE RoutingTable Methods **********************************************************/


/* NODE ID & NODE INSTANCE ID GENERATION */
void generateNodeID()
{
	//Create the node ID and the node Instance ID 
	time_t currsecs;
	currsecs = time (NULL);		

	memset(nodeID,0,MAX_NODE_ID_LENGTH);
	sprintf(nodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

	memset(nodeInstanceID,0,MAX_NODE_ID_LENGTH);
	sprintf(nodeInstanceID,"%s_%d_%ld",iniparserInfo->myhostname,iniparserInfo->myport,currsecs);

	//printf(" Node ID : %s\n Node Instance ID :%s\n ",nodeID,nodeInstanceID);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* define the pthread start routine that performs read_thread functionality upon pthread_create call
 todo, set the select interval to keepalive timeout value. when the timer expires, ready to sent KPAV msg
 set some flag to notify the other guy responsible fr KPAV handling
 todo, delete the dynamically allcated data field of msg object
 */
void *read_thread(void *sockfd)
{
	// for receiving data from neighbour node
	unsigned char recvBuf[8000];
	int byte_count = 0;
	unsigned int recvTotalLen = 0;
	unsigned int recvDataLen = 0;
	//Message *recvMsg;

	// handle the busy-waiting for read function - recv()
	struct timeval		recv_timeout;
	struct fd_set		recv_fdset;
	int max_sd = 0;
	int rc = 0;

	// for gettimeofday()
	struct timeval logtime;

	// get the associated sock info from the function parameter
	int newsockfd = (int)sockfd;

	pthread_mutex_lock(&connList_mutex); 
	neighbourConn *myConn = connList[newsockfd];
	//printf("am starting the read thread %d .. my connList size is %d \n", myConn->sockfd, connList.size());
	pthread_mutex_unlock(&connList_mutex); 

	//displayConnList();
	

	//myConn->getConnForSockfd(newsockfd, &myConn);

	/*if(myConn==NULL)
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!********am NULL conn in the read thread %d \n", newsockfd); */
   /*************************************************************/
   /* Loop waiting for incoming packet header on the socket */
   /*************************************************************/
	
	while(1)
	{
		
		//printf("in while loop \n");
		// Call select() and wait 100ms for it to complete.
		//printf("Waiting on select()...\n");
		
	   /*************************************************************/
	   /* Initialize the master fd_set                              */
	   /*************************************************************/
	   FD_ZERO(&recv_fdset);
	   max_sd = newsockfd;
	   FD_SET(newsockfd, &recv_fdset);

		/*************************************************************/
		/* Initialize the timeval struct to 100 ms.  If no        */
		/* activity after 100ms, check if it's time to quit. */
		/* if not, go back to select again till we reach shutdown timeout */
		/*************************************************************/
		recv_timeout.tv_sec  = 0;
		recv_timeout.tv_usec = SELECT_INTERVAL; 
		
		rc = select(max_sd + 1, &recv_fdset, NULL, NULL, &recv_timeout);
		// Check to see if the select call failed.                
		if (rc == ERROR)
		{
			//perror("select() failed!!");
			//printf("select rc = ERROR \n");
			//closingfn(nSocket, clientReq.data, clientResp.data);
			exit(-1);		
		}

		// Check to see if the 100ms time out expired.
		if (rc == 0)
		{
			//printf("  select() timed out.\n");

			// time to quit
			/*if(checkSignalStatus(nSocket, clientReq.data, clientResp.data))
			{
				exit(-1);
			}	*/

			pthread_mutex_lock (&shutdown_mutex);
			// time to shut down is true, free the lock and signal all the sleeping threads
			if(timeToShutdown==TRUE)
			{	
				if(PRINT_FLAG)
					printf("read thread %d is breaking out of the while loop due to autoshutdown \n", newsockfd);

				pthread_mutex_unlock (&shutdown_mutex);
				//close(newsockfd);				
				pthread_exit(NULL);


			}
			pthread_mutex_unlock (&shutdown_mutex);
			
			continue;
		}

		// One descriptor is readable.  Need to  read the incoming data
		if(rc == 1)
		{
			// Check to see if this descriptor is ready 
			if (FD_ISSET(newsockfd, &recv_fdset))
			{
				if ((byte_count = recv(newsockfd, &recvBuf[0], MAX_HEADER_LENGTH, 0)) == ERROR) 
				{
					//printf("Client encountered error while receving packet from Server");
					//closingfn(newsockfd, clientThreadInfo, joinRequest);
					pthread_exit(NULL); 
					
				}					
				else if(byte_count == 0) // broken pipe on client side
				{

					if(PRINT_FLAG)
						printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
					//closingfn(newsockfd, clientThreadInfo, joinRequest);
					pthread_exit(NULL); 							
				}	
				
				else if(byte_count == MAX_HEADER_LENGTH)
				{
					

					// store the received header information in the packet structure format
					Message recvMsg = Message();					
					recvMsg.formMsgHeaderFromBuffer(recvBuf);
					recvDataLen = recvMsg.dataLen;
					recvTotalLen = MAX_HEADER_LENGTH + recvDataLen;	
					
					recvMsg.printHeader();

					// for store msg
					unsigned char sha1_filestr[2*SHA_DIGEST_LEN] = "";
					unsigned int store_filenum = 0; // temporary file num to store the data received - meta/data 
					unsigned int get_filenum = 0; // temporary file num to store the get data received - meta/data 

					//printf("read_thread: received header length belonging to msgtype 0x%02x \n", recvMsg.msgType);

					if(findKeyInRoutingTable(recvMsg.UOID)==FALSE)
					{
					
						if(recvMsg.msgType == STOR)
						{			
							// recv the metadata length
							if ((byte_count = recv(newsockfd, &recvBuf[27], 4, 0)) == ERROR) 
							{
								//printf("Client encountered error while receving packet from Server");
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 
								
							}					
							else if(byte_count == 0) // broken pipe on client side
							{

								if(PRINT_FLAG)
									printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 							
							}	

							unsigned int metaSize = 0;
							unsigned int dataSize = 0;

							memcpy(&metaSize, &recvBuf[27], META_DATA_LEN);		
							dataSize = recvDataLen - META_DATA_LEN  - metaSize;
							//unsigned int recvtillMetaLen = MAX_HEADER_LENGTH + META_DATA_LEN  + metaSize;
							
							if(PRINT_FLAG)
							{
								printf("metasize %d \n", metaSize);
								printf("recvDataLen %d \n", recvDataLen);
								printf("datasize %d \n", dataSize);
								//printf("recvtillMetaLen %d \n", recvtillMetaLen);
							}

							// recv the metadata
							if ((byte_count = recv(newsockfd, &recvBuf[31], metaSize, 0)) == ERROR) 
							{
								//printf("Client encountered error while receving packet from Server");
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 
								
							}					
							else if(byte_count == 0) // broken pipe on client side
							{

								if(PRINT_FLAG)
									printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 							
							}	
							
							recvBuf[31+metaSize] = '\0';

							// recv the actual data
							unsigned int recvCnt = 0;
							unsigned int noOfFragments = dataSize/MAX_BUFFER_SIZE;
							unsigned int remainingBytes = dataSize % MAX_BUFFER_SIZE;		
							unsigned char sha1_file[SHA_DIGEST_LEN];
							

							if(PRINT_FLAG)
								printf("noOfFragments %d , remainingBytes %d \n ", noOfFragments, remainingBytes);

							pthread_mutex_lock(&temp_globalFileNum_mutex);
							temp_globalFileNum++;
							store_filenum = temp_globalFileNum;
							pthread_mutex_unlock(&temp_globalFileNum_mutex);

							// given the filenum, compute the meta file size and data file size
							char dataPath[256] = "";
							sprintf(dataPath, "%s/%d.data", iniparserInfo->myhomedir,store_filenum);

							if(PRINT_FLAG)
								printf("stored file recv across %s \n", dataPath);

							

							SHA_CTX ctx;	
							SHA1_Init(&ctx);
							
							FILE *pFile = fopen((const char *)dataPath,"wb");

							if(pFile==NULL)
							{
								
								printf("Invalid File name or insufficient permissions in formFiledataInBuffer!!!! \n");
							}

							else
							{			
								// recv fragments
								while(recvCnt < noOfFragments)
								{
									usleep(100);

									unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * MAX_BUFFER_SIZE);	
							
									if ((byte_count = recv(newsockfd, &fwdBuf[0], MAX_BUFFER_SIZE, 0)) == ERROR) 
									{
										//printf("Client encountered error while receving packet from Server");
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 
										
									}					
									else if(byte_count == 0) // broken pipe on client side
									{

										if(PRINT_FLAG)
											printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 							
									}	

									fwrite(&fwdBuf[0], 1, MAX_BUFFER_SIZE, pFile);		
									
									SHA1_Update(&ctx,(const void *)&fwdBuf[0],MAX_BUFFER_SIZE);	

									free(fwdBuf);
									recvCnt++;	   	
									
									if(PRINT_FLAG)
										printf("recv count %d - fragments %d \n", recvCnt, byte_count);
								}

								// recv the remaining bytes
								// recv the remaining bytes after data fragments are recv
								if(remainingBytes!=0)
								{

									unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * remainingBytes);								
									if ((byte_count = recv(newsockfd, &fwdBuf[0], remainingBytes, 0)) == ERROR) 
									{
										//printf("Client encountered error while receving packet from Server");
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 
										
									}					
									else if(byte_count == 0) // broken pipe on client side
									{

										if(PRINT_FLAG)
											printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 							
									}	

									fwrite(&fwdBuf[0], 1, remainingBytes, pFile);									
									
									SHA1_Update(&ctx,(const void *)&fwdBuf[0],remainingBytes);	
									free(fwdBuf);	
									
									if(PRINT_FLAG) printf("recv count - remaining bytes %d \n", byte_count);
								}
								
								fclose(pFile);

								SHA1_Final(sha1_file, &ctx);
								
								convert_sha1_to_hex( sha1_file, sha1_filestr);

								if(PRINT_FLAG)
									cout<< "SHA1 value computed from data file is "<<sha1_filestr <<endl;

							} // end if the file can be opened and valid existing file

							byte_count = recvDataLen;

						}
						else if(recvMsg.msgType == GTRS)
						{
							//byte_count = recv(newsockfd, &recvBuf[MAX_HEADER_LENGTH], recvDataLen, 0);
							// recv the UOID + metadata length
							if ((byte_count = recv(newsockfd, &recvBuf[27], 24, 0)) == ERROR) 
							{
								//printf("Client encountered error while receving packet from Server");
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 
								
							}					
							else if(byte_count == 0) // broken pipe on client side
							{

								if(PRINT_FLAG)
									printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 							
							}	

							unsigned int metaSize = 0;
							unsigned int dataSize = 0;

							memcpy(&metaSize, &recvBuf[47], META_DATA_LEN);		
							dataSize = recvDataLen - UOID_LEN - META_DATA_LEN  - metaSize;
							//unsigned int recvtillMetaLen = MAX_HEADER_LENGTH + META_DATA_LEN  + metaSize;
							
							if(PRINT_FLAG)
							{
								printf("metasize %d \n", metaSize);
								printf("recvDataLen %d \n", recvDataLen);
								printf("datasize %d \n", dataSize);
								//printf("recvtillMetaLen %d \n", recvtillMetaLen);
							}

							// recv the metadata
							if ((byte_count = recv(newsockfd, &recvBuf[51], metaSize, 0)) == ERROR) 
							{
								//printf("Client encountered error while receving packet from Server");
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 
								
							}					
							else if(byte_count == 0) // broken pipe on client side
							{

								if(PRINT_FLAG)
									printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								pthread_exit(NULL); 							
							}	
							
							recvBuf[51+metaSize] = '\0';

							// recv the actual data
							unsigned int recvCnt = 0;
							unsigned int noOfFragments = dataSize/MAX_BUFFER_SIZE;
							unsigned int remainingBytes = dataSize % MAX_BUFFER_SIZE;		
							unsigned char sha1_file[SHA_DIGEST_LEN];
							

							if(PRINT_FLAG)
								printf("noOfFragments %d , remainingBytes %d \n ", noOfFragments, remainingBytes);

							pthread_mutex_lock(&temp_globalFileNum_mutex);
							temp_globalFileNum++;
							get_filenum = temp_globalFileNum;
							pthread_mutex_unlock(&temp_globalFileNum_mutex);

							// given the filenum, compute the meta file size and data file size
							char dataPath[256] = "";
							sprintf(dataPath, "%s/%d.data", iniparserInfo->myhomedir,get_filenum);

							if(PRINT_FLAG)
								printf("get file recv across %s \n", dataPath);

							

							SHA_CTX ctx;	
							SHA1_Init(&ctx);
							
							FILE *pFile = fopen((const char *)dataPath,"wb");

							if(pFile==NULL)
							{
								
								printf("Invalid File name or insufficient permissions in formFiledataInBuffer!!!! \n");
							}

							else
							{			
								// recv fragments
								while(recvCnt < noOfFragments)
								{
									usleep(100);

									unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * MAX_BUFFER_SIZE);	
							
									if ((byte_count = recv(newsockfd, &fwdBuf[0], MAX_BUFFER_SIZE, 0)) == ERROR) 
									{
										//printf("Client encountered error while receving packet from Server");
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 
										
									}					
									else if(byte_count == 0) // broken pipe on client side
									{

										if(PRINT_FLAG)
											printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 							
									}	

									fwrite(&fwdBuf[0], 1, MAX_BUFFER_SIZE, pFile);		
									
									SHA1_Update(&ctx,(const void *)&fwdBuf[0],MAX_BUFFER_SIZE);	

									free(fwdBuf);
									recvCnt++;	   	
									
									if(PRINT_FLAG) printf("recv count %d - fragments %d \n", recvCnt, byte_count);
								}

								// recv the remaining bytes
								// recv the remaining bytes after data fragments are recv
								if(remainingBytes!=0)
								{

									unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * remainingBytes);								
									if ((byte_count = recv(newsockfd, &fwdBuf[0], remainingBytes, 0)) == ERROR) 
									{
										//printf("Client encountered error while receving packet from Server");
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 
										
									}					
									else if(byte_count == 0) // broken pipe on client side
									{

										if(PRINT_FLAG)
											printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
										//closingfn(newsockfd, clientThreadInfo, joinRequest);
										pthread_exit(NULL); 							
									}	

									fwrite(&fwdBuf[0], 1, remainingBytes, pFile);									
									
									SHA1_Update(&ctx,(const void *)&fwdBuf[0],remainingBytes);	
									free(fwdBuf);	
									
									if(PRINT_FLAG) printf("recv count - remaining bytes %d \n", byte_count);
								}
								
								fclose(pFile);

								SHA1_Final(sha1_file, &ctx);
								
								convert_sha1_to_hex( sha1_file, sha1_filestr);

								if(PRINT_FLAG)
									cout<< "SHA1 value computed from get file is "<<sha1_filestr <<endl;

							} // end if the file can be opened and valid existing file

							byte_count = recvDataLen;
		
						} 
						// FOR ALL THE OTHER MESSAGES - OTHER THAN GTRS AND STOR
						else
						{
			 
							byte_count = recv(newsockfd, &recvBuf[MAX_HEADER_LENGTH], recvDataLen, 0);			 			
						}


						/* CHECKING THE BYTE_COUNT MATCHES THE SUPPOSED RECVDATA LEN ***/
						if( byte_count == ERROR ) 
						{
							//printf("Client encountered error while receving packet from Server");
							//closingfn(newsockfd, clientThreadInfo, joinRequest);
							pthread_exit(NULL); 
							//continue;
						}					
					
							
						else if(byte_count == 0) // broken pipe on master server
						{
							//printf("read_thread %d: received SIGPIPE signal \n", newsockfd);
							//closingfn(newsockfd, clientThreadInfo, joinRequest);

									if(recvMsg.msgType == KPAV)
									{
										// extract relevant information from the data part
										/*
										Log log = Log();
										char str[100];
										sprintf(str, "read_thread %d: received Data size %d got the KPAV msg", newsockfd, recvMsg.dataLen);
										log.debug_log(str);
										*/

										// creating a log entry
										char rNodeID[MAX_NODE_ID_LENGTH];
										memset(rNodeID,0,MAX_NODE_ID_LENGTH);
										sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

										gettimeofday(&logtime, NULL);

										logKPAVMsg('r', logtime, rNodeID, KPAV, recvTotalLen, recvMsg.TTL, recvMsg.UOID);																			
								
									}
																	
									else
									{
										if(PRINT_FLAG)
											printf("read_thread %d: received SIGPIPE signal after reading packet header\n", newsockfd);

										pthread_exit(NULL); 
									}

							//continue;
						
						}
						// after receiving whole message
						else if(byte_count==(int)recvDataLen)
						{
							//printf("**********!!!!!!!!!!!!!!!!read_thread %d: received Data status of CHECKHELLORCVD %d the hello message !!!!!****\n", newsockfd,myConn->checkHelloRcvd());
							// process only HELLO message and JOIN request when the hello is not yet received in this connection
							if((myConn->checkHelloRcvd())==FALSE)
							{
								// received hello msg contains hostname & port info of the neighbour node, add it into neighbours list
								if(recvMsg.msgType == HLLO)
								{
									//printf("read_thread %d: received Data got the hello message \n", newsockfd);				
									recvBuf[recvTotalLen] = '\0';

									// extract relevant information from the data part
									unsigned short int recv_port = 0;	
									int hostlen = recvDataLen - 2;
									char recv_hostname[50];
									recvMsg.recvHelloMsg(recv_port, recv_hostname, recvBuf);
									recv_hostname[hostlen] = '\0';

									//printf("in read_thread %d: received hello msg.. port %d, hostname %s \n", newsockfd, recv_port, recv_hostname);					
									
									// update the info received in hello message in both the case of S and C connection type
									// updates the hellorecvd
									myConn->UpdateConnInfoForSockfd(recv_hostname, recv_port);
									
									// add the peer info into neighbours list if doesnt exist in the case of connections created by server
									if(myConn->createdBy == 'S')
									{
										// Push the hostname and portnum info gained from new connection myConn into Neighbours List
										NeighbourNode Node = NeighbourNode(recv_hostname, recv_port, newsockfd);
										addNeighbourNode(Node);
										//printf("\n\n DISPLAY NEIGHBOURS \n"); displayNeighbours(); printf("\n");									
																	
									} //end if myConn
									
									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									logHLLOMsg('r', logtime, rNodeID, HLLO, recvTotalLen, recvMsg.TTL, recvMsg.UOID, recv_port, recv_hostname);

									// creating read event and pushing into the event queue
									Event event = Event(recvMsg, newsockfd, MSG_RECEIVED);																
									pushRecvEventInEventQueue(event);													
								
								} // for HLLO msg

								// dun save the joining node info in the neighbour node list as its a temporary joining node
								else if(recvMsg.msgType == JNRQ)
								{
								
									//printf("read_thread %d: received Data got the Join request message \n", newsockfd);				
									recvBuf[recvTotalLen] = '\0';

									// extract relevant information from the data part
									unsigned int recv_location = 0;	
									unsigned short int recv_port = 0;
									int hostlen = recvDataLen - HOST_LOCATION_LEN- HOST_PORT_LEN;
									char recv_hostname[50];
									recvMsg.recvJoinReq(recv_location, recv_port, recv_hostname, recvBuf);
									recv_hostname[hostlen] = '\0';

									//printf("in read_thread %d: received joinreq msg..location %d, port %d, hostname %s \n", newsockfd, recv_location, recv_port, recv_hostname);					

									
									// update the info received in join req message in the case of S - hostname and portno
									pthread_mutex_lock(&connList_mutex); 

									memset(myConn->myhostname, '\0',strlen(recv_hostname));
									memcpy(myConn->myhostname, recv_hostname, strlen(recv_hostname));
									myConn->myhostname[hostlen] = '\0';
									myConn->myport = recv_port;
									myConn->mylocation = recv_location;
									
									pthread_mutex_unlock(&connList_mutex); 

									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									logJNRQMsg('r', logtime, rNodeID, JNRQ, recvTotalLen, recvMsg.TTL, recvMsg.UOID, recv_port, recv_hostname);
									
									// creating read event and pushing into the event queue
									Event event = Event(recvMsg, newsockfd, MSG_RECEIVED);
									pushRecvEventInEventQueue(event);													
								
								} // for JNRQ msg
								
								/* dont process any incoming messages without receiving HELLO from the neighbour node, 
								wait for the next msg to process it*/
								else
								{
								
									continue;
								}
							}

							// simply push the received message into the eventQueue for the QD to process it regardless of msgType
							else
							{
								//printf("read_thread %d: receiving Packets after Hello msg is exchanged in this link\n", newsockfd);				
								// if duplicate HLLO is received for the same existing connection, drop it - both S & C conn types
								if(recvMsg.msgType == HLLO)
								{
									continue;	
								}

								/************* otherwise, we push all the messages in the event queue ***************/

								// JNRQ is forwarded by other established connections
								else if(recvMsg.msgType == JNRQ)
								{
									//printf("read_thread %d: received Data got the Join request message after HELLO\n", newsockfd);				
									recvBuf[recvTotalLen] = '\0';

									// extract relevant information from the data part
									unsigned int recv_location = 0;	
									unsigned short int recv_port = 0;
									int hostlen = recvDataLen - HOST_LOCATION_LEN- HOST_PORT_LEN;
									char recv_hostname[50];
									recvMsg.recvJoinReq(recv_location, recv_port, recv_hostname, recvBuf);
									recv_hostname[hostlen] = '\0';

									pthread_mutex_lock(&connList_mutex); 
									myConn->mylocation = recv_location;
									pthread_mutex_unlock(&connList_mutex); 

									//printf("in read_thread %d: received joinreq msg..location %d, port %d, hostname %s \n", newsockfd, recv_location, recv_port, recv_hostname);					
			
									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									logJNRQMsg('r', logtime, rNodeID, JNRQ, recvTotalLen, recvMsg.TTL, recvMsg.UOID, recv_port, recv_hostname);
									
									// creating read event and pushing into the event queue
									Event event = Event(recvMsg, newsockfd, MSG_RECEIVED);
									pushRecvEventInEventQueue(event);													
								
								
								}

								else if(recvMsg.msgType == JNRS)
								{
									//printf("read_thread %d: received Data got the Join response message after HELLO\n", newsockfd);				
									recvBuf[recvTotalLen] = '\0';

									// extract relevant information from the data part
									unsigned int recv_dist = 0;	
									unsigned short int recv_port = 0;
									int hostlen = recvDataLen - UOID_LEN - DISTANCE_LEN - HOST_PORT_LEN;
									char recv_hostname[50];
									unsigned char recv_UOID[UOID_LEN]; 

									recvMsg.recvJoinResp(recv_UOID, recv_dist, recv_port,recv_hostname, recvBuf);								
									recv_hostname[hostlen] = '\0';
									recvBuf[recvTotalLen] = '\0';
									//printf("read_thread %d: received Data size %d got the join response d_uoid %02x \n", newsockfd, recvMsg.dataLen, recvMsg.d_UOID[19]);

									//memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									//sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport); 

									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									logJNRSMsg('r', logtime, rNodeID, JNRS, recvTotalLen, recvMsg.TTL, recvMsg.UOID, recv_UOID, recv_dist, recv_port, recv_hostname);

									// creating read event and pushing into the event queue
									// having technical difficulty in dispatching it to EventQueue Correctly
									// only forward/sent to that particular link through which this original msg was received initially		
									//printf("findKeyInRoutingTable(unsigned char findUOID[UOID_LEN]) %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n \n ", findKeyInRoutingTable(recvMsg.d_UOID));

									if(findKeyInRoutingTable(recvMsg.UOID)==FALSE)
									{
									
										// add this entry into the routing table
										addNodeInRoutingTable(recvMsg.UOID, newsockfd);	

										// new TTL to be updated with the msg received
										unsigned int fwdTTL = determineFwdTTL(recvMsg.TTL, iniparserInfo->msgTTL);									
										
										if(fwdTTL != 0)
										{
											unsigned char fTTL = (unsigned char)fwdTTL;;	
											memcpy(&recvBuf[21], &fTTL, TTL_LEN);
											recvMsg.formMsgHeaderInBuffer(recvBuf);
											
											//printf("JNRS received... forwarding it... my recvMsg.ttl %d, iniparserInfo->msgTTL %d, fwdTTL %d \n\n", recvMsg.TTL,iniparserInfo->msgTTL, fwdTTL);

											// find the sockfd for forwarding
											int fwdsockfd = getValueFromRoutingTable(recvMsg.d_UOID); 
											//FS_Event event = FS_Event(recvMsg, newsockfd, MSG_FORWARD);
											//pushRecvEventInEventQueue(event);	
											
											pthread_mutex_lock(&connList_mutex); 
											neighbourConn *fwdConn = connList[fwdsockfd];

											//printf("am processing the event belonging to %d .. my connList size is %d \n", fwdConn->sockfd, connList.size());
											pthread_mutex_unlock(&connList_mutex); 
											
											// logging
											char rfsNodeID[MAX_NODE_ID_LENGTH];
											memset(rfsNodeID,0,MAX_NODE_ID_LENGTH);
											sprintf(rfsNodeID,"%s_%d",fwdConn->myhostname,fwdConn->myport);
											//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside pushFwdEventInWriteQueue : rfsNodeID %s \n", rfsNodeID);

											gettimeofday(&logtime, NULL);
											logJNRSMsg('f', logtime, rfsNodeID, JNRS, recvTotalLen, fwdTTL, recvMsg.UOID, recv_UOID, recv_dist, recv_port, recv_hostname);
											
											// construct the event object.. by lookingup the corresponding myConn, sockfd
											FS_Event event = FS_Event(recvBuf, recvTotalLen, fwdsockfd, MSG_FORWARD);
											pushFwdEventInWriteQueue(event, fwdConn);	

											/*pthread_mutex_lock(&fwdConn->writeQueue_mutex);	
											(fwdConn->writeQueue).push(event);
											pthread_cond_broadcast(&fwdConn->writeQueue_cv);
											pthread_mutex_unlock(&fwdConn->writeQueue_mutex); */

										} // forward only if TTL is not zero

									
									}
									
									// dont process it
									else
									{
									
									
									}
									//displayRoutingTable();

									//int fwdsockfd = getValueFromRoutingTable(recvMsg.d_UOID); 
									//printf("!!!!! MY fwdsockfd %d msg.d_UOID %02x\n", fwdsockfd, recvMsg.d_UOID[19]);
									//displayRoutingTable();



								} //if(recvMsg.msgType == JNRS)

								else if(recvMsg.msgType == KPAV)
								{
								
								
								}
								else if(recvMsg.msgType == NTFY)
								{
								
								
								}
								else if(recvMsg.msgType == CKRQ)
								{
								
								
								}
								else if(recvMsg.msgType == CKRS)
								{
								
								
								}
								else if(recvMsg.msgType == STRQ)
								{
									//printf("read_thread %d: received Data got the status reqeust message after HELLO\n", newsockfd);				
									recvBuf[recvTotalLen] = '\0';

									// extract relevant information from the data part
									
									unsigned char status = '\0';
									recvMsg.recvStatusReq(status, recvBuf);

									//printf("in read_thread %d: received status resp msg..type \n", newsockfd);					
			
									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);
									
									if(status == STATUS_NEIGHBOURS)
									{
										logSTRQMsg('r', logtime, rNodeID, STRQ, recvTotalLen, recvMsg.TTL, recvMsg.UOID, STATUS_NEIGHBOURS);
									}
									else if (status == STATUS_FILE)
									{
										logSTRQMsg('r', logtime, rNodeID, STRQ, recvTotalLen, recvMsg.TTL, recvMsg.UOID, STATUS_FILE);
									}
									
									// creating read event and pushing into the event queue
									Event event = Event(recvMsg, newsockfd, MSG_RECEIVED);
									pushRecvEventInEventQueue(event);								
								
								}
								else if(recvMsg.msgType == STRS)
								{
									recvBuf[recvTotalLen] = '\0';

									// decode the received status response
									unsigned char recvUOID[UOID_LEN];
									memcpy(recvUOID, &recvBuf[27], UOID_LEN);

									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									// add this entry into the routing table
									addNodeInRoutingTable(recvMsg.UOID, newsockfd);	
									
									logSTRSMsg('r', logtime, rNodeID, STRS, recvTotalLen, recvMsg.TTL, recvMsg.UOID, recvUOID);

									// chck the status req uoid whether present in the RT, if its then fwd it
									// otherwise push into the cmdLineQueue
									// log and push - two tasks
									if(findKeyInRoutingTable(recvUOID)==FALSE) 
									{	
										//unsigned short int dport = 0;
										//memcpy(&dport, &recvBuf[47], UOID_LEN);
										FS_Event event = FS_Event(recvBuf, recvTotalLen, newsockfd, STATUS_REPONSE);

										pthread_mutex_lock(&cmdLineQueue_mutex);	
										cmdLineQueue.push(event);
										pthread_cond_broadcast(&cmdLineQueue_cv);
										pthread_mutex_unlock(&cmdLineQueue_mutex);	
									

									}
									
									// already seen message, so its a forward route, put it in the Event queue for forwarding
									else
									{
									
										// add this entry into the routing table
										addNodeInRoutingTable(recvMsg.UOID, newsockfd);	

										// new TTL to be updated with the msg received
										unsigned int fwdTTL = determineFwdTTL(recvMsg.TTL, iniparserInfo->msgTTL);									

										if(fwdTTL != 0)
										{
											unsigned char fTTL = (unsigned char)fwdTTL;;	
											
											recvMsg.formMsgHeaderInBuffer(recvBuf);
											memcpy(&recvBuf[21], &fTTL, TTL_LEN);

											//printf("STRS received... forwarding it... my recvMsg.ttl %d, iniparserInfo->msgTTL %d, fwdTTL %d \n\n", recvMsg.TTL,iniparserInfo->msgTTL, fwdTTL);

											// find the sockfd for forwarding
											int fwdsockfd = getValueFromRoutingTable(recvUOID); 
											//FS_Event event = FS_Event(recvMsg, newsockfd, MSG_FORWARD);
											//pushRecvEventInEventQueue(event);	
											
											pthread_mutex_lock(&connList_mutex); 
											neighbourConn *fwdConn = connList[fwdsockfd];

											//printf("am processing the event belonging to %d .. my connList size is %d \n", fwdConn->sockfd, connList.size());
											pthread_mutex_unlock(&connList_mutex); 
											
											// logging
											char rfsNodeID[MAX_NODE_ID_LENGTH];
											memset(rfsNodeID,0,MAX_NODE_ID_LENGTH);
											sprintf(rfsNodeID,"%s_%d",fwdConn->myhostname,fwdConn->myport);
											//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside pushFwdEventInWriteQueue : rfsNodeID %s \n", rfsNodeID);

											gettimeofday(&logtime, NULL);
											logSTRSMsg('f', logtime, rfsNodeID, STRS, recvTotalLen, fwdTTL, recvMsg.UOID, recvUOID);

											// construct the event object.. by lookingup the corresponding myConn, sockfd
											FS_Event event = FS_Event(recvBuf, recvTotalLen, fwdsockfd, MSG_FORWARD);
											pushFwdEventInWriteQueue(event, fwdConn);	

											/*pthread_mutex_lock(&fwdConn->writeQueue_mutex);	
											(fwdConn->writeQueue).push(event);
											pthread_cond_broadcast(&fwdConn->writeQueue_cv);
											pthread_mutex_unlock(&fwdConn->writeQueue_mutex); */

										} // forward only if TTL is not zero

										else
										{
											// dun forward it 
										}
										
									
									} // msg already seen
								
								} // end if(recvMsg.msgType == STRS)

								else if(recvMsg.msgType == STOR)
								{									
									/* delay in adding the routing node is causing trouble as the other thread acquires the lock to find the same uoid and result is 2 nodes with same uoid 
									gets added up, so always add the not-found node immediately. no time lagging.
									*/
									if(findKeyInRoutingTable(recvMsg.UOID)==FALSE)
									{
										// decides whether to forward or not based on the caching decision
										int isforward = TRUE;

										// add this entry into the routing table
										addNodeInRoutingTable(recvMsg.UOID, newsockfd);	

										// extract relevant information from the data part
										Log log = Log();
										char str[100];
										sprintf(str, "read_thread %d: received Data size %d got the STOR msg", newsockfd, recvMsg.dataLen);
										log.debug_log(str);

										// creating a log entry
										char rNodeID[MAX_NODE_ID_LENGTH];
										memset(rNodeID,0,MAX_NODE_ID_LENGTH);
										sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

										gettimeofday(&logtime, NULL);

										logSTORMsg('r', logtime, rNodeID, STOR, recvTotalLen, recvMsg.TTL, recvMsg.UOID);																			

										/* receiving node gets a STORE request, use StoreProb to decide if it should cache a copy of the file
										*/
										unsigned int metaSize = 0;
										unsigned int dataSize = 0;
										unsigned char sha1_readstr[2*SHA_DIGEST_LEN+1] = "";
										//unsigned char sha1_filestr[2*SHA_DIGEST_LEN] = "";
										unsigned int filenum = 0;
										memcpy(&metaSize, &recvBuf[27], META_DATA_LEN);
										dataSize =  recvMsg.dataLen - META_DATA_LEN - metaSize;

										//printf("store request metadata length %d datasize length %d \n", metaSize, dataSize);
									
										// extracting the meta data from 31 - (31+metaSize-1)
										recvMetadataInBuffer(store_filenum, 31, metaSize, recvBuf, sha1_readstr);
										
										// extracting the file data from 31+metaSize - (31+metaSize+dataSize-1)
										//recvFiledataInBuffer(filenum, 31+metaSize, dataSize, recvBuf, sha1_filestr);

										char srcPath[256] = "";
										sprintf(srcPath, "%s/%d.data", iniparserInfo->myhomedir,store_filenum);
										

										if(PRINT_FLAG)
										{
											cout<< "in process.cpp SHA1 value computed from meta read file is "<<sha1_readstr <<endl;
											cout<< "in process.cpp SHA1 value computed from data file is "<<sha1_filestr <<endl;
										}
										
										/* The receiving node of store request compute the sha1 hash value of the file and compare it against the stored file 
										description (in the recv buffer). If it does not match, the should be discarded. The action of caching it r discarding
										does not affect the forwarding activity.
										*/
										if(strcmp((const char*)sha1_readstr, (const char *)sha1_filestr) == 0)
										{
											if(PRINT_FLAG)
												printf("both meta data and file data sha1 are same \n");
											
											
											if(flipCoin(iniparserInfo->storeProb)== TRUE)
											{																					
												if(lru_store_decision(dataSize) == TRUE)
												{
													//printf("going to cache it \n");

													//use globalfilenum counter
													pthread_mutex_lock(&globalFileNum_mutex);
													globalFileNum++;
													filenum = globalFileNum;
													pthread_mutex_unlock(&globalFileNum_mutex);
													
													addNodeInLRUList(filenum, dataSize);
													displayLRUList();

													create_cache_file(filenum, 31, 31+metaSize, metaSize, dataSize, recvBuf, srcPath);


												}
												else
												{
													// do nothing - cannot be cached 
													isforward = FALSE;		
													//printf("not forwarded cuz of lru store cache not satisfied");
												}

											
											} // store prob allows the file to be cached 
											else
											{
												isforward = FALSE;		
												//printf("not forwarded cuz of store prob not satisfied");

											}

										} // if file sha1 and sha1 stored in the meta file matches
										else
										{
											isforward = FALSE;									
											//printf("not forwarded cuz sha1 hash and file hash mismatch");

										}

										/* if the temp file is not cached, then need 2 b deleted */
										removeFile(srcPath);

										/**************** COMPOSE FORWARDING MESSAGES & PUSH INTO CORRESPONDING WRITE QUEUES ****************/							


					
										if(isforward == TRUE)
										{
					pthread_mutex_lock(&storeFileNum_mutex);					
					storeFileNum = filenum;
					//printf("store_filenum for forward is %d \n", storeFileNum);
					pthread_mutex_unlock(&storeFileNum_mutex);

											// new TTL to be updated with the msg received
											unsigned int fwdTTL = determineFwdTTL(recvMsg.TTL, iniparserInfo->msgTTL);									

											if(fwdTTL != 0)
											{
												unsigned char fTTL = (unsigned char)fwdTTL;;	
												
												recvMsg.formMsgHeaderInBuffer(recvBuf);
												memcpy(&recvBuf[21], &fTTL, TTL_LEN);
												performForwarding_prob(recvBuf, recvTotalLen, newsockfd, iniparserInfo->neighborStoreProb);

											} // forward only if TTL is not zero

											else
											{
												// dun forward it 
											}
										}
										else
										{
											// dun forward it											
										}
								
									} // if not found on the routing table, process this message 

								} // end if(recvMsg.msgType == STOR)
								else if(recvMsg.msgType == SHRQ)
								{
									//printf("read_thread %d: received Data got the status reqeust message after HELLO\n", newsockfd);				
									recvBuf[recvTotalLen] = '\0';

									// extract relevant information from the data part
									
									unsigned char searchtype = '\0';

									recvMsg.recvSearchReq(searchtype, recvBuf);
									//searchtype = recvMsg.searchtype;

									//printf("in read_thread %d: received search resp %02x msg type \n", newsockfd, searchtype);					
			
									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);
																																	
									logSHRQMsg('r', logtime, rNodeID, SHRQ, recvTotalLen, recvMsg.TTL, recvMsg.UOID, searchtype, (char *)&recvBuf[28]); 

									// creating read event and pushing into the event queue
									Event event = Event(recvMsg, newsockfd, MSG_RECEIVED);
									pushRecvEventInEventQueue(event);								

									//displayAllIndexNodes();
								} // end if(recvMsg.msgType == SHRQ)
								
								else if(recvMsg.msgType == SHRS)
								{
									recvBuf[recvTotalLen] = '\0';

									// decode the received search response
									unsigned char recvUOID[UOID_LEN];
									memcpy(recvUOID, &recvBuf[27], UOID_LEN);

									if(PRINT_FLAG)
										printf("SHRS response info: recvDataLen %d recvTotalLen %d \n", recvDataLen, recvTotalLen);

									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									// add this entry into the routing table
									// todo check on this whether this addition is really required
									addNodeInRoutingTable(recvMsg.UOID, newsockfd);	
									
									logSHRSMsg('r', logtime, rNodeID, SHRS, recvTotalLen, recvMsg.TTL, recvMsg.UOID, recvUOID);

									// chck the status req uoid whether present in the RT, if its then fwd it
									// otherwise push into the cmdLineQueue
									// log and push - two tasks
									if(findKeyInRoutingTable(recvUOID)==FALSE) 
									{	

										FS_Event event = FS_Event(recvBuf, recvTotalLen, newsockfd, SEARCH_REPONSE);

										pthread_mutex_lock(&cmdLineQueue_mutex);	
										cmdLineQueue.push(event);
										pthread_cond_broadcast(&cmdLineQueue_cv);
										pthread_mutex_unlock(&cmdLineQueue_mutex);										

									}
									
									// already seen message, so its a forward route, put it in the Event queue for forwarding
									else
									{
									
										// add this entry into the routing table
										addNodeInRoutingTable(recvMsg.UOID, newsockfd);	

										// new TTL to be updated with the msg received
										unsigned int fwdTTL = determineFwdTTL(recvMsg.TTL, iniparserInfo->msgTTL);									

										if(fwdTTL != 0)
										{
											unsigned char fTTL = (unsigned char)fwdTTL;;	
											
											recvMsg.formMsgHeaderInBuffer(recvBuf);
											memcpy(&recvBuf[21], &fTTL, TTL_LEN);

											//printf("SHRS received... forwarding it... my recvMsg.ttl %d, iniparserInfo->msgTTL %d, fwdTTL %d \n\n", recvMsg.TTL,iniparserInfo->msgTTL, fwdTTL);

											// find the sockfd for forwarding
											int fwdsockfd = getValueFromRoutingTable(recvUOID); 
											//FS_Event event = FS_Event(recvMsg, newsockfd, MSG_FORWARD);
											//pushRecvEventInEventQueue(event);	
											
											pthread_mutex_lock(&connList_mutex); 
											neighbourConn *fwdConn = connList[fwdsockfd];

											//printf("am processing the event belonging to %d .. my connList size is %d \n", fwdConn->sockfd, connList.size());
											pthread_mutex_unlock(&connList_mutex); 
											
											// logging
											char rfsNodeID[MAX_NODE_ID_LENGTH];
											memset(rfsNodeID,0,MAX_NODE_ID_LENGTH);
											sprintf(rfsNodeID,"%s_%d",fwdConn->myhostname,fwdConn->myport);
											//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside pushFwdEventInWriteQueue : rfsNodeID %s \n", rfsNodeID);

											gettimeofday(&logtime, NULL);
											logSHRSMsg('f', logtime, rfsNodeID, SHRS, recvTotalLen, fwdTTL, recvMsg.UOID, recvUOID);

											// construct the event object.. by lookingup the corresponding myConn, sockfd
											FS_Event event = FS_Event(recvBuf, recvTotalLen, fwdsockfd, MSG_FORWARD);
											pushFwdEventInWriteQueue(event, fwdConn);	

											/*pthread_mutex_lock(&fwdConn->writeQueue_mutex);	
											(fwdConn->writeQueue).push(event);
											pthread_cond_broadcast(&fwdConn->writeQueue_cv);
											pthread_mutex_unlock(&fwdConn->writeQueue_mutex); */

										} // forward only if TTL is not zero

										else
										{
											// dun forward it 
										}
																			
									} // msg already seen								
																
								} // end else if(recvMsg.msgType == SHRS)

								else if(recvMsg.msgType == GTRQ)
								{							
									recvBuf[recvTotalLen] = '\0';

									/* delay in adding the routing node is causing trouble as the other thread acquires the lock to find the same uoid and result is 2 nodes with same uoid 
									gets added up, so always add the not-found node immediately. no time lagging.
									*/
									// not a duplicate message
									if(findKeyInRoutingTable(recvMsg.UOID)==FALSE)
									{
										// add this entry into the routing table
										addNodeInRoutingTable(recvMsg.UOID, newsockfd);	

										//printf("got GTRQ message \n");

										// decode the received search response
										unsigned char fileID[UOID_LEN];
										memcpy(fileID, &recvBuf[27], UOID_LEN);

										if(PRINT_FLAG)
											printf("GTRQ response info: recvDataLen %d recvTotalLen %d \n", recvDataLen, recvTotalLen);

										// extract relevant information from the data part
										Log log = Log();
										char str[100];
										sprintf(str, "read_thread %d: received Data size %d got the GTRQ msg", newsockfd, recvMsg.dataLen);
										log.debug_log(str);

										// creating a log entry
										char rNodeID[MAX_NODE_ID_LENGTH];
										memset(rNodeID,0,MAX_NODE_ID_LENGTH);
										sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

										gettimeofday(&logtime, NULL);

										logGTRQMsg('r', logtime, rNodeID, GTRQ, recvTotalLen, recvMsg.TTL, recvMsg.UOID, fileID);
										
										int filenum = 0;	
										// if the requested fileid belongs to me, then it will be found in the fileid map
										if(findFileIDInFileIDMap(filenum , fileID) == TRUE)
										{
											// compose the get response 
											unsigned char *respBuf;					
											unsigned char reqUOID[UOID_LEN];
											unsigned int respTotalLen = 0;
											unsigned int respDataLen = 0;																	
												

											// decode the received join request						
											memcpy(reqUOID, recvMsg.UOID, UOID_LEN);

											// compose the SEARCH response, then compose the flooding msg for forwarding 
											Message respMsg = Message();
					
											unsigned int metaSize = 0;
											unsigned int dataSize = 0;

											computeStoreMsgSize(filenum, metaSize, dataSize);
										
											// compose the response header
											respDataLen = UOID_LEN +  META_DATA_LEN  + metaSize + dataSize;
											respTotalLen = MAX_HEADER_LENGTH + respDataLen;

											unsigned int resptillMetaLen = MAX_HEADER_LENGTH + UOID_LEN +  META_DATA_LEN  + metaSize;	

											if(PRINT_FLAG)
												printf("GTRQ response info: respDataLen %d respTotalLen %d \n", respDataLen, respTotalLen);

											// form the whole message then later copy into data section starting from 27th byte
											respBuf = (unsigned char*) malloc(resptillMetaLen);
											memset(&respBuf[0], 0, resptillMetaLen);
																						
											respMsg.updateMsgHeader(GTRS, iniparserInfo->msgTTL,respDataLen);
											respMsg.formMsgHeaderInBuffer(respBuf);
											
											respMsg.printHeader();
											
											// compose the response data
											memcpy(&respBuf[27], &reqUOID[0], UOID_LEN);

											// storing the metadata length field in the message: bytes 47-50
											metaSize = htonl(metaSize);
											memcpy(&respBuf[47], &metaSize, META_DATA_LEN);

											// storing the meta data from 51 - (51+metaSize-1)
											formMetadataInBuffer(filenum, 51, metaSize, respBuf);
											
											// storing the file data from 51+metaSize - (51+metaSize+dataSize-1)
											//formFiledataInBuffer(filenum, 51+metaSize, dataSize, respBuf);
							
											// construct the event object
											FS_Event event = FS_Event(respBuf, respTotalLen, newsockfd, MSG_SENT);
											event.filenum = filenum; // in "files" subdir
											pushSentEventInWriteQueue(event, myConn);
										
										
										} // end if(findFileIDInFileIDMap(fileRef , fileID) == TRUE)
										else
										{
											// dun send any response as the fileID does not belong to me
											// just forward it 
										}
										

										/**************** COMPOSE FORWARDING MESSAGES & PUSH INTO CORRESPONDING WRITE QUEUES ****************/							
										

										// new TTL to be updated with the msg received
										unsigned int fwdTTL = determineFwdTTL(recvMsg.TTL, iniparserInfo->msgTTL);									

										if(fwdTTL != 0)
										{
											unsigned char fTTL = (unsigned char)fwdTTL;;	
											
											recvMsg.formMsgHeaderInBuffer(recvBuf);
											memcpy(&recvBuf[21], &fTTL, TTL_LEN);
											performForwarding(recvBuf, recvTotalLen, newsockfd);

										} // forward only if TTL is not zero

										else
										{
											// dun forward it 
										}
								
									} // if not found on the routing table (i.e duplicate msg) process this message 


								} // end if(recvMsg.msgType == GTRQ)

								else if(recvMsg.msgType == GTRS)								
								{																

									int isforward = TRUE;

									// decode the received get response
									unsigned char recvUOID[UOID_LEN];
									memcpy(recvUOID, &recvBuf[27], UOID_LEN);

									if(PRINT_FLAG)
										printf("GTRS response info: recvDataLen %d recvTotalLen %d \n", recvDataLen, recvTotalLen);

									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									// add this entry into the routing table
									// todo check on this whether this addition is really required
									addNodeInRoutingTable(recvMsg.UOID, newsockfd);	
									
									logGTRSMsg('r', logtime, rNodeID, GTRS, recvTotalLen, recvMsg.TTL, recvMsg.UOID, recvUOID);

									// chck the status req uoid whether present in the RT, if its then fwd it
									// otherwise push into the cmdLineQueue
									// log and push - two tasks
									if(findKeyInRoutingTable(recvUOID)==FALSE) 
									{	

										FS_Event event = FS_Event(recvBuf, recvTotalLen, newsockfd, GET_REPONSE);
										event.filenum = get_filenum;
										pthread_mutex_lock(&cmdLineQueue_mutex);	
										cmdLineQueue.push(event);
										pthread_cond_broadcast(&cmdLineQueue_cv);
										pthread_mutex_unlock(&cmdLineQueue_mutex);		

									}


									// already seen message, so its a forward route, put it in the Event queue for forwarding
									else
									{

										// add this entry into the routing table
										addNodeInRoutingTable(recvMsg.UOID, newsockfd);	

										/* get fname, sha1 and nonce to determine whether identical copy 
										already exists in the mini directory  
										*/

										unsigned int metaSize = 0;
										unsigned int dataSize = 0;

										char fname[256];
										unsigned int fileSize = 0;
										unsigned char sha1_readstr[2*SHA_DIGEST_LEN+1] = "";
										unsigned char nonce_readstr[2*SHA_DIGEST_LEN+1] = "";

										memcpy(&metaSize, &recvBuf[47], META_DATA_LEN);
										dataSize =  recvMsg.dataLen - UOID_LEN - META_DATA_LEN - metaSize;

										//printf("get response metadata length %d datasize length %d \n", metaSize, dataSize);
										
										pthread_mutex_lock(&temp_globalFileNum_mutex);
										temp_globalFileNum++;
										unsigned int filenum = temp_globalFileNum;
										pthread_mutex_unlock(&temp_globalFileNum_mutex);

										// extracting the meta data from 51 - (51+metaSize-1)						
							recvMetadataFromBuffer_get(filenum, 51, metaSize, recvBuf, fname, fileSize, sha1_readstr, nonce_readstr);

										char srcPath[256] = "";
										sprintf(srcPath, "%s/%d.data", iniparserInfo->myhomedir,get_filenum);

									/**************** CACHE COPY OF THE FILE BASED ON THE DECISIONS: ****************/							
									/*
									Sequence for treating the GET response in the case of forwarding:
									1. flip a coin on CacheProb to decide if it should cache a copy of the file.
									2. check whether identical copy of the file exists, if it does dont cache it
									3. check whether enough space is available in the cache space to store the new file
									*/									
										if(flipCoin(iniparserInfo->cacheProb)== TRUE)
										{	
											unsigned int dummy = 0;
											if(determineFileIdentical(fname, sha1_readstr, nonce_readstr, dummy)== FALSE)
											{
												if(lru_store_decision(dataSize) == TRUE)
												{
													printf("going to cache it \n");

													//use globalfilenum counter
													pthread_mutex_lock(&globalFileNum_mutex);
													globalFileNum++;
													filenum = globalFileNum;
													pthread_mutex_unlock(&globalFileNum_mutex);
													
													addNodeInLRUList(filenum, dataSize);
													displayLRUList();

													create_cache_file(filenum, 51, 51+metaSize, metaSize, dataSize, recvBuf, srcPath);


												}
												else
												{
													// do nothing - cannot be cached 
													isforward = FALSE;
													//printf("not forwarded cuz of lru store cache not satisfied");
												}
											}
											else
											{
												// do nothing - identical file exists
												isforward = FALSE;
												//printf("not forwarded cuz of identical file exists for GTRS");
											}

										
										} // cache prob allows the file to be cached 
										else
										{
											isforward = FALSE;		
											//printf("not forwarded cuz of cache prob not satisfied");

										}


									/**************** FORWARD MESSAGE BY PUSHING INTO CORRESPONDING WRITE QUEUE ****************/							
										if(isforward == TRUE)
										{
											pthread_mutex_lock(&storeFileNum_mutex);					
											storeFileNum = filenum;
											//printf("store_filenum for forward is %d \n", storeFileNum);
											pthread_mutex_unlock(&storeFileNum_mutex);

											// new TTL to be updated with the msg received
											unsigned int fwdTTL = determineFwdTTL(recvMsg.TTL, iniparserInfo->msgTTL);									

											if(fwdTTL != 0)
											{
												unsigned char fTTL = (unsigned char)fwdTTL;;	
												
												recvMsg.formMsgHeaderInBuffer(recvBuf);
												memcpy(&recvBuf[21], &fTTL, TTL_LEN);

												//printf("GTRS received... forwarding it... my recvMsg.ttl %d, iniparserInfo->msgTTL %d, fwdTTL %d \n\n", recvMsg.TTL,iniparserInfo->msgTTL, fwdTTL);

												// find the sockfd for forwarding
												int fwdsockfd = getValueFromRoutingTable(recvUOID); 
												//printf("GTRS: fwdsockfd %d \n", fwdsockfd);
												
												pthread_mutex_lock(&connList_mutex); 
												neighbourConn *fwdConn = connList[fwdsockfd];

												//printf("am processing the event belonging to %d .. my connList size is %d \n", fwdConn->sockfd, connList.size());
												pthread_mutex_unlock(&connList_mutex); 
												
												// logging
												char rfsNodeID[MAX_NODE_ID_LENGTH];
												memset(rfsNodeID,0,MAX_NODE_ID_LENGTH);
												sprintf(rfsNodeID,"%s_%d",fwdConn->myhostname,fwdConn->myport);
												//printf("********!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!inside pushFwdEventInWriteQueue : rfsNodeID %s \n", rfsNodeID);

												gettimeofday(&logtime, NULL);
												logGTRSMsg('f', logtime, rfsNodeID, GTRS, recvTotalLen, fwdTTL, recvMsg.UOID, recvUOID);

												// construct the event object.. by lookingup the corresponding myConn, sockfd
												FS_Event event = FS_Event(recvBuf, recvTotalLen, fwdsockfd, MSG_FORWARD);
												pushFwdEventInWriteQueue(event, fwdConn);	

											} // forward only if TTL is not zero

											else
											{
												// dun forward it 
											}

										} // can forward
										else
										{
										
											// dun forward it
										}

									} // msg already seen		

								} // end else if(recvMsg.msgType == GTRS)

								else if(recvMsg.msgType == DELT)
								{							
									recvBuf[recvTotalLen] = '\0';

									/* delay in adding the routing node is causing trouble as the other thread acquires the lock to find the same uoid and result is 2 nodes with same uoid 
									gets added up, so always add the not-found node immediately. no time lagging.
									*/
									if(findKeyInRoutingTable(recvMsg.UOID)==FALSE)
									{
										// add this entry into the routing table
										addNodeInRoutingTable(recvMsg.UOID, newsockfd);	

										// extract relevant information from the data part
										Log log = Log();
										char str[100];
										sprintf(str, "read_thread %d: received Data size %d got the DELT msg", newsockfd, recvMsg.dataLen);
										log.debug_log(str);

										// creating a log entry
										char rNodeID[MAX_NODE_ID_LENGTH];
										memset(rNodeID,0,MAX_NODE_ID_LENGTH);
										sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

										gettimeofday(&logtime, NULL);

										logDELTMsg('r', logtime, rNodeID, DELT, recvTotalLen, recvMsg.TTL, recvMsg.UOID);		
										

										/* receiving node decomposes the DELT message to extract the SHA1 and password
										*/
										unsigned int filespecSize = recvMsg.dataLen;
										unsigned char sha1_str[2*SHA_DIGEST_LEN+1] = "";
										unsigned char pwdUOID_hex[2*SHA_DIGEST_LEN+1] = "";
									
										pthread_mutex_lock(&temp_globalFileNum_mutex);
										temp_globalFileNum++;
										unsigned int filenum = temp_globalFileNum;
										pthread_mutex_unlock(&temp_globalFileNum_mutex);
			
										// 1. extracting the filespec data
										recvFileSpecFromBuffer(filenum, 27, filespecSize, recvBuf, sha1_str, pwdUOID_hex);
										
										// 2. get the list of fileref for the sha1_str
										char *sha1_string = (char *)sha1_str;
										unsigned int fileRef[30];
										unsigned int fileCnt = 0;
										unsigned int fileFound = 0;

										fileCnt = getCountForShaKey(sha1_string); //printf("filecnt at process DELT %d \n", fileCnt);
										getValueFromShaIndex (sha1_string, fileRef);

								// 3. check whether nonce generated from pwdUOID_hex matches with any of the stored fileRef nonces
										deleteNoncehit(fileRef, fileCnt, pwdUOID_hex, fileFound);

										/* 4. deleting the fileFound. 
											a. Cachefile - delete cache entry, update cache size and delete from all index nodes
											b. Permanent File - only delete from all the index nodes
										*/

										// check whether the requested file exists in the cache area, if it is, then remove it from the LRU list
										if(findFileInLRUList(fileFound)==TRUE)
										{
											deleteNodeInLRUList(fileFound);
											displayAllIndexNodes();																			
										}

										
										deleteAllIndexNodes(fileFound);
										displayAllIndexNodes();

										// delete the actual files in the mini dir
										deleteFiles(2, fileFound);
								

										/**************** COMPOSE FORWARDING MESSAGES & PUSH INTO CORRESPONDING WRITE QUEUES ****************/							
										
										// new TTL to be updated with the msg received
										unsigned int fwdTTL = determineFwdTTL(recvMsg.TTL, iniparserInfo->msgTTL);									

										if(fwdTTL != 0)
										{
											unsigned char fTTL = (unsigned char)fwdTTL;;	
											
											recvMsg.formMsgHeaderInBuffer(recvBuf);
											memcpy(&recvBuf[21], &fTTL, TTL_LEN);
											performForwarding_prob(recvBuf, recvTotalLen, newsockfd, iniparserInfo->neighborStoreProb);

										} // forward only if TTL is not zero

										else
										{
											// dun forward it 
										}

								
									} // if not found on the routing table, process this message 


								} //  end else if(recvMsg.msgType == DELT)

							} // end if Hello is exchanged in the connection


							
							/**************************** ADDING THE NEW EVENT INTO THE EVENTQUEUE ************************************/
							
							/*Event event = Event(recvMsg, newsockfd, MSG_RECEIVED);
							//printf("*******!!!! acquring lock EVENT JOB INTO EVENT QUEUE - READ THREAD !!!!!!!!!!!! **** \n");
							pthread_mutex_lock(&eventQueue_mutex);					
							printf("*******!!!! ADDING EVENT JOB INTO EVENT QUEUE - READ THREAD !!!!!!!!!!!! **** \n");
							eventQueue.push(event);
							// wake up sleeping server threads
							//printf("*******!!!! SIGNALING BROADCAST TO EVENT QUEUE - READ THREAD QUEUESIZE %d!!!!!!!!!!!! **** \n", eventQueue.size());
							pthread_cond_broadcast(&eventQueue_cv);

							pthread_mutex_unlock(&eventQueue_mutex)*/
							

							// continue waiting for next incoming msg to be pushed into the eventQueue		

							/*if(recvMsg.data != NULL)
								free(recvMsg.data); */
						
							continue; //break;
						} //end else if(byte_count==(int)recvDataLen)
						
							/*if(recvMsg.data != NULL)
								free(recvMsg.data); */

					} //end else if(byte_count == MAX_HEADER_LENGTH)				

				} // end if the received message is not seen
				
			} // End of if (FD_ISSET(i, &recv_fdset))

		} // End of if through selectable descriptors

	} //END WHILE
	
	
	//printf("am quitting the read thread %d \n", newsockfd);
	//while(!timeToShutdown);

	//pthread_exit(NULL);
	/*while(timeToShutdown==TRUE)
	{
		if(recvBuf!=NULL)
			free(recvBuf);

		pthread_exit(NULL);
	}*/		
	pthread_exit(NULL);


} //end read_thread

void *write_thread(void *sockfd)
{
	//for sending data to neighbour node
	unsigned char* sendBuf;
	int byte_count = 0;	
	unsigned int sendTotalLen = 0;
	unsigned int sendDataLen = 0;
	//Message *sendMsg;

	// get the associated sock info from the function parameter
	int newsockfd = (int)sockfd;

	// obtain the corresponding connection object associated with this sockfd
	pthread_mutex_lock(&connList_mutex); 
	neighbourConn *myConn = connList[newsockfd];
	//printf("am starting the write thread belonging to sockfd %d .. my connList size is %d \n", myConn->sockfd, connList.size());
	pthread_mutex_unlock(&connList_mutex); 

	// Create a Hello Message only for the connection initiated by the Client
	// Construct the HELLO Message to be sent to the connected Neighbour Node	
	if(myConn->createdBy == 'C')
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
			//close(newsockfd);
			pthread_exit(NULL); 

		}

		sendMsg.formMsgHeaderInBuffer(sendBuf);
		sendMsg.formHelloMsg(iniparserInfo->myport, iniparserInfo->myhostname, sendBuf);
		sendBuf[sendTotalLen] = '\0'; 

		//printf("write_thread %d: b4 sending data \n", newsockfd);
		myConn->myhostname[strlen(myConn->myhostname)] = '\0';

		// creating a log entry
		char sNodeID[MAX_NODE_ID_LENGTH];
		memset(sNodeID, '\0',MAX_NODE_ID_LENGTH);
		sprintf(sNodeID,"%s_%d",myConn->myhostname,myConn->myport);
		
		// for gettimeofday()
		struct timeval logtime;
		gettimeofday(&logtime, NULL);

		logHLLOMsg('s', logtime, sNodeID, HLLO, sendTotalLen, sendMsg.TTL, sendMsg.UOID, iniparserInfo->myport, iniparserInfo->myhostname);
		
		// send the HELLO message to the neighbour node
		if ((byte_count = send(newsockfd, &sendBuf[0], sendTotalLen, 0)) == ERROR) 
		{
			//perror("Client encountered error while sending actual data to Server");			
			//close(newsockfd);
			pthread_exit(NULL); 
		}	

		if(byte_count == 0) // broken pipe on master server
		{
			//printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
			//close(newsockfd);
			pthread_exit(NULL); 
		
		}		

			
		free(sendBuf);

		/*if(sendMsg.data != NULL)
			free(sendMsg.data);*/

		//printf("In write_thread %d: Finished transmitting hello data to the node #bytes %d\n", newsockfd, byte_count);

	} //end if (myConn->createdBy == 'C')
	

	while(timeToShutdown==FALSE)
	{   
		if(PRINT_FLAG)
			printf("write thread %d is having data to sent \n", newsockfd);

		//printf("!!!!!!!!!!****** INSIDE WRITE THREAD LOOP of socket %d \n\n", newsockfd);	
		pthread_mutex_lock(&myConn->writeQueue_mutex);		
		FS_Event event;
		while ((myConn->writeQueue).empty() && (timeToShutdown==FALSE))
		{		//printf("!!!!!!!!!!****** INSIDE WRITEQUEUE WHILE WAITING FOR JOB \n");	
				pthread_cond_wait(&myConn->writeQueue_cv, &myConn->writeQueue_mutex);
				//pthread_mutex_unlock (&queueMutex);
				//printf("!!!!!!!!!!****** WRITEQUEUE JOB BECOME AVAILABLE WOKE UP FROM COND WAIT \n");
		}
		
		pthread_mutex_lock (&shutdown_mutex);
		// time to shut down is true, free the lock and signal all the sleeping threads
		if(timeToShutdown==TRUE)
		{	
			pthread_mutex_unlock (&shutdown_mutex);
			pthread_mutex_unlock(&myConn->writeQueue_mutex);
			pthread_cond_broadcast(&myConn->writeQueue_cv); //todo required?
			
			if(PRINT_FLAG)
				printf("write thread %d is breaking out of the while loop due to autoshutdown \n", newsockfd);

			pthread_exit(NULL);
		}
		pthread_mutex_unlock (&shutdown_mutex);
		
		// if its not time to shutdown and write Queue of this connection is not empty, process the event job
		// access the front element in the queue for processing
		event = (myConn->writeQueue).front();
		
		// dequeue a job
		(myConn->writeQueue).pop();

		pthread_mutex_unlock(&myConn->writeQueue_mutex);   
		
		/* work on the job */
		if(newsockfd!=event.sockfd)
		{
			pthread_exit(NULL);	
		}					

		pthread_mutex_lock(&print_mutex);
		//printf("***********!!!!!!!!!!!!!!!!!!!!!!dequeue msg from WRITE queue is .... !!!!!!!!!!!********\n");  
		pthread_mutex_unlock(&print_mutex);
		
		sendTotalLen = event.totalLen;			
		
	//	printf("size of sendTotalLen %d size of sendmsg.data %d in WRITE THREAD \n", sendTotalLen, sizeof((const char*)recvBuf));	


		if(event.data[0] == STOR)
		{		
			pthread_mutex_lock(&storeFileNum_mutex);
			unsigned int filenum = storeFileNum;
			pthread_mutex_unlock(&storeFileNum_mutex);	
			
			unsigned int metaSize = 0;
			unsigned int dataSize = 0;

			memcpy(&metaSize, &event.data[27], META_DATA_LEN);
			
			unsigned int fwdDataLen = 0;

			memcpy(&fwdDataLen, &event.data[23], 4);

			if(PRINT_FLAG)
			{
				printf("metasize %d \n", metaSize);
				printf("fwdDataLen %d \n", fwdDataLen);
			}

			dataSize = fwdDataLen - META_DATA_LEN  - metaSize;
			unsigned int fwdtillMetaLen = MAX_HEADER_LENGTH + META_DATA_LEN  + metaSize;
			
			//unsigned int fwdTotalLen = sendTotalLen;
			
			
			unsigned int noOfFragments = dataSize/MAX_BUFFER_SIZE;
			unsigned int remainingBytes = dataSize % MAX_BUFFER_SIZE;					
			
			if(PRINT_FLAG)
				printf("noOfFragments %d , remainingBytes %d \n ", noOfFragments, remainingBytes);

			/****************** SENDING THE HEADER + METADATA ****************/
			// send the message to the neighbour node
			if ((byte_count = send(newsockfd, &event.data[0], fwdtillMetaLen, 0)) == ERROR) 
			{
				//perror("Client encountered error while sending actual data to Server");			
				close(newsockfd);
				pthread_exit(NULL); 
			}	

			if(byte_count == 0) // broken pipe on master server
			{
				if(PRINT_FLAG) printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
				//close(newsockfd);
				pthread_exit(NULL); 
			
			}	

			/****************** SENDING THE ACTUAL FILEDATA ****************/
			unsigned int sendCnt = 0; 
			char dataPath[256] = "";

			// given the filenum, compute the meta file size and data file size
			if(event.eventType == MSG_SENT)
			{	
				//printf("event type is %d \n", event.eventType);
				
				sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir,filenum);

				if(PRINT_FLAG)
					printf("stored file send across %s \n", dataPath);
			}
			
			// given the filenum, compute the meta file size and data file size
			else if(event.eventType == MSG_FORWARD)
			{	
				//printf("event type is %d \n", event.eventType);
				
				sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir,filenum);

				if(PRINT_FLAG)
					printf("stored file fwd across %s \n", dataPath);
			}			
			
			FILE *pFile = fopen((const char *)dataPath,"rb");

			if(pFile==NULL)
			{
				
				printf("Invalid File name or insufficient permissions in write thread!!!! \n");
			}

			else
			{			
				// send fragments
				while(sendCnt < noOfFragments)
				{
					usleep(100);

					unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * MAX_BUFFER_SIZE);	
					fread(&fwdBuf[0], 1, MAX_BUFFER_SIZE, pFile);									

					if ((byte_count = send(newsockfd, &fwdBuf[0], MAX_BUFFER_SIZE, 0)) == ERROR) 
					{
						//perror("Client encountered error while sending actual data to Server");			
						close(newsockfd);
						pthread_exit(NULL); 
					}	

					if(byte_count == 0) // broken pipe on master server
					{
						if(PRINT_FLAG) printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
						//close(newsockfd);
						pthread_exit(NULL); 
					
					}	
					
					free(fwdBuf);
					sendCnt++;	   	
					
					if(PRINT_FLAG)
						printf("send count %d - fragments %d \n", sendCnt, byte_count);
				}

				// send the remaining bytes
				// Transfer the remaining bytes after data fragments are transmitted
				if(remainingBytes!=0)
				{
					usleep(100);

					unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * remainingBytes);	
					fread(&fwdBuf[0], 1, remainingBytes, pFile);									

					if ((byte_count = send(newsockfd, &fwdBuf[0], remainingBytes, 0)) == ERROR) 
					{
						//perror("Client encountered error while sending actual data to Server");			
						close(newsockfd);
						pthread_exit(NULL); 
					}	

					if(byte_count == 0) // broken pipe on master server
					{
						if(PRINT_FLAG) printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
						//close(newsockfd);
						pthread_exit(NULL); 
					
					}			
					if(PRINT_FLAG) printf("send count - remaining bytes %d \n", byte_count);

					free(fwdBuf);
				}
				
				fclose(pFile);

			} // end if the file can be opened and valid existing file


		} // end else if (event.data[0] == STOR)
		
		
		else if (event.data[0] == GTRS)
		{
			pthread_mutex_lock(&storeFileNum_mutex);
			unsigned int filenum = storeFileNum;
			pthread_mutex_unlock(&storeFileNum_mutex);	
			
			
			unsigned int metaSize = 0;
			unsigned int dataSize = 0;

			memcpy(&metaSize, &event.data[47], META_DATA_LEN);
			
			unsigned int fwdDataLen = 0;

			memcpy(&fwdDataLen, &event.data[23], 4);

			if(PRINT_FLAG)
			{
				printf("metasize %d \n", metaSize);
				printf("fwdDataLen %d \n", fwdDataLen);
			}

			dataSize = fwdDataLen - UOID_LEN - META_DATA_LEN  - metaSize;
			unsigned int fwdtillMetaLen = MAX_HEADER_LENGTH + UOID_LEN + META_DATA_LEN  + metaSize;					
			
			unsigned int noOfFragments = dataSize/MAX_BUFFER_SIZE;
			unsigned int remainingBytes = dataSize % MAX_BUFFER_SIZE;					
			
			if(PRINT_FLAG)
				printf("noOfFragments %d , remainingBytes %d \n ", noOfFragments, remainingBytes);

			/****************** SENDING THE HEADER + METADATA ****************/
			// send the message to the neighbour node
			if ((byte_count = send(newsockfd, &event.data[0], fwdtillMetaLen, 0)) == ERROR) 
			{
				//perror("Client encountered error while sending actual data to Server");			
				close(newsockfd);
				pthread_exit(NULL); 
			}	

			if(byte_count == 0) // broken pipe on master server
			{
				if(PRINT_FLAG) printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
				//close(newsockfd);
				pthread_exit(NULL); 
			
			}	

			/****************** SENDING THE ACTUAL FILEDATA ****************/
			unsigned int sendCnt = 0; 
			char dataPath[256] = "";

			// given the filenum, compute the meta file size and data file size
			if(event.eventType == MSG_SENT)
			{
				//printf("event type is %d \n", event.eventType);				
				sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir, event.filenum);

				if(PRINT_FLAG)
					printf("get file send across %s \n", dataPath);
			}

			// given the filenum, compute the meta file size and data file size
			else if(event.eventType == MSG_FORWARD)
			{
				printf("event type is %d \n", event.eventType);				
				sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir, filenum);

				if(PRINT_FLAG)
					printf("get file fwd across %s \n", dataPath);
			}
			
			FILE *pFile = fopen((const char *)dataPath,"rb");

			if(pFile==NULL)
			{
				
				printf("Invalid File name or insufficient permissions in formFiledataInBuffer!!!! \n");
			}

			else
			{			
				// send fragments
				while(sendCnt < noOfFragments)
				{
					usleep(100);

					unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * MAX_BUFFER_SIZE);	
					fread(&fwdBuf[0], 1, MAX_BUFFER_SIZE, pFile);									

					if ((byte_count = send(newsockfd, &fwdBuf[0], MAX_BUFFER_SIZE, 0)) == ERROR) 
					{
						//perror("Client encountered error while sending actual data to Server");			
						close(newsockfd);
						pthread_exit(NULL); 
					}	

					if(byte_count == 0) // broken pipe on master server
					{
						if(PRINT_FLAG) printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
						//close(newsockfd);
						pthread_exit(NULL); 
					
					}	
					
					free(fwdBuf);
					sendCnt++;	   	
					
					if(PRINT_FLAG)
						printf("send count %d - fragments %d \n", sendCnt, byte_count);
				}

				// send the remaining bytes
				// Transfer the remaining bytes after data fragments are transmitted
				if(remainingBytes!=0)
				{
					usleep(100);

					unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * remainingBytes);	
					fread(&fwdBuf[0], 1, remainingBytes, pFile);									

					if ((byte_count = send(newsockfd, &fwdBuf[0], remainingBytes, 0)) == ERROR) 
					{
						//perror("Client encountered error while sending actual data to Server");			
						close(newsockfd);
						pthread_exit(NULL); 
					}	

					if(byte_count == 0) // broken pipe on master server
					{
						if(PRINT_FLAG) printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
						//close(newsockfd);
						pthread_exit(NULL); 
					
					}			
					if(PRINT_FLAG)
						printf("send count - remaining bytes %d \n", byte_count);

					free(fwdBuf);
				}
				
				fclose(pFile);

			} // end if the file can be opened and valid existing file

		}// end else if (event.data[0] == GTRS)

		
		// for all the rest of the messages
		else
		{
			if(PRINT_FLAG)
				printf("sending data other than STOR and GTRS \n");

			// send the message to the neighbour node
			if ((byte_count = send(newsockfd, &event.data[0], sendTotalLen, 0)) == ERROR) 
			{
				//perror("Client encountered error while sending actual data to Server");			
				close(newsockfd);
				pthread_exit(NULL); 
			}	

			if(byte_count == 0) // broken pipe on master server
			{
				if(PRINT_FLAG) printf("write_thread %d: received SIGPIPE signal \n", newsockfd);
				//close(newsockfd);
				pthread_exit(NULL); 
			
			}			
		
		}
		
		//printf("In write_thread %d: Finished transmitting write data to the node #bytes %d\n", newsockfd, byte_count);

		//if(byte_count == sendTotalLen)
			//sendBuf[sendTotalLen] = '\0'; 

	} // end main while

	//printf("am quitting the write thread %d \n", newsockfd);
	//printf("timeToShutdown value %d \n", timeToShutdown);	

	pthread_exit(NULL);

} // end write_thread


// define the pthread start routine that performs server_thread functionality upon pthread_create call
void *server_thread(void *serverThreadDataInfo)
{
	// for socket creation
	int nSocket = 0;
	int serverPort = 0; 

	struct sockaddr_in serv_addr;
	int on = 1;

	int newsockfd = 0;
	struct sockaddr_in cli_addr;
	int cli_len = sizeof(cli_addr);
	//char data[BUFFER];

	int rc; // to store the status returned by socket fns
	void *status;			// for pthread_join operation

	// printing the client IP addr
	char cli_ipaddr[INET_ADDRSTRLEN];

	serverThreadData *serverThreadInfo = (serverThreadData *) malloc (sizeof(serverThreadData));
	memset(serverThreadInfo,0,sizeof(serverThreadData));
	serverThreadInfo = (serverThreadData *)serverThreadDataInfo;
	serverPort = serverThreadInfo->myport;
	free(serverThreadInfo);
	//printf("myserver port no is %d \n", serverPort);
	

	/* ***** SOCKET CREATION..BINDING.. LISTEN & ACCEPT FOR INCOMING CLIENT REQUESTS *****  */	 		
	
	// Create a dynamic TCP port and a TCP socket 
	// Create listening socket
	if((nSocket = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
	{
		perror("Server: Error during server socket creation");
		pthread_exit(NULL);
	}
	
		//printf("socket created !!!! \n");

	//todo copied from test_select. identify the source and include it in the references section.
	// todo check wheather its completely necessary to allow reusability feature
	/*************************************************************/
	/* Allow socket descriptor to be reuseable                   */
	/*************************************************************/
	rc = setsockopt(nSocket, SOL_SOCKET,  SO_REUSEADDR,
				   (char *)&on, sizeof(on));
	if (rc < 0)
	{
		perror("setsockopt() failed");
		close(nSocket);
		pthread_exit(NULL);
	}

	// Perform memset for serv_addr to zero and set the fields in that structure
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort); 
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind the serv_addr 
	if((bind(nSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) == ERROR)
	{
		perror("Server: Error during server binding");
		pthread_exit(NULL);
	}

   /*************************************************************/
   /* Set the listen back log                                   */
   /*************************************************************/
	if((listen(nSocket, MAX_CLIENTS))== ERROR)
	{
		perror("Server: Error while server is listening");
		pthread_exit(NULL);
	}
	// Accept any incoming client requests till AUTOSHUTDOWN event
	// When alarm goes off, perform pthread_join... chck for that condition in child threads, terminate by pthread_exit() system call

	// upon receiving any signals, any incoming client requests should not be accepted
	//while(alarmTriggered + sigintTriggered == 0 )
	while(!timeToShutdown)
	{
		int isSockReady = sockdataAvailable(nSocket);
		if(isSockReady == 1)
		{
			//printf("inside while loop in server_thread \n");
			/* If a system call is interrupted by the specified signal and no data has been
			transferred, the system call will return -1 with the global variable
			errno set to EINTR.  */

			if((newsockfd = accept(nSocket, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_len)) == ERROR)
			{
				/* assume that this condition occurs when the alarm event takes place
				   and the accept() system call gets interrupted. Kernel completes the call
				   by notifying "EINTR" errno. Thus, alarmTriggered must be set to 1.
				*/
				if(errno == EINTR)
				{
					//printf("Inside EINTR ACCEPT \n Alarm triggered value: %d \n", alarmTriggered);
					//printf("SIGINT triggered value : %d \n", sigintTriggered);
					continue;
				}
				else
				{
					//printf("Server: Error while server is accepting requests");
					/*close(nSocket);
					pthread_exit(NULL);	
					*/
					break;
				}
			}
			

			// this thing is not working... dunno y?? need to figure out..
			pthread_mutex_lock(&gMinNeighboursCnt_mutex);
			gMinNeighboursCnt++;
			pthread_mutex_unlock(&gMinNeighboursCnt_mutex);
			

			// print client information
			inet_ntop(cli_addr.sin_family, get_in_addr((struct sockaddr *)&cli_addr), cli_ipaddr, INET_ADDRSTRLEN);
			
			/*if(cli_ipaddr!=NULL) 
				printf("Incoming connection request from Node %s is accepted from port %d at socket#%d \n", cli_ipaddr, cli_addr.sin_port,newsockfd); */
			
			// Fork read/write threads to handle this new connection
			neighbourConn *newConn;
			newConn= new neighbourConn(newsockfd); // calling server-specific Connection constructor
			
			//newConn->displayConnList();

			//todo remove it later when known how to handle pthread_join for server thread
			/****************************************************/
			/*
				void *status;			// for pthread_join operation
				
				// performing joining for server thread
				rc = pthread_join(newConn->readThread, &status);

				if (rc) {
					//printf("ERROR; return code from pthread_join() is %d\n", rc);
					exit(-1);
				 }
				printf("finished pthread_join for readthread in server \n");

				// performing joining for QueueDispatcher thread
				rc = pthread_join(newConn->writeThread, &status);

				if (rc) {
					//printf("ERROR; return code from pthread_join() is %d\n", rc);
					exit(-1);
				 }
				 printf("finished pthread_join for writethread in server \n");

			*/

			/******************************************************/
		}
		else if(isSockReady == 0)
		{		
			pthread_mutex_lock (&shutdown_mutex);

			if(timeToShutdown == TRUE)
			{
				pthread_mutex_unlock (&shutdown_mutex);

				if(PRINT_FLAG)
					printf("server thread is breaking out of the while loop due to autoshutdown \n");

				break; // break out of the loop
			
			}
			pthread_mutex_unlock (&shutdown_mutex);
		
		}
	} // end outer while


	/* performing pthread_join for all the read & write threads associated with all the connections accepted server-side of the svnode
	1. close the socket
	2. do a broadcast to wake up all the threads which are waiting on some condition. 
		- broadcast will make those threads to break out of the while loop and check for timetoShutDown condition and exit itself
	3. do a pthread_join
	*/

	// Lock mutex before operating on neighbourConn objects
	pthread_mutex_lock(&connList_mutex); 

	map<int, neighbourConn*>::iterator it;	

	// examines the contents of connList for any existing connection
	for ( it=connList.begin(); it != connList.end(); it++ )
	{
		
		int sock = (*it).first;
		neighbourConn *myConn = (*it).second;
		
		// perform join only for connections created by Server as Client_thread does the pthread_joining explictly
		if(myConn->createdBy == 'S')
		{
			// 1. close the socket
			close(sock);

			// 2. do cond_broadcast
			
			pthread_mutex_lock(&myConn->writeQueue_mutex);	
			pthread_cond_broadcast(&myConn->writeQueue_cv);
			pthread_mutex_unlock(&myConn->writeQueue_mutex);			

			// 3. do pthread_join
			
			// performing joining for read thread
			rc = pthread_join(myConn->readThread, &status);

			if (rc) {
				//printf("ERROR; return code from pthread_join() is %d\n", rc);
				exit(-1);
			 }
			
			if(PRINT_FLAG)
				printf("finished pthread_join for readthread in server_thread \n");

			// performing joining for write thread
			rc = pthread_join(myConn->writeThread, &status);

			if (rc) {
				//printf("ERROR; return code from pthread_join() is %d\n", rc);
				exit(-1);
			 }
			 if(PRINT_FLAG)
				 printf("finished pthread_join for writethread in server_thread \n");		

		} // end if

	} // end for

	// Unlock mutex after operating on neighbourConn objects
	pthread_mutex_unlock(&connList_mutex);
	
	if(PRINT_FLAG)
		printf("server_thread is exiting !!! \n");

	close(nSocket);

	pthread_exit(NULL); 

} // end server_thread

void closingfn(int nSocket, clientThreadData *clientThreadInfo)
{
	//printf("inside the closing fn !!!! \n");

	// free up malloc memory & kill the thread

	// free the malloc memory
	if(clientThreadInfo != NULL){
		free(clientThreadInfo);
		memset(&clientThreadInfo, '\0', sizeof(clientThreadInfo));
	}
	

	//printf("Closing the client socket\n");
	
	// closing the socket
	close(nSocket);					

}

// thread handling regular node joining in the network
void *tempJoining_thread(void *clientThreadDataInfo)
{

	// client socket descriptor
	int nSocket = 0;

	// socket related variables
	struct sockaddr_in serv_addr;
	int status=0; //len=0;
	//int byte_count = 0;

	// for computing the IP Address of server hostname
	struct sockaddr_in temphe;
	struct hostent *he;


	//for sending data to server
	unsigned char* joinReq;
	int byte_count = 0;
	unsigned int sendTotalLen = 0;
	unsigned int sendDataLen = 0;
	Message joinRequest;


	// for receiving data from server
	unsigned char recvBuf[1000];
	unsigned int recvTotalLen = 0;
	unsigned int recvDataLen = 0;
	Message joinResponse;

	// store the UOID to validate the JOIN response's UOID
	unsigned char UOID_send[UOID_LEN];
	//unsigned char UOID_recv[UOID_LEN];
/*
	// for obtaining the IP address of the server
	struct sockaddr_in peer_addr;
	socklen_t peer_len;
	int peer_port = 0;
*/

	// handle the busy-waiting for read function - recv()
	struct timeval		recv_timeout;
	struct fd_set		recv_fdset;
	int max_sd = 0;
	int rc = 0;


	// retrieving the information
	clientThreadData *clientThreadInfo = (clientThreadData *) malloc (sizeof(clientThreadData));
	memset(clientThreadInfo,0,sizeof(clientThreadData));
	clientThreadInfo = (clientThreadData *)clientThreadDataInfo;
	
	/***** print received information *****/
	/*printf("TEMP CLIENT THREAD RECEIVED FOLLOWING INFORMATION \n");	
	printf("clientThreadInfo->hostport %d \n", clientThreadInfo->hostport);
	printf("clientThreadInfo->hostname %s \n", clientThreadInfo->hostname);
	printf("clientThreadInfo->myServerPort %d \n", clientThreadInfo->myServerPort);
	printf("clientThreadInfo->retry %d \n", clientThreadInfo->retry);
	printf(" clientThreadInfo->msgttl %d \n", clientThreadInfo->msgttl);*/


	//while(1)
	//{
		// create a socket to communicate & transmit data to/from the server
		if((nSocket = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
		{
			perror("Client: Error during client socket creation\n"); 
			exit(1);
		}


		memset(&serv_addr, 0, sizeof(struct sockaddr_in));

		/*********************** Obtain IP Address for the server hostname **************/
		// Compute the IP address of the string host
		he=gethostbyname((const char*)clientThreadInfo->hostname);
		//printf("in client thread.. hostname to be connected to %s at port %d \n", clientThreadInfo->hostname, clientThreadInfo->hostport);
		if(he == NULL)
		{
			perror("Client: host does not exist\n");
			free(clientThreadDataInfo);
			exit(1);			
		}
		else
		{
			temphe.sin_addr=*((struct in_addr *)he->h_addr);															
			//Display the IP Address
			//printf("IP Address of server host: %s\n",inet_ntoa(temphe.sin_addr)); 									
		}


		/******* ESTABLISH CONNECTIVITY ************/
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(clientThreadInfo->hostport); //update with the neighbouring node portno got from initconfig file
		serv_addr.sin_addr.s_addr = inet_addr(inet_ntoa(temphe.sin_addr)); //inet_addr("127.0.0.1"); - only for localhost

		// client establishes a connection with the beacon node
		if((status = connect(nSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) == ERROR)
		{
			//printf("Attempt to connect to %s at port %d FAILED attempting retry in %d sec. \n",clientThreadInfo->hostname,clientThreadInfo->hostport,clientThreadInfo->retry);
			
			// sleep for the retry time interval
			//sleep(clientThreadInfo->retry);
			exit(1);
		}

		// connect is successful in contacting the Beacon Node
		else
		{
			isConnToBeaconNode = TRUE;

			//printf("Temp Client: connected to beacon node at %d port...... status %d  !!!!!\n", clientThreadInfo->hostport, status);

			/**************************************** SENDING DATA *************************************************/

			// Construct the JOIN request to sent to the connected Beacon Node
			sendTotalLen = MAX_HEADER_LENGTH + HOST_LOCATION_LEN + HOST_PORT_LEN + strlen(iniparserInfo->myhostname);
			sendDataLen = HOST_LOCATION_LEN + HOST_PORT_LEN + strlen(iniparserInfo->myhostname);

			joinReq = (unsigned char*) malloc(sizeof(unsigned char) *sendTotalLen);					

			if(joinReq == NULL)
			{
				//printf("tempJoining_thread: Failure in Allocating Dynamic Memory \n");

				// free up malloc memory & kill the thread
				closingfn(nSocket, clientThreadInfo);
				pthread_exit(NULL); 

			}

			joinRequest = Message();
			joinRequest.updateMsgHeader(JNRQ, iniparserInfo->msgTTL ,sendDataLen);
			joinRequest.printHeader();
			//printf("message: msgtype %d, ttl %d, datalen %d \n", joinRequest->msgType, joinRequest->TTL, joinRequest->dataLen);

			// store the UOID generated for the join request to validate it against the join responses 
			// received from the other nodes in the network
			memcpy(&UOID_send[0], &joinRequest.UOID[0], UOID_LEN);
			
			joinRequest.formMsgHeaderInBuffer(joinReq);
			joinRequest.formJoinReq(iniparserInfo->myinitLocation, iniparserInfo->myport, iniparserInfo->myhostname, joinReq);

			// creating a log entry
			char sNodeID[MAX_NODE_ID_LENGTH];
			memset(sNodeID, '\0',MAX_NODE_ID_LENGTH);
			sprintf(sNodeID,"%s_%d", clientThreadInfo->hostname, clientThreadInfo->hostport);
			
			// for gettimeofday()
			struct timeval logtime;
			gettimeofday(&logtime, NULL);

			logJNRQMsg('s', logtime, sNodeID, JNRQ, sendTotalLen, joinRequest.TTL, joinRequest.UOID, iniparserInfo->myport, iniparserInfo->myhostname);
				
			//printf("tempJoining_Thread: b4 sending data \n");
			// send the JOIN request to the beacon node
			if ((byte_count = send(nSocket, &joinReq[0], sendTotalLen, 0)) == ERROR) 
			{
				//perror("Client encountered error while sending actual data to Server");			
				closingfn(nSocket, clientThreadInfo);
				pthread_exit(NULL); 
			}	

			if(byte_count == 0) // broken pipe on master server
			{
				//printf("Client received SIGPIPE signal \n");
				closingfn(nSocket, clientThreadInfo);
				pthread_exit(NULL); 
			
			}		
			

			//printf("In tempJoining_Thread: Finished transmitting join data to the node server side \n");
		
			//printf("In tempJoining_Thread: Waiting for join responses from the other nodes \n");
					

			/**************************************** RECEIVING DATA *************************************************/
			// todo whether this timeout initialization needs to be performed inside the while loop
	
			//todo shift the code up which handles the select timeout initialization with the read fdset
			
			// while loop receiving join responses 

			// list that stores all the joining responses
			nbrList joinList = nbrList();
			int joinTimeout = FALSE;
			int joinTimer = iniparserInfo->joinTimeout;

		   /*************************************************************/
		   /* Loop waiting for incoming data in packet on the socket */
		   /*************************************************************/
			while (!joinTimeout)
			{
				// Call select() and wait 100ms for it to complete.
				//printf("Waiting on select()...\n");

			   /*************************************************************/
			   /* Initialize the master fd_set                              */
			   /*************************************************************/
			   FD_ZERO(&recv_fdset);
			   max_sd = nSocket;
			   FD_SET(nSocket, &recv_fdset);


				/*************************************************************/
				/* Initialize the timeval struct to 100 ms.  If no        */
				/* activity after 100ms, check if it's time to quit. */
				/* if not, go back to select again till we reach shutdown timeout */
				/*************************************************************/
				recv_timeout.tv_sec  = 2; //iniparserInfo->joinTimeout; 
				recv_timeout.tv_usec = 0;  //todo change it to joinTimeout in iniparserInfo
				//printf("jointimeout: %d jointimer remaining %d \n", iniparserInfo->joinTimeout, joinTimer);

				rc = select(max_sd + 1, &recv_fdset, NULL, NULL, &recv_timeout);

				// Check to see if the select call failed.                
				if (rc == ERROR)
				{
					//perror("  select() failed");
					closingfn(nSocket, clientThreadInfo);
					pthread_exit(NULL); 
				}

				// Check to see if the join timeout expired.
				else if (rc == 0) //todo fix it properly
				{
					//printf("  select() timed out.  End program. remaining \n");

					// for each select timeout interval, decrement the timer value by 2secs - select timeout 
					joinTimer -= 2;
					
					if(joinTimer <= 0)
						joinTimeout = TRUE;

					// time to quit
					/*if(checkSignalStatus(nSocket, clientReq.data, clientResp.data))
					{
						exit(-1);
					}

					continue;
					*/
					continue; //break;
				}
				// One descriptor is readable. Need to read the incoming data
				else if(rc == 1)
				{

					// Check to see if this descriptor is ready 
					if (FD_ISSET(nSocket, &recv_fdset))
					{
						//printf("client socket is readable\n");		
						memset(recvBuf,0,sizeof(recvBuf));

						if ((byte_count = recv(nSocket, &recvBuf[0], MAX_HEADER_LENGTH, 0)) == ERROR) 
						{
							//printf("Client encountered error while receving packet from Server");
							closingfn(nSocket, clientThreadInfo);
							pthread_exit(NULL); 
						}					
			

						
						//printf("hey recv byte_count value in data part : %d \n", byte_count);

						if(byte_count == 0) // broken pipe on master server
						{
							//printf("Client received SIGPIPE signal \n");
							closingfn(nSocket, clientThreadInfo);
							pthread_exit(NULL); 
						
						}		
						   	   

						// time to quit - check whether jointimed out or time for autoshutdown
						/*if(checkSignalStatus(nSocket, clientReq.data, clientResp.data))
						{
							exit(-1);
						}
						*/

						else if(byte_count == MAX_HEADER_LENGTH)
						{
							//printf("read_thread: received header length \n");

							// store the received header information in the packet structure format
							Message recvMsg = Message();					
							recvMsg.formMsgHeaderFromBuffer(recvBuf);
							recvDataLen = recvMsg.dataLen;
							recvTotalLen = MAX_HEADER_LENGTH + recvDataLen;	

							recvMsg.printHeader();
							
							if ((byte_count = recv(nSocket, &recvBuf[MAX_HEADER_LENGTH], recvDataLen, 0)) == ERROR) 
							{
								//printf("Client encountered error while receving packet from Server");
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								//pthread_exit(NULL); 
								//continue;
							}					
						
								
							else if(byte_count == 0) // broken pipe on master server
							{
								//printf("read_thread %d: received SIGPIPE signal \n", nSocket);
								//closingfn(newsockfd, clientThreadInfo, joinRequest);
								//pthread_exit(NULL); 
								//continue;
							
							}
							// after receiving whole message
							else if(byte_count==(int)recvDataLen && recvMsg.msgType==JNRS)
							{
								// process the JOIN response

								//printf("tempJoining_thread %d: received Data got the Join response message \n", nSocket);				
								recvBuf[recvTotalLen] = '\0';

								// extract relevant information from the data part
								unsigned int recv_dist = 0;	
								unsigned short int recv_port = 0;
								int hostlen = recvDataLen - UOID_LEN - DISTANCE_LEN - HOST_PORT_LEN;
								char recv_hostname[50];
								unsigned char recv_UOID[20]; 

								recvMsg.recvJoinResp(recv_UOID, recv_dist, recv_port,recv_hostname, recvBuf);
								//recvMsg.recvJoinReq(recv_location, recv_port, recv_hostname, recvBuf);
								
								recv_hostname[hostlen] = '\0';

								//printf("in tempJojning_thread %d: received joinresp msg..dist %d, port %d, hostname %s \n", nSocket, recv_dist, recv_port, recv_hostname);			
								
								// check send_UOID == recv_UOID
								//if(memcmp((const char *)UOID_send, (const char *)recv_UOID, UOID_LEN)==0)
								if(memcmp(UOID_send,recv_UOID, UOID_LEN)==0)
								{
									// push the joining node into sorting list
									Node n1(recv_hostname, recv_port, recv_dist); 
									joinList.addNode(n1);
									//joinTimeout = TRUE; //todo be changed

									// Log the Join Response
									// creating a log entry
									char rNodeID[MAX_NODE_ID_LENGTH];
									memset(rNodeID,0,MAX_NODE_ID_LENGTH);
									sprintf(rNodeID,"%s_%d",iniparserInfo->myhostname,iniparserInfo->myport);

									gettimeofday(&logtime, NULL);

									logJNRSMsg('r', logtime, rNodeID, JNRS, recvTotalLen, recvMsg.TTL, recvMsg.UOID, UOID_send, recv_dist, recv_port, recv_hostname);

								} //end verifying send and recv uoid
								

							}
							
				
						} //end if(byte_count == MAX_HEADER_LENGTH)
						
					} // End of if (FD_ISSET(i, &recv_fdset))
				} // End of if through selectable descriptors

			} //end while for receiving joining responses
			
			// after the join timeout, sort the list and write the contents into init_neighbour_list
			joinList.delNodes();
			joinList.write_init_file();

			//break;
			
		} // end else connect is successful

	//} //end while loop till the connection is established
	

	closingfn(nSocket, clientThreadInfo);
	pthread_exit(NULL); 

}


// todo replace all the exit with free(struct) and pthread_exit(NULL) and/or close(socket)
// define the pthread start routine that performs client_thread functionality upon pthread_create call
void *client_thread(void *clientThreadDataInfo)
{
	// client socket descriptor
	int nSocket = 0;

	// socket related variables
	struct sockaddr_in serv_addr;
	int status=0; //len=0;
	//int byte_count = 0;

	// for computing the IP Address of server hostname
	struct sockaddr_in temphe;
	struct hostent *he;

/*
	//for sending data to server
	unsigned char sendHeader[HEADERSIZE];
	unsigned int sendCnt = 0;
	unsigned int sendDataLen = 0;

	// for receiving data from server
	unsigned char recvHeader[HEADERSIZE];
	unsigned int recvCnt = 0;
	unsigned int recvDataLen = 0;


	// for obtaining the IP address of the server
	struct sockaddr_in peer_addr;
	socklen_t peer_len;
	int peer_port = 0;

	// handle the busy-waiting for read function - recv()
	struct timeval		recv_timeout;
	struct fd_set		recv_fdset;
	int max_sd = 0;
	int rc = 0;
*/

	// retrieving the information
	clientThreadData *clientThreadInfo = (clientThreadData *) malloc (sizeof(clientThreadData));
	memset(clientThreadInfo,0,sizeof(clientThreadData));
	clientThreadInfo = (clientThreadData *)clientThreadDataInfo;
	
	/***** print received information *****/
	/*printf("CLIENT THREAD RECEIVED FOLLOWING INFORMATION \n");	
	printf("clientThreadInfo->hostport %d \n", clientThreadInfo->hostport);
	printf("clientThreadInfo->hostname %s \n", clientThreadInfo->hostname);
	printf("clientThreadInfo->myServerPort %d \n", clientThreadInfo->myServerPort);
	printf("clientThreadInfo->retry %d \n", clientThreadInfo->retry);
	printf(" clientThreadInfo->msgttl %d \n", clientThreadInfo->msgttl); */


	while(1)
	{
		// create a socket to communicate & transmit data to/from the server
		if((nSocket = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
		{
			perror("Client: Error during client socket creation\n"); 
			exit(1);
		}


		memset(&serv_addr, 0, sizeof(struct sockaddr_in));

		/*********************** Obtain IP Address for the server hostname **************/
		// Compute the IP address of the string host
		he=gethostbyname((const char*)clientThreadInfo->hostname);
		//printf("in client thread.. hostname to be connected to %s at port %d \n", clientThreadInfo->hostname, clientThreadInfo->hostport);
		if(he == NULL)
		{
			perror("Client: host does not exist\n");
			exit(1);			
		}
		else
		{
			temphe.sin_addr=*((struct in_addr *)he->h_addr);															
			//Display the IP Address
			//printf("IP Address of server host: %s\n",inet_ntoa(temphe.sin_addr)); 									
		}


		/******* ESTABLISH CONNECTIVITY ************/
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(clientThreadInfo->hostport); //update with the neighbouring node portno got from initconfig file
		serv_addr.sin_addr.s_addr = inet_addr(inet_ntoa(temphe.sin_addr)); //inet_addr("127.0.0.1"); - only for localhost

		if(checkConnStatus(clientThreadInfo->hostname, clientThreadInfo->hostport)== TRUE)
		{
			//printf("!!!!!!!!!!!!!!Attempt to connect to %s at port %d FAILED connection EXIST !!!!! \n",clientThreadInfo->hostname,clientThreadInfo->hostport);
			free(clientThreadInfo);
			close(nSocket);
			pthread_exit(NULL);
		}
		// client establishes a connection with the beacon node
		if((status = connect(nSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) == ERROR)
		{
			//printf("Attempt to connect to %s at port %d FAILED attempting retry in %d sec. \n",clientThreadInfo->hostname,clientThreadInfo->hostport,clientThreadInfo->retry);
			
			// sleep for the retry time interval
			if(iniparserInfo->checkNodeType()==BEACON)	// Forking client threads for each beacon nodes
			{
				pthread_mutex_lock (&shutdown_mutex);
				if(timeToShutdown == TRUE)
				{		
					pthread_mutex_unlock (&shutdown_mutex);

					close(nSocket);

					// free up malloc memory
					free(clientThreadInfo);

					if(PRINT_FLAG)
						printf("client thread is exiting from the retry attempt!!!! \n");


					pthread_exit(NULL); 
				}
				pthread_mutex_unlock (&shutdown_mutex);


				sleep(clientThreadInfo->retry);		//printf("waiting \n");
			}
			
			/*
			if((checkConnStatus(clientThreadInfo->hostname, clientThreadInfo->hostport)) == TRUE)
			{	
				printf("Attempt to connect to %s at port %d FAILED connection EXIST !!!!! \n",clientThreadInfo->hostname,clientThreadInfo->hostport,clientThreadInfo->retry);
				free(clientThreadInfo);
				pthread_exit(NULL);
				
			}
			*/
			//exit(1);
		}
		else
		{	
			// todo this is not working dunno y ?? need to figure out
			pthread_mutex_lock(&gMinNeighboursCnt_mutex);
			gMinNeighboursCnt++;
			pthread_mutex_unlock(&gMinNeighboursCnt_mutex);


			//printf("Client: connected with %d port...... status %d, socket# %d \n !!!!!", clientThreadInfo->hostport, status, nSocket);
			break;
		}

	} //end while loop till the connection is established

	
	neighbourConn *newConn;
	newConn = new neighbourConn(clientThreadInfo->hostname, clientThreadInfo->hostport, nSocket); // calling client-specific Connection constructor
	//newConn->displayConnList();

	// Push the hostname and portnum info into Neighbours List as client knows abt its neighbour 
	NeighbourNode Node = NeighbourNode(clientThreadInfo->hostname,  clientThreadInfo->hostport, nSocket);
	addNeighbourNode(Node);
	//printf("\n\n DISPLAY NEIGHBOURS at client side \n"); displayNeighbours(); printf("\n");	



	while(1)
	{
		pthread_mutex_lock (&shutdown_mutex);
		if(timeToShutdown == TRUE)
		{
			pthread_mutex_unlock (&shutdown_mutex);

			// free up malloc memory
			free(clientThreadInfo);

			close(nSocket);

			void *joinStatus;			// for pthread_join operation
			int rc;


			pthread_mutex_lock(&newConn->writeQueue_mutex);	
			pthread_cond_broadcast(&newConn->writeQueue_cv);
			pthread_mutex_unlock(&newConn->writeQueue_mutex);	

			// performing joining for client-read thread
			rc = pthread_join(newConn->readThread, &joinStatus);

			if(PRINT_FLAG)
				printf("finished pthread_join for readthread in client_thread \n");

			if (rc) {
			//printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
			}


			// performing joining for client-write thread
			rc = pthread_join(newConn->writeThread, &joinStatus);

			if(PRINT_FLAG)
				printf("finished pthread_join for writethread in client_thread \n");

			if (rc) {
			//printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
			}


			if(PRINT_FLAG)
				printf("client thread is exiting after joining from read/write threads !!!! \n");


			pthread_exit(NULL); 

		} // end if(timeToShutdown == TRUE)

		pthread_mutex_unlock (&shutdown_mutex);

		// if not time to shutdown, sleep
		usleep(100);
		

	} // end while loop for shuting down condition



} // end client_thread

// it will set the shortCircuit true, so whichever threads need to exit upon this condition, needs to chck it. 
// add all the broadcast on those threads dependent on ctrl+c
void sigint_handler(int signal)
{
	pthread_mutex_lock(&shortCircuit_mutex);
	shortCircuit = TRUE;
	pthread_mutex_unlock(&shortCircuit_mutex);
	
	pthread_cond_broadcast(&cmdLineQueue_cv);     // this will signal on thread that is waiting for messages in status replies
	
	printf("\nservant: %d>", iniparserInfo->myport); //port is read from iniconfig file

}

void *sigint_thread(void *threadid)
{
	while(1)
	{
		pthread_mutex_lock (&shutdown_mutex);
		if(timeToShutdown == TRUE)
		{
			pthread_mutex_unlock (&shutdown_mutex);
			if(PRINT_FLAG)
				printf("sigint_thread is exiting \n");

			pthread_exit(NULL);
		}
		pthread_mutex_unlock (&shutdown_mutex);

		usleep(100);
		
		signal(SIGINT, sigint_handler);
	}

	pthread_exit(NULL);
}

void *KeepAlive_thread(void* th)
{
	struct timeval tv;
	int rc = 0;


	pthread_mutex_lock (&shutdown_mutex);
	while (!timeToShutdown)
	{
		// Call select() and wait 100ms for it to complete.
		//printf("Waiting on select()...\n");
		pthread_mutex_unlock (&shutdown_mutex);

		tv.tv_sec = (iniparserInfo->keepAliveTimeout / 2);
		tv.tv_usec = 0;
		
		//wait for the keepalive timeout, then take the necessary actions
		rc = select(0, NULL, NULL, NULL, &tv);   

		// Check to see if the select call failed.                
		if (rc == ERROR)
		{						
			pthread_exit(NULL); 
		}

		/* Check to see if the select timeout expired.  If expired, its time to flood the KPAV to all the neighbours */
		else if (rc == 0) 
		{
			//printf("  select() timed out.  End program. remaining in kpav.. timeout interval %d \n", (iniparserInfo->keepAliveTimeout / 2));
			
			// if its time to shutdown, then terminate the thread and dont sent keep alive message
			pthread_mutex_lock (&shutdown_mutex);
			if(timeToShutdown == TRUE) { 
				pthread_mutex_unlock (&shutdown_mutex);

				if(PRINT_FLAG)
					printf("keep alive thread is exiting \n");
				
				pthread_exit(NULL);
			}
			pthread_mutex_unlock (&shutdown_mutex);

			// compose KPAV msg, dispatch it into EventQueue, for EVQD to process it 
			Message msg = Message();
			msg.updateMsgHeader(KPAV, 1, 0);

			// creating read event and pushing into the event queue
			Event event = Event(msg, 0, KPAV_THREAD); // 0 - PROGRAM-SPECIFIC GENERATED messages
			pushRecvEventInEventQueue(event);	

			//continue;
		}

		pthread_mutex_lock (&shutdown_mutex);

	} //end while loop (timetoshutdown)
	
	pthread_mutex_unlock (&shutdown_mutex);

	if(PRINT_FLAG) printf("keep alive thread is exiting \n");

	pthread_exit(NULL);
}


int Process()//iniparserClass *iniparserInfo)
{
	//alarm(iniparserInfo->autoShutDown);
	//printf("inside process\n");

	signal(SIGPIPE, SIG_IGN);

	// pthreads creation
	pthread_attr_t attr;
	int rc;	
	void *status;			// for pthread_join operation

	// for timer thread 
	pthread_t timerThread;

	// for sigint thread 
	pthread_t sigintThread;
	
	// for command thread	
	pthread_t commandThread;

	// for keepalive thread	
	pthread_t keepaliveThread;

	// for queue dispatcher thread
	//serverThreadData serverThreadInfo;
	pthread_t queueDpThread;

	// for server thread
	serverThreadData serverThreadInfo;
	pthread_t serverThread;

	// for client threads
	int	bsize = iniparserInfo->noOfBeacons;
	pthread_t clientThread[bsize-1];
	int tcntr = 0; // thread counter

	// for temporary client thread
	pthread_t tmpClientThread;


	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// create a delete function to perform all the file deletions
	//The log file should be deleted only when a node is started with the -reset commandline option. 
	/* todo If -reset is specified, you must first reset this node to the state as if it has never been started before. 
	This includes deleting the init_neighbor_list file, the log file, all cached and permanent files, etc. */
	if(resetOption == TRUE)
	{
		//delete init_neighbour_file
		if(init_file_exist()==TRUE)
		{
			del_init_neighbour_file();
		}

		// delete log file
		del_Log();
	
		// delete all the permanent and cached files in node's home dir - "files" subdirectory
		deleteAllFiles();

		// delete index and lru files
		deleteAllExtIndexs();

		globalFileNum = 0;
		
		displayAllIndexNodes();
	}
	else
	{
		readAllExtIndexs();
	}

	if(PRINT_FLAG)
	{
		printf("status of kwrd_index is %d \n", kwrd_index_exist());
		printf("status of name_index is %d \n", name_index_exist());
		printf("status of sha1_index is %d \n", sha1_index_exist());
		printf("status of lru_index is %d \n", lru_index_exist());
	}

	
	
	if(PRINT_FLAG)
	{
		printf("globalFileNum : %d \n" , globalFileNum);
		displayAllIndexNodes();
		displayLRUList();
	}

	generateNodeID();

	// for autoshutdown functionality, to manage all the timers
	// create timer thread 
	rc = pthread_create(&timerThread, &attr, timer_thread, NULL);

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}

	// create sigint thread 
	rc = pthread_create(&sigintThread, NULL, sigint_thread, NULL);

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}	


	// create command thread 
	rc = pthread_create(&commandThread, &attr, cmdThread, (void *) 0); 

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}		

	// for keepalive thread	
	rc = pthread_create(&keepaliveThread, &attr, KeepAlive_thread, (void *) 0); 

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}	


	cacheSize = iniparserInfo->cacheSize * 1024; 
	do
	{
		if(PRINT_FLAG) printf("entering the main loop in process \n");

		//The global variables to maintain for restart decision
		restartValid = FALSE;
		initNeighbourFilePresent = FALSE;
		gMinNeighboursCnt = iniparserInfo->minNeighbors;

		// Forking queue dispatcher thread to handle all the new incoming message requests	
		//printf("Process: creating queue dispatcher thread \n");
		rc = pthread_create(&queueDpThread, &attr, eventQueueDp_thread, (void *) 0); 

		if (rc) {
			//printf("ERROR; return code from pthread_create()is %d\n", rc);
			exit(-1);
		}		


		// construct argument to be passed into server_thread
		serverThreadInfo.myport = iniparserInfo->myport;

		// Forking new server thread to handle all the new incoming client requests	
		//printf("Process: creating server thread \n");
		rc = pthread_create(&serverThread, &attr, server_thread, (void *) &serverThreadInfo); 

		if (rc) {
			//printf("ERROR; return code from pthread_create()is %d\n", rc);
			exit(-1);
		}		
		

		// establish connection with all fellow beacon neighbours if am a beacon node
		// if am a beacon, fork queue thread, server thread before forking my client threads
		if(iniparserInfo->checkNodeType()==BEACON)	// Forking client threads for each beacon nodes
		{

			for(int i=0; i<bsize; i++)
			{
				// validating the condition, not connecting to myself
				if((iniparserInfo->myport==iniparserInfo->bports[i]) && (!(strcmp(iniparserInfo->myhostname, iniparserInfo->bhostname[i]))))
				{
					// continue creating the next client thread
					//printf("
					continue; 
				}

				//clientThreadData clientThreadInfo; // = (clientThreadData *)malloc (sizeof(clientThreadData));
				//memset(clientThreadInfo,0,sizeof(clientThreadData));

				clientThreadInfo[tcntr].hostport = iniparserInfo->bports[i];

				int size = strlen(iniparserInfo->bhostname[i]);
				strncpy(clientThreadInfo[tcntr].hostname, iniparserInfo->bhostname[i], size);
				clientThreadInfo[tcntr].hostname[size] = '\0';

				//printf("	client  %d- need to connect hostname %s at portno %d  \n", tcntr,	clientThreadInfo.hostname,clientThreadInfo.hostport );

				clientThreadInfo[tcntr].myServerPort = iniparserInfo->myport;
				clientThreadInfo[tcntr].retry = iniparserInfo->beaconRetry;
				clientThreadInfo[tcntr].msgttl = iniparserInfo->msgTTL;
				clientThreadInfo[tcntr].index = tcntr;

				rc = pthread_create(&clientThread[tcntr], &attr, client_thread, (void *) &clientThreadInfo[tcntr]); 

				if (rc) {
					//printf("ERROR; return code from pthread_create()is %d\n", rc);
					exit(-1);
				}		

				tcntr++;
			
			} //end for

		} // end if beacon node

		// try to establish connection with one beacon node if am a regular/non-beacon node
		// if none found, giveup??
		else if(iniparserInfo->checkNodeType()==NON_BEACON)	// Forking client threads for each beacon nodes
		{

			// TODO
			// function to verify whether the init_neighbor_list file is present in the home directory of a non-beacon node.
			if(init_file_exist()==TRUE)
			{
				initNeighbourFilePresent = TRUE;
			}
			else
			{
				initNeighbourFilePresent = FALSE;
			
			}


			//If this file is not present, the node should join the SERVANT network when it starts or restarts.
			if(!initNeighbourFilePresent)
			{
				for(int i=0; i<bsize; i++)
				{

					clientThreadData clientThreadInfo; // = (clientThreadData *)malloc (sizeof(clientThreadData));
					memset(&clientThreadInfo,0,sizeof(clientThreadData));

					clientThreadInfo.hostport = iniparserInfo->bports[i];

					int size = strlen(iniparserInfo->bhostname[i]);
					strncpy(clientThreadInfo.hostname, iniparserInfo->bhostname[i], size);
					clientThreadInfo.hostname[size] = '\0';

					//printf("	client  %d- need to connect hostname %s at portno %d  \n", tcntr,	clientThreadInfo.hostname,clientThreadInfo.hostport );

					clientThreadInfo.myServerPort = iniparserInfo->myport;
					clientThreadInfo.retry = iniparserInfo->beaconRetry;
					clientThreadInfo.msgttl = iniparserInfo->msgTTL;
					clientThreadInfo.index = i;

					rc = pthread_create(&tmpClientThread, &attr, tempJoining_thread, (void *) &clientThreadInfo); 

					if (rc) {
						//printf("ERROR; return code from pthread_create()is %d\n", rc);
						exit(-1);
					}		

					// performing joining for tmpClient thread
					rc = pthread_join(tmpClientThread, &status);

					if (rc) 
					{
						// printf("%d ERROR; return code from pthread_join() is %d\n", tmpClientThread,rc);
					}
					else
					{
						//printf("TmpClient %d : completed join with thread %d having a statusof %ld\n",i,tmpClientThread,(long)status);
					}	

					if(isConnToBeaconNode == TRUE)
					{
						//printf("%d regular node - connected to beacon %d node \n", iniparserInfo->myport, i);
						break;
					}

				} //end for	
				
				// if not able to connect to any of the beacon nodes
				// todo the appropriate handling
				if(isConnToBeaconNode == FALSE)
				{
					restartValid = TRUE;
					//printf("%d regular node not connected to any beacon node \n", iniparserInfo->myport);
					// todo write code to exit ?
				}
			
			} // end if(!initNeighbourFilePresent)
			
			//If this file is present, the regular node should participate in the SERVANT network when it starts or restarts, skipping the joining phase
			// first fork queue dispatcher & server thread, followed my forking all the client threads to establish connection with my neighbours specified in the init_neighbour_list
			if(!restartValid)
			{			
				// we shud extract the hostname and port no from the init_neighbour_list
				bsize = 0; 
			    
				if(PRINT_FLAG)
					printf("bsize %d \n", bsize);
				
				read_init_file(bsize);
				

				// todo write function to parse the init_neighbour_list and store it in the bsize array
				for(int i=0; i<bsize; i++)
				{

					//clientThreadData clientThreadInfo; // = (clientThreadData *)malloc (sizeof(clientThreadData));
					//memset(clientThreadInfo,0,sizeof(clientThreadData));

					clientThreadInfo[tcntr].hostport = iniparserInfo->bports[i];

					int size = strlen(iniparserInfo->bhostname[i]);
					strncpy(clientThreadInfo[tcntr].hostname, iniparserInfo->bhostname[i], size);
					clientThreadInfo[tcntr].hostname[size] = '\0';

					//printf("	client  %d- need to connect hostname %s at portno %d  \n", tcntr,	clientThreadInfo.hostname,clientThreadInfo.hostport );

					clientThreadInfo[tcntr].myServerPort = iniparserInfo->myport;
					clientThreadInfo[tcntr].retry = iniparserInfo->beaconRetry;
					clientThreadInfo[tcntr].msgttl = iniparserInfo->msgTTL;
					clientThreadInfo[tcntr].index = tcntr;

					rc = pthread_create(&clientThread[tcntr], &attr, client_thread, (void *) &clientThreadInfo[tcntr]); 

					if (rc) {
						//printf("ERROR; return code from pthread_create()is %d\n", rc);
						exit(-1);
					}		

					tcntr++;
				
				} //end for
			
				pthread_mutex_lock(&gMinNeighboursCnt_mutex);

				// this rejoining concept is not working.. for now, left it with incorrect behaviour
				if(gMinNeighboursCnt < iniparserInfo->minNeighbors)
				{
					restartValid = TRUE;
					//printf("Not enough neighbours \n");
					gMinNeighboursCnt = 0;

					del_init_neighbour_file();

				}

				pthread_mutex_unlock(&gMinNeighboursCnt_mutex);
				
				
			} // end if(!restartValid)
		
		} // end else if regular node
	
		if(PRINT_FLAG)
		{
			printf("exiting from the main loop in process \n");
			printf("restartValid value is %d \n", restartValid);
		}

	}while(restartValid);//main while loop
	
	
	if(PRINT_FLAG) printf("#client threads created %d \n", tcntr);
	/******************************************** JOINING PROCESS ********************************************/
	pthread_attr_destroy(&attr);

	// Performing joining for client threads
	for(int i=0; i<tcntr; i++)
	{
		//printf("inside join loop %d \n", i);
		rc = pthread_join(clientThread[i], &status);

		if (rc) {
			//printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		 }
		
		if(PRINT_FLAG)
			printf("Main: completed join with thread %d having a status of %ld\n",i,(long)status);
	}

	// performing joining for server thread
	rc = pthread_join(serverThread, &status);

	if (rc) {
		//printf("ERROR; return code from pthread_join() is %d\n", rc);
		exit(-1);
	 }
	

	// performing joining for QueueDispatcher thread
	rc = pthread_join(queueDpThread, &status);

	if (rc) {
		//printf("ERROR; return code from pthread_join() is %d\n", rc);
		exit(-1);
	 }

	// performing joining for keepalive thread
	rc = pthread_join(keepaliveThread, &status);

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}		

	// performing joining for command thread 
	rc = pthread_join(commandThread, &status);

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}		

	// performing joining for sigint thread
	rc = pthread_join(sigintThread, &status);

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}		
	

	// performing joining for timer thread
	rc = pthread_join(timerThread, &status);

	if (rc) {
		//printf("ERROR; return code from pthread_create()is %d\n", rc);
		exit(-1);
	}	

	if(PRINT_FLAG)
		printf("process function is exiting \n");

	

	if(PRINT_FLAG)
	{
		printf("data structures before externalizing \n");
		printf("globalFileNum : %d \n" , globalFileNum);
		displayAllIndexNodes();
		displayLRUList();
	}

	// EXTERNALIZE
	writeAllExtIndexs();


	return SUCCESS; 

} //end process function



