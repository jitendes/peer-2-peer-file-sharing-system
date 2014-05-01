


#include "iniparserClass.h"

using namespace std;

/************************************************** DEFINE KEYWORD INDEX ***************************************************************************/

// KEYWORD INDEX LIST STORE THE BIT-VECTOR KEY IN HEXSTRING FORMAT AS A KEY AND LIST OF FILE REFERENCES FOR THAT KEY

// source: //http://advancedcppwithexamples.blogspot.com/2009/04/example-of-c-multimap.html

extern std::multimap<string, unsigned int> kwrdIndex;
extern pthread_mutex_t kwrdIndex_mutex; 

extern std::multimap<string, unsigned int> nameIndex;
extern pthread_mutex_t nameIndex_mutex; 

extern std::multimap<string, unsigned int> shaIndex;
extern pthread_mutex_t shaIndex_mutex; 

extern std::vector<RoutingNode> fileidMap;
extern pthread_mutex_t fileidMap_mutex; 

extern std::vector<RoutingNode> searchidMap;
extern pthread_mutex_t searchidMap_mutex; 


extern int gen_onetime_pwd(unsigned char  password[]);

extern void Tokenize(const string& str,vector<string>& tokens,const string& delimiters = " ");

extern int bitvec_hit(unsigned char b1[], unsigned char b2[]);

// add <key-value> pair in the KWRDINDEX <bit-vector, fileRef>
void addKwrdIndexNode(string key, unsigned int fileRef )
{
	pthread_mutex_lock(&kwrdIndex_mutex);
	
	kwrdIndex.insert(pair<string, unsigned int>(key,fileRef));

	/*if(PRINT_FLAG) 
		printf("**** KEYWORD INDEX NODE ADDED: string %s fileRef %d kwrdIndex.size %d \n", key.c_str(), fileRef, kwrdIndex.size()); */

	pthread_mutex_unlock(&kwrdIndex_mutex);


}//end addKwrdIndexNode


// deleting the particular instance of fileRef associated with the file bit-vector key
// delete <key-value> pair in the KWRDINDEX <bit-vector, fileRef> given the particular instance
void deleteKwrdIndexNode(unsigned int fileRef )
{

	pthread_mutex_lock(&kwrdIndex_mutex);

	multimap<string, unsigned int>::iterator it = kwrdIndex.begin();
	
	while(it != kwrdIndex.end())
	{
		if (fileRef == it->second )
		{
			kwrdIndex.erase(it);
	
			if(PRINT_FLAG)
				cout<<"deleting Key = "<< it->first <<"    Value = "<< it->second <<endl;
			
			break;
		}

		it++;
	}
	pthread_mutex_unlock(&kwrdIndex_mutex);

	return;

} // end deleteKwrdIndexNode


// returns true or false whether key is found or not
int findKeyInKwrdIndex(string key)
{

	int returnvalue = FALSE;
	
	pthread_mutex_lock(&kwrdIndex_mutex);

	multimap<string, unsigned int>::iterator it;
	
	it = kwrdIndex.find(key);

	if(it!= kwrdIndex.end())
	{
		returnvalue = TRUE;

		if(PRINT_FLAG)
			printf("\n\nfound key %s TRUE \n", key.c_str());
	}

	pthread_mutex_unlock(&kwrdIndex_mutex);

	return returnvalue;
	
} // end findKeyInKwrdIndex


// returns the list of file references with the bit-vector key
void getValueFromKwrdIndex (string key, unsigned int fileRef[])
{

	int cnt = 0;

	pthread_mutex_lock(&kwrdIndex_mutex);

	//Print all Joe from the list and then erase them
	pair<multimap<string,unsigned int>::iterator, multimap<string,unsigned int>::iterator> ii;

	multimap<string, unsigned int>::iterator it; //Iterator to be used along with ii

	ii = kwrdIndex.equal_range(key); //We get the first and last entry in ii;

	if(PRINT_FLAG)
		cout<<"\n\nPrinting all info abt BIT VECTOR "<< key << endl;

	for(it = ii.first; it != ii.second; ++it)
	{
		fileRef[cnt] = it->second;

		if(PRINT_FLAG)
			cout<<"Value = "<< fileRef[cnt] <<endl;
		cnt++;
	}

 	pthread_mutex_unlock(&kwrdIndex_mutex);

	return;

}//end getValueFromKwrdIndex


// returns the number of file references stored for the particular bit vector pattern
// returns 0 - if not found.. o/w returns the count
int getCountForKwrdKey (string key)
{
	int count = 0;

	pthread_mutex_lock(&kwrdIndex_mutex);
	count = kwrdIndex.count(key);
	pthread_mutex_unlock(&kwrdIndex_mutex);

	return count;

} // end getCountForKwrdKey


// returns the size of the kwrdIndex List
int getKwrdIndexSize()
{

	int size = 0;

	pthread_mutex_lock(&kwrdIndex_mutex);
	size = kwrdIndex.size();
	pthread_mutex_unlock(&kwrdIndex_mutex);

	return size;


}// end getKwrdIndexSize

// displays the contents of the kwrdIndex map
void displayKwrdIndex() 
{
	pthread_mutex_lock(&kwrdIndex_mutex);

	if(PRINT_FLAG)
	{
		cout<<"\n\nMultimap displayKwrdIndex method"<<endl;
		cout<<"Map size = "<<kwrdIndex.size()<<endl;
	}

	multimap<string, unsigned int>::iterator it = kwrdIndex.begin();
	
	while(it != kwrdIndex.end())
	{
		if(PRINT_FLAG)
			cout<<"Key = "<<it->first<<"    Value = "<<it->second<<endl;
	
		it++;
	}
	pthread_mutex_unlock(&kwrdIndex_mutex);

} // end displayKwrdIndex


/************************************************** DEFINE NAME INDEX ***************************************************************************/

// NAME INDEX LIST STORE THE FILE NAME IN STRING FORMAT AS A KEY AND LIST OF FILE REFERENCES FOR THAT KEY 


void addNameIndexNode(string key, unsigned int fileRef)
{
	pthread_mutex_lock(&nameIndex_mutex);
	
	nameIndex.insert(pair<string, unsigned int>(key,fileRef));

	/*if(PRINT_FLAG) 
		printf("**** KEYWORD INDEX NODE ADDED: string %s fileRef %d kwrdIndex.size %d \n", key.c_str(), fileRef, kwrdIndex.size()); */

	pthread_mutex_unlock(&nameIndex_mutex);


}//end addNameIndexNode

// delete it by fileRef
void deleteNameIndexNode(unsigned int fileRef )
{

	pthread_mutex_lock(&nameIndex_mutex);

	multimap<string, unsigned int>::iterator it = nameIndex.begin();
	
	while(it != nameIndex.end())
	{
		if (fileRef == it->second )
		{
			nameIndex.erase(it);
	
			if(PRINT_FLAG)
				cout<<"deleting Key = "<< it->first <<"    Value = "<< it->second <<endl;
			
			break;
		}

		it++;
	}
	pthread_mutex_unlock(&nameIndex_mutex);

	return;

} // end deleteNameIndexNode


int findKeyInNameIndex(string key)
{

	int returnvalue = FALSE;
	
	pthread_mutex_lock(&nameIndex_mutex);

	multimap<string, unsigned int>::iterator it;
	
	it = nameIndex.find(key);

	if(it!= nameIndex.end())
	{
		returnvalue = TRUE;

		if(PRINT_FLAG)
			printf("\n\nfound key %s TRUE \n", key.c_str());
	}

	pthread_mutex_unlock(&nameIndex_mutex);

	return returnvalue;
	
} // end findKeyInNameIndex


void getValueFromNameIndex (string key, unsigned int fileRef[])
{

	int cnt = 0;

	pthread_mutex_lock(&nameIndex_mutex);

	//Print all Joe from the list and then erase them
	pair<multimap<string,unsigned int>::iterator, multimap<string,unsigned int>::iterator> ii;

	multimap<string, unsigned int>::iterator it; //Iterator to be used along with ii

	ii = nameIndex.equal_range(key); //We get the first and last entry in ii;

	if(PRINT_FLAG)
		cout<<"\n\nPrinting all info abt BIT VECTOR "<< key << endl;

	for(it = ii.first; it != ii.second; ++it)
	{
		fileRef[cnt] = it->second;

		if(PRINT_FLAG)
			cout<<"getValue = "<< fileRef[cnt] <<endl;
		cnt++;
	}

 	pthread_mutex_unlock(&nameIndex_mutex);

	return;

}//end getValueFromNameIndex

int getCountForNameKey (string key)
{
	int count = 0;

	pthread_mutex_lock(&nameIndex_mutex);
	count = nameIndex.count(key);
	pthread_mutex_unlock(&nameIndex_mutex);

	return count;

} // end getCountForNameKey


int getNameIndexSize()
{

	int size = 0;

	pthread_mutex_lock(&nameIndex_mutex);
	size = nameIndex.size();
	pthread_mutex_unlock(&nameIndex_mutex);

	return size;


}// end getNameIndexSize


void displayNameIndex()
{
	pthread_mutex_lock(&nameIndex_mutex);

	if(PRINT_FLAG)
	{
		cout<<"\n\nMultimap displaynameIndex method"<<endl;
		cout<<"Map size = "<<nameIndex.size()<<endl;
	}

	multimap<string, unsigned int>::iterator it = nameIndex.begin();
	
	while(it != nameIndex.end())
	{
		if(PRINT_FLAG)
			cout<<"Key = "<<it->first<<"    Value = "<<it->second<<endl;
		it++;
	}
	pthread_mutex_unlock(&nameIndex_mutex);

} // end displayNameIndex



/************************************************** DEFINE SHA1 INDEX ***************************************************************************/

// SHA1 INDEX LIST STORE THE SHA1 (COMPUTED BASED ON FILE-CONTENT) IN HEXSTRING FORMAT AS A KEY AND LIST OF FILE REFERENCES FOR THAT KEY 

void addShaIndexNode(string key, unsigned int fileRef )
{
	pthread_mutex_lock(&shaIndex_mutex);
	
	shaIndex.insert(pair<string, unsigned int>(key,fileRef));

	if(PRINT_FLAG) 
		printf("**** KEYWORD INDEX NODE ADDED: string %s fileRef %d shaIndex.size %d \n", key.c_str(), fileRef, shaIndex.size()); 

	pthread_mutex_unlock(&shaIndex_mutex);


}//end addShaIndexNode



void deleteShaIndexNode(unsigned int fileRef )
{

	pthread_mutex_lock(&shaIndex_mutex);

	multimap<string, unsigned int>::iterator it = shaIndex.begin();
	
	while(it != shaIndex.end())
	{
		if(fileRef == it->second )
		{
			shaIndex.erase(it);
	
			if(PRINT_FLAG)
				cout<<"deleting Key = "<< it->first <<"    Value = "<< it->second <<endl;
			
			break;
		}

		it++;
	}
	pthread_mutex_unlock(&shaIndex_mutex);

	return;

} // end deleteShaIndexNode

int findKeyInShaIndex(string key)
{

	int returnvalue = FALSE;
	
	pthread_mutex_lock(&shaIndex_mutex);

	multimap<string, unsigned int>::iterator it;
	
	it = shaIndex.find(key);

	if(it!= shaIndex.end())
	{
		returnvalue = TRUE;

		if(PRINT_FLAG)
			printf("\n\nfound key %s TRUE \n", key.c_str());
	}

	pthread_mutex_unlock(&shaIndex_mutex);

	return returnvalue;
	
} // end findKeyInShaIndex


void getValueFromShaIndex (string key, unsigned int fileRef[])
{
	int cnt = 0;

	pthread_mutex_lock(&shaIndex_mutex);

	//Print all Joe from the list and then erase them
	pair<multimap<string,unsigned int>::iterator, multimap<string,unsigned int>::iterator> ii;

	multimap<string, unsigned int>::iterator it; //Iterator to be used along with ii

	ii = shaIndex.equal_range(key); //We get the first and last entry in ii;

	if(PRINT_FLAG)
		cout<<"\n\nPrinting all info abt FILE SHA1 "<< key << endl;

	for(it = ii.first; it != ii.second; ++it)
	{
		fileRef[cnt] = it->second;

		if(PRINT_FLAG)
			cout<<"Value = "<< fileRef[cnt] <<endl;
		cnt++;
	}

 	pthread_mutex_unlock(&shaIndex_mutex);

	return;

}//end getValueFromShaIndex

void getKeyFromShaIndex (string key, unsigned int fileRef)
{	
	pthread_mutex_lock(&shaIndex_mutex);

	multimap<string, unsigned int>::iterator it = shaIndex.begin();
	
	while(it != shaIndex.end())
	{
		if(it->second == fileRef)
		{
			key = it->first;
		
			if(PRINT_FLAG)
				cout<<"Key = "<<it->first<<"    Value = "<<it->second<<endl;
		}
	
		it++;
	}

	pthread_mutex_unlock(&shaIndex_mutex);

} // end getKeyFromShaIndex

int getCountForShaKey (string key)
{
	int count = 0;

	pthread_mutex_lock(&shaIndex_mutex);
	count = shaIndex.count(key);
	pthread_mutex_unlock(&shaIndex_mutex);

	return count;

} // end getCountForShaKey

int getShaIndexSize()
{

	int size = 0;

	pthread_mutex_lock(&shaIndex_mutex);
	size = shaIndex.size();
	pthread_mutex_unlock(&shaIndex_mutex);

	return size;


}// end getShaIndexSize


void displayShaIndex()
{
	pthread_mutex_lock(&shaIndex_mutex);

	if(PRINT_FLAG)
	{
		cout<<"\n\nMultimap displayshaIndex method"<<endl;
		cout<<"Map size = "<<shaIndex.size()<<endl;
	}

	multimap<string, unsigned int>::iterator it = shaIndex.begin();
	
	while(it != shaIndex.end())
	{
		if(PRINT_FLAG)
			cout<<"Key = "<<it->first<<"    Value = "<<it->second<<endl;
	
		it++;
	}
	pthread_mutex_unlock(&shaIndex_mutex);

} // end displayShaIndex


/************************************************** DEFINE FILEID MAP ***************************************************************************/

/* add <key-value> pair in the fileid map <fileRef, fileID>
	given the fileRef, it generates unique fileID to be used across the network
*/
void addNodeInFileIDMap(unsigned char fileID[UOID_LEN], int fileRef)
{
	pthread_mutex_lock(&fileidMap_mutex);

	gen_onetime_pwd(fileID);
	RoutingNode node = RoutingNode(fileID, fileRef);

	// Add it to the FILEID MAP
	fileidMap.push_back(node);	

	if(PRINT_FLAG)
		printf("**** FILEID NODE ADDED %02x%02x%02x%02x: sockfd_new %d msgRoutingTable.size() %d \n", fileID[16], fileID[17], fileID[18], fileID[19], node.sockfd, fileidMap.size());

	pthread_mutex_unlock(&fileidMap_mutex);

}

// delete <key-value> pair in the fileid map <fileRef, fileID> given the fileRef
void deleteNodeInFileIDMap(int fileRef)
{
	pthread_mutex_lock(&fileidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=fileidMap.begin() ; it != fileidMap.end(); it++ )
	{
		if((*it).sockfd == fileRef)
		{
			fileidMap.erase(it);
		}
	} // end for 
	//printf("!!!!!!! msgRoutingTable.size() %d AFTER DELETETION !!!!!!!!\n", msgRoutingTable.size());


	pthread_mutex_unlock(&fileidMap_mutex);

}

// returns true upon finding fileRef in the fileid map or else returns false
int findFileIDInFileIDMap(int fileRef)
{
	int returnvalue = FALSE;

	pthread_mutex_lock(&fileidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=fileidMap.begin() ; it != fileidMap.end(); it++ )
	{
		if((*it).sockfd == fileRef)
		{
			returnvalue = TRUE; //(*it).sockfd;
			break;
		}
	} // end for 
	
	pthread_mutex_unlock(&fileidMap_mutex);

	return returnvalue;

	// return after releasing the lock

}

// returns true upon finding given fileID in the fileid map or else returns false
// returns the fileRef corresponding to fileID
int findFileIDInFileIDMap(int &fileRef, unsigned char fileID[])
{
	int returnvalue = FALSE;

	pthread_mutex_lock(&fileidMap_mutex);

	vector<RoutingNode>::iterator it;
	fileRef = 0;
	for ( it=fileidMap.begin() ; it != fileidMap.end(); it++ )
	{
		if(memcmp((*it).UOID, fileID, UOID_LEN)==0)
		{
			returnvalue = TRUE; //(*it).sockfd;
			fileRef = (*it).sockfd;
			printf("!!!!fileid found in the fileid map, fileRef %d \n!!!", fileRef);
			break;
		}
	} // end for 
	
	pthread_mutex_unlock(&fileidMap_mutex);

	return returnvalue;

	// return after releasing the lock

} // end findFileIDInFileIDMap

int getFileIDFromFileIDMap(int fileRef, unsigned char fileID[UOID_LEN])
{
	int returnvalue = FALSE;

	pthread_mutex_lock(&fileidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=fileidMap.begin() ; it != fileidMap.end(); it++ )
	{
		if((*it).sockfd == fileRef)
		{
			RoutingNode node = *it;

			memcpy(&fileID[0], &node.UOID[0],UOID_LEN);
			returnvalue = TRUE;
		//printf("!!!!!!! msgRoutingTable.size() %d DURING ADDITION uoid %02x%02x%02x%02x, sockfd %d *********\n", msgRoutingTable.size(),
			//node.UOID[16], node.UOID[17], node.UOID[18], node.UOID[19], node.sockfd);

		}
	} // end for 

	pthread_mutex_unlock(&fileidMap_mutex);

	return returnvalue;
}

void displayFileIDMap()
{

	pthread_mutex_lock(&fileidMap_mutex);
	
	//printf("!!!!*********!! MY FILEID MAP IS SIZE: %d!!!!!!!****!!!!!!!!!!!!!!!! \n\n", fileidMap.size());
	
	vector<RoutingNode>::iterator it;

	for ( it=fileidMap.begin() ; it != fileidMap.end(); it++ )
	{
		RoutingNode node = *it;
		
		if(PRINT_FLAG)
			printf("!!!*********!!! fileID %02x%02x%02x%02x, fileRef %d *********\n", node.UOID[16], node.UOID[17], node.UOID[18], node.UOID[19], node.sockfd);
	}


	pthread_mutex_unlock(&fileidMap_mutex);

}

int findIDMatchInFileIDMap(unsigned char fileID[UOID_LEN])
{

	int returnvalue = FALSE;
	int sock = 0;

	pthread_mutex_lock(&fileidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=fileidMap.begin() ; it != fileidMap.end(); it++ )
	{
		if(memcmp((*it).UOID, fileID, UOID_LEN)==0)
		{
			returnvalue = TRUE; //(*it).sockfd;
			sock = (*it).sockfd;
			break;
		}
	} // end for 
	
	if(PRINT_FLAG)
		printf("!!!!!!! fileidMap.size() %d AFTER FINDING %02x%02x%02x%02x return value %d sockfd %d !!!!!!!!\n", fileidMap.size(), fileID[16], fileID[17], fileID[18], fileID[19], returnvalue, sock);

	pthread_mutex_unlock(&fileidMap_mutex);

	return returnvalue;

}

unsigned int getFileRefFromFileIDMap(unsigned char fileID[UOID_LEN])
{

	int returnvalue = 0;

	pthread_mutex_lock(&fileidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=fileidMap.begin() ; it != fileidMap.end(); it++ )
	{
		if(memcmp((*it).UOID, fileID, UOID_LEN)==0)
		{
			returnvalue = (*it).sockfd;
			break;
		}
	} // end for 
	
	if(PRINT_FLAG)
		printf("!!!!!!! fileidMap.size() %d AFTER FINDING %02x%02x%02x%02x return value %d sockfd %d !!!!!!!!\n", fileidMap.size(), fileID[16], fileID[17], fileID[18], fileID[19], returnvalue, returnvalue);

	pthread_mutex_unlock(&fileidMap_mutex);

	return returnvalue;
}

/************************************************** COMMON METHODS ***************************************************************************/

// todo check the usage of this function
// adds the index information related to fileRef - either by store, get commands 
void addAllIndexNodes(string kwrd_key, string name_key, string sha_key,  unsigned int fileRef)
{

	addKwrdIndexNode(kwrd_key, fileRef);
	addNameIndexNode(name_key, fileRef);
	addShaIndexNode(sha_key, fileRef);

} // end addAllIndexNodes

void deleteAllIndexNodes(unsigned int fileRef)
{
	deleteNameIndexNode(fileRef);

	// pull out the corresponding 
	deleteKwrdIndexNode(fileRef);
	
	deleteShaIndexNode(fileRef);

} // end deleteAllIndexNodes


// todo check the usage of this function
// adds the index information related to fileRef - either by store, get commands 
void displayAllIndexNodes()
{

	displayKwrdIndex();
	displayNameIndex();
	displayShaIndex();

} // end addAllIndexNodes


void key_wrd_search(char *str, unsigned int &len, unsigned int fileRef[])    // should pass pointer to all the keywords with some delimiter
{
	// first calculate the bitvector of the key words  //tokanizing it to get seprate keywords
	vector<string> command;
	Tokenize(str, command, " =\""); //todo is the delimiters specified correctly?
	int token = command.size();

	//cout << token <<" : keywords passed"<<endl;

	bitvec kwrdBV=bitvec();
	
	for(int h=0; h<token; h++)
	{
		 kwrdBV.setBV_keywd((char*)command[h].c_str());
	}
	
	kwrdBV.printBV_hex();

	// go thru each bit vector index to identify all the files

	// convert the bv hexstring into bitvec, then AND

	pthread_mutex_lock(&kwrdIndex_mutex);

	multimap<string, unsigned int>::iterator it = kwrdIndex.begin();
	
	len = 0;
	while(it != kwrdIndex.end())
	{
		bitvec fileBV;
		fileBV.convertFromHex((unsigned char *)((it->first).c_str()));
		//printf("converting the hexstring to bitvec \n");
		fileBV.printBV_hex();
						
		int retval = bitvec_hit(fileBV.bitvector, kwrdBV.bitvector);

		if(retval==TRUE)
		{
			fileRef[len] = it->second;
			len++;

			if(PRINT_FLAG)
				cout<<"Key = "<<it->first<<"    Value = "<<it->second<<endl;		
		}
	
		it++;
	}
	pthread_mutex_unlock(&kwrdIndex_mutex);
}

/************************************************** DEFINE SEARCHID MAP ***************************************************************************/

/* - declared above in cmdthread section
std::vector<RoutingNode> searchidMap;
pthread_mutex_t searchidMap_mutex; 
*/
void addNodeInSearchIDMap(unsigned char fileID[UOID_LEN], int fileRef, unsigned char sha1_filestr[])
{
	pthread_mutex_lock(&searchidMap_mutex);

	RoutingNode node = RoutingNode(fileID, fileRef, sha1_filestr);

	// Add it to the SEARCHID MAP
	searchidMap.push_back(node);	

	if(PRINT_FLAG)
		printf("**** FILEID NODE ADDED %02x%02x%02x%02x: sockfd_new %d msgRoutingTable.size() %d \n", fileID[16], fileID[17], fileID[18], fileID[19], node.sockfd, searchidMap.size());

	pthread_mutex_unlock(&searchidMap_mutex);


}

void deleteNodeInSearchIDMap(int fileRef)
{
	pthread_mutex_lock(&searchidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=searchidMap.begin() ; it != searchidMap.end(); it++ )
	{
		if((*it).sockfd == fileRef)
		{
			searchidMap.erase(it);
		}
	} // end for 
	
	if(PRINT_FLAG)
		printf("!!!!!!! SEARCH NODE %d deleted.. msgRoutingTable.size() %d AFTER DELETETION !!!!!!!!\n", fileRef, searchidMap.size());


	pthread_mutex_unlock(&searchidMap_mutex);


}

int findFileIDInSearchIDMap(int fileRef)
{

	int returnvalue = FALSE;

	pthread_mutex_lock(&searchidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=searchidMap.begin() ; it != searchidMap.end(); it++ )
	{
		if((*it).sockfd == fileRef)
		{
			returnvalue = TRUE; //(*it).sockfd;
			break;
		}
	} // end for 
	
	pthread_mutex_unlock(&searchidMap_mutex);

	return returnvalue;

	// return after releasing the lock

}

int getFileIDFromSearchIDMap(int fileRef, unsigned char fileID[UOID_LEN])
{
	int returnvalue = FALSE;

	pthread_mutex_lock(&searchidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=searchidMap.begin() ; it != searchidMap.end(); it++ )
	{
		if((*it).sockfd == fileRef)
		{
			RoutingNode node = *it;

			memcpy(&fileID[0], &node.UOID[0],UOID_LEN);
			returnvalue = TRUE;
			break;
		//printf("!!!!!!! msgRoutingTable.size() %d DURING ADDITION uoid %02x%02x%02x%02x, sockfd %d *********\n", msgRoutingTable.size(),
			//node.UOID[16], node.UOID[17], node.UOID[18], node.UOID[19], node.sockfd);

		}
	} // end for 

	pthread_mutex_unlock(&searchidMap_mutex);

	return returnvalue;
}

int getSHA1FromSearchIDMap(int fileRef, unsigned char sha1file[])
{
	int returnvalue = FALSE;

	pthread_mutex_lock(&searchidMap_mutex);

	vector<RoutingNode>::iterator it;
	
	for ( it=searchidMap.begin() ; it != searchidMap.end(); it++ )
	{
		if((*it).sockfd == fileRef)
		{
			RoutingNode node = *it;

			memcpy(&sha1file[0], &node.sha1_file[0],SHA_DIGEST_LEN);
			returnvalue = TRUE;
			if(PRINT_FLAG)
				printf("***************found SHA1 \n");
			break;
		//printf("!!!!!!! searchidMap.size() %d DURING ADDITION uoid %02x%02x%02x%02x, sockfd %d *********\n", msgRoutingTable.size(),
			//node.UOID[16], node.UOID[17], node.UOID[18], node.UOID[19], node.sockfd);

		}
	} // end for 

	pthread_mutex_unlock(&searchidMap_mutex);

	return returnvalue;
}


void displaySearchIDMap()
{
	pthread_mutex_lock(&searchidMap_mutex);
	
	//printf("!!!!*********!! MY FILEID MAP IS SIZE: %d!!!!!!!****!!!!!!!!!!!!!!!! \n\n", searchidMap.size());
	
	vector<RoutingNode>::iterator it;

	for ( it=searchidMap.begin() ; it != searchidMap.end(); it++ )
	{
		RoutingNode node = *it;
		
		if(PRINT_FLAG)
			printf("!!!*********!!! SEARCH fileID %02x%02x%02x%02x, fileRef %d *********\n", node.UOID[16], node.UOID[17], node.UOID[18], node.UOID[19], node.sockfd);
	}


	pthread_mutex_unlock(&searchidMap_mutex);
}


// clear the search id list before populating it with the new search info
void clearSearchIDMap()
{

	if(PRINT_FLAG)
		printf("**** SEARCHID NODE DELETION BEGINS SEARCHID MAP Size() %d \n", searchidMap.size());

	pthread_mutex_lock(&searchidMap_mutex);

	while (!searchidMap.empty())
	{		
		searchidMap.pop_back();
	}
	
	pthread_mutex_unlock(&searchidMap_mutex);
}

