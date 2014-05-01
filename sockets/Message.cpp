#include "iniparserClass.h"

/************************************************** DEFINE MESSAGE CLASS **********************************************************/

///// TO BE SHIFTED TO MESSAGE.H FILE ONCE MULTIPLE REDEFINTION PROBLEM IS FIXED 

extern iniparserClass *iniparserInfo;
extern char nodeInstanceID[MAX_NODE_INSTANCE_ID_LENGTH];
/**************************** DEFINE CLASS STATIC MEMBERS************************************/


/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* DEFAULT CONSTRUCTOR */
Message::Message()
{
	// empty body 
   


}

/* CONSTRUCTOR */
void Message::updateMsgHeader(unsigned char msgType1,unsigned int msgTTL, unsigned int dataLen1)
{

	msgType = (unsigned char)msgType1;
	TTL = (unsigned char)msgTTL;
	memset(&resvd, 0, sizeof(char));
	dataLen = htonl(dataLen1); 
    getuoid(nodeInstanceID, (char *)"msg",(char*) UOID, 20);
	// todo to be inserted when GetUOID is implemented correctly
	//strcpy((char *)UOID, (const char *)"12222");
	//UOID[0] = '\0';
	

	//data = (unsigned char*) malloc(sizeof(unsigned char) *sendTotalLen);	
	/*if(dataLen1!=0)
	{
		data = (unsigned char*) malloc(dataLen);	
		memset(&data[0], '\0', dataLen);
	}*/

	printHeader();
}

/* DESTRUCTOR */
Message::~Message()
{
	
	/*if(data!=NULL || strlen((char *)data)!=0)
		free(data); */

	//delete(this);


}
		/**************************** COMMON METHODS ************************************/

// define this size at the place where u call the getuoid function int  uoid_buf_sz=20
int Message::getuoid(char *node_inst_id, char *obj_type, char *uoid_buf, unsigned int uoid_buf_size)
{
	static unsigned long seq_no = (unsigned long)1;
	char sha1_buf[SHA_DIGEST_LEN] , str_buf[104];
	snprintf(str_buf, sizeof(str_buf), "%s_%s_%1ld", node_inst_id, obj_type, (long)seq_no++);
	SHA1((unsigned char *)str_buf, strlen(str_buf), (unsigned char *)sha1_buf);
	memset(uoid_buf, 0, uoid_buf_size);
	memcpy(uoid_buf, sha1_buf, min(uoid_buf_size, sizeof(sha1_buf)));
	return 1;
}



// writes into msg buffer the header fields contained in this object
void Message::formMsgHeaderInBuffer(unsigned char* msg)
{
	dataLen = htonl(dataLen);
	memcpy(&msg[0], &msgType, MSG_TYPE_LEN);
	memcpy(&msg[1],(const char *)&UOID[0], UOID_LEN);
	memcpy(&msg[21], &TTL, TTL_LEN);
	memcpy(&msg[22], &resvd, RESERVED_LEN);
	memcpy(&msg[23], &dataLen, DATA_LENGTH_LEN);	
	//printf("formMsgHeaderInBuffer: ");//msgtype %02x, ttl %d, datalen %d \n", msgType, TTL, dataLen);

}



// reads the header fields from msg buffer and writes into this object
void Message::formMsgHeaderFromBuffer(unsigned char* msg)
{
	memcpy(&msgType, &msg[0], MSG_TYPE_LEN);
	memcpy(&UOID[0], (const char*)&msg[1],UOID_LEN);
	memcpy(&TTL, &msg[21], TTL_LEN);
	memcpy(&resvd, &msg[22], RESERVED_LEN);
	memcpy(&dataLen, &msg[23], DATA_LENGTH_LEN);	
	dataLen = ntohl(dataLen);
	//printf("formMsgHeaderFromBuffer: ");//msgtype %02x, ttl %d, datalen %d \n", msgType, TTL, dataLen);
	//printf("formMsgHeaderFromBuffer: msgtype %02x, ttl %d, datalen %d \n", msgType, TTL, dataLen);

}

void Message::printHeader()
{
	
	if(PRINT_FLAG)
		printf("message: msgtype %02x, ttl %d, datalen %d \n", msgType, TTL, dataLen);

}



/******************************************* MSG - SPECIFIC FUNCTIONS ***********************************************/

/* Part 1 Project */

		/**************************** JOIN MSG - SPECIFIC METHODS ************************************/
// todo tested
void Message::formJoinReq(unsigned int mylocation, unsigned short int myport, char* myhostname, unsigned char *joinReq)
{
	unsigned int location = htonl(mylocation);
	unsigned short int port = htons(myport);

	memcpy(&joinReq[27], &location, HOST_LOCATION_LEN);
	memcpy(&joinReq[31], &port, HOST_PORT_LEN);
	memcpy(&joinReq[33],myhostname, strlen(myhostname));

	dataLen = getDataLenJoinReq(myhostname);
	
	// update the data field of the message from the joinReq
	//if(data==NULL || strlen((char *)data)==0)
	{
		data = (unsigned char*) malloc(dataLen);	
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &joinReq[27], dataLen);
	}

	//printf("form join req data : loc %d, port %d, hostname %s \n",location, port, myhostname);
	//data[dataLen] = '\0';
}

// todo tested
void Message::recvJoinReq(unsigned int &mylocation, unsigned short int &myport, char* myhostname, unsigned char *joinReq)
{
	//printf("************************* \n recvHelloMsg my data is %d ********************\n", strlen((char *)data));

	//if(data==NULL)// || strlen((char *)data)==0) // sometimes giving bugs, sometimes not !!! should check it out
	{
		data = (unsigned char*) malloc(dataLen);	
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &joinReq[27], dataLen);
	}

	memcpy(&mylocation, &joinReq[27],HOST_LOCATION_LEN);
	memcpy(&myport, &joinReq[31], HOST_PORT_LEN);
	memcpy(myhostname, &joinReq[33], (dataLen-HOST_LOCATION_LEN-HOST_PORT_LEN));

	mylocation = ntohs(mylocation);
	myport = ntohs(myport);

	//printf("recv join req data : loc %d, port %d, hostname %s \n",mylocation, myport, myhostname);
	//data[dataLen] = '\0';


}

// todo havent tested
void Message::formJoinResp(unsigned char joinUOID[UOID_LEN], unsigned int dist, unsigned short int myport, char* myhostname, unsigned char *joinResp)
{
	//unsigned int dist1 = htonl(dist);
	unsigned short int port = htons(myport);

	memset(d_UOID, 0, UOID_LEN);
	memcpy(&joinResp[27],&d_UOID[0], UOID_LEN);

	memcpy(&joinResp[27], &joinUOID[0], UOID_LEN);
	memcpy(&joinResp[47], &dist, DISTANCE_LEN);
	memcpy(&joinResp[51], &port, HOST_PORT_LEN);
	memcpy(&joinResp[53], myhostname, strlen(myhostname));
	
	dataLen = getDataLenJoinResp(myhostname);

	// update the data field of the message from the joinResp
	//if(data==NULL)// || strlen((char *)data)==0)
	{
		data = (unsigned char*) malloc(dataLen);	
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &joinResp[27], dataLen);
	}

	//printf("form join resp data : dist %d, port %d, hostname %s \n", dist1, port, myhostname);
	//data[dataLen] = '\0';
}

// todo havent tested
void Message::recvJoinResp(unsigned char joinUOID[UOID_LEN], unsigned int &dist, unsigned short int &myport, char* myhostname, unsigned char *joinResp)
{
	

	memset(d_UOID, '\0', UOID_LEN);
	memcpy(&d_UOID[0],&joinResp[27], UOID_LEN); 

	memcpy(&joinUOID[0],&joinResp[27], UOID_LEN);
	memcpy(&dist, &joinResp[47], DISTANCE_LEN);
	memcpy(&myport, &joinResp[51], HOST_PORT_LEN);
	memcpy(myhostname, &joinResp[53], (dataLen- UOID_LEN - DISTANCE_LEN - HOST_PORT_LEN));

	//printf("************************* \n recvJoinResp my data is %d ********************\n", strlen((char *)data));
	//dataLen = getDataLenJoinResp(myhostname);
	
	if(data==NULL)// || strlen((char *)data)==0)
	{
		//printf("!!!!!!!!!!******** RECVJOINRESP FUNCTION DATA IS EMPTY, size of data %d \n", dataLen);
		data = (unsigned char*) malloc(dataLen);		
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &joinResp[27], dataLen);
	}
	else
	{
		//printf("!!!!!!!!!!******** RECVJOINRESP FUNCTION DATA IS not EMPTY, size of data %d, strlen(data) %d \n", dataLen, strlen((char*)data));

		//free(data);
	

		data = (unsigned char*) malloc(sizeof(unsigned char) *dataLen);			
		//printf("!!!!!!!!!!******** RECVJOINRESP FUNCTION DATA - after  malloc  datalen %d , size of data %d \n", dataLen, strlen((char*)data));

		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &joinResp[27], dataLen);
		
		memcpy(&data[0],&joinResp[27], UOID_LEN);
		memcpy(&data[20], &joinResp[47], DISTANCE_LEN);
		memcpy(&data[24], &joinResp[51], HOST_PORT_LEN);
		memcpy(&data[26], &joinResp[53], (dataLen- UOID_LEN - DISTANCE_LEN - HOST_PORT_LEN));

	
	}
	
	//printf("!!!!!!!!!!******** RECVJOINRESP FUNCTION DATA - outside malloc  datalen %d , size of data %d \n", dataLen, strlen((char*)data));
	
	dist = ntohs(dist);
	myport = ntohs(myport);


	//printf("recv join resp data size %d : d_uoid %02x%02x%02x%02x, dist %d, port %d, hostname %s \n", strlen((char*)data), d_UOID[16], d_UOID[17], d_UOID[18], d_UOID[19], dist, myport, myhostname);
	//data[dataLen] = '\0';
}

		/**************************** HELLO MSG - SPECIFIC METHODS ************************************/

void Message::formHelloMsg(unsigned short int myport, char* myhostname, unsigned char *helloBuf)
{	
	unsigned short int port = htons(myport);
		
	memcpy(&helloBuf[27], &port, HOST_PORT_LEN);	
	memcpy(&helloBuf[29], myhostname, strlen(myhostname));
	
	dataLen = getDataLenHello(myhostname);

	// update the data field of the message from the helloBuf
	//if(data==NULL || strlen((char *)data)==0)
	{
		data = (unsigned char*) malloc(dataLen);	
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &helloBuf[27], dataLen);
		//data[dataLen] = '\0';
	}
	

	//printf("form hello msg data : port %d, hostname %s \n", port, myhostname);
	//data[dataLen] = '\0';
	

}

void Message::recvHelloMsg(unsigned short int &myport, char* myhostname, unsigned char *helloBuf)
{

	memcpy(&myport, &helloBuf[27], HOST_PORT_LEN);	
	memcpy(myhostname, &helloBuf[29], (dataLen-HOST_PORT_LEN));
	
	//printf("************************* \n recvHelloMsg my data is %d ********************\n", strlen((char *)data));
	//if(data==NULL)// || strlen((char *)data)==0)
	{	
		//printf("************************* \n recvHelloMsg my data is NULL ********************\n");
		data = (unsigned char*) malloc(sizeof(unsigned char) *dataLen);			
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &helloBuf[27], dataLen);	
		//data[dataLen] = '\0';
	}

	//printf("************************* \n recvHelloMsg my data is %d ********************\n", strlen((char *)data));

	myport = ntohs(myport);

	//printf("recv hello msg data :  port %d, hostname %s \n", myport, myhostname);
	

	//data[dataLen] = '\0';

}


		/**************************** HELLO MSG - SPECIFIC METHODS ************************************/


		/**************************** KEEPALIVE MSG - SPECIFIC METHODS ************************************/

		/**************************** NOTIFY MSG - SPECIFIC METHODS ************************************/


		/**************************** CHECK MSG - SPECIFIC METHODS ************************************/


		/**************************** STATUS MSG - SPECIFIC METHODS ************************************/
void Message::formStatusReq(unsigned char status, unsigned char *buf)
{
	statustype = status;
	//buf[27] = status;
	memcpy(&buf[27], &status, STATUS_TYPE_LEN);	
	
	//dataLen = getDataLenStatusReq();

	// update the data field of the message from the helloBuf
	//if(data==NULL || strlen((char *)data)==0)
	{
		//printf("GOING INSIDE DATA = NULL FOR MSG CREATION \n");
		data = (unsigned char*) malloc(dataLen+1);	
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &buf[27], dataLen);
		//data[0] = status;
		data[dataLen] = '\0';
	}
	

	//printf("!!!!!!!!!!!!!!!!!!form status req msg data : status type %02x  datalen %d datalen %d !!!!!!!!!!!!!!!\n", &buf[27], statustype, dataLen);
	//data[dataLen] = '\0';
	

}

		
void Message::recvStatusReq(unsigned char &status, unsigned char *buf)
{
	memcpy(&statustype, &buf[27], STATUS_TYPE_LEN);	
	
	//dataLen = getDataLenStatusReq();

	// update the data field of the message from the helloBuf
	//if(data==NULL || strlen((char *)data)==0)
	{
		//printf("GOING INSIDE DATA = NULL FOR MSG RECEIVING \n");
		data = (unsigned char*) malloc(dataLen+1);	
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &buf[27], dataLen);
		//data[0] = status;
		data[dataLen] = '\0';
	}
	
	status = statustype;	

	//printf("!!!!!!!!!!!!!!!!!!recv status req msg data : status type %02x  datalen %d !!!!!!!!!!!!!!!\n", &buf[27], statustype);
	//data[dataLen] = '\0';
	

}


void Message::recvSearchReq(unsigned char &search, unsigned char *buf)
{
	memcpy(&searchtype, &buf[27], SEARCH_TYPE_LEN);	
	printf("dataLen % d \n", dataLen);
	//dataLen = getDataLenStatusReq();
	//printf("strlen((char *)data) %d \n" , strlen((char *)data));

	/*if(data!=NULL)
	{
		printf("GOING INSIDE DATA = NOT NULL FOR MSG RECEIVING \n");
		free(data);
		printf("after freeing strlen((char *)data) %d \n" , strlen((char *)data));
	}*/

	// update the data field of the message from the helloBuf
	//if(data==NULL || strlen((char *)data)==0)
	//{
		//printf("GOING INSIDE DATA = NULL FOR MSG RECEIVING \n");
		data = (unsigned char*) malloc(dataLen+1);	
		memset(&data[0], '\0', dataLen);
		memcpy(&data[0], &buf[27], dataLen);
		//data[0] = status;
		data[dataLen] = '\0';
	//}
	
	search = searchtype;	

	//printf("!!!!!!!!!!!!!!!!!!recv search req msg data : status type %02x  search query %s !!!!!!!!!!!!!!!\n", searchtype, &buf[28]);
	//printf("!!!!!!!!!!!!!!!!!!recv search req msg data : status type %02x  search query %s !!!!!!!!!!!!!!!\n", data[0], &data[1]);
	//data[dataLen] = '\0';
	

}

/******************************************* MSG - SPECIFIC FUNCTIONS ***********************************************/

/* Part 2 Project */



		/**************************** MSG-SPECIFIC DATA LENGTH COMPUTING METHODS ************************************/
		/* Part 1 Project */

int Message::getDataLenJoinReq(char* myhostname)
{
	return  HOST_LOCATION_LEN + HOST_PORT_LEN + strlen(myhostname);
}

int Message::getDataLenJoinResp(char* myhostname)
{

	return UOID_LEN + DISTANCE_LEN + HOST_PORT_LEN + strlen(myhostname);
}

int Message::getDataLenHello(char* myhostname)
{
	dataLen = HOST_PORT_LEN + strlen(myhostname);
	return dataLen;
}

int Message::getDataLenKeepAlive() // empty body, datalength = 0
{
	return 0;
}

int Message::getDataLenNotify()	// return error code
{
	return ERROR_CODE_LEN;
}

int Message::getDataLenCheckReq() // empty body, dataLength = 0
{
	return 0;
}

int Message::getDataLenCheckResp()
{

	return UOID_LEN;
}

int Message::getDataLenStatusReq()
{
	return STATUS_TYPE_LEN;
}

int Message::getDataLenStatusResp(char* myhostname)
{

	return UOID_LEN + HOST_INFO_LEN + HOST_PORT_LEN + strlen(myhostname); // + record length
}



		/* Part 2 Project */





