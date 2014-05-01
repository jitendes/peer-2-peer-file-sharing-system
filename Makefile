all: sv_node 

CFLAGS= -g -Wall -I/home/scf-22/csci551b/openssl/include 
LFLAGS = -lcrypto -lresolv -lnsl -lsocket -lm -lcrypto -L/home/scf-22/csci551b/openssl/lib

dictionary.o: dictionary.cpp dictionary.h
	g++ -c dictionary.cpp $(CFLAGS)

iniparser.o: iniparser.cpp iniparser.h
	g++ -c iniparser.cpp $(CFLAGS)

svnode.o: svnode.cpp global.h
	g++ -c svnode.cpp $(CFLAGS)

strlib.o: strlib.cpp strlib.h
	g++ -c strlib.cpp $(CFLAGS)

init.o: init.cpp global.h
	g++ -c init.cpp $(CFLAGS)

process.o: process.cpp global.h
	g++ -c process.cpp $(CFLAGS)  

Cleanup.o: Cleanup.cpp global.h
	g++ -c Cleanup.cpp $(CFLAGS)

eventDispatcher.o: eventDispatcher.cpp 
	g++ -c eventDispatcher.cpp $(CFLAGS)

iniparserClass.o: iniparserClass.cpp iniparserClass.h
	g++ -c iniparserClass.cpp $(CFLAGS)

Event.o: Event.cpp 
	g++ -c Event.cpp $(CFLAGS)

Message.o: Message.cpp iniparserClass.h
	g++ -c Message.cpp $(CFLAGS)

neighbourConn.o: neighbourConn.cpp
	g++ -c neighbourConn.cpp $(CFLAGS)

init_neighbour_gen.o: init_neighbour_gen.cpp
	g++ -c init_neighbour_gen.cpp $(CFLAGS)

NeighbourNode.o: NeighbourNode.cpp
	g++ -c NeighbourNode.cpp $(CFLAGS)

RoutingNode.o: RoutingNode.cpp
	g++ -c RoutingNode.cpp $(CFLAGS)

log.o: log.cpp iniparserClass.h
	g++ -c log.cpp $(CFLAGS)

cmdThread.o: cmdThread.cpp iniparserClass.h
	g++ -c cmdThread.cpp $(CFLAGS)

store.o: store.cpp iniparserClass.h
	g++ -c store.cpp $(CFLAGS)

externalize.o: externalize.cpp iniparserClass.h
	g++ -c externalize.cpp $(CFLAGS)

bitvec.o: bitvec.cpp iniparserClass.h
	g++ -c bitvec.cpp $(CFLAGS)

index.o: index.cpp iniparserClass.h
	g++ -c index.cpp $(CFLAGS)

lru.o: lru.cpp iniparserClass.h
	g++ -c lru.cpp $(CFLAGS)

sv_node: svnode.o iniparser.o dictionary.o strlib.o init.o process.o Cleanup.o eventDispatcher.o iniparserClass.o Event.o Message.o neighbourConn.o init_neighbour_gen.o NeighbourNode.o log.o RoutingNode.o cmdThread.o store.o externalize.o bitvec.o index.o lru.o
	g++ -o sv_node svnode.o strlib.o iniparser.o dictionary.o init.o process.o Cleanup.o eventDispatcher.o iniparserClass.o Event.o Message.o neighbourConn.o log.o init_neighbour_gen.o NeighbourNode.o RoutingNode.o cmdThread.o store.o externalize.o bitvec.o index.o lru.o $(LFLAGS) -g  

clean:
	rm *.o  
