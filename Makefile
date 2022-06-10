all: netdemo
fsdemo:
	gcc fsserver.c -o fsserver
netdemo:
	gcc netserver.c -o netserver -pthread
install:
	cp fsserver ~/bin
clean:
	 rm -f *.o
	 rm -f *server
	 rm -f *~
