#include "iniparserClass.h"

/************************************************** DEFINE NEIGHBOURNODE CLASS **********************************************************/


extern vector<NeighbourNode> Neighbours;
extern pthread_mutex_t Neighbours_mutex; 


/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* DEFAULT CONSTRUCTOR */
NeighbourNode::NeighbourNode()
{
	// empty body 
	memset(myhostname, '\0',50);



}

/* CONSTRUCTOR */
// TODO remember to deleted this msg in the calling function delete(msg)
NeighbourNode::NeighbourNode(char hostname[50], unsigned short int port, int sockfd_new)
{

	memset(myhostname, '\0',strlen(hostname));
	memcpy(myhostname, hostname, strlen(hostname));	
	myport = port;
	sockfd = sockfd_new;
	//printf("**** NEIGHBOUR NODE GENERATED: hostname %s, port %d, sockfd_new %d neighbours.size()\n", myhostname, myport, sockfd);

}

/* DESTRUCTOR */
NeighbourNode::~NeighbourNode()
{
	/*
	pthread_mutex_lock(&Neighbours_mutex);

	delete(this);

	pthread_mutex_unlock(&Neighbours_mutex);
	*/
}
		/**************************** COMMON METHODS ************************************/


void addNeighbourNode(NeighbourNode node)
{
	pthread_mutex_lock(&Neighbours_mutex);

	// Add it to the Neighbours map
	Neighbours.push_back(node);	
	//printf("**** NEIGHBOUR NODE ADDED: hostname %s, port %d, sockfd_new %d neighbours.size() %d \n", node.myhostname, node.myport, node.sockfd, Neighbours.size());

	pthread_mutex_unlock(&Neighbours_mutex);

} // end addNeighbourNode

// find the corresponding NeighbourNode belonging to given hostname and port
void deleteNeighbourNode(char hostname[50], unsigned short int port)
{
	pthread_mutex_lock(&Neighbours_mutex);
	
	vector<NeighbourNode> ::iterator it;
	
	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{
		if(strncmp(((*it).myhostname) ,hostname, strlen(hostname))
			&& ((*it).myport == port))
		{
			Neighbours.erase(it);
		}
	} // end for 
	//printf("!!!!!!! Neighbours.size() %d AFTER DELETETION !!!!!!!!\n", Neighbours.size());
	pthread_mutex_unlock(&Neighbours_mutex);

} // end deleteNeighbourNode

void displayNeighbours()
{

	pthread_mutex_lock(&Neighbours_mutex);
	
	//printf("!!!!*********!! MY NEIGHBOURS ARE SIZE: %d!!!!!!!****!!!!!!!!!!!!!!!! \n", Neighbours.size());
	
	vector<NeighbourNode>::iterator it;
	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{
		//printf("!!!*********!!! hostname %s, port %d, sockfd_new %d *********\n", (*it).myhostname, (*it).myport, (*it).sockfd);
	}

	pthread_mutex_unlock(&Neighbours_mutex);
} //end displayNeighbours

int getNeighbourSize()
{
	  return Neighbours.size();
} // end getNeighbourSize
