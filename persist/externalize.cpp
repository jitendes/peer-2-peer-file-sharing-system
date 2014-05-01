#include "global.h"
#include "iniparserClass.h"


// This file contains variables and data structures that must be externalized before the node goes down

extern iniparserClass *iniparserInfo;

extern std::multimap<string, unsigned int> kwrdIndex;
extern pthread_mutex_t kwrdIndex_mutex; 

extern std::multimap<string, unsigned int> nameIndex;
extern pthread_mutex_t nameIndex_mutex; 

extern std::multimap<string, unsigned int> shaIndex;
extern pthread_mutex_t shaIndex_mutex; 

extern std::list<LRU_Node> LRUList;
extern pthread_mutex_t LRUList_mutex; 

extern void addKwrdIndexNode(string key, unsigned int fileRef );

extern void addNameIndexNode(string key, unsigned int fileRef );

extern void addShaIndexNode(string key, unsigned int fileRef );

extern int addNodeInLRUList(unsigned int newfileRef, unsigned int newfileRef_Size);

extern unsigned int globalFileNum; // later read it from the external file wenever node restarts - currently initialized in initialize() func

extern unsigned int cacheSize;  // it is in KB so multiply by 1024 to get total byte size

// in bytes
extern unsigned int currentCacheSize;

// in bytes
extern unsigned int remainingCacheSize;

// checks whether the specified file exists in the home dir of the node
int ext_file_exist(char *filename)
{
	if(PRINT_FLAG)
		printf("ext_file exist is %s\n",filename);

    FILE  *fp = fopen(filename, "r");	

	if(!fp) {
		return FALSE;
	}
	return TRUE;
}

int removeFile(char *filename)
{
	if(ext_file_exist(filename) == TRUE)
	{
		remove(filename);
		return TRUE;
	}
	else
		return FALSE;

}
/********************************* GLOBALFILECOUNT: REMEMBERS THE LARGEST FILE WE HAVE STORED IN THE NODE'S DIR *********************/

int gfileCnt_exist()
{
	char gfilePath[256] = "";
	sprintf(gfilePath, "%s/gfileCnt", iniparserInfo->myhomedir);

	if(ext_file_exist(gfilePath) == TRUE)
		return TRUE;
	else
		return FALSE;
} // end gfileCnt_exist

int gfileCnt_delete()
{
	char gfilePath[256] = "";
	sprintf(gfilePath, "%s/gfileCnt", iniparserInfo->myhomedir);

	if(ext_file_exist(gfilePath) == TRUE)
	{
		remove(gfilePath);
		if(PRINT_FLAG)
			printf("gfilePath is deleted successfully \n");
		return TRUE;
	}
	else
		return FALSE;

} // end gfileCnt_delete

int gfileCnt_write()
{
	char gfilePath[256] = "";
	sprintf(gfilePath, "%s/gfileCnt", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", gfilePath);

	ofstream write(gfilePath);//writing to a file
	if (write.is_open())
	{				
		write << globalFileNum << endl ;

	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	write.close();

	return TRUE;
	
}

int gfileCnt_read()
{
	char gfilePath[256] = "";
	sprintf(gfilePath, "%s/gfileCnt", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", gfilePath);


	FILE *fr;            /* declare the file pointer */
	char line[500];
		
	if(gfileCnt_exist() == TRUE)
	{
		fr = fopen (gfilePath, "rb");  /* open the file for reading */
		

		fgets(line, 500, fr);

		globalFileNum = atoi(line);

		//printf("num of entries %d \n", globalFileNum);

	} // end if(gfileCnt_exist() == TRUE)

	else
	{
	
		return FALSE;
	}

	fclose(fr);

	return globalFileNum;

} // end gfileCnt_read

/********************************* KWRD_INDEX:  MAPS A BIT-VECTOR TO A LIST OF FILE REFERENCES *********************/

int kwrd_index_exist()
{
	char kwrdPath[256] = "";
	sprintf(kwrdPath, "%s/kwrd_index", iniparserInfo->myhomedir);

	if(ext_file_exist(kwrdPath) == TRUE)
		return TRUE;
	else
		return FALSE;
} // end kwrd_index_exist

int kwrd_index_delete()
{
	char kwrdPath[256] = "";
	sprintf(kwrdPath, "%s/kwrd_index", iniparserInfo->myhomedir);

	if(ext_file_exist(kwrdPath) == TRUE)
	{
		remove(kwrdPath);
		if(PRINT_FLAG)
			printf("kwrd_index is deleted successfully \n");
		return TRUE;
	}
	else
		return FALSE;

} // end kwrd_index_delete

int kwrd_index_write()
{

	char kwrdPath[256] = "";
	sprintf(kwrdPath, "%s/kwrd_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", kwrdPath);

	ofstream write(kwrdPath);//writing to a file
	if (write.is_open())
	{				
		write << kwrdIndex.size() << endl ;

		multimap<string, unsigned int>::iterator it = kwrdIndex.begin();
		
		while(it != kwrdIndex.end())
		{

			write << it->first << endl;
			write << it->second << endl;
		
			it++;
		}
	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	write.close();

	return TRUE;

} // end kwrd_index_write


int kwrd_index_read()
{
	char kwrdPath[256] = "";
	sprintf(kwrdPath, "%s/kwrd_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", kwrdPath);


	FILE *fr;            /* declare the file pointer */
	char line[500];
		
	if(kwrd_index_exist() == TRUE)
	{
		fr = fopen (kwrdPath, "rb");  /* open the file for reading */
		
		int i=1;

		fgets(line, 500, fr);

		int num = atoi(line);

		//printf("num of kwrd entries %d \n", num);

		while(i <= num)
		{				
			fgets(line, 500, fr);
			
			unsigned char *tempstr = (unsigned  char*) strtok(line, "=");
			tempstr[2*BIT_VECTOR_LEN] = '\0';

			unsigned char bitvec_readstr[2*BIT_VECTOR_LEN + 1] =  "";		
			memset(&bitvec_readstr[0], 0 ,2*BIT_VECTOR_LEN);
			memcpy(&bitvec_readstr[0], tempstr ,2*BIT_VECTOR_LEN);
			bitvec_readstr[2*BIT_VECTOR_LEN] = '\0'; // avoid the /n character

			
			if(PRINT_FLAG)
				cout << " KWRD key is " << bitvec_readstr << endl;

			fgets(line, 500, fr);
			unsigned int fileRef = atoi(line);

			if(PRINT_FLAG)
				cout<< " KWRD value is "<< fileRef << endl;
			char *bitvec_string = (char *)bitvec_readstr;
			addKwrdIndexNode(bitvec_string, fileRef);

			i++;

		}
	} // end if(kwrd_index_exist() == TRUE)

	else
	{
	
		return FALSE;
	}

	fclose(fr);
	return TRUE;
} // end kwrd_index_read

/********************************* NAME_INDEX:  MAPS A FILENAME TO A LIST OF FILE REFERENCES *********************/

int name_index_exist()
{
	char namePath[256] = "";
	sprintf(namePath, "%s/name_index", iniparserInfo->myhomedir);

	if(ext_file_exist(namePath) == TRUE)
		return TRUE;
	else
		return FALSE;

} // end name_index_exist



int name_index_delete()
{
	char namePath[256] = "";
	sprintf(namePath, "%s/name_index", iniparserInfo->myhomedir);

	if(ext_file_exist(namePath) == TRUE)
	{
		remove(namePath);
		if(PRINT_FLAG)
			printf("name_index is deleted successfully \n");
		return TRUE;
	}
	else
		return FALSE;

} // end name_index_delete

int name_index_write()
{
	char namePath[256] = "";
	sprintf(namePath, "%s/name_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", namePath);

	ofstream write(namePath);//writing to a file
	if (write.is_open())
	{				
		write << nameIndex.size() << endl ;

		multimap<string, unsigned int>::iterator it = nameIndex.begin();
		
		while(it != nameIndex.end())
		{

			write << it->first << endl;
			write << it->second << endl;
		
			it++;
		}
	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	write.close();

	return TRUE;
}

int name_index_read()
{
	char namePath[256] = "";
	sprintf(namePath, "%s/name_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", namePath);

	FILE *fr;            /* declare the file pointer */
	char line[500];
		
	if(name_index_exist() == TRUE)
	{
		fr = fopen (namePath, "rb");  /* open the file for reading */
		
		int i=1;

		fgets(line, 500, fr);

		int num = atoi(line);

		//printf("num of name entries %d \n", num);

		while(i <= num)
		{				
			fgets(line, 500, fr);
			
			char fname_readstr[256] = "";
			char *tempstr = (char *)strtok(line, "=");	
			memcpy(&fname_readstr[0], tempstr , strlen(tempstr));
			fname_readstr[strlen(tempstr)-1] = '\0';
			
			if(PRINT_FLAG)
				cout<< "FNAME key is "<<fname_readstr <<endl;

			fgets(line, 500, fr);
			unsigned int fileRef = atoi(line);

			if(PRINT_FLAG)
				cout<< "FNAME value is "<< fileRef << endl;

			addNameIndexNode(fname_readstr, fileRef);

			i++;

		}
	} // end if(name_index_exist() == TRUE)

	else
	{
	
		return FALSE;
	}

	fclose(fr);
	return TRUE;
}

/********************************* SHA1_INDEX:  MAPS A SHA1 TO A LIST OF FILE REFERENCES *********************/
int sha1_index_exist()
{
	char shaPath[256] = "";
	sprintf(shaPath, "%s/sha1_index", iniparserInfo->myhomedir);

	if(ext_file_exist(shaPath) == TRUE)
		return TRUE;
	else
		return FALSE;

} // end sha1_index_exist

int sha1_index_delete()
{
	char shaPath[256] = "";
	sprintf(shaPath, "%s/sha1_index", iniparserInfo->myhomedir);

	if(ext_file_exist(shaPath) == TRUE)
	{
		remove(shaPath);
		if(PRINT_FLAG)
			printf("sha1_index is deleted successfully \n");
		return TRUE;
	}
	else
		return FALSE;

} // end sha1_index_delete

int sha1_index_write()
{
	char shaPath[256] = "";
	sprintf(shaPath, "%s/sha1_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", shaPath);

	ofstream write(shaPath);//writing to a file
	if (write.is_open())
	{				
		write << shaIndex.size() << endl ;

		multimap<string, unsigned int>::iterator it = shaIndex.begin();
		
		while(it != shaIndex.end())
		{

			write << it->first << endl;
			write << it->second << endl;
		
			it++;
		}
	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	write.close();

	return TRUE;

}

int sha1_index_read()
{
	char shaPath[256] = "";
	sprintf(shaPath, "%s/sha1_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", shaPath);

	FILE *fr;            /* declare the file pointer */
	char line[500];
		
	if(sha1_index_exist() == TRUE)
	{
		fr = fopen (shaPath, "rb");  /* open the file for reading */
		
		int i=1;

		fgets(line, 500, fr);

		int num = atoi(line);

		//printf("num of sha1 entries %d \n", num);

		while(i <= num)
		{				
			fgets(line, 500, fr);
					
			unsigned char *tempstr = (unsigned  char*) strtok(line, "=");
			tempstr[2*SHA_DIGEST_LEN] = '\0';
			
			unsigned char sha1_readstr[2*SHA_DIGEST_LEN + 1] = "";	
			memset(&sha1_readstr[0], 0 ,2*SHA_DIGEST_LEN);
			memcpy(&sha1_readstr[0], tempstr ,2*SHA_DIGEST_LEN);
			sha1_readstr[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character
						
			if(PRINT_FLAG)
				cout<< "SHA1 key is "<<sha1_readstr <<endl;

			fgets(line, 500, fr);
			unsigned int fileRef = atoi(line);

			if(PRINT_FLAG)
				cout<< "SHA1 value is "<< fileRef << endl;
			
			char *sha1_readstring = (char *)sha1_readstr;
			addShaIndexNode(sha1_readstring, fileRef);
			i++;

		}
	} // end if(kwrd_index_exist() == TRUE)

	else
	{
	
		return FALSE;
	}

	fclose(fr);
	return TRUE;
}
/********************************* LRU_INDEX:  LIST OF A LRU NODES - CACHED FILES *********************/
int lru_index_exist()
{
	char lruPath[256] = "";
	sprintf(lruPath, "%s/lru_index", iniparserInfo->myhomedir);

	if(ext_file_exist(lruPath) == TRUE)
		return TRUE;
	else
		return FALSE;

} // end lru_index_exist



int lru_index_delete()
{
	char lruPath[256] = "";
	sprintf(lruPath, "%s/lru_index", iniparserInfo->myhomedir);

	if(ext_file_exist(lruPath) == TRUE)
	{
		remove(lruPath);
		if(PRINT_FLAG)
			printf("lru_index is deleted successfully \n");
		return TRUE;
	}
	else
		return FALSE;

} // end lru_index_delete

int lru_index_write()
{
	char lruPath[256] = "";
	sprintf(lruPath, "%s/lru_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", lruPath);

	ofstream write(lruPath);//writing to a file
	if (write.is_open())
	{				
		write << currentCacheSize << endl ;
		write << remainingCacheSize << endl ;
		write << LRUList.size() << endl ;

		list<LRU_Node>::iterator it;

		for ( it=LRUList.begin() ; it != LRUList.end(); it++ )
		{
			LRU_Node Node = *it;

			if(PRINT_FLAG)
				printf("!!!!!! LRU Node: FileRef %d, FileSize %d  \n !!!!!!!!", Node.fileRef, Node.fileRef_Size);

			write << Node.fileRef << endl;
			write << Node.fileRef_Size << endl;
		}

	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	write.close();

	return TRUE;

}

int lru_index_read()
{
	char lruPath[256] = "";
	sprintf(lruPath, "%s/lru_index", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("new filepath %s \n", lruPath);

	FILE *fr;            /* declare the file pointer */
	char line[500];
		
	if(lru_index_exist() == TRUE)
	{
		fr = fopen (lruPath, "rb");  /* open the file for reading */
		
		int i=1;

		fgets(line, 500, fr);		
		currentCacheSize = atoi(line); 

		fgets(line, 500, fr);	
		remainingCacheSize = atoi(line); 

		fgets(line, 500, fr);	
		int num = atoi(line); 

		//printf("num of lru entries %d \n", num);

		while(i <= num)
		{				
			fgets(line, 500, fr);			
			unsigned int newfileRef = atoi(line);
			
			if(PRINT_FLAG)
				cout << " newfileRef is " << newfileRef << endl;

			fgets(line, 500, fr);
			unsigned int newfileRef_Size = atoi(line);

			if(PRINT_FLAG)
				cout<< " newfileRef_Size is "<< newfileRef_Size << endl;		

			LRU_Node node = LRU_Node(newfileRef, newfileRef_Size);
			
			// Add it to the LRUList
			LRUList.push_back (node);

			i++;

		}
	} // end if(kwrd_index_exist() == TRUE)

	else
	{
	
		return FALSE;
	}

	fclose(fr);
	return TRUE;

}

/********************************* ALL OPERATIONS *********************************************/

void deleteAllExtIndexs()
{
	kwrd_index_delete();
	name_index_delete();
	sha1_index_delete();
	lru_index_delete();
	gfileCnt_delete();

} // end deleteAllExtIndexs


void writeAllExtIndexs()
{
	kwrd_index_write();
	name_index_write();
	sha1_index_write();
	lru_index_write();
	gfileCnt_write();

} // end writeAllExtIndexs

void readAllExtIndexs()
{
	kwrd_index_read();
	name_index_read();
	sha1_index_read();
	lru_index_read();
	gfileCnt_read();

} // end readAllExtIndexs

