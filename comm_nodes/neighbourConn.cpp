#include "iniparserClass.h"

extern void *read_thread(void *sockfd);

extern void *write_thread(void *sockfd);


extern std::map<int, neighbourConn*> connList;
extern pthread_mutex_t connList_mutex; 
extern pthread_cond_t connList_cv;                // cv to wait on

// static variables definitions - outside class declaration
unsigned int neighbourConn::currConnections = 0;

	/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* this constructor does not generate new neighbourConn object as its assumed that corresponding overloading constructor will 
 be called by either clientThread or serverThread. Therefore, default constructor just takes care of initialization part */
neighbourConn::neighbourConn() // default constructor
{
	pthread_mutex_lock(&connList_mutex); 

	/* Initialize mutex and condition variable objects */
	pthread_mutex_init(&writeQueue_mutex, NULL);
	pthread_cond_init (&writeQueue_cv, NULL);
	sockfd = 0;
	helloRcvd = FALSE;
	memset(myhostname, '\0', 50);
	mylocation = 0;
	// UnLock mutex after initializing the object
	pthread_mutex_unlock(&connList_mutex);

}

/* This constructor is invoked by the clientThread who has knowledge about destination's hostname and 
destination's well-known portno
*/
neighbourConn::neighbourConn(char hostname[50], unsigned short int port, int sockfd_new)
{
	
	// Lock mutex before initializing mutex/cv objects and forking reading/writing threads
	pthread_mutex_lock(&connList_mutex); 

	//printf("INSIDE NEIGHBOUR CONN CREATION CONSTURCTOR \n");
	memset(myhostname, '\0',strlen(hostname));
	memcpy(myhostname, hostname, strlen(hostname));
	myhostname[strlen(hostname)] = '\0';
	memcpy(&myport, &port, HOST_PORT_LEN);
	sockfd = sockfd_new;
	mylocation = 0;
	helloRcvd = FALSE;	
	createdBy = 'C';

	/* Initialize mutex and condition variable objects */
	pthread_mutex_init(&writeQueue_mutex, NULL);
	pthread_cond_init (&writeQueue_cv, NULL);

	// Create read thread
	pthread_create(&readThread, NULL, read_thread, (void *)sockfd_new);	

	// Create write thread
	pthread_create(&writeThread, NULL, write_thread, (void *)sockfd_new);

	// Add it to the connList map
	connList[sockfd_new] = this;	//printf("***************neighbour Conn %d by Client is added.. sockfd_new %d port %d *********\n", sockfd_new, connList[sockfd_new]->sockfd,  connList[sockfd_new]->myport);

	// increment the total neighbours of the node
	currConnections = connList.size();

	// UnLock mutex after initializing the object
	pthread_mutex_unlock(&connList_mutex);

}

/* This constructor is invoked by the serverThread who has no knowledge about destination's hostname and 
destination's well-known portno. It needs to wait to receive Hello message to acquire this information.
when readThread receives hello message, it invokes UpdateConnInfoForSockfd member function, if corresponding info
about the neighbour node (identified by sockfd) is not found on the database, that function will update its corresponding neighbourConn object 
with the new information - hostname & portno obtained by the hello.
*/
neighbourConn::neighbourConn(int sockfd_new)
{
	
	// Lock mutex before initializing mutex/cv objects and forking reading/writing threads
	pthread_mutex_lock(&connList_mutex); 

	/* // hostname, port of the dest not unknown - not yet added, need to wait for the Hello message
	memcpy(myhostname, hostname, strlen(hostname));
	*/
	//printf("INSIDE NEIGHBOUR CONN CREATION CONSTURCTOR \n");

	myport = 0;
	sockfd = sockfd_new;
	helloRcvd = FALSE;
	mylocation = 0;
	createdBy = 'S';
	memset(myhostname, '\0', 50);

	/* Initialize mutex and condition variable objects */
	pthread_mutex_init(&writeQueue_mutex, NULL);
	pthread_cond_init (&writeQueue_cv, NULL);

	// Create read thread
	pthread_create(&readThread, NULL, read_thread, (void *)sockfd_new);	

	// Create write thread
	pthread_create(&writeThread, NULL, write_thread, (void *)sockfd_new);

	// Add it to the connList map
	connList[sockfd_new] = this;	//printf("****************neighbour Conn %d by Server is added.. sockfd_new %d \n", sockfd_new, connList[sockfd_new]->sockfd);

	// increment the total neighbours of the node
	currConnections = connList.size();

	// UnLock mutex before spawning reading/writing thread.
	pthread_mutex_unlock(&connList_mutex);

}

neighbourConn::~neighbourConn() // default destructor
{
	// Lock mutex before operating on neighbourConn objects
	pthread_mutex_lock(&connList_mutex); 

	// decrement the total neighbours of the node
	//printf("neighbourConn %d deleted.. currentneighbour %d \n", sockfd, currConnections);
	currConnections = connList.size();
	memset(myhostname, '\0', 50);

	/* Clean up and exit */
	// destroy the mutex and cv
	pthread_mutex_destroy(&writeQueue_mutex);
	pthread_cond_destroy(&writeQueue_cv);
	
	// erase my entry in the connList before destroying me
	if(connList[sockfd] != NULL)
		connList.erase(sockfd);   // erasing by key  

	// Unlock mutex after operating on neighbourConn objects
	pthread_mutex_unlock(&connList_mutex);

	delete(this);
} 

		/**************************** COMMON METHODS ************************************/
		
	
/*returns true upon detecting duplicate connections. checks the existing connections in the connList with the given information, 
if a matching information is found in another connection, then duplicate connection is detected */
int neighbourConn::checkForTieBreak(char hostname[50], unsigned short int port)
{

	// Lock mutex before operating on neighbourConn objects
	pthread_mutex_lock(&connList_mutex); 

	map<int, neighbourConn*>::iterator it;

	
	// connection created/accepted by server thread. need to update the hostname and wellknown port no info obtained from hello
	/*if(myhostname == NULL)
	{		
		memcpy(myhostname, hostname, strlen(hostname));
		memcpy(&myport, &port, HOST_PORT_LEN);	
	}
	*/

	// examines the contents of connList for duplicate connection
	for ( it=connList.begin() ; it != connList.end(); it++ )
	{
		int sock = (*it).first;
		neighbourConn *conn = (*it).second;

		if(sockfd!=sock)
		{
			// duplicate connection found
			if( (strncmp((char*)myhostname,(char*)conn->myhostname,strlen(conn->myhostname))==0) &&
				(myport == conn->myport) )
			{
				//printf("DUPLICATE CONNECTION found !!!!!!!!!!!!!!! \n");
				conn = NULL;
				pthread_mutex_unlock(&connList_mutex); 
				return TRUE;
			}				
		} //end if for reading different conn object
	}
	
	// Unlock mutex after operating on neighbourConn objects
	pthread_mutex_unlock(&connList_mutex);
	
	// yea, am the only unique connection existing, so return false !!
	return FALSE;	

} //end checkForTieBreak

/*returns true upon existing connection. checks the existing connections in the connList with the given information, 
if a matching information is found in another connection, then upcoming connection will not be instantiated */
int checkConnStatus(char hostname[50], unsigned short int port)
{

	// Lock mutex before operating on neighbourConn objects
	pthread_mutex_lock(&connList_mutex); 

	map<int, neighbourConn*>::iterator it;
	//printf("***********inside checkConnStatus *****check**** hostname %s, port %d\n", hostname, port);

	// examines the contents of connList for any existing connection
	for ( it=connList.begin(); it != connList.end(); it++ )
	{
		//int sock = (*it).first;
		neighbourConn *conn = (*it).second;

		// duplicate connection found
		if( (strncmp((char*)hostname,(char*)conn->myhostname,strlen(conn->myhostname))==0) &&
			(port == conn->myport) )
		{
			//printf("CONNECTION ALREADY EXISTS !!!!!!!!!!!!!!! \n");
			conn = NULL;
			pthread_mutex_unlock(&connList_mutex); 
			return TRUE;
		}				
	}
	
	// Unlock mutex after operating on neighbourConn objects
	pthread_mutex_unlock(&connList_mutex);
	
	// yea, am the only unique connection existing, so return false !!
	return FALSE;	

} //end checkForTieBreak

// displays the contents of the connList 
void displayConnList()
{
	pthread_mutex_lock(&connList_mutex); 

	map<int, neighbourConn*>::iterator it;
	for ( it=connList.begin() ; it != connList.end(); it++ )
	{
		//printf("!!!!!! %d => %d, %s, %d.... \n !!!!!!!!", (*it).first, ((*it).second)->sockfd, ((*it).second)->myhostname, ((*it).second)->myport);
	}

	pthread_mutex_unlock(&connList_mutex); 

} // end displayConnList


void neighbourConn::getConnForSockfd(int sock, neighbourConn **conn)
{
	pthread_mutex_lock(&connList_mutex); 

	map<int, neighbourConn*>::iterator it;
	//printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!getConnForSockfd found for sock %d \n", sock);
	if ((it = connList.find(sock)) != connList.end())
	{
		//printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1getConnForSockfd found for sock %d \n", sock);
	}
	if(connList[sock] != NULL){
		*conn = connList[sock]; 
	}
	else
		*conn = NULL;
	
	pthread_mutex_unlock(&connList_mutex); 

}

/* Assume this function is called after receiving Hellomsg, so update the helloRcvd status to TRUE for both connections 
created by server/client threads. This function is used by readThread to update the conn-specific info after receiving 
hello message for connections created by serverThread */
void neighbourConn::UpdateConnInfoForSockfd(char hostname[50], unsigned short int port)
{
	pthread_mutex_lock(&connList_mutex); 

	if(helloRcvd == FALSE)
		helloRcvd = TRUE;

	if(createdBy == 'S')
	{
		myport = port;
		//myhostname[0] = '\0'; 
		//printf(" strlen(hostname) %d in update \n", strlen(hostname));
		memset(myhostname, '\0', strlen(hostname));
		memcpy(myhostname, hostname, strlen(hostname));	
		//myhostname[strlen(hostname)] = '\0';
		//printf("***********inside UpdateConnInfoForSockfd ********** hostname %s, strlen(hostname) % d, port %d, sockfd %d \n", myhostname, strlen(myhostname), myport, sockfd);
	}

	pthread_mutex_unlock(&connList_mutex); 


}

int neighbourConn::checkHelloRcvd()
{
	pthread_mutex_lock(&connList_mutex); 

	int i = helloRcvd;

	pthread_mutex_unlock(&connList_mutex); 

	return i;

}


