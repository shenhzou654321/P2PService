include ../common.inc

PROJECT_TARGET := p2plibtest 
PROJECT_OBJS := p2plibtest.o
	

#INCLUDES := $(BOOST_INCLUDE) -I../common -I../stuncore -I../networkutils -I../p2plib -I/usr/local/jsoncpp/include
#LIB_PATH := -L../common -L../stuncore -L../networkutils -L../p2plib -L/usr/local/jsoncpp/lib
#LIBS := -lnetworkutils -lstuncore -lcommon -lcrypto -lp2plib -ljsoncpp


INCLUDES := -I../p2plib
LIB_PATH := -L../p2plib 
LIBS := -lp2plib


all: $(PROJECT_TARGET)

clean:
	rm -f $(PROJECT_OBJS) $(PROJECT_TARGET) 

$(PROJECT_TARGET): $(PROJECT_OBJS)
	$(LINK.cpp) -o $@ $^ $(LIB_PATH) $(LIBS) $(SOCKET_LIBS)


