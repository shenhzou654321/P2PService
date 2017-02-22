include ../common.inc

PROJECT_TARGET := libp2plib.so
PROJECT_TARGETA := libp2plib.a
PROJECT_SRCS := $(notdir $(wildcard *.cpp))
PROJECT_OBJS := $(patsubst %.cpp, %.o, $(PROJECT_SRCS))


INCLUDES := $(BOOST_INCLUDE) -I../common -I../stuncore -I../networkutils -I/usr/local/jsoncpp/include
LIB_PATH := -L../common -L../stuncore -L../networkutils -L/usr/local/jsoncpp/lib
LIBS := -lnetworkutils -lstuncore -lcommon -lpthread -lcrypto -ljsoncpp 

all: $(PROJECT_TARGETA) $(PROJECT_TARGET)

clean:
	rm -f $(PROJECT_OBJS) $(PROJECT_TARGET) 

$(PROJECT_TARGET): $(PROJECT_OBJS)
	$(LINK.cpp) -shared -o $@ $^ $(LIB_PATH) $(LIBS) $(SOCKET_LIBS)

$(PROJECT_TARGETA) : $(PROJECT_OBJS)
	$(AR) rv $@ $^



