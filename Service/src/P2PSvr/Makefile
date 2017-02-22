objects = main.o P2PSvr.o TCPHandler.o ServerHandler.o BCSMsgSender.o BCSClientProxy.o \
	ClientHandler.o MsgHandler.o NSClientProxy.o NSClientRepo.o Device.o TopologyInfo.o \
	Units.o TraversalSession.o TraversalStatus.o SvrConf.o
LIB_ACE = -L/usr/local/ACE/lib
INC_ACE = -I/usr/local/ACE/include

INCLUDE = -I../../include -I../Common $(INC_ACE)
LIBPATH = -L../../lib $(LIB_ACE)

CC = g++ -g

P2PSvr : $(objects)
	$(CC) -o P2PSvr $(objects) $(LIBPATH) -lACE -lprotocol -ljsoncpp

Units.o		: ../Common/Units.cpp ../Common/Units.h
	$(CC) -c ../Common/Units.cpp  $(INCLUDE)

SvrConf.o	: SvrConf.cpp SvrConf.h
	$(CC) -c SvrConf.cpp $(INCLUDE)

TraversalSession.o	: TraversalSession.cpp TraversalSession.h
	$(CC) -c TraversalSession.cpp $(INCLUDE)

TraversalStatus.o	: TraversalStatus.cpp TraversalStatus.h
	$(CC) -c TraversalStatus.cpp $(INCLUDE)

TopologyInfo.o 	: TopologyInfo.cpp TopologyInfo.h
	$(CC) -c TopologyInfo.cpp $(INCLUDE)

Device.o 	: Device.cpp Device.h 
	$(CC) -c Device.cpp $(INCLUDE)

NSClientRepo.o	: NSClientRepo.cpp ClientProxy.h
	$(CC) -c NSClientRepo.cpp $(INCLUDE)

NSClientProxy.o	: NSClientProxy.cpp ClientProxy.h
	$(CC) -c NSClientProxy.cpp $(INCLUDE)

MsgHandler.o	: MsgHandler.cpp MsgHandler.h
	$(CC) -c MsgHandler.cpp $(INCLUDE)

ClientHandler.o	: ClientHandler.cpp ClientHandler.h
	$(CC) -c ClientHandler.cpp $(INCLUDE)

BCSMsgSender.o	: BCSMsgSender.cpp ClientHandler.h
	$(CC) -c BCSMsgSender.cpp $(INCLUDE)

ClientProxy.o	: ClientProxy.cpp ClientProxy.h
	$(CC) -c ClientProxy.cpp $(INCLUDE)

BCSClientProxy.o: BCSClientProxy.cpp ClientProxy.h ClientHandler.h
	$(CC)  -c BCSClientProxy.cpp $(INCLUDE)

ServerHandler.o	: ServerHandler.cpp TCPHandler.h
	$(CC)  -c ServerHandler.cpp $(INCLUDE)

TCPHandler.o	: TCPHandler.cpp TCPHandler.h 
	$(CC)  -c TCPHandler.cpp $(INCLUDE)

P2PSvr.o	: P2PSvr.cpp P2PSvr.h TCPHandler.h
	$(CC) -c P2PSvr.cpp $(INCLUDE)

main.o	: main.cpp
	$(CC)  -c main.cpp $(INCLUDE)

.PHONY	: clean
clean :
	rm -rf P2PSvr $(objects)
