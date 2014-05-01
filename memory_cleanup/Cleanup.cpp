#include "global.h"
#include "iniparserClass.h"

/*
TODOs:
******
- take care the cleanup functions - related to cleaning up shared list/vectors

*/

/*********************************  SHARED DATA, MUTEX & CV MEMBERS ****************************************/



extern pthread_mutex_t shortCircuit_mutex;

extern pthread_mutex_t searchidMap_mutex; 


extern pthread_mutex_t cmdLineQueue_mutex;
extern pthread_cond_t cmdLineQueue_cv;

extern pthread_mutex_t log_mutex;
extern pthread_mutex_t shutdown_mutex;
extern pthread_mutex_t globalFileNum_mutex;
extern pthread_mutex_t temp_globalFileNum_mutex;
extern pthread_mutex_t kwrdIndex_mutex;
extern pthread_mutex_t nameIndex_mutex;
extern pthread_mutex_t shaIndex_mutex;
extern pthread_mutex_t LRUList_mutex;


extern std::queue<Event> eventQueue;      // queue for acessing common objects in event queue
extern pthread_mutex_t eventQueue_mutex; 
extern pthread_cond_t eventQueue_cv;                // cv to wait on


extern std::map<int, neighbourConn*> connList;
extern pthread_mutex_t connList_mutex; 
extern pthread_cond_t connList_cv;                // cv to wait on

extern pthread_mutex_t msgRT_mutex; 

extern std::vector<NeighbourNode> Neighbours;
extern pthread_mutex_t Neighbours_mutex; 
extern pthread_mutex_t gMinNeighboursCnt_mutex; 

void CleanUpConnList();
void CleanUpeventQueue();
void CleanUpNeighbours();

void CleanUpConnList()
{	
	//pthread_mutex_lock(&connList_mutex); 

	//printf("connList.size() %d \n", connList.size());
	map<int, neighbourConn*>::iterator it;
	for ( it=connList.begin() ; it != connList.end(); it++ )
	{
		delete((*it).second);
	}

	//pthread_mutex_unlock(&connList_mutex); 
}

void CleanUpeventQueue()
{	
	//printf("eventQueue.size() %d \n", eventQueue.size());

	while (!eventQueue.empty())
	{		
		eventQueue.pop();
	}

}


void CleanUpNeighbours()
{	
	//pthread_mutex_lock(&Neighbours_mutex);

	//printf("Neighbours.size() %d \n", Neighbours.size());
	
	vector<NeighbourNode> ::iterator it;
	for ( it=Neighbours.begin() ; it != Neighbours.end(); it++ )
	{
		Neighbours.erase(it);
	}
	
	//pthread_mutex_unlock(&Neighbours_mutex);
}


/*  Clean up and exit 
Destroy ALL THE data structures, Mutex and Condition Variable objects */
void CleanUp()
{
	
	//CleanUpConnList();
	//CleanUpeventQueue();
	//CleanUpNeighbours();

	if(PRINT_FLAG)
		printf("cleanup process begins \n");

	pthread_mutex_destroy(&gMinNeighboursCnt_mutex);
	pthread_mutex_destroy(&shortCircuit_mutex);

	pthread_mutex_destroy(&cmdLineQueue_mutex);
	pthread_cond_destroy(&cmdLineQueue_cv);
	
	pthread_mutex_destroy(&searchidMap_mutex);
	
	pthread_mutex_destroy(&msgRT_mutex);

	pthread_mutex_destroy(&eventQueue_mutex);
	pthread_cond_destroy (&eventQueue_cv);

	pthread_mutex_destroy(&connList_mutex);
	pthread_cond_destroy (&connList_cv);

	pthread_mutex_destroy(&Neighbours_mutex);
	
	pthread_mutex_destroy(&log_mutex);

	pthread_mutex_destroy(&shutdown_mutex);

	pthread_mutex_destroy(&globalFileNum_mutex);

	pthread_mutex_destroy(&temp_globalFileNum_mutex);

	pthread_mutex_destroy(&kwrdIndex_mutex);

	pthread_mutex_destroy(&nameIndex_mutex);

	pthread_mutex_destroy(&shaIndex_mutex);

	pthread_mutex_destroy(&LRUList_mutex);	
	
	if(PRINT_FLAG)
		printf("cleanup process is compeleted \n");

} // end CleanUp function
