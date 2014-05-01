# include "global.h"
#include "iniparserClass.h"

using namespace std;

iniparserClass *iniparserInfo;

void Initialize();
void Process();
void CleanUp();

// parameters read from the commandline argumetns
char startup_iniName[100];
int resetOption; // //FALSE - no reset, TRUE - reset reqeuest received from the user

void ParseCommandLine(int argc, char *argv[]);
void Usage();

// define the commandline syntax for the server
void Usage()
{
	/* print out usage information, i.e. commandline syntax */
	//printf("usage: sv_node [-reset] startup.ini \n");
	exit(1);
}
        
// todo remove printfs
void ParseCommandLine(int argc, char *argv[])
{

	//printf("sv_node: Size of the arguments: %d \n", argc);

    if ( argc < 2 || argc > 3) 
    {
        Usage();
    }

	// no optional arguments are specified
	// get the startup configuration file name
	else if(argc == 2)
	{				
		strncpy(startup_iniName, argv[1], strlen(argv[1]));
		//printf("startup_iniName name is %s \n", startup_iniName);
	}

	// if optional arguments are specified, read the reset option
	else if(argc > 2 && argc <= 3)
	{			
		if(*argv[1]=='-' && *(argv[1]+1)=='r')
		{				
				if(strcmp((argv[1]+1), "reset") == 0) // -reset option
				{					
					resetOption	= TRUE;
				}

				//printf("-reset option specified!!! \n");

		} // end if(*argv[1]=='-' && *(argv[1]+1)=='r')
		
		strncpy(startup_iniName, argv[2], strlen(argv[2]));
		//printf("startup_iniName name is %s \n", startup_iniName);

	} //end else if(argc > 2 && argc <= 3)

} // end ParseCommandLine function


void Init(char *ininame)
{
	iniparserInfo = new iniparserClass();

	iniparserInfo->parseStartupConfigFile(ininame);

	//iniparserInfo->printConfigFile();

	iniparserInfo->checkNodeType();
}

int main(int argc, char *argv[])
{
	resetOption = FALSE;

	srand48(time(NULL));

    if(PRINT_FLAG)
		printf("main sv_node starts\n");

	ParseCommandLine(argc, argv);

	Init(startup_iniName);
	
	Initialize();
	Process();


	if(PRINT_FLAG)
		printf("reached main thread after the process exited\n");

	//Externalize();
	CleanUp();

	if(PRINT_FLAG)
		printf("main thread is exiting \n");

	//gnShutdown = FALSE;
		
	//while (!gnShutdown) {
       //pass the name of ini file to init function
	//Process();
	//CleanUp();
	//}

   return SUCCESS;


}


