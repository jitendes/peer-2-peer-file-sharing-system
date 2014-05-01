#include "iniparserClass.h"

// whenever you receive a reply just create a stuct obj of type Node and add it in list of nodes
extern iniparserClass *iniparserInfo;

extern void addAllIndexNodes(string kwrd_key, string name_key, string sha_key,  unsigned int fileRef);
extern void displayAllIndexNodes();

extern unsigned int globalFileNum; 

extern pthread_mutex_t globalFileNum_mutex; 

extern void deleteNodeInLRUList(unsigned int newfileRef, unsigned int newfileRef_Size);
extern void deleteNodeInLRUList(unsigned int newfileRef);
extern void deleteAllIndexNodes(unsigned int fileRef);
extern int findFileInLRUList(unsigned int newfileRef);

extern int gfileCnt_read();

extern int ext_file_exist(char *filename);

/********************************************** GENERAL FUNCTIONS *******************************************************************/

// source http://www.cprogramming.com/tips/showTip.php?tip=59&count=30&page=0
void tolowerKeyword(char keywd[])
{

	for ( int ix = 0; keywd[ix] != '\0'; ix++)
	{
		 keywd[ix] = tolower(keywd[ix] );
	}

} //tolowerKeyword

unsigned char hexToByte(unsigned char ch)
{
	unsigned char newbyte;

	switch(ch)
	{
		case '0':        
		{
			newbyte = 0x00;
		}
		break;

		case '1':        
		{
			newbyte = 0x01;
		}
		break;

		case '2':        
		{
			newbyte = 0x02;

		}
		break;

		case '3':        
		{
			newbyte = 0x03;

		}
		break;

		case '4':        
		{
			newbyte = 0x04;

		}
		break;

		case '5':       
		{
			newbyte = 0x05;

		}
		break;

		case '6':        
		{
			newbyte = 0x06;

		}
		break;

		case '7':        
		{
			newbyte = 0x07;

		}

		break;

		case '8':        
		{
			newbyte = 0x08;

		}
		break;

		case '9':       
		{
			newbyte = 0x09;

		}
		break;

		case 'A':     
		case 'a': 
		{
			newbyte = 0x0A;

		}
		break;

		case 'B':       
		case 'b': 
		{
			newbyte = 0x0B;

		}
		break;

		case 'C':       
		case 'c': 
		{
			newbyte = 0x0C;

		}
		break;

		case 'D':      
		case 'd': 			
		{
			newbyte = 0x0D;

		}
		break;

		case 'E':       
		case 'e': 
		{
			newbyte = 0x0E;

		}
		break;

		case 'F':     
		case 'f': 			
		{
			newbyte = 0x0F;

		}
		break;

		default:
			break;
	} // end switch
	
	return newbyte;
	
} //hexToByte


/* type takes:
'm' - to compute the filesize of the file located in the minifilesystem
'c' - current working directory	
'h' - temporary files located in the home dir of the node
*/

unsigned int computeFileSize(char type, char *filename)
{
	// for computing the file size - fsz request
	struct stat fileStat;
	char fileName[256] = "";


	if(type == 'c')
	{
		sprintf(fileName, "%s", filename);	
	}
	
	// construct the proper file path
	else if(type == 'm') // 
	{		   
		sprintf(fileName, "%s/files/%s", iniparserInfo->myhomedir, filename);	
	}
	else if(type == 'h') 
	{		   
		sprintf(fileName, "%s/%s", iniparserInfo->myhomedir, filename);	
	}



	if(stat((const char *)fileName, &fileStat) < 0)
	{
		return -1;

	}

	else
	{
		return fileStat.st_size;
	
	
	}
} // end computeFileSize

// CacheProb, StoreProb, NeighborStoreProb - flips a coin (with this probability of getting a positive outcome) to decide if it should cache a copy of it.
int flipCoin(double prob) 
{
	//srand48(time(NULL));

	double random = drand48();
	
	if(PRINT_FLAG)
		printf("drand value is %f\n",random);
	
	if(random < prob) //if satisfy the prob given in ini file 
	{
		return TRUE;				
	}			
	return FALSE;
}


// prints sha1 digest computed in hex string format
void print_sha1_hex(unsigned char sha1[]) // works on 20 bytes
{
	//printf(" SHA1_hex = ");
	
	for(int i=0;i<SHA_DIGEST_LEN;i++)
	{
		if(PRINT_FLAG)
			printf("%02x", sha1[i]);
	}
	
	if(PRINT_FLAG)
		printf("\n");

}

// prints md5 digest computed in hex string format
void print_md5_hex(unsigned char md5[]) // works on 16 bytes
{
	if(PRINT_FLAG)
		printf(" MD5_hex = ");
	
	for(int i=0;i<MD5_DIGEST_LEN;i++)
	{
		if(PRINT_FLAG)
			printf("%02x", md5[i]);
	}
	
	if(PRINT_FLAG)
		printf("\n");

}

// prints sha1 digest computed in hex string format
// sha1 - works on 20 bytes, sha1buf - works on 40 bytes
void convert_sha1_to_hex(unsigned char sha1[], unsigned char sha1buf[])
{
	//printf(" SHA1_to_hex = ");
	
	for(int i=0;i<SHA_DIGEST_LEN;i++)
	{
		sprintf((char *)sha1buf, "%s%02x", sha1buf, (unsigned char)sha1[i]);
	}	

	if(PRINT_FLAG)
		printf("%s\n", sha1buf);
}

// updates sha1 - 20bytes
void convert_hex_to_sha1(unsigned char sha1[], unsigned char sha1buf[])
{

	for(int i=0; i<SHA_DIGEST_LEN; i++)
	{
		unsigned char byte1 = hexToByte(sha1buf[2*i]);
		unsigned char byte2 = hexToByte(sha1buf[2*i+1]);

		sha1[i] = (byte1 << 4) + byte2;
	}	
	print_sha1_hex(sha1);
	
	return;

}

// computes sha1 digest for the given filename
int gen_sha1_file(unsigned char sha1[], char *filename )
{
	//char file[256]="b3-n00.ini";
	long fl_buff_sz=5000;
	long lSize;
	FILE* fp = fopen(filename, "rb");
	if(fp==NULL) {
       return(0);
	    }	
		fseek (fp , 0 , SEEK_END);
		lSize = ftell (fp);
		rewind (fp);
		unsigned char datatwv[fl_buff_sz];	
		SHA_CTX ctx;
		SHA1_Init(&ctx);
		long fl_sz=0;
		while(lSize)           
		{
			size_t r=1;
		    fl_sz = fread(datatwv, sizeof(char),r, fp);
		    //printf("byte read from file is %s\n",datatwv);
			static int h=0;
			//printf("calliung sha1 update %d\n",h);
			h++;
			SHA1_Update(&ctx,(const void *)&datatwv[0],1);	
			lSize--;
		}
		fclose (fp);				 
		SHA1_Final(sha1, &ctx);
		
		if(PRINT_FLAG)
			printf("generated sha1 for file %s:  ", filename);
		
		print_sha1_hex(sha1);
		
		return SUCCESS;
			
}

// computes sha1 digest for the given keywd
int gen_sha1_keywd(unsigned char sha1[], char *keywd) //sha1 array is 20bytes
{

	SHA_CTX ctx;
	SHA1_Init(&ctx);

	for ( int ix = 0; keywd[ix] != '\0'; ix++)
	{
		SHA1_Update(&ctx,(const void *)&keywd[ix],1);	
	}
		SHA1_Final(sha1, &ctx);


	print_sha1_hex(sha1);


	return TRUE;
			
}

// computes md5 digest for the given keywd
int gen_md5_keywd(unsigned char md5[], char *keywd) //md5 array is 16 byte
{

	MD5_CTX ctx;
	MD5_Init(&ctx);

	for ( int ix = 0; keywd[ix] != '\0'; ix++)
	{
		MD5_Update(&ctx,(const void *)&keywd[ix],1);	
	}
	MD5_Final(md5, &ctx);


	print_md5_hex(md5); //to be changed
	

	/* extract the 9th bit by masking it from 15th byte and shift it left by 8 bits
		and then OR it with 16th byte to get the MD5(k) mod n where n is 512, k=md5
	*/	
	unsigned int md5_bit = md5[14];	
	md5_bit = md5_bit & 1;
	md5_bit = md5_bit << 8;
	md5_bit = md5_bit | md5[15];
	
	if(PRINT_FLAG)
		printf("md5 bit - bit vector: %d \n", md5_bit);
	
	return TRUE;
			
}


// given the sha1 digest, this function gives the sha1 bit to be set in the bit vector
unsigned int sha1_keywd_bit(unsigned char sha1[])	
{
	/* extract the 9th bit by masking it from 19th byte and shift it left by 8 bits
		and then OR it with 20th byte to get the SHA1(k) mod n where n is 512, k=shal
	*/
	unsigned int sha1_bit = sha1[18];
	sha1_bit = sha1_bit & 1;
	sha1_bit = sha1_bit << 8;
	sha1_bit = sha1_bit | sha1[19];

	if(PRINT_FLAG)
		printf("sha1 bit - bit vector: %d \n", sha1_bit);

	return sha1_bit;
}

// given the md5 digest, this function gives the md5 bit to be set in the bit vector
unsigned int md5_keywd_bit(unsigned char md5[])	
{

	/* extract the 9th bit by masking it from 15th byte and shift it left by 8 bits
		and then OR it with 16th byte to get the MD5(k) mod n where n is 512, k=md5
	*/	
	unsigned int md5_bit = md5[14];	
	md5_bit = md5_bit & 1;
	md5_bit = md5_bit << 8;
	md5_bit = md5_bit | md5[15];
	
	if(PRINT_FLAG)
		printf("md5 bit - bit vector: %d \n", md5_bit);

	return md5_bit;

}

// define this size at the place where u call the getuoid function int  uoid_buf_sz=20
// this function is replicated from msg.cpp for generating OTP
int getuoid(char *node_inst_id, char *obj_type, char *uoid_buf, unsigned int uoid_buf_size)
{
	static unsigned long seq_no = (unsigned long)1;
	char sha1_buf[SHA_DIGEST_LEN] , str_buf[104];
	snprintf(str_buf, sizeof(str_buf), "%s_%s_%1ld", node_inst_id, obj_type, (long)seq_no++);
	SHA1((unsigned char *)str_buf, strlen(str_buf), (unsigned char *)sha1_buf);
	memset(uoid_buf, 0, uoid_buf_size);
	memcpy(uoid_buf, sha1_buf, min(uoid_buf_size, sizeof(sha1_buf)));
	return 1;
}

/* A random 20-byte long one-time password is first generated. 
password - UOID_LEN
*/
int gen_onetime_pwd(unsigned char  password[])
{

	// GETTIME OF DAY - FOR UNIQUELY GENERATING THE OTP
	//Create the node ID and the node Instance ID 
	time_t currsecs;
	currsecs = time (NULL);		
	char currtime[MAX_NODE_INSTANCE_ID_LENGTH];

	memset(currtime,0,MAX_NODE_ID_LENGTH);
	sprintf(currtime,"%ld",currsecs);

    getuoid(currtime, (char *)"file",(char*) password, 20);

	if(PRINT_FLAG) 
		printf("OTP generated.. sha1 in hex: ");

	print_sha1_hex(password);


	return SUCCESS;

}

// The SHA1 hash of the password is then computed and is to be used as the nonce for this file.
// pwdsha1 - SHA_DIGEST_LEN, pwdUOID - UOID_LEN, generated by gen_onetime_pwd function
int gen_nonce(unsigned char pwdsha1[], unsigned char  pwdUOID[])
{
    int pwdUoid_sz=20;
	int h=0;
	SHA_CTX ctx;
	SHA1_Init(&ctx);
	while(pwdUoid_sz)           
	{
		SHA1_Update(&ctx,(const void *)&pwdUOID[h],1);	
		pwdUoid_sz--;
		h++;
	}
	SHA1_Final(pwdsha1, &ctx);
	
	if(PRINT_FLAG) 
		printf("nonce generated.. sha1 in hex: ");

	print_sha1_hex(pwdsha1);

	return SUCCESS;

}

/******************************************* FILE CREATION FUNCTIONS ************************************************************/

// todo hav to test it
//taken from http://www.dreamincode.net/code/snippet2306.htm
int create_data_file (const char SRC[], const char DEST[])
{
    std::ifstream src; // the source file
    std::ofstream dest; // the destination file
    src.open (SRC, std::ios::binary); // open in binary to prevent jargon at the end of the buffer
    dest.open (DEST, std::ios::binary); // same again, binary
    if (!src.is_open() || !dest.is_open())
        return false; // could not be copied
    dest << src.rdbuf (); // copy the content
    dest.close (); // close destination file
    src.close (); // close source file
    return true; // file copied successfully

}



// this function creates pass for filenum
// todo have to test it
// Here is an example (<CR><LF>'s have been left out and the backslashes have been inserted for readability): 
int create_pass_file(int filenum, unsigned char pwdUOID[])
{
	if(PRINT_FLAG)
		printf("create_pass_file function filenum %d \n", filenum);

    char filePath[256] = "";
    sprintf(filePath, "%s/files/%d.pass", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new filepath %s \n", filePath);

	ofstream write(filePath);//writing to a file
	if (write.is_open())
	{
		
		/******* WRITE PASSWORD *********/
		write << pwdUOID;		

		write.close();
	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	return SUCCESS;
}

/******************************************* STORE SEND FUNCTIONS ******************************************************************/

// this function creates metadata for filenum
// todo have to test it
// Here is an example (<CR><LF>'s have been left out and the backslashes have been inserted for readability): 
// creating the "meta" file, "data" files, creates pass file, adding into all the indexs - addAllIndexNodes
int create_store_file(int filenum, char *fileName, unsigned int fileSize, char *keywords, unsigned char bitvec_str[])
{
		//will get the home direstory throught iniparserInfo->homedir
	   //taken from http://www.go4expert.com/forums/showthread.php?t=9031
	char pathName[256] = "";
	sprintf(pathName, "%s/files", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("create_metadata_file function filenum %d \n", filenum);

	if(mkdir(pathName,0777)==-1)//creating a directory if the directory does not exist
	{
		//printf("unable to create directory\n");
		//cerr<<"Error :"<<strerror(errno)<<endl;

		/*if(PRINT_FLAG)
			printf("errno: %d\n",errno);*/

		if(errno==EEXIST)
		{
			if(PRINT_FLAG)
				printf("directory already exist\n");
		}
		else
		{
		  return ERROR; //exit(1);
		}
	}

	// write the [metadata] section header into the file
    char filePath[256] = "";
    sprintf(filePath, "%s/files/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new filepath %s \n", filePath);

	ofstream write(filePath);//writing to a file
	if (write.is_open())
	{
		write << "[metadata]"<<endl; // creating the section header

		/******* WRITE FILENAME & FILESIZE *********/
		write << "FileName=" << fileName << endl;
		write << "FileSize=" << fileSize << endl;
		
		/******* GENERATE SHA1 FOR THE FILE *********/
		unsigned char sha1_file[SHA_DIGEST_LEN];
		unsigned char sha1_filestr[2*SHA_DIGEST_LEN] = "";

		// it takes from the current working directory
		gen_sha1_file(sha1_file, fileName);
		convert_sha1_to_hex(sha1_file, sha1_filestr);

		write << "SHA1="<< sha1_filestr << endl;

		/******* GENERATE SHA1 FOR THE FILE *********/
		unsigned char pwdUOID[UOID_LEN];
		unsigned char pwdUOID_str[2*UOID_LEN] = ""; // always do this b4 passing it to the function that constructs the string

		unsigned char sha1_pwd[SHA_DIGEST_LEN];
		unsigned char sha1_pwdstr[2*SHA_DIGEST_LEN] = ""; // always do this b4 passing it to the function that constructs the string
		
		// A random 20-byte long one-time password is first generated
		gen_onetime_pwd(pwdUOID);

		// The SHA1 hash of the password is then computed and is to be used as the nonce for this file
		gen_nonce(sha1_pwd, pwdUOID);			

		// generate 40-char string corresponding to SHA1 hash of the password
		convert_sha1_to_hex(sha1_pwd, sha1_pwdstr);  

		write << "Nonce=" << sha1_pwdstr << endl;

		/******* WRITE KEYWORDS & BIT-VECTOR *********/
		write << "Keywords=" << keywords << endl;
		write << "Bit-vector=" << bitvec_str;

		write.close();

		// creating the "data" file
		char destFilePath[256] = "";
		sprintf(destFilePath, "%s/files/%d.data", iniparserInfo->myhomedir,filenum);

		create_data_file(fileName,destFilePath);

		// generate 40-char string corresponding to the password
		convert_sha1_to_hex(pwdUOID, pwdUOID_str);  

		// creating the "pass"file 
		// store the password associated with the file stored
		create_pass_file(filenum, pwdUOID_str);

		// add the corresponding entries in all the index lists
		char *bitvec_string = (char *)bitvec_str;

		gen_sha1_file(sha1_file, fileName);
		convert_sha1_to_hex(sha1_file, sha1_filestr);
		char *sha1_filestring = (char *)sha1_filestr;
		
		if(PRINT_FLAG)
			printf("sha1_filestring to be added from the create_store_file %s \n", sha1_filestring);
			
		addAllIndexNodes(bitvec_string,  fileName, sha1_filestring, filenum);

	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	return SUCCESS;
}


void computeStoreMsgSize(unsigned int filenum, unsigned int &metaSize, unsigned int &dataSize)
{

	// given the filenum, compute the meta file size and data file size
    char metaPath[256] = "";
    sprintf(metaPath, "%d.meta", filenum);

	if(PRINT_FLAG)
		printf("new metaPath %s \n", metaPath);

    char dataPath[256] = "";
    sprintf(dataPath, "%d.data", filenum);

	if(PRINT_FLAG)
		printf("new dataPath %s \n", dataPath);

	metaSize = computeFileSize('m', metaPath);
	dataSize = computeFileSize('m', dataPath);

	//printf("FILENUMBER %d : metaSize %d dataSize %d \n", filenum, metaSize, dataSize);

}

// reading the meta file and writing it into storeMsg buffer
void formMetadataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *storeMsg)
{
	// given the filenum, compute the meta file size and data file size
    char metaPath[256] = "";
    sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new metaPath %s \n", metaPath);


	FILE *pFile = fopen((const char *)metaPath,"rb");

	// determine whether the specified file exists r not
	// if the file does not exist, GET_FAIL packet should be sent instead with no data
	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in formMetadataInBuffer !!!! \n");
	}

	else
	{

		// copy the meta data into the buffer:
		unsigned int result = fread(&storeMsg[startpos], 1, metaSize, pFile);

		if(result != metaSize)
		{
			
		}
		else
		{
			if(PRINT_FLAG)
				printf("data copied successfully \n");		
		}


	}
	
	fclose(pFile);


} // end formMetadataInBuffer

// reading the data file and writing it into storeMsg buffer
void formFiledataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int dataSize, unsigned char *storeMsg)
{
	// given the filenum, compute the meta file size and data file size
    char dataPath[256] = "";
    sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new dataPath %s \n", dataPath);


	FILE *pFile = fopen((const char *)dataPath,"rb");

	// determine whether the specified file exists r not
	// if the file does not exist, GET_FAIL packet should be sent instead with no data
	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in formFiledataInBuffer!!!! \n");
	}

	else
	{

		// copy the file data into the buffer:
		unsigned int result = fread(&storeMsg[startpos], 1, dataSize, pFile);

		if(result != dataSize)
		{
			
		}
		else
		{
			if(PRINT_FLAG)
				printf("data copied successfully \n");

			/*** to be removed .. testing purpose only ****/
			/*FILE * pFile1;
			
			pFile1 = fopen ( "myfile.data" , "wb" );
			fwrite (&storeMsg[startpos] , 1 , dataSize , pFile1 );
			fclose (pFile1);
			*/
		
		}


	}
	
	fclose(pFile);


} // end formFiledataInBuffer

/******************************************* METAFILE FUNCTIONS ******************************************************************/

// read the fname from the meta file
void get_fname_from_metafile(char *filename, char fname_readstr[])
{
	FILE *fr;            /* declare the file pointer */
	char line[256];
	
	if(PRINT_FLAG)
		cout<<"filename is "<< filename<<endl;
	
	fr = fopen (filename, "rb");  /* open the file for reading */
	
	if(!fr)
	   cout<<"could not open file"<<endl;
	
	int i=1;

	while(fgets(line, 256, fr) != NULL)
	{	
		if(i==2)
		{
			//unsigned char *tempstr; //[2*SHA_DIGEST_LEN] = "";

			if(PRINT_FLAG)
				printf("Have to Split on token = \"%s\"\n",line);
	
			strtok(line, "=");			
			char *tempstr = (char *)strtok(NULL, "=");			
			memcpy(&fname_readstr[0], tempstr , strlen(tempstr));
			fname_readstr[strlen(tempstr)-1] = '\0';
			
			if(PRINT_FLAG)
				cout<< "FNAME computed from meta file is "<<fname_readstr <<endl;
			break;
		}
		i++;

	}

	fclose(fr);


}

// read the file size from the metafile
void get_fsize_from_metafile(char *filename, unsigned int &fileSize)
{
	FILE *fr;            /* declare the file pointer */
	char line[256];
	
	if(PRINT_FLAG)
		cout<<"filename is "<< filename<<endl;
	
	fr = fopen (filename, "rb");  /* open the file for reading */
	
	if(!fr)
	   cout<<"could not open file"<<endl;
	
	int i=1;

	while(fgets(line, 256, fr) != NULL)
	{	
		if(i==3)
		{
			if(PRINT_FLAG)
				printf("Have to Split on token = \"%s\"\n",line);
	
			strtok(line, "=");			
			char *tempstr = (char *)strtok(NULL, "=");			
			fileSize = atoi(tempstr);	
			if(PRINT_FLAG)
				cout<< "FILESIZE computed from meta file is "<<fileSize <<endl;
			break;
		}
		i++;

	}

	fclose(fr);


}

// sha1_readstr is in terms of hexstring format - 2*SHA_DIGEST_LEN
void get_sha1_from_metafile(char *filename, unsigned char sha1_readstr[])
{
	FILE *fr;            /* declare the file pointer */
	char line[256];
	
	if(PRINT_FLAG)
		cout<<"filename is "<< filename<<endl;
	
	fr = fopen (filename, "rb");  /* open the file for reading */
	
	if(!fr)
	   cout<<"could not open file"<<endl;
	
	int i=1;

	while(fgets(line, 256, fr) != NULL)
	{	
		if(i==4)
		{
			//unsigned char *tempstr; //[2*SHA_DIGEST_LEN] = "";

			if(PRINT_FLAG)
				printf("Have to Split on token = \"%s\"\n",line);
	
			strtok(line, "=");			
			unsigned char *tempstr = (unsigned  char*) strtok(NULL, "=");
			tempstr[2*SHA_DIGEST_LEN] = '\0';

			memcpy(&sha1_readstr[0], tempstr ,2*SHA_DIGEST_LEN);
			sha1_readstr[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character

			if(PRINT_FLAG)
				cout<< "SHA1 value computed from meta file is "<<sha1_readstr <<endl;
			break;
		}
		i++;

	}

	fclose(fr);
	
} // end get_sha1_from_file

// nonce_readstr is in terms of hexstring format - 2*SHA_DIGEST_LEN
void get_nonce_from_metafile(char *filename, unsigned char nonce_readstr[])
{
	FILE *fr;            /* declare the file pointer */
	char line[256];
	
	if(PRINT_FLAG)
		cout<<"filename is "<< filename<<endl;
	
	fr = fopen (filename, "rb");  /* open the file for reading */
	
	if(!fr)
	   cout<<"could not open file"<<endl;
	
	int i=1;

	while(fgets(line, 256, fr) != NULL)
	{	
		if(i==5)
		{			
			if(PRINT_FLAG)
				printf("Have to Split on token = \"%s\"\n",line);
	
			strtok(line, "=");			
			char *tempstr = (char *)strtok(NULL, "=");			
			memcpy(&nonce_readstr[0], tempstr , 2*SHA_DIGEST_LEN);
			nonce_readstr[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character
			
			if(PRINT_FLAG)
				cout<< "NONCE computed from meta file is "<<nonce_readstr <<endl;
			break;
		}
		i++;

	}

	fclose(fr);
}

// bitvec_readstr is in terms of hexstring format - 2*BIT_VECTOR_LEN
void get_bitvec_from_metafile(char *filename, unsigned char bitvec_readstr[])
{
	FILE *fr;            /* declare the file pointer */
	char line[500];
	
	if(PRINT_FLAG)
		cout<<"filename is "<< filename<<endl;
	
	fr = fopen (filename, "rb");  /* open the file for reading */
	
	if(!fr)
	   cout<<"could not open file"<<endl;
	
	int i=1;

	while(fgets(line, 500, fr) != NULL)
	{	
		if(i==7)
		{
			//unsigned char *tempstr; //[2*SHA_DIGEST_LEN] = "";

			if(PRINT_FLAG)
				printf("Have to Split on token = \"%s\"\n",line);
	
			strtok(line, "=");			
			unsigned char *tempstr = (unsigned  char*) strtok(NULL, "=");
			tempstr[2*BIT_VECTOR_LEN] = '\0';
			
			//cout << "tempstr BITVECTOR is " << tempstr << endl;

			memset(&bitvec_readstr[0], 0 ,2*BIT_VECTOR_LEN);
			memcpy(&bitvec_readstr[0], tempstr ,2*BIT_VECTOR_LEN);
			bitvec_readstr[2*BIT_VECTOR_LEN] = '\0'; // avoid the /n character

			if(PRINT_FLAG)
				cout<< "BITVECTOR computed from meta file is "<<bitvec_readstr << endl;
			break;
		}
		i++;

	}

	fclose(fr);
	
} // end get_bitvec_from_metafile

void get_pwd_from_passfile(int filenum, unsigned char pwdUOID_hex[])
{

	FILE *fr;            /* declare the file pointer */
	char line[256];

	// given the filenum, compute the meta file size and data file size
    char passPath[256] = "";
    sprintf(passPath, "%s/files/%d.pass", iniparserInfo->myhomedir,filenum);


	if(PRINT_FLAG)
		cout<<"passPath is "<< passPath<<endl;
	
	fr = fopen (passPath, "rb");  /* open the file for reading */
	
	if(!fr)
	   cout<<"could not open file"<<endl;
	
	if(fgets(line, 256, fr) != NULL)
	{		
		memcpy(&pwdUOID_hex[0], line , 2*SHA_DIGEST_LEN);
		pwdUOID_hex[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character
		
		if(PRINT_FLAG)
			cout<< "pwdUOID_hex computed from meta file is "<<pwdUOID_hex <<endl;
	}

	fclose(fr);



}

/******************************************* STORE RECV FUNCTIONS ******************************************************************/

// write the meta file in buffer into temporary meta file to read the file-sha1 data
void recvMetadataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *storeMsg, unsigned char sha1_readstr[])
{
	// given the filenum, compute the meta file size and data file size
    char metaPath[256] = "";
    sprintf(metaPath, "%s/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new metaPath %s \n", metaPath);


	FILE *pFile = fopen((const char *)metaPath,"wb");

	// determine whether the specified file exists r not
	// if the file does not exist, GET_FAIL packet should be sent instead with no data
	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in recvMetadataInBuffer !!!! \n");
	}

	else
	{

		// copy the meta data into the buffer:
		unsigned int result = fwrite(&storeMsg[startpos], 1, metaSize, pFile);
		fclose(pFile);

		if(result != metaSize)
		{
			
		}
		else
		{			
			get_sha1_from_metafile(metaPath, sha1_readstr);
			
			if(PRINT_FLAG)
				cout<< "in recvMeta function SHA1 value computed from meta file is "<<sha1_readstr <<endl;

			// removing the temp file
			if( remove((const char *) metaPath) != 0 )
			{
				//perror( "Error deleting file" );
			}
			else
			{
				if(PRINT_FLAG)
					printf("File successfully deleted \n" ); 
			}
			


		}

	}

} // end recvMetadataInBuffer

// not used as data files are created directly from recv function itself
// this function computes the sha1 on the received file content in the receive buffer
void recvFiledataInBuffer(unsigned int filenum, unsigned int startpos, unsigned int dataSize, unsigned char *storeMsg, unsigned char sha1_filestr[])
{		
		unsigned char sha1_file[SHA_DIGEST_LEN];

		SHA_CTX ctx;	
		SHA1_Init(&ctx);

		SHA1_Update(&ctx,(const void *)&storeMsg[startpos],dataSize);	
	
		SHA1_Final(sha1_file, &ctx);
		
		convert_sha1_to_hex( sha1_file, sha1_filestr);

		if(PRINT_FLAG)
			cout<< "SHA1 value computed from data file is "<<sha1_filestr <<endl;

} // end recvFiledataInBuffer


// creating the "meta" file, "data" file, creates pass file, adding into all the indexs - addAllIndexNodes
int create_cache_file(unsigned int filenum, unsigned int m_startpos, unsigned int f_startpos, unsigned int metaSize, unsigned int dataSize, unsigned char *storeMsg)
{

		//will get the home direstory throught iniparserInfo->homedir
	   //taken from http://www.go4expert.com/forums/showthread.php?t=9031
	char pathName[256] = "";
	sprintf(pathName, "%s/files", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("create_cache_file function filenum %d \n", filenum);

	if(mkdir(pathName,0777)==-1)//creating a directory if the directory does not exist
	{
		//printf("unable to create directory\n");
		//cerr<<"Error :"<<strerror(errno)<<endl;

		/*if(PRINT_FLAG)
			printf("errno: %d\n",errno);*/

		if(errno==EEXIST)
		{
			if(PRINT_FLAG)
				printf("directory already exist\n");
		}
		else
		{
		  return ERROR; //exit(1);
		}
	}

	/**************************** CREATE THE CACHE META FILE ************************************/
    char metaPath[256] = "";
    sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new cache metaPath %s \n", metaPath);


	FILE *pFile = fopen((const char *)metaPath,"wb");

	// determine whether the specified file exists r not
	if(pFile==NULL)
	{		
		printf("Invalid File name or insufficient permissions in  create_cache_file!!!! \n");
		return ERROR;
	}

	else
	{
		// copy the meta data into the buffer:
		unsigned int result = fwrite(&storeMsg[m_startpos], 1, metaSize, pFile);
		fclose(pFile);

		if(result != metaSize)
		{
			
		}
		else
		{
			//printf("metadata cached successfully in node's homedir/files \n");

		}

	}

	/**************************** CREATE THE CACHE DATA FILE ************************************/
	char dataPath[256] = "";
    sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new cache dataPath %s \n", dataPath);


	pFile = fopen((const char *)dataPath,"wb");

	// determine whether the specified file exists r not
	if(pFile==NULL)
	{		
		printf("Invalid File name or insufficient permissions in create_cache_file!!!! \n");
		return ERROR;
	}

	else
	{

		// copy the file data into the buffer:
		unsigned int result = fwrite(&storeMsg[f_startpos], 1, dataSize, pFile);
		fclose(pFile);

		if(result != dataSize)
		{
			
		}
		else
		{
			//printf("file data cached successfully in node's homedir \n");
		
		}

	}

	/**************************** ADDING INDEXS ************************************/

	/******* GET THE BITVEC FILE *********/
	unsigned char bitvec_readstr[2*BIT_VECTOR_LEN+1];
	bitvec_readstr[0] = '\0'; 

	get_bitvec_from_metafile(metaPath, bitvec_readstr);
	char *bitvec_string = (char *)bitvec_readstr;
	
	if(PRINT_FLAG)
		printf("bitvec_string to be added from the create_CACHE_file %s \n", bitvec_string);

	/******* GET THE FILE NAME *********/
	char fileName[256];
	get_fname_from_metafile(metaPath, fileName);

	/******* GENERATE SHA1 FOR THE FILE *********/
	unsigned char sha1_file[SHA_DIGEST_LEN];
	unsigned char sha1_filestr[2*SHA_DIGEST_LEN] = "";	
	gen_sha1_file(sha1_file, dataPath);
	convert_sha1_to_hex(sha1_file, sha1_filestr);
	char *sha1_string = (char *)sha1_filestr;
		
	if(PRINT_FLAG)
		printf("sha1_string to be added from the create_CACHE_file %s \n", sha1_string);

	addAllIndexNodes(bitvec_string,  fileName, sha1_string, filenum);
	//displayAllIndexNodes();

	return SUCCESS;

} // end create_cache_file

// 2nd one
// creating the "meta" file, "data" file, creates pass file, adding into all the indexs - addAllIndexNodes
int create_cache_file(unsigned int filenum, unsigned int m_startpos, unsigned int f_startpos, unsigned int metaSize, unsigned int dataSize, unsigned char *storeMsg, char *srcpath)
{

		//will get the home direstory throught iniparserInfo->homedir
	   //taken from http://www.go4expert.com/forums/showthread.php?t=9031
	char pathName[256] = "";
	sprintf(pathName, "%s/files", iniparserInfo->myhomedir);

	if(PRINT_FLAG)
		printf("create_cache_file function filenum %d \n", filenum);

	if(mkdir(pathName,0777)==-1)//creating a directory if the directory does not exist
	{
		//printf("unable to create directory\n");
		//cerr<<"Error :"<<strerror(errno)<<endl;

		/*if(PRINT_FLAG)
			printf("errno: %d\n",errno);*/

		if(errno==EEXIST)
		{
			if(PRINT_FLAG)
				printf("directory already exist\n");
		}
		else
		{
		  return ERROR; //exit(1);
		}
	}

	/**************************** CREATE THE CACHE META FILE ************************************/
    char metaPath[256] = "";
    sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new cache metaPath %s \n", metaPath);


	FILE *pFile = fopen((const char *)metaPath,"wb");

	// determine whether the specified file exists r not
	if(pFile==NULL)
	{		
		printf("Invalid File name or insufficient permissions in  create_cache_file!!!! \n");
		return ERROR;
	}

	else
	{
		// copy the meta data into the buffer:
		unsigned int result = fwrite(&storeMsg[m_startpos], 1, metaSize, pFile);
		fclose(pFile);

		if(result != metaSize)
		{
			
		}
		else
		{
			//printf("metadata cached successfully in node's homedir/files \n");

		}

	}

	/**************************** CREATE THE CACHE DATA FILE ************************************/
	char dataPath[256] = "";
    sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new cache dataPath %s \n", dataPath);


	create_data_file(srcpath, dataPath);

	// removing the temp file
	if( remove((const char *) srcpath) != 0 )
	{
		//perror( "Error deleting file" );
	}	
	else
	{
		if(PRINT_FLAG)
			printf("File successfully deleted \n" ); 
	}
	

	/**************************** ADDING INDEXS ************************************/

	/******* GET THE BITVEC FILE *********/
	unsigned char bitvec_readstr[2*BIT_VECTOR_LEN+1];
	bitvec_readstr[0] = '\0'; 

	get_bitvec_from_metafile(metaPath, bitvec_readstr);
	char *bitvec_string = (char *)bitvec_readstr;
	
	if(PRINT_FLAG)
		printf("bitvec_string to be added from the create_CACHE_file %s \n", bitvec_string);

	/******* GET THE FILE NAME *********/
	char fileName[256];
	get_fname_from_metafile(metaPath, fileName);

	/******* GENERATE SHA1 FOR THE FILE *********/
	unsigned char sha1_file[SHA_DIGEST_LEN];
	unsigned char sha1_filestr[2*SHA_DIGEST_LEN] = "";	
	gen_sha1_file(sha1_file, dataPath);
	convert_sha1_to_hex(sha1_file, sha1_filestr);
	char *sha1_string = (char *)sha1_filestr;
		
	if(PRINT_FLAG)
		printf("sha1_string to be added from the create_CACHE_file %s \n", sha1_string);

	addAllIndexNodes(bitvec_string,  fileName, sha1_string, filenum);
	//displayAllIndexNodes();

	return SUCCESS;

} // end create_cache_file



/******************************************* SEARCH FUNCTIONS ******************************************************************/

// read the meta data in the search response (single record) and display it to the user.
void recvMetadataFromBuffer_search(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *searchMsg)
{
	// given the filenum, compute the meta file size and data file size
    char metaPath[256] = "";
    sprintf(metaPath, "%s/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new metaPath %s \n", metaPath);


	FILE *pFile = fopen((const char *)metaPath,"wb");

	// determine whether the specified file exists r not
	// if the file does not exist, GET_FAIL packet should be sent instead with no data
	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in recvMetadataFromBuffer_search!!!! \n");
	}

	else
	{

		// copy the meta data into the buffer:
		unsigned int result = fwrite(&searchMsg[startpos], 1, metaSize, pFile);
		fclose(pFile);

		if(result != metaSize)
		{
			
		}
		else
		{
			
			// printing the meta data
			/***/
				char line[500];

				pFile =  fopen((const char *)metaPath,"rb");
				
				if(!pFile)
				   cout<<"could not open file"<<endl;
				
				int i=1;

				while(fgets(line, 500, pFile) != NULL)
				{	

					// dun display the first line [METADATA] header and bit vector info to the user
					if(i >=2 && i<=6)
					{				
						printf("\t%s",line);				
					}
					i++;

				}

				fclose(pFile);

			/***/

			// removing the temp file
			if( remove((const char *) metaPath) != 0 )
			{
				//perror( "Error deleting file" );
			}
			else
			{
				if(PRINT_FLAG)
					printf("File successfully deleted \n" ); 
			}


		}		

	}

} // end recvMetadataFromBuffer_search


// USED BY CMDTHREAD SEARCH HANDLER
// read the meta data in the .meta file in minifile directory and display it to the user b4 search response handler thread is created
void printMetadataFromFile_search(unsigned int filenum) 
{
	// given the filenum, compute the meta file size and data file size
    char metaPath[256] = "";
    sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new metaPath %s \n", metaPath);


	FILE *pFile = fopen((const char *)metaPath,"rb");

	// determine whether the specified file exists r not
	// if the file does not exist
	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in printMetadataFromFile_search !!!! \n");
	}

	else
	{		
		// printing the meta data
		char line[500];

		int i=1;

		while(fgets(line, 500, pFile) != NULL)
		{	

			// dun display the first line [METADATA] header and bit vector info to the user
			if(i >=2 && i<=6)
			{				
				printf("\t%s",line);				
			}
			i++;

		}

		fclose(pFile);
	}

} // end printMetadataFromFile_search

/******************************************* GET FUNCTIONS ******************************************************************/

// for get responses... filenum - tmp created in the mini directory for info extraction purpose
void recvMetadataFromBuffer_get(unsigned int filenum, unsigned int startpos, unsigned int metaSize, unsigned char *getMsg,  char fname_readstr[], unsigned int &fileSize, unsigned char sha1_readstr[], unsigned char nonce_readstr[])
{
	// given the filenum, compute the meta file size and data file size
    char metaPath[256] = "";
    sprintf(metaPath, "%s/%d.meta", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new metaPath %s \n", metaPath);


	FILE *pFile = fopen((const char *)metaPath,"wb");

	// determine whether the specified file exists r not
	// if the file does not exist, GET_FAIL packet should be sent instead with no data
	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in recvMetadataFromBuffer_get!!!! \n");
	}

	else
	{

		// copy the meta data into the buffer:
		unsigned int result = fwrite(&getMsg[startpos], 1, metaSize, pFile);
		fclose(pFile);

		if(result != metaSize)
		{
			
		}
		else
		{	
			get_fname_from_metafile(metaPath, fname_readstr);
			get_fsize_from_metafile(metaPath, fileSize);
			get_sha1_from_metafile(metaPath, sha1_readstr);
			get_nonce_from_metafile(metaPath, nonce_readstr);
			
			if(PRINT_FLAG)
			{
				cout<< "in recvMetadataFromBuffer_get function" << endl;
				cout << "fname_readstr computed from meta file is "<<fname_readstr <<endl;
				cout << "fileSize computed from meta file is "<<fileSize <<endl;
				cout << "sha1_readstr computed from meta file is "<<sha1_readstr <<endl;
				cout << "nonce_readstr computed from meta file is "<<nonce_readstr <<endl;
			}

			// removing the temp file
			if( remove((const char *) metaPath) != 0 )
			{
				//perror( "Error deleting file" );
			}
			else
			{
				if(PRINT_FLAG)
					printf("File successfully deleted \n" ); 
			}
			


		} // end if fwrite is successful

	} // end if(pFile==NULL)
 
} // end recvMetadataFromBuffer_get

int determineFileIdentical(char fname_readstr[], unsigned char sha1_readstr[], unsigned char nonce_readstr[], unsigned int &filenum_create)
{

    pthread_mutex_lock(&globalFileNum_mutex);
    unsigned int filenum = globalFileNum;
    pthread_mutex_unlock(&globalFileNum_mutex);
   
    unsigned int count = 1;

    for(count = 1; count <= filenum; count ++)
    {
        char metaP[256] = "";
        sprintf(metaP, "%d.meta",count);

        int exist = computeFileSize('m', metaP);
        if(exist != -1)
        {
            char fname_readstring[256];
            unsigned char sha1_readstring[2*SHA_DIGEST_LEN+1] = "";
            unsigned char nonce_readstring[2*SHA_DIGEST_LEN+1] = "";

            char metaPath[256] = "";
            sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,count);

            get_fname_from_metafile(metaPath, fname_readstring);
            get_sha1_from_metafile(metaPath, sha1_readstring);
            get_nonce_from_metafile(metaPath, nonce_readstring);
           
            if( (strcmp((const char*)fname_readstr, (const char*)fname_readstring) == 0)
                && (strcmp((const char*)sha1_readstr, (const char*)sha1_readstring) == 0)
                && (strcmp((const char*)nonce_readstr, (const char*)nonce_readstring) == 0) )
            {
                filenum_create = count;
                printf("found identical files with fileref %d \n", count );
                return TRUE;
           
            }
        }
   
   
    }

    return FALSE;

} // end determineFileIdentical


/******************************************* DELETE FUNCTIONS ******************************************************************/


int deletePwdhit(unsigned int fileRef[], unsigned int &fileCnt, unsigned char nonce_sha1hex[], unsigned char pwdUOID_hex[], unsigned int &fileFound)
{
	unsigned int count = 1;

	for(count = 0; count < fileCnt; count++)
	{	
		// given the filenum, compute the meta file size and data file size
		char passPath[256] = "";
		sprintf(passPath, "%d.pass", fileRef[count]);

		// 1. get password from the pass file if it exists
		int exist = computeFileSize('m', passPath);
		if(exist != -1)
		{
			memset(&pwdUOID_hex[0], 0, 2*SHA_DIGEST_LEN);
			get_pwd_from_passfile(fileRef[count], pwdUOID_hex);
			
			// 2. convert hex to sha1 - password
			unsigned char pwdUOID[UOID_LEN+1] = "";
			convert_hex_to_sha1(pwdUOID , pwdUOID_hex);
			pwdUOID[UOID_LEN] = '\0';

			// 3. generate nonce for pwdUOID - 20bytes
			unsigned char nonce_sha1[SHA_DIGEST_LEN+1];
			gen_nonce(nonce_sha1, pwdUOID);
			nonce_sha1[SHA_DIGEST_LEN] = '\0';

			//4. convert nonce to hex string - 40bytes
			unsigned char nonce_sha1hex_cmp[2*SHA_DIGEST_LEN+1] =  "";
			convert_sha1_to_hex(nonce_sha1, nonce_sha1hex_cmp);
			nonce_sha1hex_cmp[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character

			if(strcmp((const char*)nonce_sha1hex, (const char*)nonce_sha1hex_cmp) == 0)
			{
				printf("found matching password \n");
				fileFound = fileRef[count];
				return TRUE;
			}

		}
		else
		{
			printf("trying to access invalid pass file \n");
			return FALSE;
		}

	}
	return FALSE;

} // end deletePwdhit

/* format of delete spec
  FileName=foo
  SHA1=63de...
  Nonce=fcca...
  Password=bac9...
*/
int create_delete_filespec(unsigned int filenum, char *filename, unsigned char sha1_hex[], unsigned char nonce_sha1hex[], unsigned char pwdUOID_hex[]) // all hex are 40bytes
{
    char filePath[256] = "";
    sprintf(filePath, "%s/%d.filespec", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new filepath %s \n", filePath);

	ofstream write(filePath);//writing to a file
	if (write.is_open())
	{
		write << "FileName=" << filename << endl;
		write << "SHA1=" << sha1_hex << endl;
		write << "Nonce=" << nonce_sha1hex << endl;
		write << "Password=" << pwdUOID_hex << endl;
	}
	else
	{
		if(PRINT_FLAG)
			cout << "Unable to open file";

		return ERROR;
	}

	write.close();

	return SUCCESS;

} // end create_delete_filespec

// filespecpath refers to the path in the mini home dir 
// reading data 
void formFileSpecInBuffer(unsigned int filenum, unsigned int startpos, unsigned int filespecSize, unsigned char *Msg)
{

	char filespecPath[256] = "";
    sprintf(filespecPath, "%s/%d.filespec", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new filespecPath in formFileSpecInBuffer %s \n", filespecPath);


	FILE *pFile = fopen((const char *)filespecPath,"rb");

	//printf("new filespecPath in formFileSpecInBuffer %s \n", filespecPath);


	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in formFileSpecInBuffer!!!! \n");
	}

	else
	{

		// copy the filespec data into the buffer:
		unsigned int result = fread(&Msg[startpos], 1, filespecSize, pFile);
		fclose(pFile);
				
		if(result != filespecSize)
		{
			printf("result != fileSpecSize \n");

		}
		else
		{			

			/*** to be removed .. testing purpose only ****/
			/*FILE * pFile1;
			
			pFile1 = fopen ( "myfile.data" , "wb" );
			fwrite (&Msg[startpos] , 1 , filespecSize , pFile1 );
			fclose (pFile1);
			*/
			
			
			// removing the temp file
			if( remove((const char *) filespecPath) != 0 )
			{
				//perror( "Error deleting file" );
			}
			else
			{
				if(PRINT_FLAG)
					printf("File successfully deleted \n" ); 
			}						

		}

	}

} // end formFileSpecInBuffer

// creating a temp file
void recvFileSpecFromBuffer(unsigned int filenum, unsigned int startpos, unsigned int filespecSize, unsigned char *Msg, unsigned char sha1_hex[], unsigned char pwdUOID_hex[])
{	
	char filespecPath[256] = "";
    sprintf(filespecPath, "%s/%d.filespec", iniparserInfo->myhomedir,filenum);

	if(PRINT_FLAG)
		printf("new filespecPath %s \n", filespecPath);


	FILE *pFile = fopen((const char *)filespecPath,"wb");

	if(pFile==NULL)
	{
		
		printf("Invalid File name or insufficient permissions in recvFileSpecFromBuffer !!!! \n");
	}

	else
	{
		// copy the filespec data into the buffer:
		unsigned int result = fwrite(&Msg[startpos], 1, filespecSize, pFile);
		fclose(pFile);

		if(result != filespecSize)
		{
			
		}
		else
		{		
			
			pFile = fopen (filespecPath, "rb");  /* open the file for reading */
			
			if(!pFile)
			{
			   cout<<"could not open file"<<endl;
			   return;
			}
	
			char line[500];
			
			int i = 1;
			while(fgets(line, 256, pFile) != NULL)
			{					
				if(i==2)
				{
					if(PRINT_FLAG)
						printf("Have to Split on token = \"%s\"\n",line);
			
					strtok(line, "=");			
					unsigned char *tempstr = (unsigned  char*) strtok(NULL, "=");
					tempstr[2*SHA_DIGEST_LEN] = '\0';

					memcpy(&sha1_hex[0], tempstr ,2*SHA_DIGEST_LEN);
					sha1_hex[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character

					if(PRINT_FLAG)
						cout<< "SHA1 value computed from file spec is "<<sha1_hex <<endl;
					
				}

				if(i==4)
				{
					if(PRINT_FLAG)
						printf("Have to Split on token = \"%s\"\n",line);
			
					strtok(line, "=");			
					unsigned char *tempstr = (unsigned  char*) strtok(NULL, "=");
					tempstr[2*SHA_DIGEST_LEN] = '\0';

					memcpy(&pwdUOID_hex[0], tempstr ,2*SHA_DIGEST_LEN);
					pwdUOID_hex[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character

					if(PRINT_FLAG)
						cout<< "pwdUOID_hex computed from file spec is "<<pwdUOID_hex <<endl;					
				}

				i++;

			}
			
			fclose(pFile);

			// removing the temp file
			if( remove((const char *) filespecPath) != 0 )
			{
				//perror( "Error deleting file" );
			}
			else
			{
				if(PRINT_FLAG)
					printf("File successfully deleted \n" ); 
			}
			
		}

	}

} // recvFileSpecFromBuffer

/* pwdUOID_hex received in the delete message. compute SHA1 for the provided pwd in hexstring 
	and check whether there is a match with stored file References found for the received SHA1
*/
int deleteNoncehit(unsigned int fileRef[], unsigned int &fileCnt, unsigned char pwdUOID_hex[], unsigned int &fileFound)
{
	unsigned int count = 1;

	printf("received pwdUOID_hex in deleteNoncehit function %s \n", pwdUOID_hex);
/***/	
	// 1. convert hex to sha1 - password
	unsigned char pwdUOID[UOID_LEN+1] = "";
	convert_hex_to_sha1(pwdUOID , pwdUOID_hex);
	pwdUOID[UOID_LEN] = '\0';

	// 2. generate nonce for pwdUOID - 20bytes
	unsigned char nonce_sha1[SHA_DIGEST_LEN+1];
	gen_nonce(nonce_sha1, pwdUOID);
	nonce_sha1[SHA_DIGEST_LEN] = '\0';

	//3. convert nonce to hex string - 40bytes
	unsigned char nonce_sha1hex_cmp[2*SHA_DIGEST_LEN+1] =  "";
	convert_sha1_to_hex(nonce_sha1, nonce_sha1hex_cmp);
	nonce_sha1hex_cmp[2*SHA_DIGEST_LEN] = '\0'; // avoid the /n character

	for(count = 0; count < fileCnt; count++)
	{	
		unsigned char nonce_sha1hex[2*SHA_DIGEST_LEN+1] =  "";

		// given the filenum, compute the meta file size and data file size
		char metaPath[256] = "";
		sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,fileRef[count]);

		// 1. get corresponding nonce for the fileRef[count]
		get_nonce_from_metafile(metaPath, nonce_sha1hex);

		if(strcmp((const char*)nonce_sha1hex, (const char*)nonce_sha1hex_cmp) == 0)
		{
			fileFound = fileRef[count];
			printf("found matching delete file %d in the node dir \n", fileFound);
			return TRUE;
		}

	}

	return FALSE;

} // end deleteNoncehit

void deleteFiles(int num, unsigned int fileRef)
{
	
	if(num >= 2)
	{
		// given the filenum, compute the meta path
		char metaPath[256] = "";
		sprintf(metaPath, "%s/files/%d.meta", iniparserInfo->myhomedir,fileRef);

		// removing the meta file
		if( remove((const char *) metaPath) != 0 )
		{
			//perror( "Error deleting file" );
		}
		else
		{
			if(PRINT_FLAG)
				printf("Meta File %s successfully deleted \n", metaPath); 
		}		

		// given the filenum, compute the data path
		char dataPath[256] = "";
		sprintf(dataPath, "%s/files/%d.data", iniparserInfo->myhomedir,fileRef);

		// removing the data file
		if( remove((const char *) dataPath) != 0 )
		{
			//perror( "Error deleting file" );
		}
		else
		{
			if(PRINT_FLAG)
				printf("data File %s successfully deleted \n", dataPath); 
		}	
	}

	if(num == 3)
	{
		// given the filenum, compute the pass path
		char passPath[256] = "";
		sprintf(passPath, "%s/files/%d.pass", iniparserInfo->myhomedir,fileRef);
		if(ext_file_exist(passPath) == TRUE)
		{
			// removing the data file
			if( remove((const char *) passPath) != 0 )
			{
				//perror( "Error deleting file" );
			}
			else
			{
				if(PRINT_FLAG)
					printf("pass File %s successfully deleted \n", passPath); 
			}	
		}
	}

} // end deleteFiles

void deleteAllFiles()
{
	gfileCnt_read();

	pthread_mutex_lock(&globalFileNum_mutex);
	unsigned int filenum = globalFileNum;
	pthread_mutex_unlock(&globalFileNum_mutex);
	
	//printf("inside deleteAllFiles function and current filenum is %d \n", filenum);

	unsigned int count = 1;

	for(count = 1; count <= filenum; count++)
	{
		// given the filenum, compute the meta file size and data file size
		char metaPath[256] = "";
		sprintf(metaPath, "%d.meta", count);

		int exist = computeFileSize('m', metaPath);

		printf("exist value %d and fileRef is %d \n", exist, count );

		// if the file exists then delete the file based on whether its present in cache or permanent space
		if(exist != -1)
		{
			// check whether the requested file exists in the cache area, if it is, then remove it from the LRU list
			if(findFileInLRUList(count)==TRUE)
			{	
				// 1. delete cache entry
				deleteNodeInLRUList(count);
				
				// 2. delete from the indexs
				deleteAllIndexNodes(count);			

				// 3. delete the actual files in the mini dir - meta/data
				deleteFiles(2, count);				
			}
			else
			{					
				// 2. delete from the indexs
				deleteAllIndexNodes(count);

				// 3. delete the actual files in the mini dir - meta/data
				deleteFiles(3, count);					
			
			}
		

		}

	}

} // end deleteAllFiles()

