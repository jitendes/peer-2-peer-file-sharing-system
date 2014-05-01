#include "iniparserClass.h"
extern iniparserClass *iniparserInfo;

	/**************************** DEFINE FUNCTION MEMBERS ************************************/
extern FILE *fp;
extern int fileOpen;
extern pthread_mutex_t  log_mutex;

Log::Log() // default constructor
{
		size = 0;
}

Log::~Log() // default destructor
{
	//delete(this);
}

	/**************************** COMMON METHODS ************************************/
int iflog_file_exist()
{
	//printf("messagetypeis %c\n",msgtype);

	char fileName[100];
    sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"servant.log");
	//printf("log file to be creared is %s\n",fileName);
    fp = fopen(fileName, "r");	
	if(!fp) {
		printf("Error logfile does not exist\n");
		return FALSE;
	}
	return TRUE;
}

int create_logfile()
{
	char fileName[100];
    sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"servant.log");
    fp = fopen(fileName, "w");	
	if(!fp) {
		//printf("Error log filr could not be created does not exist\n");
		
		return FALSE;
	}
	//printf("sucessfully created file\n");
    fileOpen=TRUE;
	return TRUE;


}

int del_Log()
{
	char fileName[100];
	sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"servant.log");
	int q=remove(fileName);
	return q;
}


void Log::write_log()    //return type is 
{
	pthread_mutex_lock(&log_mutex);

	char fileName[100];	 
	sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"servant.log");
	//printf("log file to be opened in appened mode is %s\n",fileName);
			//printf("INSIDE WRITE_LOG messagetype is %c\n",msgtype);
   // FILE * pFile;
    long sizef;

    fp = fopen (fileName,"a");
    if (fp==NULL) {
		perror ("Error in opening log file");
		exit(0);
	}
    else
    {
    fseek (fp, 0, SEEK_END);
    sizef=ftell(fp);
   // printf ("Size of LOG file is: %ld bytes.\n",sizef);
    }
	//printf("second is %ld\n",sec);


	int r;

	switch(msgtype)
	{
		case JNRQ:   //to do make sure we need %02x for msgtype
			
			//printf("%c, %10ld.%03ld, %s , %ld, %d, %d, %02x,%02x,%02x,%02x, %d, %s \n",type ,sec, msec, from,"JNRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_port, d_hostname);
			if(type=='r')
				r=fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %d %s \n",type ,sec, msec, from,"JNRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_port, d_hostname);
			else
				r=fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %d %s \n",type ,sec, msec, to,"JNRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_port, d_hostname);

			fflush(fp);
			//printf("no of bytes written is %d\n",r);
			//fprintf(fp, "%c, %010ld.%03ld, %02x, %ld, %d, %s, %d, %s \n",type ,sec, msec, msgtype, size, TTL, msgid, d_port, d_hostname);

			// myfile << type ,sec, msec, msgtype, size, TTL, msgid, d_port, d_hostname;
	        break;
		case JNRS:        //join response	<uoid> <distance> <port> <hostname>
            // to do distance 
	        // printf("%c, %10ld.%03ld, %s, %ld, %d, %02x%02x%02x%02x,  \n",type ,sec, msec, "JNRS", size, TTL, d_UOID[16]d_UOID[17],d_UOID[18],d_UOID[19], <uoid> <distance> <port>  <hostname>);
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x %d %d %s\n", type ,sec, msec, from,"JNRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19],d_distance, d_port,d_hostname);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x %d %d %s\n", type ,sec, msec, to,"JNRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19],d_distance, d_port,d_hostname);

			fflush(fp);

		//	fprintf(fp, "%c, %010ld.%03ld, %02x, %ld, %d, %s, %s, %d, %d, %s \n",type ,sec, msec, msgtype, size, TTL, msgid, d_UOID, d_distance,d_port,d_hostname );

			break;
		case HLLO:       //hello	<port> <hostname>
			//printf( "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %d %s \n",type ,sec, msec, to,"HLLO", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_port, d_hostname);

              //printf("%c, %10ld.%03ld, %s, %ld, %d, %s, %d, %s \n",type ,sec, msec, "HLLO", size, TTL, msgid, d_port, d_hostname);
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %d %s \n",type ,sec, msec, from,"HLLO", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_port, d_hostname);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %d %s \n",type ,sec, msec, to,"HLLO", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19] , d_port, d_hostname);

			fflush(fp);

			//fprintf(fp, "%c, %010ld.%03ld, %02x, %ld, %d, %s, %d, %s \n",type ,sec, msec, msgtype, size, TTL, msgid, d_port, d_hostname);
			break;
		case KPAV:       //keepalive	(none)

             // printf("%c, %10ld.%03ld, %s, %ld, %d, %s, %d, %s \n",type ,sec, msec, "KPAV", size, TTL, msgid, d_port, d_hostname);
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x \n",type ,sec, msec, from,"KPAV", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19]);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x \n",type ,sec, msec, to,"KPAV", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19]);

			fflush(fp);

			break;
		case NTFY:       //notify	<errorcode>

             //printf("%c, %10ld.%03ld, %s, %ld, %d, %s, %d, %s \n",type ,sec, msec, "NTFY", size, TTL, msgid, d_port, d_hostname);
			 if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %d\n",type ,sec, msec, from,"NTFY", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19],d_errorcode);
			 else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %d\n",type ,sec, msec, to,"NTFY", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19],d_errorcode);

			 fflush(fp);


			break;
		case CKRQ:       //check request	(none)
				// todo 
			//printf("%c, %10ld.%03ld, %s, %ld, %d, %s, %d, %s \n",type ,sec, msec, "CKRQ", size, TTL, msgid, d_port, d_hostname);

			fflush(fp);
			break;
		case CKRS:       //check response	<uoid>
			
				//todo
			//printf("%c, %10ld.%03ld, %s, %ld, %d, %s, %d, %s \n",type ,sec, msec, "CKRS", size, TTL, msgid, d_port, d_hostname);
			fflush(fp);

			break;

		case SHRQ:       //search request	<searchtype> <query>
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x 0x%02x %s \n",type ,sec, msec, from,"SHRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_searchtype, d_query);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x 0x%02x %s \n",type ,sec, msec, to,"SHRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_searchtype, d_query);

			fflush(fp);

			break;

		case SHRS:       //search response	<uoid>
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, from,"SHRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, to,"SHRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);


			fflush(fp);
			break;
		case GTRQ:       //get request	<fileid>
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, from,"GTRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, to,"GTRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);


			fflush(fp);
			break;
		case GTRS:       //get response	<uoid>
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, from,"GTRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, to,"GTRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);


			fflush(fp);
			break;
		case STOR:       //store	(none)
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x \n",type ,sec, msec, from,"STOR", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19]);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x \n",type ,sec, msec, to,"STOR", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19]);

			fflush(fp);

			break;

		case DELT:       //delete	(none)
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x \n",type ,sec, msec, from,"DELT", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19]);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x \n",type ,sec, msec, to,"DELT", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19]);

			fflush(fp);

			break;

		case STRQ:       //status request	<statustype>

              //printf("%c, %10ld.%03ld, %s, %ld, %d, %s, %d, %s \n",type ,sec, msec, "STRQ", size, TTL, msgid, d_port, d_hostname);
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x 0x%02x \n",type ,sec, msec, from,"STRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_statustype);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x 0x%02x \n",type ,sec, msec, to,"STRQ", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_statustype);


			fflush(fp);

			break;

		case STRS:       //status response	<uoid>
			
			if(type=='r')
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, from,"STRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);
			else
				fprintf(fp, "%c %10ld.%03ld %s %s %ld %d %02x%02x%02x%02x %02x%02x%02x%02x \n",type ,sec, msec, to,"STRS", size,TTL,UOID[16],UOID[17],UOID[18],UOID[19], d_UOID[16],d_UOID[17],d_UOID[18],d_UOID[19]);


			fflush(fp);
			break;
		default:



			break;
		

	} // end switch case 

	pthread_mutex_unlock(&log_mutex);

	fclose(fp);

}

/* 
You should write error messages into you log file and you may also output debugging information there. 
Please begin each debugging lines with // (like a comment line in C++) and error messages with **. 
*/
void Log::error_log(char *str)    //return type is 
{
	pthread_mutex_lock(&log_mutex);

	char fileName[100];	 
	sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"servant.log");
	//printf("log file to be opened in appened mode is %s\n",fileName);
			
    fp = fopen (fileName,"a");
    if (fp==NULL) {
		perror ("Error in opening file");
		exit(0);
	}
    else
    {
		fseek (fp, 0, SEEK_END);
		//int sizef=ftell(fp);
		//printf ("Size of LOG file is: %ld bytes.\n",sizef);
    }


	fprintf(fp, "**%s\n", str);

	fflush(fp);

	pthread_mutex_unlock(&log_mutex);

	fclose(fp);
}

void Log::debug_log(char *str)    //return type is 
{
	pthread_mutex_lock(&log_mutex);

	char fileName[100];	 
	sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"servant.log");
	//printf("log file to be opened in appened mode is %s\n",fileName);
			
    fp = fopen (fileName,"a");
    if (fp==NULL) {
		perror ("Error in opening file");
		exit(0);
	}
    else
    {
		fseek (fp, 0, SEEK_END);
		//int sizef=ftell(fp);
		//printf ("Size of LOG file is: %ld bytes.\n",sizef);
    }


	fprintf(fp, "//%s\n", str);

	fflush(fp);

	pthread_mutex_unlock(&log_mutex);

	fclose(fp);
}

void logHLLOMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned short int d_port, char d_hostname[50])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logHLLOMsg: received hello msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logHLLOMsg: received hello msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);
	//printf("in logHLLOMsg: received hello msg.. UOID is %02x%02x%02x%02x \n", UOID[16], UOID[17],logEvent.UOID[18],logEvent.UOID[19]);	
	//printf("in logHLLOMsg: received hello msg.. UOID is %02x%02x%02x%02x \n", logEvent.UOID[16],logEvent.UOID[17],logEvent.UOID[18],logEvent.UOID[19]);	

	// log the data section for the HELLO msg
	logEvent.d_port = d_port;
	memset(logEvent.d_hostname, '\0', strlen(logEvent.d_hostname));
	memcpy(logEvent.d_hostname, d_hostname, strlen(d_hostname));
	logEvent.d_hostname[strlen(d_hostname)] = '\0';
	logEvent.write_log();
} // end logHLLOMsg

void logJNRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned short int d_port, char d_hostname[50])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logJNRQMsg: received join msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logJNRQMsg: received join req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);
	//printf("in logJNRQMsg: received join req msg.. UOID is %02x%02x%02x%02x \n", UOID[16], UOID[17],logEvent.UOID[18],logEvent.UOID[19]);	
	//printf("in logJNRQMsg: received join req msg.. UOID is %02x%02x%02x%02x \n", logEvent.UOID[16],logEvent.UOID[17],logEvent.UOID[18],logEvent.UOID[19]);	

	// log the data section for the JOIN REQUEST msg
	logEvent.d_port = d_port;
	memset(logEvent.d_hostname, '\0', strlen(logEvent.d_hostname));
	memcpy(logEvent.d_hostname, d_hostname, strlen(d_hostname));
	logEvent.d_hostname[strlen(d_hostname)] = '\0';
	
	logEvent.write_log();

} // end logJNRQMsg

void logJNRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, unsigned char TTL, 
				unsigned char UOID[UOID_LEN], unsigned char d_UOID[UOID_LEN], unsigned int d_distance, unsigned short int d_port, char d_hostname[50])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logJNRSMsg: received join msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logJNRSMsg: received join req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);
	//printf("in logJNRSMsg: received join resp msg.. UOID is %02x%02x%02x%02x \n", UOID[16], UOID[17],logEvent.UOID[18],logEvent.UOID[19]);	
	//printf("in logJNRSMsg: received join resp msg.. UOID is %02x%02x%02x%02x \n", logEvent.UOID[16],logEvent.UOID[17],logEvent.UOID[18],logEvent.UOID[19]);	

	// log the data section for the JOIN RESPONSE msg
	memset(logEvent.d_UOID, 0, UOID_LEN);
	memcpy(logEvent.d_UOID, d_UOID, UOID_LEN);
	//logEvent.d_hostname[strlen(d_hostname)] = '\0';
	//printf("in logJNRSMsg: received join resp msg.. UOID is %02x%02x%02x%02x \n", logEvent.d_UOID[16],logEvent.d_UOID[17],logEvent.d_UOID[18],logEvent.d_UOID[19]);	


	logEvent.d_distance = d_distance;
	logEvent.d_port = d_port;

	memset(logEvent.d_hostname, '\0', strlen(logEvent.d_hostname));
	memcpy(logEvent.d_hostname, d_hostname, strlen(d_hostname));
	logEvent.d_hostname[strlen(d_hostname)] = '\0';
	
	logEvent.write_log();

} // end logJNRQMsg

void logSTRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char d_statustype)
{
	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logSTRQMsg: received join msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logJNRSMsg: received join req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);

	// log the data section for the STRQ msg
	//memset(logEvent.d_statustype, 0, STATUS_TYPE_LEN);
	//memcpy(logEvent.d_statustype, d_statustype, STATUS_TYPE_LEN);
	logEvent.d_statustype = d_statustype; 

	
	logEvent.write_log();

}

void logSTRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logSTRSMsg: received join msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logSTRSMsg: received join req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);

	// log the data section for the STRQ msg
	memset(logEvent.d_UOID, 0, UOID_LEN);
	memcpy(logEvent.d_UOID, dUOID, UOID_LEN);
	
	logEvent.write_log();



}

void logKPAVMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH); //printf("in logKPAVMsg: receiving kpav msg.. rfsNodeID is %s \n",logEvent.to);		
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logKPAVMsg: SENDING kpav msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logKPAVMsg: received KPAV req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);

	// not logging as empty data 
	
	logEvent.write_log();


} //END logKPAVMsg

void logSTORMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH); //printf("in logKPAVMsg: receiving kpav msg.. rfsNodeID is %s \n",logEvent.to);		
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logKPAVMsg: SENDING kpav msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logKPAVMsg: received KPAV req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);
	
	logEvent.write_log();


} //END logSTORMsg


void logSHRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char d_searchtype, char *d_query)
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH); //printf("in logKPAVMsg: receiving kpav msg.. rfsNodeID is %s \n",logEvent.to);		
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logKPAVMsg: SENDING kpav msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logKPAVMsg: received KPAV req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);

	// search type n query
	logEvent.d_searchtype = d_searchtype; 

	logEvent.d_query = (char *)malloc(strlen(d_query)+1); 
	//(unsigned char*) malloc(sendTotalLen+1);	
	memset(logEvent.d_query, '\0', strlen(logEvent.d_query));
	memcpy(logEvent.d_query, d_query, strlen(d_query));
	logEvent.d_query[strlen(d_query)] = '\0';
	
	logEvent.write_log();


} //END logSHRQMsg

void logSHRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logSTRSMsg: received join msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logSTRSMsg: received join req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);

	// log the data section for the SHRS msg
	memset(logEvent.d_UOID, 0, UOID_LEN);
	memcpy(logEvent.d_UOID, dUOID, UOID_LEN);
	
	logEvent.write_log();


} //END logSHRSMsg

void logGTRQMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logSTRSMsg: received join msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logSTRSMsg: received join req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);

	// log the data section for the GTRQ msg
	memset(logEvent.d_UOID, 0, UOID_LEN);
	memcpy(logEvent.d_UOID, dUOID, UOID_LEN);
	
	logEvent.write_log();


} //END logGTRQMsg

void logGTRSMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN], unsigned char dUOID[UOID_LEN])
{
	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH);
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logSTRSMsg: received join msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logSTRSMsg: received join req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);

	// log the data section for the GTRS msg
	memset(logEvent.d_UOID, 0, UOID_LEN);
	memcpy(logEvent.d_UOID, dUOID, UOID_LEN);
	
	logEvent.write_log();

}


void logDELTMsg(char rfs, struct timeval logtime, char ft_NodeID[MAX_NODE_ID_LENGTH], unsigned char msgType, unsigned int totalLen, 
				unsigned char TTL, unsigned char UOID[UOID_LEN])
{

	//create log object
	Log logEvent = Log();
	logEvent.type = rfs;

	// obtain the current time for logging
	logEvent.sec = logtime.tv_sec;
	logEvent.msec = logtime.tv_usec/1000.0;

	// create node id of the sender
	if(rfs == 'r')
	{
		memset(logEvent.from, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.from, ft_NodeID, MAX_NODE_ID_LENGTH); //printf("in logKPAVMsg: receiving kpav msg.. rfsNodeID is %s \n",logEvent.to);		
	}
	else
	{
		memset(logEvent.to, 0, MAX_NODE_ID_LENGTH);
		memcpy(logEvent.to, ft_NodeID, MAX_NODE_ID_LENGTH);	//printf("in logKPAVMsg: SENDING kpav msg.. rfsNodeID is %s \n",logEvent.to);			
	}

	logEvent.msgtype = msgType;
	logEvent.size = totalLen; //printf("in logKPAVMsg: received KPAV req msg.. size is %ld \n", logEvent.size);					
	logEvent.TTL = (int)TTL;

	// log the UOID of the sender in the case of r log type or receiver in the case of fs logtype
	memset(logEvent.UOID, 0, UOID_LEN);
	memcpy(logEvent.UOID, UOID, UOID_LEN);
	
	logEvent.write_log();


} //END logDELTMsg
