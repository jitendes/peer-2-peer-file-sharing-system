
#include "iniparserClass.h"

using namespace std;
/************************************************** DEFINE iniparserClass CLASS **********************************************************/

/**************************** DEFINE FUNCTION MEMBERS ************************************/

/* CONSTRUCTOR */
iniparserClass::iniparserClass()
{
	// The following keys are defined for the [init] section of a start-up configuration file.
	// Optional Fields (if these keys are not specified, the default values should be assumed):

	// Name of a log file. This file is assumed to be under the node's home directory. You should write error messages into this log file instead of printing them to stdout or stderr. The default value is "servant.log".
	logFilename = (char *) "servant.log";

	//The number of seconds for the node to auto-shutdown after it starts.
	autoShutDown=900;

	// The TTL value to be used for all outgoing messages. The default value is 30.		
	msgTTL=30; 

	/*The lifetime (in seconds) of a message. Because of the way routing is done in the SERVANT network, the UOID of all messages must be saved in memory so that duplicate messages can be discarded. 
	It is unreasonable to keep these UIOD's forever. Therefore, we assume that a message cannot stay in the network longer than the value specified here. 
	So, if an UOID has been saved in memory longer than the time specified by this value, the UOID can be discarded. The default value is 30. */
	msgLifetime=30;

	/* The lifetime (in seconds) of a get message. Since file transfer may take a very long time, the UOID of a get message needs to 
	cached in memory longer than other message types. The default value is 300. */
	getMsgLifetime=300;

	/* The number of neighbors to establish when a node first joins the network. This value should be used to pick the number of neighbors during the join process. 
	The default value is 3. (A beacon node should ignore this value.) */
	initNeighbors=3;

	/* The maximum amount to wait (in seconds) for the join response command. 
	If this timeout has occurred and if the joining node has not heard from enough nodes (specified by the InitNeighbors key), it must shut itself down. 
	This timeout is also used to timeout a check message. The default value is 15. (A beacon node should ignore this value.)*/
	joinTimeout=15;

	/* If a node has not seen any message from a neighbor after this timeout (in seconds) has occurred, 
	it should close its connection to this neighbor. The default value is 60. */
	keepAliveTimeout=60;

	/* The minimum number of neighbors a node needs to establish connections with when it comes up and when it's participating in the network. 
	If it cannot connect to this many neighbors, it must delete the init_neighbor_list file and rejoin the network (by contacting a beacon node). 
	The default value is 2. (A beacon node should ignore this value.) */
	minNeighbors=2;

	/*  If this value is 0, sending of check messages is enabled when a non-beacon node loses connection with one of its neighbors. 
	If this value is 1, sending of cehck messgaes should be disabled for a non-beacon node. For a beacon node, it must ignore all check messages. 
	The default value is 0. Please note that for grading of part (2), this value will always be 1 for all nodes. */
	noCheck=0; 

	/* When a node forwards a get response message from a neighbor, it flips a coin (with this probability of getting a positive outcome) 
	to decide if it should cache a copy of it. If the result is positive, a copy of the file is stored. The default value is 0.1. */
	cacheProb=0.1;

	/* When a node receives a store request from a neighbor, it flips a coin (with this probability of getting a positive outcome) 
	to decide if it should cache a copy of it. If the result is positive, a copy of the file is stored. The node where this store request was originated ignores this value. The default value is 0.1. */
	storeProb=0.1;

	/* When a node originates or receives a store request, it asks its neighbors to cache a copy of the file. 
	For each neighbor, it flips a coin (with this probability of getting a positive outcome) to decide if it should forward the store request to the corresponding neighbor. The default value is 0.2. */
	neighborStoreProb=0.2;

	/*  An integer value specifying the maximum total storage (in kilobytes) for cached files. The default value is 500. A kilobyte is 1,024 bytes. Please only count file sizes for the .data files. */
	cacheSize=500;


	// following variables belongs to [beacons] section
	beaconRetry=15; 
	noOfBeacons = 0;
	myNodeType = NON_BEACON;
	
	// node-specific attributes
	int rc = gethostname(myhostname,50);
	if(rc != SUCCESS)
	{
		 //printf (" Error in obtaining current node's gethostname() %d\n",rc);
		 exit(0);
	}
	

}

/* DESTRUCTOR */
iniparserClass::~iniparserClass()
{
	delete(this);
}

// todo parse all the parameters in the startup configuration file
/* parses startupconfig file belonging to the node named ininame parameter */
int iniparserClass::parseStartupConfigFile(char* ininame)
{	
	int sec;

	//printf("ininame: %s \n", ininame);
	
	// parse the startup configuration file
	// extract different fields in the configfile and store it in corresponding attributes
	dictionary * dict = iniparser_load(ininame, &bhostname[0], &bports[0], &noOfBeacons);

	/* ***** EXTRACT [init] SECTION *****  */
	myport = iniparser_getint(dict, (char *) "init:port", -1);
	myinitLocation = iniparser_getint(dict, (char *) "init:location", -1);

	// get homedir
	int tempstrlen = strlen(iniparser_getstring(dict, (char *) "init:homedir", (char *) ""));
	myhomedir = (char*)malloc(tempstrlen+1);
	strncpy(myhomedir, iniparser_getstring(dict, (char *) "init:homedir", (char *) ""), tempstrlen);
	myhomedir[tempstrlen] = '\0';
	//printf("%s \n", myhomedir);
	//printf("%s \n", iniparser_getstring(dict, (char *) "init:homedir", (char *) ""));	

	// get logfilename
	tempstrlen = strlen(iniparser_getstring(dict, (char *) "init:logfilename", (char *) "servant.log"));
	logFilename = (char*)malloc(tempstrlen+1);
	strncpy(logFilename, iniparser_getstring(dict, (char *) "init:logfilename", (char *) "servant.log"), tempstrlen);
	logFilename[tempstrlen] = '\0';
	//printf("%s \n", logFilename);
		
	autoShutDown = iniparser_getint(dict, (char *) "init:autoshutdown", 900);
	msgTTL = iniparser_getint(dict, (char *) "init:msgttl", 30);
	msgLifetime = iniparser_getint(dict, (char *) "init:msglifetime", 30);
	getMsgLifetime = iniparser_getint(dict, (char *) "init:getmsglifetime", 300);
	initNeighbors = iniparser_getint(dict, (char *) "init:initneighbors", 3);
	joinTimeout = iniparser_getint(dict, (char *) "init:jointimeout", 15);
	keepAliveTimeout = iniparser_getint(dict, (char *) "init:keepalivetimeout", 60);
	minNeighbors = iniparser_getint(dict, (char *) "init:minNeighbors", 2);
	noCheck = iniparser_getint(dict, (char *) "init:nocheck", 0);
	cacheProb = iniparser_getdouble(dict, (char *) "init:cacheprob", 0.1);
	storeProb = iniparser_getdouble(dict, (char *) "init:storeprob", 0.1);
	neighborStoreProb = iniparser_getdouble(dict, (char *) "init:neighborstoreprob", 0.2);
	cacheSize = iniparser_getint(dict, (char *) "init:cachesize", 500);
	
	/* ***** EXTRACT [beacons] SECTION *****  */
	beaconRetry = iniparser_getint(dict, (char *) "beacons:retry", 15);
	//printf("beaconRetry %d \n", beaconRetry);
	//printf("noOfbeacons %d \n", noOfBeacons);

	/***** printing sections ******/
	sec = iniparser_getnsec(dict);
	//printf("no of secs in ini file: %d \n", sec);

	/*	
	for(int count = 0; count < sec; count++)
	{
	
		char * secname = iniparser_getsecname(dict, count);
		printf("sec %d - %s \n", count, secname);
	}
	*/

	//printf("MY HOSTNAME IS %s @port %d\n", myhostname, myport);
	// free up the dict
	iniparser_freedict(dict);
	return 0;

}

/* checks the nodetype of the current node. returns BEACON if beacon node or NON_BEACON otherwise */
enum NODE_TYPE iniparserClass::checkNodeType()
{
	unsigned int cnt =0;

	// check if myport exist in any of the bports list
	for(cnt=0; cnt<noOfBeacons; cnt++)
	{
		//printf("port %d %d\n", bports[cnt],myport);
		if(myport==bports[cnt])		
		{
			//printf("Hey am a beacon node !!! \n");
			myNodeType = BEACON;
			return BEACON;
		}
	}

	//printf("Hey am not a beacon node !!!!! \n");

	return NON_BEACON;

}

void iniparserClass::printConfigFile()
{
	printf("*********************************************************\n");
	printf("STARTUP CONFIGURATION CONTENT: \n");

	printf("myport %d\n", myport);
	printf("myinitLocation %d\n", myinitLocation);
	printf("myhomedir %s\n", myhomedir);
	printf("logFilename %s\n", logFilename);
	printf("autoShutDown %d \n", autoShutDown);
	printf("msgTTL %d \n", msgTTL);
	printf("msgLifetime %d\n", msgLifetime);
	printf("getMsgLifetime %d\n", getMsgLifetime);
	printf("initNeighbors %d\n", initNeighbors);
	printf("joinTimeout %d\n", joinTimeout);
	printf("keepAliveTimeout %d\n", keepAliveTimeout);
	printf("minNeighbors %d\n", minNeighbors);
	printf("noCheck %d \n", noCheck);
	printf("cacheProb %f \n", cacheProb);
	printf("storeProb %f \n", storeProb);
	printf("neighborStoreProb %f \n", neighborStoreProb);
	printf("cacheSize %d \n", cacheSize) ;
	printf("retry %d \n", beaconRetry);

	printf("*********************************************************\n");
}

