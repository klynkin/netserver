CXX		= g++
CXXFLAGS	= -std=c++17 -ggdb -Wall

all: netdemo
netdemo:
	${CXX} ${CXXFLAGS} netserver.cpp -o netserver -pthread
clean:
	 rm -f *.o
	 rm -f *server
	 rm -f *~
