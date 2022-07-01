all: netdemo
netdemo:
	g++ netserver.cpp -o netserver -pthread
clean:
	 rm -f *.o
	 rm -f *server
	 rm -f *~
