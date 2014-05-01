#include "iniparserClass.h"



extern std::list<LRU_Node> LRUList;
extern pthread_mutex_t LRUList_mutex; 

extern iniparserClass *iniparserInfo;

extern unsigned int cacheSize; 
extern unsigned int currentCacheSize;
extern unsigned int remainingCacheSize;

extern void deleteAllIndexNodes(unsigned int fileRef);


	/**************************** DEFINE FUNCTION MEMBERS ************************************/


LRU_Node::LRU_Node() // default constructor
{
	fileRef = 0;	// filenumber obtained from globalFileNum
	fileRef_Size = 0;		// filesize of fileRef to be added/deleted

}

LRU_Node::LRU_Node(unsigned int newfileRef, unsigned int newfileRef_Size)
{  
	fileRef = newfileRef;					// filenumber obtained from globalFileNum
	fileRef_Size = newfileRef_Size;		// filesize of fileRef to be added/deleted

}



LRU_Node::~LRU_Node() // default dstructor
{


}



// assume its always called from lru_store_decision, so no need to acquire the lock
// assume its the node thats at end of the list getting deleted, update the currentCacheSize
void deleteNodeInLRUList(unsigned int newfileRef, unsigned int newfileRef_Size)
{
	if(PRINT_FLAG)
	{
		cout<<"in delete function"<<endl;
		cout<<"Cache size before deletion is :"<<currentCacheSize<<endl;
	}

	//always delete from the end of list
	LRUList.pop_back();	
	currentCacheSize -= newfileRef_Size;
	remainingCacheSize = cacheSize - currentCacheSize;
	
	if(PRINT_FLAG)
		cout<< "Cache size after deletion is : " << currentCacheSize << endl;

	// update the index lists
	deleteAllIndexNodes(newfileRef);


}

// this function is used to remove an entry from the LRU List, not necessarily from the end of the list
// acquires the lock for performing the operations as its called independently
void deleteNodeInLRUList(unsigned int newfileRef)
{
	pthread_mutex_lock(&LRUList_mutex); 

	list<LRU_Node>::iterator it;

	for ( it=LRUList.begin() ; it != LRUList.end(); it++ )
	{
		LRU_Node Node = *it;

		if(Node.fileRef == newfileRef)
		{
			//if(PRINT_FLAG)
				printf("!!!!!! LRU Node found to delete: FileRef %d, FileSize %d  \n !!!!!!!!", Node.fileRef, Node.fileRef_Size);

			//always delete from the end of list
			LRUList.erase(it);
			currentCacheSize -= Node.fileRef_Size;
			remainingCacheSize = cacheSize - currentCacheSize;

			// dont update the index lists as this file must appear in all the search results
			// we are simply it from cache - perm area, but the file still exists in the mini directory system
			// moving simply means to remove it from the lrulist and update the cache data counters.
			//deleteAllIndexNodes(newfileRef);

			break;
		}
	}

	
	if(PRINT_FLAG)
		cout<< "Cache size after deletion is : " << currentCacheSize << endl;

	pthread_mutex_unlock(&LRUList_mutex); 


}


int lru_store_decision(unsigned int fileRef_Size)
{
	int returnvalue = TRUE;

	pthread_mutex_lock(&LRUList_mutex); 

    //need lru chahe prob to decode whether to store it or not
    if(fileRef_Size>cacheSize)
        returnvalue = FALSE;

	if(PRINT_FLAG)
	    cout<<"NewFile to be Added: " << fileRef_Size<< "Available cache space : " << currentCacheSize << endl;
	
	if(returnvalue==TRUE)
	{
		while(fileRef_Size+currentCacheSize>cacheSize)
		{
			if(PRINT_FLAG) cout<<"inside while loop "<<endl;

			LRU_Node node = LRUList.back();
			//should delete an item from the end of list
			deleteNodeInLRUList(node.fileRef, node.fileRef_Size);
		}

		if(PRINT_FLAG)
			printf("lru cache memory can accomodate the new file to be inserted \n");		
	}

	pthread_mutex_unlock(&LRUList_mutex); 	
	
	return returnvalue; //good to start storing the file in cache

}


int addNodeInLRUList(unsigned int newfileRef, unsigned int newfileRef_Size)
{
	if(lru_store_decision(newfileRef_Size)==TRUE)
	{
		pthread_mutex_lock(&LRUList_mutex); 

		LRU_Node node = LRU_Node(newfileRef, newfileRef_Size);
		
		// Add it to the LRUList
		LRUList.push_back (node);
		
		currentCacheSize += newfileRef_Size;
		remainingCacheSize = cacheSize - currentCacheSize;

		if(PRINT_FLAG)
		{
			printf("!!!!!! LRU Node: FileRef %d, FileSize %d  added  !!!!!!!! \n", newfileRef, newfileRef_Size);
			printf("!!!!!! currentCacheSize %d, remainingCacheSize %d  after addition !!!!!!!! \n\n ", currentCacheSize, remainingCacheSize);
		}

		pthread_mutex_unlock(&LRUList_mutex); 	

		return TRUE;
	}
	else
		return FALSE;

	// the file does not get added
}



int findFileInLRUList(unsigned int newfileRef)
{

	pthread_mutex_lock(&LRUList_mutex); 

	int returnvalue = FALSE;

	list<LRU_Node>::iterator it;


	for ( it=LRUList.begin() ; it != LRUList.end(); it++ )
	{
		LRU_Node Node = *it;

		if(Node.fileRef == newfileRef)
		{
			returnvalue = TRUE;

			if(PRINT_FLAG)
				printf("!!!!!! LRU Node found: FileRef %d, FileSize %d  \n !!!!!!!!", Node.fileRef, Node.fileRef_Size);
			
		}
	}

	pthread_mutex_unlock(&LRUList_mutex); 

	return returnvalue;

} // findFileInLRUList

int updateLRU_SearchResp(unsigned int newfileRef)
{
	pthread_mutex_lock(&LRUList_mutex); 

	int returnvalue = FALSE;

	list<LRU_Node>::iterator it;

	for ( it=LRUList.begin() ; it != LRUList.end(); it++ )
	{
		LRU_Node Node = *it;

		if(Node.fileRef == newfileRef)
		{
			returnvalue = TRUE;
			
			LRUList.erase(it);
			LRUList.push_front(Node);

			if(PRINT_FLAG)
				printf("!!!!!! LRU Node Updated to front: FileRef %d, FileSize %d  \n !!!!!!!!", Node.fileRef, Node.fileRef_Size);
			
			break;
		
		}
	}

	pthread_mutex_unlock(&LRUList_mutex); 

	return returnvalue;

} // end updateLRU_SearchResp

// displays the contents of the connList 
void displayLRUList()
{
	pthread_mutex_lock(&LRUList_mutex); 

	list<LRU_Node>::iterator it;

	if(PRINT_FLAG)
	{
		cout<<"\n\n displayLRUList method" << endl;
		cout<<"List size = "<< LRUList.size() << endl;
	}

	for ( it=LRUList.begin() ; it != LRUList.end(); it++ )
	{
		LRU_Node Node = *it;

		if(PRINT_FLAG)
			printf("!!!!!! LRU Node: FileRef %d, FileSize %d  \n !!!!!!!!", Node.fileRef, Node.fileRef_Size);
	}

	pthread_mutex_unlock(&LRUList_mutex); 

} // end displayLRUList


