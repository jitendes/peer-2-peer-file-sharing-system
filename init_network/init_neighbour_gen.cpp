//#include "init_neighbour_gen.h"
#include "iniparserClass.h"


// whenever you receive a reply just create a stuct obj of type Node and add it in list of nodes
extern iniparserClass *iniparserInfo;

int compare_distance(Node& n1, Node& n2) 
{
		//printf("inside compare distance\n");
	if(n1.distance < n2.distance) {
		return TRUE;
	     }
	else {
		return FALSE;
	     }
	     return TRUE;
}

nbrList::nbrList()
{

}
Node::Node(string hostnamei, uint16_t porti, uint32_t distancei) 
{  
	//printf("inside node constructor\n");
	host_name = hostnamei;
	port = porti;
	distance = distancei;
}

void nbrList::sdistance() 
{
	//printf("inside sort distance\n");

	nodes.sort(compare_distance);
}
int nbrList::addNode(Node node) 
{
	//printf("adding node to the list\n");
	nodes.push_back(node);
	return 0;
}
void nbrList::delNodes()  /// 
{
	//nodes to delete we get from ini file 
	//printf("inside delete node\n");
	int p=0;
	unsigned int size = nodes.size();
	Node node("", 0, 0);
	sdistance();
	  for(unsigned int k = 0; k < size; k++) {
	  if(k < iniparserInfo->initNeighbors) {
			node = nodes.front();
			nodes.push_back(node);
			nodes.pop_front();
		}
		else {
			//nodes that are to to be removed
			nodes.pop_front();
			p++;
			//printf("count is %d\n",p);
		}
	}
	
}




void read_init_file(int &count)
{
    //char myline[256];
    //int LINE_MAX;
	count = 0;
    char line[100];
    char fileName[256] = "";
    sprintf(fileName, "%s/%s", iniparserInfo->myhomedir, "init_neighbor_list");
	
	/*if(PRINT_FLAG)
		printf("fileName %s \n", fileName); */

    FILE *fp;
    fp=fopen(fileName, "r");
	
	/*if(PRINT_FLAG)
		printf("count %d \n", count); */

    //char *s;
	if(fp)
	{
		while(fgets(line, 500, fp)!= NULL )
		{
			//printf("inside the init while loop \n");
			/*if(PRINT_FLAG)
				printf("line %s \n", line); */
			char *key = strtok (line,":");  //stores hostname
			
			if(iniparserInfo->bhostname[count] == NULL)
				iniparserInfo->bhostname[count] = (char *)malloc(strlen(key)); 

			memcpy(iniparserInfo->bhostname[count], key, strlen(key));

			unsigned short int value = atoi(strtok (NULL, ":"));  //stores portno
		
			iniparserInfo->bports[count] = value;

			/*while (key != NULL)
			 {
			 printf ("key :%s",key);
			 *val = atoi(strtok (NULL, ":"));  //stores port
			 printf ("val : %d",*val);
			 }*/   
			 count++;
		}
		
		/*if(PRINT_FLAG)
			printf("file has come to end \n"); */

		 fclose(fp);  
	}

   
	
}

void nbrList::write_init_file() 
{
	//printf("inside write ini file\n");
	Node node("", 0, 0);
	unsigned int g=0;
	list<Node>::iterator itr;
	char fileName[80] = "";	
	sprintf(fileName, "%s/%s", iniparserInfo->myhomedir, "init_neighbor_list");

	FILE* fp = fopen(fileName, "w");
	if(!fp) {
		printf("Error opening init_nbrList.\n");
		exit(1);
	}
	for( itr = nodes.begin(); itr != nodes.end(); itr++) {

		if( (g+1) == nodes.size())
			fprintf(fp, "%s:%d", itr->host_name.c_str(), itr->port);
		else
			fprintf(fp, "%s:%d\n", itr->host_name.c_str(), itr->port);

		g++;
		//printf("count is %d\n",g);
	}

	fclose(fp);

}


int del_init_neighbour_file()
{
    char fileName[100];
    sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"init_neighbor_list");
    int q=remove(fileName);
    return q;
}

int init_file_exist()
{
	//printf("messagetypeis %c\n",msgtype);

	char fileName[100];
    sprintf(fileName, "%s/%s", iniparserInfo->myhomedir,"init_neighbor_list");
	//printf("init_file exist is %s\n",fileName);
    FILE  *fp = fopen(fileName, "r");	

	if(!fp) {
		//printf("Error logfile does not exist\n");
		return FALSE;
	}
	return TRUE;
}
