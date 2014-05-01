#include "iniparserClass.h"

/************************************************** DEFINE NEIGHBOURNODE CLASS **********************************************************/
extern std::vector<RoutingNode> msgRoutingTable;
extern pthread_mutex_t msgRT_mutex; 


extern void addNodeInRoutingTable(unsigned char addUOID[UOID_LEN], int addSockNode);
extern void deleteNodeInRoutingTable(unsigned char deleteUOID[UOID_LEN]);
extern int findKeyInRoutingTable(unsigned char findUOID[UOID_LEN]);
extern int getValueFromRoutingTable (unsigned char findUOID[UOID_LEN]);
extern void displayRoutingTable();

extern void convert_hex_to_sha1(unsigned char sha1[], unsigned char sha1buf[]);

/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* DEFAULT CONSTRUCTOR */
RoutingNode::RoutingNode()
{
	// empty body 
	memset(UOID, '\0',UOID_LEN);
	sockfd = 0;
	memset(sha1_file, '\0',UOID_LEN);

}

/* CONSTRUCTOR */
// TODO remember to deleted this msg in the calling function delete(msg)
RoutingNode::RoutingNode(unsigned char addUOID[UOID_LEN], int addsockfd)
{

	memset(UOID, '\0', UOID_LEN);
	memcpy(UOID, addUOID, UOID_LEN);	
	sockfd = addsockfd;
	//printf("**** NEIGHBOUR NODE GENERATED: sockfd_new %d \n", sockfd);

}

/* CONSTRUCTOR */
// constructor for creating search resp nodes - remember sha1 hash for the get req
// sha1_filestr - 2*UOID_LEN.. convert the hex to sha1 format and store it in the search node
RoutingNode::RoutingNode(unsigned char addUOID[UOID_LEN], int addsockfd, unsigned char sha1_filestr[])
{

	memset(UOID, '\0', UOID_LEN);
	memcpy(UOID, addUOID, UOID_LEN);	
	sockfd = addsockfd;

	// convert sha1_filestr in hexstring to string format
	memset(sha1_file, '\0', UOID_LEN); //printf("printing the sha1 hash for the search node : ");
	convert_hex_to_sha1(sha1_file, sha1_filestr);

	if(PRINT_FLAG)
		printf("**** SEARCH NODE GENERATED: SEARCH OPTION %d \n", sockfd);

}

/* DESTRUCTOR */
RoutingNode::~RoutingNode()
{
	
	
}





// add <key-value> pair in the routing table <UOID, sockfd>
void addNodeInRoutingTable(unsigned char addUOID[UOID_LEN], int sockfd_new)//(RoutingNode node)
{
	pthread_mutex_lock(&msgRT_mutex);
	
	RoutingNode node = RoutingNode(addUOID, sockfd_new);
	// Add it to the RT 
	msgRoutingTable.push_back(node);	

	if(PRINT_FLAG)
		printf("**** ROUTING NODE ADDED %02x%02x%02x%02x: sockfd_new %d msgRoutingTable.size() %d \n", addUOID[16], addUOID[17], addUOID[18], addUOID[19], node.sockfd, msgRoutingTable.size());

	pthread_mutex_unlock(&msgRT_mutex);
}

// delete <key-value> pair in the routing table <UOID, sockfd> given the UOID
void deleteNodeInRoutingTable(unsigned char deleteUOID[UOID_LEN])
{
	pthread_mutex_lock(&msgRT_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=msgRoutingTable.begin() ; it != msgRoutingTable.end(); it++ )
	{
		if(memcmp((*it).UOID, deleteUOID, UOID_LEN)==0)
		{
			msgRoutingTable.erase(it);
		}
	} // end for 
	//printf("!!!!!!! msgRoutingTable.size() %d AFTER DELETETION !!!!!!!!\n", msgRoutingTable.size());


	pthread_mutex_unlock(&msgRT_mutex);
}
	


// returns true upon finding UOID in the routing table or else returns false
/* if the msg with same UOID is seen by the node, its gonna drop it. use tis function to check 
whether same msg is received from other node */
int findKeyInRoutingTable(unsigned char findUOID[UOID_LEN])
{

	int returnvalue = FALSE;
	int sock = 0;

	pthread_mutex_lock(&msgRT_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=msgRoutingTable.begin() ; it != msgRoutingTable.end(); it++ )
	{
		if(memcmp((*it).UOID, findUOID, UOID_LEN)==0)
		{
			returnvalue = TRUE; //(*it).sockfd;
			sock = (*it).sockfd;
			break;
		}
	} // end for 
	
	if(PRINT_FLAG)
		printf("!!!!!!! msgRoutingTable.size() %d AFTER FINDING %02x%02x%02x%02x return value %d sockfd %d !!!!!!!!\n", msgRoutingTable.size(), findUOID[16], findUOID[17], findUOID[18], findUOID[19], returnvalue, sock);

	pthread_mutex_unlock(&msgRT_mutex);

	return returnvalue;

	// return after releasing the lock
}

// returns the corresponding value for the findUOID key in the Routing Table
/* used by calling function to get the corresponding sockfd for the findUOID
*/
int getValueFromRoutingTable (unsigned char findUOID[UOID_LEN])
{
	int returnvalue = FALSE;

	pthread_mutex_lock(&msgRT_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=msgRoutingTable.begin() ; it != msgRoutingTable.end(); it++ )
	{
		if(memcmp((*it).UOID, findUOID, UOID_LEN)==0)
		{
			RoutingNode node = *it;
			returnvalue = (*it).sockfd;
		//printf("!!!!!!! msgRoutingTable.size() %d DURING ADDITION uoid %02x%02x%02x%02x, sockfd %d *********\n", msgRoutingTable.size(),
			//node.UOID[16], node.UOID[17], node.UOID[18], node.UOID[19], node.sockfd);

		}
	} // end for 

	pthread_mutex_unlock(&msgRT_mutex);

	return returnvalue;

}

void displayRoutingTable()
{
	pthread_mutex_lock(&msgRT_mutex);
	
	//printf("!!!!*********!! MY ROUTING TABLE ARE SIZE: %d!!!!!!!****!!!!!!!!!!!!!!!! \n\n", msgRoutingTable.size());
	
	vector<RoutingNode>::iterator it;

	for ( it=msgRoutingTable.begin() ; it != msgRoutingTable.end(); it++ )
	{
		RoutingNode node = *it;

		//printf("!!!*********!!! uoid %02x%02x%02x%02x, sockfd %d *********\n", node.UOID[16], node.UOID[17], node.UOID[18], node.UOID[19], node.sockfd);
	}


	pthread_mutex_unlock(&msgRT_mutex);


}
