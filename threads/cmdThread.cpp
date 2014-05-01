#include "global.h"
#include "iniparserClass.h"

/* to be deleted */
extern int create_cache_file(unsigned int filenum, unsigned int m_startpos, unsigned int f_startpos, unsigned int metaSize, unsigned int dataSize, unsigned char *storeMsg);
extern int create_cache_file(unsigned int filenum, unsigned int m_startpos, unsigned int f_startpos, unsigned int metaSize, unsigned int dataSize, unsigned char *storeMsg, char *srcpath);
extern void recvFiledataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int dataSize, unsigned char *storeMsg, unsigned char sha1_filestr[]);

extern int gen_sha1_file(unsigned char sha1[], char *filename);

/**/

// for store file
extern unsigned int storeFileNum;
extern pthread_mutex_t storeFileNum_mutex;

// for get file 
int extfileSpec = FALSE;
char getfile[256] = "";


extern std::queue<FS_Event> cmdLineQueue;
extern pthread_mutex_t cmdLineQueue_mutex;
extern pthread_cond_t cmdLineQueue_cv;

extern pthread_cond_t eventQueue_cv;  

extern void pushRecvEventInEventQueue(Event &event);
extern void performFlooding(unsigned char *fs_data, unsigned int fs_len, int sockfd);

extern FILE *fp_nam;
 
extern int shortCircuit;
extern pthread_mutex_t shortCircuit_mutex; 

extern iniparserClass *iniparserInfo;

extern int timeToShutdown;
extern pthread_mutex_t shutdown_mutex; 

extern unsigned int temp_globalFileNum; // this is a temporary file counter for generating temp meta files upon receiving store request

extern pthread_mutex_t temp_globalFileNum_mutex; 

extern std::map<int, neighbourConn*> connList;
extern pthread_mutex_t connList_mutex; 

/** part 2 proj.. **/
extern unsigned int globalFileNum; 
extern pthread_mutex_t globalFileNum_mutex; 

/**** store functions ***/
extern int gen_onetime_pwd(unsigned char  password[]);
extern void convert_sha1_to_hex(unsigned char sha1[], unsigned char sha1buf[]);

extern int create_store_file(int filenum, char *fileName, unsigned int fileSize, char *keywords, unsigned char bitvec[]);

extern void tolowerKeyword(char keywd[]);

extern unsigned int computeFileSize(char type, char *filename);

extern int create_data_file (const char SRC[], const char DEST[]);

extern void computeStoreMsgSize(unsigned int filenum, unsigned int &metaSize, unsigned int &dataSize);
extern void formMetadataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *storeMsg);
extern void formFiledataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int dataSize, unsigned char *storeMsg);

extern void get_sha1_from_metafile(char *filename, unsigned char sha1_readstr[]);
extern void get_fname_from_metafile(char *filename, char fname_readstr[]);

extern void convert_hex_to_sha1(unsigned char sha1[], unsigned char sha1buf[]);
extern void recvMetadataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *storeMsg, unsigned char sha1_readstr[]);

// for get responses... filenum - tmp created in the mini directory for info extraction purpose
extern void recvMetadataFromBuffer_get(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *getMsg,  char fname_readstr[], unsigned int &fileSize, unsigned char sha1_readstr[], unsigned char nonce_readstr[]);
extern int determineFileIdentical(char fname_readstr[], unsigned char sha1_readstr[], unsigned char nonce_readstr[], unsigned int &filenum);


// for search responses
extern void recvMetadataFromBuffer_search(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *searchMsg);
extern void printMetadataFromFile_search(unsigned int filenum);

// delete functions
extern int deletePwdhit(unsigned int fileRef[], unsigned int &fileCnt, unsigned char nonce_sha1hex[], unsigned char pwdUOID_hex[], unsigned int &fileFound);
extern int create_delete_filespec(unsigned int filenum, char *filename, unsigned char sha1_hex[], unsigned char nonce_sha1hex[], unsigned char pwdUOID_hex[]);
extern void formFileSpecInBuffer(unsigned int filenum, unsigned int startpos, unsigned int filespecSize, unsigned char *Msg);
extern void deleteFiles(int num, unsigned int fileRef);

/**** index functions ***/
extern void addKwrdIndexNode(string key, unsigned int fileRef );
extern void deleteKwrdIndexNode(string key, unsigned int fileRef );
extern int findKeyInKwrdIndex(string key);
extern void getValueFromKwrdIndex (string key, unsigned int fileRef[]);
extern int getCountForKwrdKey (string key);
extern int getKwrdIndexSize();
extern void displayKwrdIndex();
extern void displayAllIndexNodes();
extern void getValueFromShaIndex (string key, unsigned int fileRef[]);
extern int getCountForShaKey (string key);

/*** LRU functions **/
extern void addNodeInLRUList(unsigned int newfileRef, unsigned int newfileRef_Size);
extern int updateLRU_SearchResp(unsigned int newfileRef);
extern void displayLRUList();
extern int findFileInLRUList(unsigned int newfileRef);
extern void deleteNodeInLRUList(unsigned int newfileRef);

/**** flooding function ***/
extern void performFlooding_prob(unsigned char *fs_data, unsigned int fs_len, int sockfd, double prob);

// search-specific functions
extern int determineSearchRecordInfo_fname(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);
extern int determineSearchRecordInfo_sha1hash(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);
extern int determineSearchRecordInfo_kwrd(char *fname, unsigned int &rsize, unsigned int &rlen, unsigned int fileRef[], unsigned int nxtlen[]);

/*** index functions **/
extern void addNodeInSearchIDMap(unsigned char fileID[UOID_LEN], int fileRef, unsigned char sha1_filestr[]);
extern void deleteNodeInSearchIDMap(int fileRef);
extern int findFileIDInSearchIDMap(int fileRef);
extern int getFileIDFromSearchIDMap(int fileRef, unsigned char fileID[UOID_LEN]);
extern int getSHA1FromSearchIDMap(int fileRef, unsigned char sha1file[]);
extern void displaySearchIDMap();
extern void clearSearchIDMap();

extern void deleteAllIndexNodes(unsigned int fileRef);

/********** DEFINE FILEID FUNCTIONS ************************/

extern void addNodeInFileIDMap(unsigned char fileID[UOID_LEN], int fileRef);
extern int findFileIDInFileIDMap(int fileRef);
extern int getFileIDFromFileIDMap(int fileRef, unsigned char fileID[UOID_LEN]);
extern int findIDMatchInFileIDMap(unsigned char fileID[UOID_LEN]);
extern unsigned int getFileRefFromFileIDMap(unsigned char fileID[UOID_LEN]);

/** others **/

extern int bitvec_equal(unsigned char b1[], unsigned char b2[]);

void *cmdLinTh(void *para)
{
	unsigned int *cmd_time = (unsigned int *)para;
	unsigned int cmd_timer = *cmd_time;

	if(PRINT_FLAG) printf("cmd_timer timeout value %d ... while parsing %d \n", cmd_timer, *cmd_time);

	while(1)
	{
	  //sleep for 1 sec
		sleep(1);

		cmd_timer--;

		if(cmd_timer == 0)
		{
			pthread_mutex_lock(&shortCircuit_mutex);
			shortCircuit = TRUE;
			pthread_mutex_unlock(&shortCircuit_mutex);
		
			pthread_cond_broadcast(&cmdLineQueue_cv);     // this will signal on thread that is waiting for messages in status replies

			if(PRINT_FLAG) 
				printf("cmd timer thread timer expired %d \n", cmd_timer);
			break;
		}

		pthread_mutex_lock (&shortCircuit_mutex);
		if(shortCircuit == TRUE)
		{		
			pthread_mutex_unlock (&shortCircuit_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv);     // this will signal on thread that is waiting for messages in status replies

			if(PRINT_FLAG)
				printf("cmd timer is existing due to user ctrl+c \n");
			break;
		}
		pthread_mutex_unlock (&shortCircuit_mutex);
		
		
		pthread_mutex_lock (&shutdown_mutex);
		if(timeToShutdown == TRUE)
		{		
			pthread_mutex_unlock (&shutdown_mutex);

			pthread_mutex_lock(&shortCircuit_mutex);
			shortCircuit = TRUE;
			pthread_mutex_unlock(&shortCircuit_mutex);

			pthread_cond_broadcast(&cmdLineQueue_cv);     // this will signal on thread that is waiting for messages in status replies

			break;
		}
		pthread_mutex_unlock (&shutdown_mutex);
		

	} //end main while

	if(PRINT_FLAG)
		printf("cmd timer thread is exiting after timeout expired %d \n", cmd_timer);

	pthread_exit(NULL);

} // end cmdLinTh



// STATUS NEIGHBORS RESPONSE HANDLER
void* nam_gen_th(void*)  
{	
	map<int, int> nodesTaken;
	map<int, int>::iterator it; // to iterate through nodesTaken

	nodesTaken[iniparserInfo->myport] = 1;
	
	Log l = Log();

	while(1)
	{   
		//printf("!!!!!!!!!!****** INSIDE cmdLineQueue LOOP \n");	
		pthread_mutex_lock(&cmdLineQueue_mutex);		
		
		while (cmdLineQueue.empty() && (timeToShutdown==FALSE) && (shortCircuit == FALSE))
		{		
			
				//l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue WHILE WAITING FOR JOB DP");	
				pthread_cond_wait(&cmdLineQueue_cv, &cmdLineQueue_mutex);
				//pthread_mutex_unlock (&queueMutex);
				//l.debug_log((char*)"!!!!!!!!!!****** cmdLineQueue JOB BECOME AVAILABLE WOKE UP FROM COND WAIT");
		}
		
		pthread_mutex_lock (&shutdown_mutex);
		// time to shut down is true, free the lock and signal all the sleeping threads
		if(timeToShutdown==TRUE)
		{	
			pthread_mutex_unlock (&shutdown_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("nam_gen_th is exiting due to autoshutdown\n");

			pthread_exit(NULL);
		}
		pthread_mutex_unlock (&shutdown_mutex);

		pthread_mutex_lock(&shortCircuit_mutex);
		// time to exit this thread - either msgLifetime expired or ctrl+c received from the commandline
		if(shortCircuit == TRUE)
		{
			pthread_mutex_unlock(&shortCircuit_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("nam_gen_th is exiting either due to msgLifetime or ctrl+c hit \n");

			pthread_exit(NULL);				
		
		}
		pthread_mutex_unlock(&shortCircuit_mutex);

		//printf("***********!!!!!!!!!!!!!!!!!!!!!!dequeue msg from command line queue is !!!!!!!!!!!********\n");  
		// if its not time to shutdown and cmdLineQueue is not empty, process the event job
		// access the front element in the queue for processing
		FS_Event event = cmdLineQueue.front();
		
		// dequeue a job
		cmdLineQueue.pop();

		pthread_mutex_unlock(&cmdLineQueue_mutex);   
		
		// work on the job 
		//unsigned int len = strlen((const char *)event.data);
		 
		// form the whole message then later copy into data section starting from 27th byte
		unsigned int respTotalLen = event.totalLen;

		//unsigned char* respBuf; // = event.data; //(unsigned char*) malloc(respTotalLen);
		
		//memcpy(&respBuf[0], &event.data[0], respTotalLen);
		
		//int sockfd = event.sockfd;
		
		// grab the buffer pointer that was pushed into the queuetor ob	
		int eventtype =	event.eventType;

		if(eventtype == STATUS_REPONSE)
		{
			l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue STATUS RESPONSE RECEIVED");
			unsigned int rstartbyte = 0;

			unsigned short int hinfo_len = 0;
			unsigned char reqUOID[UOID_LEN];
			unsigned short int srcport = 0;

			// compose the response data
			memcpy(&reqUOID[0], &event.data[27],UOID_LEN);
			memcpy(&hinfo_len, &event.data[47], HOST_INFO_LEN);
			
			memcpy(&srcport, &event.data[49], HOST_PORT_LEN);
			//memcpy(iniparserInfo->myhostname, &respBuf[51], strlen(iniparserInfo->myhostname));
			rstartbyte = MAX_HEADER_LENGTH + UOID_LEN + HOST_INFO_LEN + hinfo_len;
			int count = 0;

			it = nodesTaken.find(srcport);
			// add the source node
			if(it == nodesTaken.end())
			{
				nodesTaken[srcport] = 1;  // this nodes has to be ploted in nam
				fprintf(fp_nam, "n -t * -s %d -c red -i black\n", srcport);
				fflush(fp_nam);												
			 }

			while(rstartbyte < respTotalLen){
			
				//unsigned int hlen = 0;
				unsigned int rlen_data = 0;
				unsigned short int destport = 0;

				memcpy(&rlen_data, &event.data[rstartbyte], 4);

				if(rlen_data == 0) //reached last record
				{
					memcpy(&destport, &event.data[rstartbyte+4], HOST_PORT_LEN);
					rstartbyte = respTotalLen; //rstartbyte + 4 + rlen_data;
				}
				else
				{
					memcpy(&destport, &event.data[rstartbyte+4], HOST_PORT_LEN);
					rstartbyte = rstartbyte + 4 + rlen_data;
				}
			
				
				it = nodesTaken.find(destport);
				//printf("rstartbyte %d count %d \n", rstartbyte,count);
				//printf("destport adding INSIDE cmdLineQueue NODE %d count %d \n", destport,count);
				if(it == nodesTaken.end())
				{
					nodesTaken[destport] = 1;  // this nodes has to be ploted in nam
					fprintf(fp_nam, "n -t * -s %d -c red -i black\n", destport);
					fflush(fp_nam);
					//l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue ADDING A NODE");
					//printf("!!!!!!!!!!****** INSIDE cmdLineQueue ADDING A NODE %d into nodesTaken List", destport);
				 }
				//this link belongs to nam file so plot it
				 fprintf(fp_nam, "l -t * -s %d -d %d -c blue\n", srcport,destport);
				 fflush(fp_nam);
				count++;
			 } // end while

			
		} // if(eventtype==STATUS_REPONSE)

	} // end main while

	pthread_exit(NULL);

} // end nam_gen_th



// TODO NEED TO MODIFY
// STATUS FILES HANDLER
void* status_files_th(void*)  
{	
	map<int, int> nodesTaken;
	map<int, int>::iterator it; // to iterate through nodesTaken

	nodesTaken[iniparserInfo->myport] = 1;
	
	Log l = Log();

	l.debug_log((char*)"!!!!!!!!!!****** ENTERED status_files_thread processing STATUS_FILES commands");
	
	if(PRINT_FLAG) printf("entering status_files_th thread \n");

	/**************** WRITING STATUS FILES PERTAINING TO MY NODE ****************/						

	pthread_mutex_lock(&globalFileNum_mutex);
	unsigned int filenum = globalFileNum;
	pthread_mutex_unlock(&globalFileNum_mutex);

	fprintf(fp_nam, "%s:%d", iniparserInfo->myhostname,iniparserInfo->myport);
	fflush(fp_nam);

	int currentFileNum = getKwrdIndexSize();
	printf("currentFileNum %d \n", currentFileNum);

	if(currentFileNum == 0)
	{
		fprintf(fp_nam, " has no file\r\n");
		fflush(fp_nam);
	}				
	else if(currentFileNum == 1)
	{
		fprintf(fp_nam, " has the following file\r\n");
		fflush(fp_nam);			
	}
	else
	{
		fprintf(fp_nam, " has the following files\r\n");
		fflush(fp_nam);						
	}
	

	unsigned int count = 1;

	for(count = 1; count <= filenum; count++)
	{
		unsigned int metaSize = 0;
		unsigned int dataSize = 0;

		// given the filenum, compute the meta file size and data file size
		char metaPath[256] = "";
		sprintf(metaPath, "%d.meta", count);

		int exist = computeFileSize('m', metaPath);
		// if the file exists then populate the status files
		if(exist != -1)
		{

			computeStoreMsgSize(count, metaSize, dataSize);	

			unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * (metaSize+1));	
			memset(&fwdBuf[0], 0, metaSize);
			
			// storing the meta data into the buffer
			formMetadataInBuffer(count, 0, metaSize, fwdBuf);	
			fwdBuf[metaSize] = '\0';
			
			fwrite (&fwdBuf[0] , 1 , metaSize , fp_nam );
			
					
			// to display [metadata] section header in a new line. bitvector was not written into a file with \n
			fprintf(fp_nam, "\r\n");
			fflush(fp_nam);	
		}

	}

	// to mark the next nodes status files info
	fprintf(fp_nam, "\r\n");
	fflush(fp_nam);		

	while(1)
	{   
		//printf("!!!!!!!!!!****** INSIDE cmdLineQueue LOOP \n");	
		pthread_mutex_lock(&cmdLineQueue_mutex);		
		
		while (cmdLineQueue.empty() && (timeToShutdown==FALSE) && (shortCircuit == FALSE))
		{		
			
				//l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue WHILE WAITING FOR JOB DP");	
				pthread_cond_wait(&cmdLineQueue_cv, &cmdLineQueue_mutex);
				//pthread_mutex_unlock (&queueMutex);
				//l.debug_log((char*)"!!!!!!!!!!****** cmdLineQueue JOB BECOME AVAILABLE WOKE UP FROM COND WAIT");
		}
		
		pthread_mutex_lock (&shutdown_mutex);
		// time to shut down is true, free the lock and signal all the sleeping threads
		if(timeToShutdown==TRUE)
		{	
			pthread_mutex_unlock (&shutdown_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("status_files_th is exiting due to autoshutdown\n");

			pthread_exit(NULL);
		}
		pthread_mutex_unlock (&shutdown_mutex);

		pthread_mutex_lock(&shortCircuit_mutex);
		// time to exit this thread - either msgLifetime expired or ctrl+c received from the commandline
		if(shortCircuit == TRUE)
		{
			pthread_mutex_unlock(&shortCircuit_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("status_files_th is exiting either due to msgLifetime or ctrl+c hit \n");

			pthread_exit(NULL);				
		
		}
		pthread_mutex_unlock(&shortCircuit_mutex);

		//printf("***********!!!!!!!!!!!!!!!!!!!!!!dequeue msg from command line queue is !!!!!!!!!!!********\n");  
		// if its not time to shutdown and cmdLineQueue is not empty, process the event job
		// access the front element in the queue for processing
		FS_Event event = cmdLineQueue.front();
		
		// dequeue a job
		cmdLineQueue.pop();

		pthread_mutex_unlock(&cmdLineQueue_mutex);   
		
		// work on the job 
		//unsigned int len = strlen((const char *)event.data);
		 
		// form the whole message then later copy into data section starting from 27th byte
		unsigned int respTotalLen = event.totalLen;

		
		// grab the buffer pointer that was pushed into the queuetor ob	
		int eventtype =	event.eventType;


		if(eventtype == STATUS_REPONSE)
		{
			l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue STATUS RESPONSE RECEIVED for STATUS_FILES command");

			unsigned int rstartbyte = 0;

			unsigned short int hinfo_len = 0;
			unsigned char reqUOID[UOID_LEN];
			unsigned short int srcport = 0;
		
			// compose the response data
			memcpy(&reqUOID[0], &event.data[27],UOID_LEN);
			memcpy(&hinfo_len, &event.data[47], HOST_INFO_LEN);
			
			memcpy(&srcport, &event.data[49], HOST_PORT_LEN);

			char *srchost = (char*) malloc(hinfo_len-1);
			memcpy(srchost, &event.data[51], hinfo_len-2);
			srchost[hinfo_len-2] = '\0';

			rstartbyte = MAX_HEADER_LENGTH + UOID_LEN + HOST_INFO_LEN + hinfo_len;			
			/*For each response should list the Hostname and Hostport of the reporting host, 
			the number of files it has, and the complete metadata for each of the files.
			*/
			fprintf(fp_nam, "%s:%d", srchost,srcport);
			fflush(fp_nam);

			if(respTotalLen == rstartbyte)
			{
				fprintf(fp_nam, " has no file\r\n\r\n");
				fflush(fp_nam);
				continue;
			}

			unsigned int next_len = 0;
			memcpy(&next_len, &event.data[rstartbyte], 4);			
			
			if(next_len == 0)
			{
				fprintf(fp_nam, " has the following file\r\n");
				fflush(fp_nam);			
			}
			else
			{
				fprintf(fp_nam, " has the following files\r\n");
				fflush(fp_nam);						
			}

			while(rstartbyte < respTotalLen){
			
				// the length of the metadata field. If the field is zero, another length field will follow the next descriptor
				next_len = 0;				
				unsigned int startpos = rstartbyte+4;	

				memcpy(&next_len, &event.data[rstartbyte], 4);

				if(PRINT_FLAG)
					printf("next_len % d\n", next_len);

				if(next_len == 0) //reached last record
				{
					next_len = respTotalLen - rstartbyte - 4; // respTotalLen - rstartbyte = gives the size of the last record
					
					rstartbyte = respTotalLen; //to break out of the loop				
				}
				else // another record follows this record
				{
					rstartbyte = rstartbyte + 4 + next_len; //next byte
				}
	
				if(PRINT_FLAG)
				{
					printf("rstartbyte % d\n", rstartbyte);
					printf("next_len % d\n", next_len);
				}

				fwrite (&event.data[startpos] , 1 , next_len , fp_nam );
				
				// to display [metadata] section header in a new line. bitvector was not written into a file with \n
				fprintf(fp_nam, "\r\n");
				fflush(fp_nam);	
				
			 } // end while

			// to denote the end of status files display for one host
			fprintf(fp_nam, "\r\n");
			fflush(fp_nam);	

		} // if(eventtype==STATUS_REPONSE)

	} // end while

	pthread_exit(NULL);

} // end status_files_th


// SEARCH RESPONSE HANDLER
void* search_resp_th(void *cnt)  
{		
	Log l = Log();

	l.debug_log((char*)"!!!!!!!!!!****** ENTERED search_resp_thread processing SEARCH RESPONSES");
	
	if(PRINT_FLAG)
		printf("entering search_resp_th thread \n");

	unsigned int count = (unsigned int)cnt;

	while(1)
	{   
		//printf("!!!!!!!!!!****** INSIDE cmdLineQueue LOOP \n");	
		pthread_mutex_lock(&cmdLineQueue_mutex);		
		
		while (cmdLineQueue.empty() && (timeToShutdown==FALSE) && (shortCircuit == FALSE))
		{		
			
				//l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue WHILE WAITING FOR JOB DP");	
				pthread_cond_wait(&cmdLineQueue_cv, &cmdLineQueue_mutex);
				//pthread_mutex_unlock (&queueMutex);
				//l.debug_log((char*)"!!!!!!!!!!****** cmdLineQueue JOB BECOME AVAILABLE WOKE UP FROM COND WAIT");
		}
		
		pthread_mutex_lock (&shutdown_mutex);
		// time to shut down is true, free the lock and signal all the sleeping threads
		if(timeToShutdown==TRUE)
		{	
			pthread_mutex_unlock (&shutdown_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("search_resp_th is exiting due to autoshutdown\n");

			pthread_exit(NULL);
		}
		pthread_mutex_unlock (&shutdown_mutex);

		pthread_mutex_lock(&shortCircuit_mutex);
		// time to exit this thread - either msgLifetime expired or ctrl+c received from the commandline
		if(shortCircuit == TRUE)
		{
			pthread_mutex_unlock(&shortCircuit_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("search_resp_th is exiting either due to msgLifetime or ctrl+c hit \n");

			pthread_exit(NULL);				
		
		}
		pthread_mutex_unlock(&shortCircuit_mutex);

		//printf("***********!!!!!!!!!!!!!!!!!!!!!!dequeue msg from command line queue is !!!!!!!!!!!********\n");  
		// if its not time to shutdown and cmdLineQueue is not empty, process the event job
		// access the front element in the queue for processing
		FS_Event event = cmdLineQueue.front();
		
		// dequeue a job
		cmdLineQueue.pop();

		pthread_mutex_unlock(&cmdLineQueue_mutex);   
		
		// work on the job 
		//unsigned int len = strlen((const char *)event.data);
		 
		// form the whole message then later copy into data section starting from 27th byte
		unsigned int respTotalLen = event.totalLen;

		
		// grab the buffer pointer that was pushed into the queuetor ob	
		int eventtype =	event.eventType;


		if(eventtype == SEARCH_REPONSE)
		{
			l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue SEARCH_REPONSE RECEIVED");

			unsigned char reqUOID[UOID_LEN];
			
			// decompose the response data
			memcpy(&reqUOID[0], &event.data[27],UOID_LEN);
								
			unsigned int rstartbyte = MAX_HEADER_LENGTH + UOID_LEN; // 47th byte

			if(PRINT_FLAG)
				printf("rstartbyte % d\n", rstartbyte);

			while(rstartbyte < respTotalLen){
			
				// the length of the metadata field. If the field is zero, another length field will follow the next descriptor
				unsigned int next_len = 0;				
				unsigned char fileID[FILE_ID_LEN];
				unsigned int startpos = rstartbyte+NEXT_LENGTH_LEN + FILE_ID_LEN;	

				memcpy(&next_len, &event.data[rstartbyte], NEXT_LENGTH_LEN);

				if(PRINT_FLAG)
					printf("next_len % d\n", next_len);

				if(next_len == 0) //reached last record
				{
					memcpy(&fileID[0], &event.data[rstartbyte+NEXT_LENGTH_LEN], FILE_ID_LEN); 
					
					next_len = respTotalLen - rstartbyte - NEXT_LENGTH_LEN - FILE_ID_LEN;
					
					rstartbyte = respTotalLen; //to break out of the loop				
				}
				else // another record follows this record
				{
					memcpy(&fileID[0], &event.data[rstartbyte+NEXT_LENGTH_LEN], FILE_ID_LEN); 
					rstartbyte = rstartbyte + NEXT_LENGTH_LEN + FILE_ID_LEN + next_len; //next byte

				}
	
				if(PRINT_FLAG)
				{
					printf("rstartbyte % d\n", rstartbyte);
					printf("next_len % d\n", next_len);
				}
				
				pthread_mutex_lock(&temp_globalFileNum_mutex);
				temp_globalFileNum++;
				unsigned int filenum = temp_globalFileNum;
				pthread_mutex_unlock(&temp_globalFileNum_mutex);

				// extracting the SHA1 from the meta file
				unsigned char sha1_filestr[2*SHA_DIGEST_LEN+1] = "";				
				recvMetadataInBuffer(filenum, startpos, next_len, event.data, sha1_filestr); 
				
				addNodeInSearchIDMap(fileID, count+1, sha1_filestr);

				// in each iteration, display fileid & metadata of the current record
				printf("[%d]\tFileID=", count+1);
				
				for(int i=0;i<FILE_ID_LEN;i++)
				{
					printf("%02x", fileID[i]);
				}
				
				printf("\n");

				//recvMetadataFromBuffer_search(1, rstartbyte+NEXT_LENGTH_LEN+FILE_ID_LEN, next_len, event.data);				
				recvMetadataFromBuffer_search(filenum, startpos, next_len, event.data);				

				count++;
			 } // end while

		} // if(eventtype==SEARCH_REPONSE)

	} // end while

	pthread_exit(NULL);

} // end search_resp_th

// GET RESPONSE HANDLER
void* get_resp_th(void *cnt)  
{		
	Log l = Log();

	l.debug_log((char*)"!!!!!!!!!!****** ENTERED get_resp_thread processing GET RESPONSE");
	
	if(PRINT_FLAG)
		printf("entering get_resp_th thread \n");

	while(1)
	{   
		//printf("!!!!!!!!!!****** INSIDE cmdLineQueue LOOP \n");	
		pthread_mutex_lock(&cmdLineQueue_mutex);		
		
		while (cmdLineQueue.empty() && (timeToShutdown==FALSE) && (shortCircuit == FALSE))
		{		
			
				//l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue WHILE WAITING FOR JOB DP");	
				pthread_cond_wait(&cmdLineQueue_cv, &cmdLineQueue_mutex);
				//pthread_mutex_unlock (&queueMutex);
				//l.debug_log((char*)"!!!!!!!!!!****** cmdLineQueue JOB BECOME AVAILABLE WOKE UP FROM COND WAIT");
		}
		
		pthread_mutex_lock (&shutdown_mutex);
		// time to shut down is true, free the lock and signal all the sleeping threads
		if(timeToShutdown==TRUE)
		{	
			pthread_mutex_unlock (&shutdown_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("get_resp_th is exiting due to autoshutdown\n");

			pthread_exit(NULL);
		}
		pthread_mutex_unlock (&shutdown_mutex);

		pthread_mutex_lock(&shortCircuit_mutex);
		// time to exit this thread - either msgLifetime expired or ctrl+c received from the commandline
		if(shortCircuit == TRUE)
		{
			pthread_mutex_unlock(&shortCircuit_mutex);
			pthread_mutex_unlock(&cmdLineQueue_mutex);
			pthread_cond_broadcast(&cmdLineQueue_cv); //todo required?


			if(PRINT_FLAG)
				printf("get_resp_th is exiting either due to GETmsgLifetime or ctrl+c hit \n");

			pthread_exit(NULL);				
		
		}
		pthread_mutex_unlock(&shortCircuit_mutex);

		//printf("***********!!!!!!!!!!!!!!!!!!!!!!dequeue msg from command line queue is !!!!!!!!!!!********\n");  
		// if its not time to shutdown and cmdLineQueue is not empty, process the event job
		// access the front element in the queue for processing
		FS_Event event = cmdLineQueue.front();
		
		// dequeue a job
		cmdLineQueue.pop();

		pthread_mutex_unlock(&cmdLineQueue_mutex);   
		
		// work on the job 
		 unsigned int respTotalLen = event.totalLen;
		
		// grab the buffer pointer that was pushed into the queuetor ob	
		int eventtype =	event.eventType;

		if(eventtype == GET_REPONSE)
		{
			l.debug_log((char*)"!!!!!!!!!!****** INSIDE cmdLineQueue GET_REPONSE RECEIVED");

			unsigned char reqUOID[UOID_LEN];
			
			// decompose the response data
			memcpy(&reqUOID[0], &event.data[27],UOID_LEN);
								
			if(PRINT_FLAG)
				printf("GET_RESPONSE RECEIVED BY GET RESP HANDLER - CMDLINEQUEUE \n");

			/* receiving node gets a GET Response, store it in the permanent space
			*/
			unsigned int metaSize = 0;
			unsigned int dataSize = 0;
			unsigned char sha1_readstr[2*SHA_DIGEST_LEN+1] = "";
			unsigned char sha1_filestr[2*SHA_DIGEST_LEN] = "";

			memcpy(&metaSize, &event.data[47], META_DATA_LEN);
			dataSize =  respTotalLen - MAX_HEADER_LENGTH - UOID_LEN - META_DATA_LEN - metaSize;

			if(PRINT_FLAG)
				printf("get response metadata length %d datasize length %d \n", metaSize, dataSize);
			
			/*pthread_mutex_lock(&temp_globalFileNum_mutex);
			temp_globalFileNum++;
			unsigned int filenum = temp_globalFileNum;
			pthread_mutex_unlock(&temp_globalFileNum_mutex);
			*/

			unsigned int filenum = event.filenum;

			// extracting the meta data from 51 - (51+metaSize-1)
			recvMetadataInBuffer(filenum, 51, metaSize, event.data, sha1_readstr);
			
			// extracting the file data from 51+metaSize - (51+metaSize+dataSize-1)
			//recvFiledataInBuffer(filenum, 51+metaSize, dataSize, event.data, sha1_filestr);
			
			// generating the file sha1 in hexstring
			char dataPath[256] = "";
			sprintf(dataPath, "%s/%d.data", iniparserInfo->myhomedir,filenum);

			unsigned char sha1_file[SHA_DIGEST_LEN];			
			gen_sha1_file(sha1_file, dataPath);
			convert_sha1_to_hex(sha1_file, sha1_filestr);

			if(PRINT_FLAG)
			{
				cout<< "in get_resp_th SHA1 value computed from meta read file is "<<sha1_readstr <<endl;
				cout<< "in get_resp_th SHA1 value computed from data file is "<<sha1_filestr <<endl;
			}
			
			/* The receiving node of store request compute the sha1 hash value of the file and compare it against the stored file 
			description (in the recv buffer). If it does not match, the should be discarded. 
			2 copies created - current working dir / node home dir
			*/
			if(strcmp((const char*)sha1_readstr, (const char *)sha1_filestr) == 0)
			{
				if(PRINT_FLAG)
					printf("both meta data and file data sha1 are same for the get response \n");
			/************************** CHECKING FOR DUPLICATES IN MINI FILE DIR / CURRENT WORKING DIR **************************/	

				/**************** CREATE COPY ON MINI FILE DIR IF IDENTICAL FILE DOES NOT EXIST BASED ON: ****************/	

				/* get fname, sha1 and nonce to determine whether identical copy 
				already exists in the mini directory  
				*/
				//use globalfilenum counter - note down the fileno.. later copy it into the working directory
				pthread_mutex_lock(&globalFileNum_mutex);
				globalFileNum++;
				unsigned int filenum_create = globalFileNum; 
				pthread_mutex_unlock(&globalFileNum_mutex);

				char fname[256];
				unsigned int fileSize = 0;
				unsigned char nonce_readstr[2*SHA_DIGEST_LEN+1] = "";
				
				/*pthread_mutex_lock(&temp_globalFileNum_mutex);
				temp_globalFileNum++;
				unsigned int filenum = temp_globalFileNum;
				pthread_mutex_unlock(&temp_globalFileNum_mutex);
				*/

				// extracting the meta data from 51 - (51+metaSize-1)						
				recvMetadataFromBuffer_get(filenum, 51, metaSize, event.data, fname, fileSize, sha1_readstr, nonce_readstr);

				if(determineFileIdentical(fname, sha1_readstr, nonce_readstr, filenum_create)== FALSE)
				{


					//printf("new get file created with %d file number as no duplicate exists in mini dir\n", filenum_create);
					// todo have to change this function for GET resp and STOR in process to handle large file sizes
					create_cache_file(filenum_create, 51, 51+metaSize, metaSize, dataSize, event.data, dataPath);

				}
				else
				{
					//printf("new get file not created as duplicate file exists %d in mini dir\n", filenum_create);
				
				}


				/**************** CREATE COPY ON CURRENT WORKING DIR IF IDENTICAL FILE NAME DOES NOT EXIST ****************/	


				// take the filename from the metadata
				if(extfileSpec == FALSE)
				{
					/*
					strcpy(getfile, fname);
					getfile[strlen(fname)] = "\0";
					printf("get file name %s" getfile);
					*/

					// copying the "data" file into current working directory 
					
					char metaPath[256] = "";
					sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,filenum_create);
					
					get_fname_from_metafile(metaPath, getfile);
					
				}
				
				//printf("get file name %s", getfile);

				// copying the "data" file into current working directory in either cases
				char srcFilePath[256] = "";
				sprintf(srcFilePath, "%s/files/%d.data", iniparserInfo->myhomedir,filenum_create);
				
				// check whether the file with the same name as getfile exist in the current working directory, if it exists, should prompt for the replacement
				char replace[20] = "\0";

				int exist = computeFileSize('c', getfile);
				if(exist == -1)
				{
					//printf("getfile %s does not exist in the current working directory return value %d \n", getfile, exist);
					replace[0] = 'y';
				}
				else
				{
					//printf("getfile %s does exist in the current working directory return value %d \n", getfile, exist);
					printf("Okay to replace %s in your current working directory? [yes/no] ", getfile);
					scanf("%s", replace);
					tolowerKeyword(replace);
					//printf("option choosen is %s \n" , replace);								
				}

				if(replace[0] == 'y')
				{
					//printf("user decided to copy it if it exists or else creating a new copy \n");
					create_data_file(srcFilePath,getfile);
				}
				else
				{
				
					//printf("user decided not to make a copy in his working directory \n");
				}
							

			} // if file sha1 and sha1 stored in the meta file matches

		} // if(eventtype==GET_REPONSE)

	} // end while

	pthread_exit(NULL);

} // end get_resp_th


//taken from online open source code
void Tokenize(const string& str,vector<string>& tokens,const string& delimiters = " ")
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

void* cmdThread(void *)
{
	fd_set readfds;
	struct timeval tv;
	//char cmd[80];
	fflush(stdout);
	int flag = 1;
	
	// that a get command should not be allowed if it does not immediately follw a search or a get command
	int searchTyped = FALSE;

	while (1)
	{
		if(flag == 1) {
		printf("servant:%d>", iniparserInfo->myport);
		fflush(stdout);
		flag = 0;
		}

		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		int s = select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv); 

		if(s==0)
		{
			pthread_mutex_lock (&shutdown_mutex);

			if(timeToShutdown == TRUE)
			{
				pthread_mutex_unlock (&shutdown_mutex);

				if(PRINT_FLAG)
					printf("cmdThread is exiting due to shutdown \n");


				pthread_exit(NULL);
			
			}
			pthread_mutex_unlock (&shutdown_mutex);		
		
		}

		if (s > 0) 
		{

			if (FD_ISSET(STDIN_FILENO, &readfds)) 
			{
				string str = "";

				getline(cin,str);
				
				if(str.length() == 0)
					continue;				
				vector<string> command;
				Tokenize(str, command, " =\""); //todo is the delimiters specified correctly?

				if(command.size() == 0)
					continue;

				flag = 1;

				if(command[0] == "status") {					
					if(command.size() == 1)
						continue;

					if(command[1] == "neighbors") 
					{
						
						if(command.size() < 4)
							continue;

						 fflush(stdout);
						 searchTyped = FALSE;

						 //------------------------------------------------------------
							Log logEvent = Log();
							logEvent.debug_log((char*)"status neighbours received");
						//---------------------------------------------------------------

						// get the TTL
						const char *ttl_c = command[2].c_str();
						int TTL = atoi(ttl_c);

						// get the output file name
						const char *file = command[3].c_str();
						fp_nam = fopen(file, "w");
						fprintf(fp_nam, "V -t * -v 1.0a5\n");
						fflush(fp_nam);

						fprintf(fp_nam, "n -t * -s %d -c red -i black\n", iniparserInfo->myport);
						fflush(fp_nam);				
						
						// composing the status request message to be flooded to all the neighbors
						Message msg = Message();
						msg.statustype = STATUS_NEIGHBOURS;
						msg.updateMsgHeader(STRQ, TTL, 1);

						// creating read event and pushing into the event queue
						Event event = Event(msg, 0, CMD_THREAD); // -0 sockfd = user-generated
						pushRecvEventInEventQueue(event);	

						pthread_mutex_lock(&shortCircuit_mutex);
						shortCircuit = FALSE;
						pthread_mutex_unlock(&shortCircuit_mutex);		

						// waiting for status responses after transmitting status request - status response timer thread
						pthread_t cmdLineTh_t;
						pthread_create(&cmdLineTh_t, NULL, cmdLinTh, (void *)&iniparserInfo->msgLifetime);						

						// thread that collects and processes all the status responses - status_neighbors
						pthread_t nam_gen_th_t;
						void* status;
						pthread_create(&nam_gen_th_t, NULL, nam_gen_th, NULL);
						pthread_join(nam_gen_th_t, &status);		
						fclose(fp_nam);
							
					}	// end if(command[1] == "neighbors") 

					else if(command[1] == "files") 
					{
						if(command.size() < 4)
							continue;

						 fflush(stdout);
						 searchTyped = FALSE;
						 //------------------------------------------------------------
							Log logEvent = Log();
							logEvent.debug_log((char*)"status files received");
						//---------------------------------------------------------------

						// get the TTL
						const char *ttl_c = command[2].c_str();
						int TTL = atoi(ttl_c);

						// get the output file name
						const char *file = command[3].c_str();
						fp_nam = fopen(file, "wb");

						// composing the status request message to be flooded to all the neighbors
						Message msg = Message();
						msg.statustype = STATUS_FILE;
						msg.updateMsgHeader(STRQ, TTL, 1); 

						// creating read event and pushing into the event queue
						Event event = Event(msg, 0, CMD_THREAD); // -0 sockfd = user-generated
						pushRecvEventInEventQueue(event);	
						
						pthread_mutex_lock(&shortCircuit_mutex);
						shortCircuit = FALSE;
						pthread_mutex_unlock(&shortCircuit_mutex);
						
						// waiting for status responses after transmitting status request - status response timer thread
						pthread_t cmdLineTh_t;
						pthread_create(&cmdLineTh_t, NULL, cmdLinTh, (void *)&iniparserInfo->msgLifetime);						

						
						// thread that collects and processes all the status responses - status_files
						pthread_t status_files_th_t;
						void* status;
						pthread_create(&status_files_th_t, NULL, status_files_th, NULL);
						pthread_join(status_files_th_t, &status);		
						
						fclose(fp_nam);

					} // end if(command[1] == "files") 
						
				} // end if command[0] == "status"

				else if(command[0] == "shutdown") {
						//kill_this_node(1); 
						searchTyped = FALSE;
						
						pthread_mutex_lock (&shutdown_mutex);
						timeToShutdown = TRUE;
						pthread_mutex_unlock (&shutdown_mutex);

						if(PRINT_FLAG)
							printf("received shutdown command from the user.. node is shutting \n");

						//exit(1);						
				} // end else if(command[0] == "shutdown") 

				else if(command[0] == "store") {		
					
					if(command.size() == 1)
						continue;

					if(command.size() < 3){
						continue;
					}	
					
					//todo wen to do fflush
					fflush(stdout);
					searchTyped = FALSE;

					 //------------------------------------------------------------
					Log logEvent = Log();
					logEvent.debug_log((char*)"store command received");
					//---------------------------------------------------------------
					
					pthread_mutex_lock(&storeFileNum_mutex);					
					storeFileNum = 0;
					pthread_mutex_unlock(&storeFileNum_mutex);

					int tokensize = command.size();

					// get the store file name
					const char *srcfile = command[1].c_str();
					
					// get the TTL
					const char *w = command[2].c_str();
					int TTL = atoi(w);
		
					// compute the filesize
					unsigned int fsize = computeFileSize('c', (char *)srcfile);
					
					string keywords;
					
					// always do this b4 passing it to the function that constructs the string
					unsigned char bitvec_str[2*BIT_VECTOR_LEN] = "";

					if(PRINT_FLAG){
						printf("filename %s ttl %d size %d \n", srcfile, TTL, fsize);
					}
					

					if(PRINT_FLAG){
						printf("total cmd tokens to parse %d, remaining %d \n", tokensize, tokensize-3);
					}

					keywords += command[3].c_str();
					
					//nbrList joinList = nbrList();
					bitvec fileBV;
					fileBV.setBV_keywd((char*)command[3].c_str());
					fileBV.printBV_hex();

					for(int i=4; i<tokensize; i++)
					{
						//convert the keyword tolower case
						tolowerKeyword((char*)command[i].c_str());

						fileBV.setBV_keywd((char*)command[i].c_str());
						fileBV.printBV_hex();

						// concatenate the string to keywords
						keywords += " ";
						keywords += command[i].c_str();
						
						if(PRINT_FLAG)
							printf("%s \n", keywords.c_str());
							
					}
					
					pthread_mutex_lock(&globalFileNum_mutex);
					globalFileNum++;
					unsigned int filenum = globalFileNum;
					pthread_mutex_unlock(&globalFileNum_mutex);

					pthread_mutex_lock(&storeFileNum_mutex);					
					storeFileNum = filenum; 
					pthread_mutex_unlock(&storeFileNum_mutex);

					//printf("filenum is %d \n", filenum);
					//printf("storeFileNum is %d \n", storeFileNum);
					
					// forms the file - keyword bit vector into hex string of 2*BIT_VECTOR_LEN bytes
					fileBV.storeBV_str(bitvec_str);
					

					// creating the "meta" file, "data" file, creates pass file, adding into all the indexs - addAllIndexNodes
					create_store_file(filenum, (char *)srcfile, fsize, (char *)keywords.c_str(), bitvec_str);


					/*********** TESTING ************************/
					/*addNodeInLRUList(filenum, fsize);
					displayLRUList();
					*/
					
					// compose the store message
					Message msg = Message();
					
					unsigned int metaSize = 0;
					unsigned int dataSize = 0;

					computeStoreMsgSize(filenum, metaSize, dataSize);
					
					unsigned int fwdDataLen = META_DATA_LEN  + metaSize + dataSize;
					unsigned int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;
/****/
					unsigned int fwdtillMetaLen = MAX_HEADER_LENGTH + META_DATA_LEN  + metaSize;
/****/
					msg.updateMsgHeader(STOR, TTL, fwdDataLen);
					msg.printHeader();

					unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * fwdtillMetaLen);	
					memset(&fwdBuf[0], 0, fwdtillMetaLen);

					msg.formMsgHeaderInBuffer(fwdBuf);

					// storing the metadata length field in the message: bytes 27-30
					metaSize = htonl(metaSize);
					memcpy(&fwdBuf[27], &metaSize, META_DATA_LEN);
					
					// storing the meta data from 31 - (31+metaSize-1)
					formMetadataInBuffer(filenum, 31, metaSize, fwdBuf);
					
					// storing the file data from 31+metaSize - (31+metaSize+dataSize-1)
					//formFiledataInBuffer(filenum, 31+metaSize, dataSize, fwdBuf);
					
					// flood this store request to all the neighbors 
					performFlooding_prob(fwdBuf, fwdTotalLen, filenum, iniparserInfo->neighborStoreProb);

					

					//fwdBuf[fwdTotalLen] = '\0';			
				
				} // end else if(command[0] == "store") 	
				
				else if(command[0] == "search") 
				{		
					
					if(command.size() == 1)
						continue;

					if(command.size() < 3){
						continue;
					}	
					
					//todo wen to do fflush
					fflush(stdout);
				
					searchTyped=TRUE;

					 //------------------------------------------------------------
					Log logEvent = Log();
					logEvent.debug_log((char*)"search command received");
					//---------------------------------------------------------------

					int tokensize = command.size();
					
					// get the search string
					const char *search_str = command[1].c_str();
					if(PRINT_FLAG)
					{
						printf("search string %s \n", search_str);
						printf("commands size %d \n", command.size());
					}
					
					// compose the store message
					Message msg = Message();

					string query;


					// the search type is 1 - exact file name search
					if(strncmp(search_str, "filename", strlen("filename")) == 0)
					{
						//printf("received filename search type \n");

						// get the filename to search
						query += command[2].c_str();
						msg.searchtype = SEARCH_FILENAME;

						//printf("filename to search %s \n", query.c_str());
						
					
					} // end if(strncmp(search_str, "filename", strlen("filename")) == 0)

					// the search type is 2 - exact SHA1 hash value
					else if(strncmp(search_str, "sha1hash", strlen("sha1hash")) == 0)
					{
					
						//printf("received sha1hash search type \n");

						// get the sha1hash to search
						query += command[2].c_str();
						msg.searchtype = SEARCH_SHA1HASH;

						/* unsigned char *search_sha1hash[2*SHA_DIGEST_LEN];
						memcpy(&search_sha1hash[0] ,command[2].c_str(), strlen(command[2].c_str())); 
						*/

						//printf("sha1hash to search %s \n", query.c_str());
					
					} // end if(strncmp(search_str, "sha1hash", strlen("sha1hash")) == 0)

					// the search type is 3 - on list of keywords
					else if(strncmp(search_str, "keywords", strlen("keywords")) == 0)
					{
						//printf("received keywords search type \n");											
						
						query += command[2].c_str();						
						msg.searchtype = SEARCH_KEYWORDS;

						for(int i=3; i<tokensize; i++)
						{
							//convert the keyword tolower case
							tolowerKeyword((char*)command[i].c_str());

							// concatenate the string to keywords
							query += " ";
							query += command[i].c_str();
							
								
						}

						if(PRINT_FLAG)
							printf("%s \n", query.c_str());
					

					} // end if(strncmp(search_str, "keywords", strlen("keywords")) == 0)
					
					
					unsigned int fwdDataLen = SEARCH_TYPE_LEN  + query.size();
					unsigned int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;
					//printf("fwdDataLen %d fwdTotalLen %d \n", fwdDataLen, fwdTotalLen);

					msg.updateMsgHeader(SHRQ, iniparserInfo->msgTTL, fwdDataLen);
					msg.printHeader();

					unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * (fwdTotalLen+1));	
					memset(&fwdBuf[0], 0, fwdTotalLen+1);

					msg.formMsgHeaderInBuffer(fwdBuf);

					
					// storing the search type len in the message: bytes 27
					memcpy(&fwdBuf[27], &msg.searchtype, SEARCH_TYPE_LEN);
					
					unsigned char *query_str = (unsigned char *)query.c_str();
					
					// storing the query in the message: bytes 28 - strlen(query)-1
					memcpy(&fwdBuf[28], &query_str[0], fwdDataLen-SEARCH_TYPE_LEN);
									
					// flood this store request to all the neighbors 
					performFlooding(fwdBuf, fwdTotalLen, 0);	
					
					//printf("%s \n", &fwdBuf[28]);

					fwdBuf[fwdTotalLen] = '\0';

					pthread_mutex_lock(&shortCircuit_mutex);
					shortCircuit = FALSE;
					pthread_mutex_unlock(&shortCircuit_mutex);
					
					// waiting for search responses after transmitting search request - search response timer thread
					pthread_t cmdLineTh_t;
					pthread_create(&cmdLineTh_t, NULL, cmdLinTh, (void *)&iniparserInfo->msgLifetime);						


					/**************************** DISPLAY SEARCH RESPONSES - HIT IN THE CURRENT NODE ************************************/
					// display search responses for the current node
					unsigned int rsize = 0;
					unsigned int rlen = 0;
					unsigned int fileRef[30];
					unsigned int nxtlen[30];
					
					int retValue = FALSE;

					// based on the search type, call the corresponding search record functions
					if(msg.searchtype == SEARCH_FILENAME)
						retValue = determineSearchRecordInfo_fname((char*)query.c_str(), rsize, rlen, fileRef, nxtlen);
					else if(msg.searchtype == SEARCH_SHA1HASH)
						retValue = determineSearchRecordInfo_sha1hash((char*)query.c_str(), rsize, rlen, fileRef, nxtlen);
					else if(msg.searchtype == SEARCH_KEYWORDS)
						retValue = determineSearchRecordInfo_kwrd((char*)query.c_str(), rsize, rlen, fileRef, nxtlen);
					
					unsigned int count = 0;

					// clear the search id list before populating it with the new search info
					//displaySearchIDMap();
					clearSearchIDMap();

					if(retValue == TRUE)
					{		
						// compose the record section																		
						for ( count=0; count < rlen; count++ )
						{		
							
							unsigned int metaSize = nxtlen[count];
							unsigned char fileID[FILE_ID_LEN];							
							string sha1key;

							if(PRINT_FLAG)
								printf("count %d metaSize %d fileRef[count] %d \n", count, metaSize, fileRef[count]);
	
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
								
							//unsigned char sha1_file[SHA_DIGEST_LEN] = "";
							unsigned char sha1_filestr[2*SHA_DIGEST_LEN+1] = "";

							char metaPath[256] = "";
							sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,fileRef[count]);
							get_sha1_from_metafile(metaPath, sha1_filestr);							
						
							addNodeInSearchIDMap(fileID, count+1, sha1_filestr);

							// in each iteration, display fileid & metadata of the current record
							printf("[%d]\tFileID=", count+1);
							
							for(int i=0;i<FILE_ID_LEN;i++)
							{
								printf("%02x", fileID[i]);
							}
							
							printf("\n");

							// print the meta data
							printMetadataFromFile_search(fileRef[count]);

						} //end for					

						// end composing the record section 

					} // end if(retValue == TRUE) - if atleast one record exist 

					/*****/

					// thread that collects and processes all the search responses from neighbors
					pthread_t search_resp_th_t;
					void* status;
					pthread_create(&search_resp_th_t, NULL, search_resp_th, (void *)count);
					pthread_join(search_resp_th_t, &status);		
			
				} // end else if(command[0] == "search") 

				else if(command[0] == "get") 
				{		
					/* Usage:
					 get 2 [<extfile>]
					 */
					/*
					If <extfile> is not specified, the filename to use should be the same as FileName in the file metadata. 
					If such a file already exists, you should ask the user if the file should be replaced. 
					*/
					if(command.size() == 1)
						continue;

					if(command.size() < 2){
						continue;
					}	
					
					//todo wen to do fflush
					fflush(stdout);

					 //------------------------------------------------------------
					Log logEvent = Log();
					logEvent.debug_log((char*)"get command received");
					//---------------------------------------------------------------
					
					if(searchTyped == TRUE)
					{
						extfileSpec = FALSE;
						getfile[0] = '\0';

						// extfile is not specified
						if(command.size() == 2)
						{							
							extfileSpec = FALSE;								
						}
						
						// external file is specified
						if(command.size() == 3)
						{					
							//printf("extfile is specified \n");					
							extfileSpec = TRUE;

							// get the output file name
							
							const char *file_tmp = command[2].c_str();
							memcpy(&getfile[0], file_tmp , strlen(file_tmp));
							getfile[strlen(file_tmp)] = '\0';
												
						}
						
						// get the search option num
						const char *w = command[1].c_str();
						int searchOption = atoi(w);				
						
						//printf("searchOption is %d \n", searchOption);

						// check whether the requested file option is valid or not by looking up in the search id list
						if(findFileIDInSearchIDMap(searchOption)==TRUE)
						{
							unsigned char fileID[FILE_ID_LEN];
							unsigned char sha1_file[SHA_DIGEST_LEN]; 
							unsigned int fileRef = 0;

							getFileIDFromSearchIDMap(searchOption, fileID);
							getSHA1FromSearchIDMap(searchOption, sha1_file);

							// requested file exists in the current node, so no get message is flooded
							if(findIDMatchInFileIDMap(fileID) == TRUE)
							{
								
								fileRef = getFileRefFromFileIDMap(fileID);
								if(PRINT_FLAG)
								{
									printf("fileID found in the current node, so not sending the get message to neighbors \n");
									printf("**** FILEID NODE EXISTING is %02x%02x%02x%02x \n", fileID[16], fileID[17], fileID[18], fileID[19]);
								}

								// check whether the requested file exists in the cache area, if it is, then remove it from the LRU list
								if(findFileInLRUList(fileRef)==TRUE)
								{
									deleteNodeInLRUList(fileRef);
									displayAllIndexNodes();
							
								}
								
								// take the filename from the metadata
								if(extfileSpec == FALSE)
								{
									// copying the "data" file into current working directory in either cases
									char metaPath[256] = "";
									sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,fileRef);

									get_fname_from_metafile(metaPath, getfile);
								}

								// if the file exists in the permanent area, then just make a copy in the current working directory

								// copying the "data" file into current working directory in either cases
								char srcFilePath[256] = "";
								sprintf(srcFilePath, "%s/files/%d.data", iniparserInfo->myhomedir,fileRef);
								
								// check whether the file with the same name as getfile exist in the current working directory, if it exists, should prompt for the replacement
								char replace[20] = "\0";

								int exist = computeFileSize('c', getfile);
								if(exist == -1)
								{
									//printf("getfile %s does not exist in the current working directory return value %d \n", getfile, exist);
									replace[0] = 'y';
								}
								else
								{
									//printf("getfile %s does exist in the current working directory return value %d \n", getfile, exist);
									printf("Okay to replace %s in your current working directory? [yes/no] ", getfile);
									scanf("%s", replace);
									tolowerKeyword(replace);
									//printf("option choosen is %s \n" , replace);								
								}

								if(replace[0] == 'y')
								{
									//printf("user decided to copy it if it exists or else creating a new copy \n");
									create_data_file(srcFilePath,getfile);
								}

							} // get within my node

							// flood the get message by composing it and pushing it in the sent event queue
							else
							{
								if(PRINT_FLAG)
								{
									printf("fileID not found in the current node, so sending the get message to neighbors \n");
									printf("**** FILEID NOT NODE EXISTING %02x%02x%02x%02x: \n", fileID[16], fileID[17], fileID[18], fileID[19]);
								}					
								
								// compose the get message

								Message msg = Message();

								unsigned int fwdDataLen = UOID_LEN  + SHA_DIGEST_LEN;
								unsigned int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;
								//printf("fwdDataLen %d fwdTotalLen %d of GET req \n", fwdDataLen, fwdTotalLen);

								msg.updateMsgHeader(GTRQ, iniparserInfo->msgTTL, fwdDataLen);
								msg.printHeader();

								unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * (fwdTotalLen+1));	
								memset(&fwdBuf[0], 0, fwdTotalLen+1);

								msg.formMsgHeaderInBuffer(fwdBuf);

								// storing the Get FileID and SHA1 has value of the file with the above FileID
								memcpy(&fwdBuf[27], &fileID[0], UOID_LEN);														
								memcpy(&fwdBuf[47], &sha1_file[0], SHA_DIGEST_LEN);

								fwdBuf[fwdTotalLen] = '\0';				
								
								// flood this store request to all the neighbors 
								performFlooding(fwdBuf, fwdTotalLen, 0);			

								pthread_mutex_lock(&shortCircuit_mutex);
								shortCircuit = FALSE;
								pthread_mutex_unlock(&shortCircuit_mutex);		

								// waiting for get responses after transmitting get request - get response timer thread
								pthread_t cmdLineTh_t; 
								pthread_create(&cmdLineTh_t, NULL, cmdLinTh, (void *)&iniparserInfo->getMsgLifetime);						

								// thread that collects and processes the get response from neighbor
								pthread_t get_resp_th_t;
								void* status;
								pthread_create(&get_resp_th_t, NULL, get_resp_th, NULL);
								pthread_join(get_resp_th_t, &status);	
								
							} // flood the get message as the fileid is not found in my node

						
						} // end if(findFileIDInSearchIDMap(searchOption)==TRUE) valid option specified 
					}
				
				} // end else if(command[0] == "get") 

				else if(command[0] == "delete") 
				{		

					searchTyped = FALSE;
					 //------------------------------------------------------------
					Log logEvent = Log();
					logEvent.debug_log((char*)"delete command received");
					//---------------------------------------------------------------

					int tokensize = command.size();
					//printf("#tokens %d \n", tokensize);
					
					// get the search string
					const char *fname_str = command[2].c_str();

					unsigned char sha1_str[2*SHA_DIGEST_LEN+1] =  "";
					memcpy(&sha1_str[0], command[4].c_str() ,2*SHA_DIGEST_LEN);
					sha1_str[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character

					unsigned char nonce_sha1hex[2*SHA_DIGEST_LEN+1] =  "";
					memcpy(&nonce_sha1hex[0], command[6].c_str() ,2*SHA_DIGEST_LEN);
					nonce_sha1hex[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character
					
					if(PRINT_FLAG)
					{
						printf("fname_str string %s \n", fname_str);
						printf("sha1_str %sEEEE \n", sha1_str);
						printf("nonce_sha1hex %sEEEE \n", nonce_sha1hex);
						
						printf("\n");
					}

					// check whether the password file exist for this file info
					char *sha1_string = (char *)sha1_str;
					unsigned int fileRef[30];
					unsigned int fileCnt = 0;
					unsigned int fileFound = 0;

					fileCnt = getCountForShaKey(sha1_string); //printf("filecnt %d \n", fileCnt);
					getValueFromShaIndex (sha1_string, fileRef);

					unsigned char pwdUOID[UOID_LEN+1] = "";
					unsigned char pwdUOID_hex[2*UOID_LEN+1] = ""; // always do this b4 passing it to the function that constructs the string
					
					/* check if OTP can be found for user provided filename, SHA1 and nonce
					if found, take the OTP for composing the delete message
					or else, generate a random 20-byte password					 
					*/
					if(deletePwdhit(fileRef, fileCnt, nonce_sha1hex, pwdUOID_hex, fileFound) == TRUE)
					{
					
						//printf("return true \n");
						//printf("matching password found is %s found in file %d \n", pwdUOID_hex, fileFound);

						deleteAllIndexNodes(fileFound);
						displayAllIndexNodes();

						// delete the actual files in the mini dir
						deleteFiles(3, fileFound);
					}
					
					// generate the random password. prompt user. then compose the delete message
					else
					{
						char candelete[20] = "\0";

						printf("No one-time password found.\n");
						printf("Okay to use a random password [yes/no]?");
						scanf("%s", candelete);
						tolowerKeyword(candelete);
						//printf("option choosen is %s \n" , candelete);								


						if(candelete[0] == 'y')
						{
							//printf("user decided to delete the file using random password \n");
							


							// A random 20-byte long one-time password is first generated
							gen_onetime_pwd(pwdUOID);
							pwdUOID[UOID_LEN] = '\0';
						
							// generate 40-char string corresponding to the password
							convert_sha1_to_hex(pwdUOID, pwdUOID_hex);  
							pwdUOID_hex[2*UOID_LEN] = '\0';

							//printf("RANDOM PASSWORD GENERATED : %s \n", pwdUOID_hex);

						}
						else
						{
							// user decided not to delete the file using the random password
							//printf("user decided not to delete the file using the random password \n");
							continue;
						}
						

					} // end else if(deletePwdhit(fileRef, fileCnt, nonce_sha1hex, pwdUOID_hex) == TRUE)

					pthread_mutex_lock(&temp_globalFileNum_mutex);
					temp_globalFileNum++;
					unsigned int filenum = temp_globalFileNum;
					pthread_mutex_unlock(&temp_globalFileNum_mutex);

					// create delete file spec
					create_delete_filespec(filenum, (char *)fname_str, sha1_str, nonce_sha1hex, pwdUOID_hex);

					// compute the size of the file					
					char filespecPath[256] = "";
					sprintf(filespecPath, "%d.filespec", filenum);
					unsigned int filespecSize = computeFileSize('h', filespecPath);
					//printf("filespecSize %d \n", filespecSize);

					
					// compose the store message
					Message msg = Message();
					unsigned int fwdDataLen = filespecSize;
					unsigned int fwdTotalLen = MAX_HEADER_LENGTH + fwdDataLen;
					
					//printf("DELETE MSG fwdDataLen %d fwdTotalLen %d \n", fwdDataLen, fwdTotalLen);

					msg.updateMsgHeader(DELT, iniparserInfo->msgTTL, fwdDataLen);
					msg.printHeader();

					unsigned char* fwdBuf = (unsigned char*) malloc(sizeof(char) * (fwdTotalLen+1));	
					memset(&fwdBuf[0], 0, fwdTotalLen+1);

					msg.formMsgHeaderInBuffer(fwdBuf);

					// store the file into the buffer
					formFileSpecInBuffer(filenum, 27, filespecSize, fwdBuf);

					fwdBuf[fwdTotalLen] = '\0';				
								
					// flood this store request to all the neighbors 
					performFlooding(fwdBuf, fwdTotalLen, 0);							
					

				} // end else if(command[0] == "delete") 

			} // end if (FD_ISSET(STDIN_FILENO, &readfds))

		} // end if (s > 0) 
			
		else {
			//printf("Time out :)");
			fflush(stdout);
		}			


	} // end main while loop waiting for user inputs - trap signal

	if(PRINT_FLAG)
		printf("cmd_thread is exiting \n");

	pthread_exit(NULL);
}


void *timer_thread(void *)
{

	unsigned int autoShutDownTimer = iniparserInfo->autoShutDown;

	Log logEvent = Log();
	logEvent.debug_log((char*)"entered timer thread");

	while(1)
	{
		   //sleep for 1 sec
		sleep(1);

		// check whether autoshutdown expired
		autoShutDownTimer--;

		if(autoShutDownTimer == 0)
		{
			pthread_mutex_lock (&shutdown_mutex);
			timeToShutdown = TRUE;
			pthread_mutex_unlock (&shutdown_mutex);


			logEvent.debug_log((char*)"autoshutdown timer expired");

			if(PRINT_FLAG)
				printf("AUTOSHUTDOWN TIMER EXPIRED \n");
			
			// broadcast all the cvs
			pthread_cond_broadcast(&eventQueue_cv);
			pthread_cond_broadcast(&cmdLineQueue_cv); 

			break;
		}
		
		
		pthread_mutex_lock (&shutdown_mutex);
		if(timeToShutdown == TRUE)
		{		
			pthread_mutex_unlock (&shutdown_mutex);

			logEvent.debug_log((char*)"SHUTDOWN COMMAND RECEIVED");
			
			if(PRINT_FLAG)
				printf("SHUTTING DOWN DUE TO USER SHUTDOWN COMMAND \n");
			
			// broadcast all the cvs
			pthread_cond_broadcast(&eventQueue_cv);
			pthread_cond_broadcast(&cmdLineQueue_cv); 

			break;
		}
		pthread_mutex_unlock (&shutdown_mutex);
		

	} //end main while






	if(PRINT_FLAG)
		printf("timer_thread is exiting \n");

	pthread_exit(NULL);


}
