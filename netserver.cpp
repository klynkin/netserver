#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <set>
#include <iostream>

#define MAX_CLIENTS_QTY 3
#define BUF_SIZE 256

static int clients_qty = 0;

static int sock;

static pthread_mutex_t sock_number_mutex;

static std::set<int> sockets;

void term_handler(int){

	printf ("Terminating\n");
	for (auto socket: sockets){
		close(socket);
	}
	close(sock);
	exit(EXIT_SUCCESS);
}

void * thread_func(void *arg)
{
	int sock_number = *((int *)arg);
	pthread_mutex_unlock(&sock_number_mutex);

	char buf[BUF_SIZE];
	char answer[BUF_SIZE];

	while(1){
		
		/* Wait for a message from another client */
		read(sock_number, buf, BUF_SIZE-1);

		/* Check if shutdown command */
		if (strcmp(buf, "shutdown\r\n") == 0){
			close(sock_number);
			printf("Clients terminated\n");
			clients_qty--;
			printf("clients_qty %d\n", clients_qty);
			sockets.extract(sock_number);
			return NULL;
		}
		/* Prepare to retransmitting it to another client */
		sprintf(answer, "%s\0", buf);
		
		for (auto socket: sockets){
			if (socket != sock_number){
				write(socket, answer, strlen(answer));
			}
		}

		/* Clear buffer */
		for (int i=0; i<BUF_SIZE; i++){
			buf[i]=0;
		}
	}
	return NULL;
}

int main(int argc, char ** argv)
{

	int port;
	socklen_t clen; 
	struct sigaction sa;
	sigset_t newset;
	pthread_mutex_init(&sock_number_mutex, NULL);

	/* TODO Add sigmask */
	sigemptyset(&newset);
	sa.sa_handler = term_handler;
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);
	
	pthread_t threads;

	struct sockaddr_in serv_addr, cli_addr;

	if (argc < 2) 
	{
		fprintf(stderr,"usage: %s <port_number>\n", argv[0]);
		return EXIT_FAILURE;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0){
		printf("socket() failed: %d\n", errno);
		return EXIT_FAILURE;
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	port = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		printf("bind() failed: %d\n", errno);
		return EXIT_FAILURE;
	}
	listen(sock, 1);
	clen = sizeof(cli_addr);

	char *wait_info_message = "No other clients now. Wait for partners\n";

	do  {
		if (clients_qty < MAX_CLIENTS_QTY) {
			pthread_mutex_lock(&sock_number_mutex);

			int sock_number = accept(sock, (struct sockaddr *)&cli_addr, &clen);
			if (sock_number < 0){
				printf("accept() failed: %d\n", errno);
				return EXIT_FAILURE;
			}
			sockets.insert(sock_number);

			clients_qty ++;

			if (clients_qty == 1){
				write(sock_number, wait_info_message, strlen(wait_info_message));
			}

			pthread_create(&threads, NULL, thread_func, &sock_number);

			pthread_mutex_lock(&sock_number_mutex);

			/* TODO fix write to closed scoket */
			char *connection_info_message = "New partner has connected!\n";
			for (auto socket: sockets){
				write(socket, connection_info_message, strlen(connection_info_message));
			}

			printf("clients_qty %d\n", clients_qty);
			pthread_mutex_unlock(&sock_number_mutex);
		}
		else {
			printf("clients_qty %d\n", clients_qty);
			sleep(1);
		}
	} while (clients_qty > 0);

	printf("lol\n");
	// for (int i = 0; i < MAX_CLIENTS_QTY; i++){
	// 	pthread_join(threads[i], NULL);
	// }

	return 0;
}