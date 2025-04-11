TGT=PUSimulator 
PLATFORM=x86

ifeq ($(OS), t31)
PLATFORM=t31
CROSS_COMPILE=mips-linux-gnu-
endif

ifeq ($(OS), t31u)
PLATFORM=t31u
CROSS_COMPILE=mips-linux-uclibc-gnu-
CFLAGS+=-muclibc -Wno-unused-local-typedefs 
endif

ifeq ($(OS), ja310)
PLATFORM=ja310
CROSS_COMPILE=aarch64-linux-gnu-
endif

ifeq ($(OS), x86)
PLATFORM=x86
endif


CC	      = $(CROSS_COMPILE)gcc
CXX	      = $(CROSS_COMPILE)g++
STRIP         = $(CROSS_COMPILE)strip


CFLAGS	     += -Wall -std=c++11 
INCLUDE      += -I./$(PLATFORM)/include
INCLUDE      += -I../src
INCLUDE      += -I../src/auth
INCLUDE      += -I../src/base

LDFLAGS		 += -L./$(PLATFORM)/lib
LIBS         += -lm -lpthread -lstdc++ -ldl -lprotobuf-lite -lbvrtc -lBVCSP

CXXSRC       +=  ../src/auth/bv_auth.cpp
CXXSRC       +=  ../src/base/dialog.cpp  ../src/base/filetransfer.cpp    ../src/base/session.cpp
CXXSRC       +=  ../src/auth.cpp  ../src/config.cpp  ../src/file.cpp  ../src/gps.cpp   ../src/media.cpp
CXXSRC       +=  ../src/pusession.cpp  ../src/tsp.cpp  ../src/utils.cpp  ../src/main.cpp
 
OBJS          = $(CXXSRC:.cpp=.o) 
BIN           = $(TGT)

all:$(BIN)

$(BIN): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)
	$(STRIP) $(BIN)

clean:
	rm -fr $(OBJS)
	rm -fr $(BIN)

%.o:    %.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ -c $<
