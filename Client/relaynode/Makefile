include ../common.inc

PROJECT_TARGET := relaynode
PROJECT_SRCS := $(notdir $(wildcard *.cpp))
PROJECT_OBJS := $(patsubst %.cpp, %.o, $(PROJECT_SRCS))


INCLUDES := $(BOOST_INCLUDE) -I../rtsppuller -I../p2plib -I../common -I../stuncore -I../networkutils -I/usr/local/jsoncpp/include 
LIB_PATH := -L../p2plib -L../common -L../stuncore -L../networkutils -L/usr/local/jsoncpp/lib -L../rtsppuller
LIBS := -lp2plib -lnetworkutils -lRTSPPuller 


all: $(PROJECT_TARGET)

clean:
	rm -f $(PROJECT_OBJS) $(PROJECT_TARGET) 

$(PROJECT_TARGET): $(PROJECT_OBJS)
	$(LINK.cpp) -o $@ $^ $(LIB_PATH) $(LIBS) $(SOCKET_LIBS)


