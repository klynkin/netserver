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
#include <map>
#include <iostream>
#include <algorithm>

#define MAX_CLIENTS_QTY 3
#define BUF_SIZE 256

static int clients_qty = 0;

static int sock;

static pthread_mutex_t sock_number_mutex;

static std::set<int> sockets;

/* Matches socket numbers and user names */
static std::map <int, std::string> user_names;

/* Removes specials symbols */
static std::string removeSpecials(std::string str) {

	std::string chars = "\r\n";
 
    for (char c: chars)
        str.erase(std::remove(str.begin(), str.end(), c), str.end());

	return str;
}

void term_handler(int){

	std::cout << "Terminating\n" << std::endl;
	
	for (auto socket: sockets)
		close(socket);

	close(sock);
	exit(EXIT_SUCCESS);
}

void * thread_func(void *arg)
{
	int sock_number = *((int *)arg);
	pthread_mutex_unlock(&sock_number_mutex);

	char buf[BUF_SIZE];
	std::string answer;
	std::string name;

	/* TODO Name checking */
	write(sock_number, std::string("Write your name\n").c_str(), std::string("Write your name\n").length());
	read(sock_number, buf, BUF_SIZE-1);

	sockets.insert(sock_number);

	name = removeSpecials(std::string(buf));

	/* TODO log wrapper */
	std::cout << "New client " << name << std::endl;

	if (user_names.count(sock_number) == 0)
		user_names[sock_number] = name;

	std::string server_message = std::string("Server: new client ") + name + std::string(" has connected\n");

	for (auto socket: sockets)
		if (socket != sock_number) 
			write(socket, server_message.c_str(), strlen(server_message.c_str()));

	while(1) {
		
		/* Wait for a message from another client */
		read(sock_number, buf, BUF_SIZE-1);

		/* Check if shutdown command */
		if (strcmp(buf, "shutdown\r\n") == 0) {
			close(sock_number);
			std::cout << "Clients terminated" << std::endl;
			clients_qty--;
			std::cout << "clients_qty " << clients_qty << std::endl;
			sockets.extract(sock_number);
			return NULL;
		}

		std::cout << "Client " << name << ": " << std::string(buf) << std::endl;
		
		/* Prepare to retransmitting it to another client */
		answer = name + std::string(": ") + std::string(buf);
		
		std::cout << answer << std::endl;

		for (auto socket: sockets)
			if (socket != sock_number)
				write(socket, answer.c_str(), strlen(answer.c_str()));

		/* Clear buffer */
		for (int i=0; i<BUF_SIZE; i++) 
			buf[i]=0;
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
		std::cerr << "usage: " << argv[0] << "<port_number>" << std::endl;
		return EXIT_FAILURE;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0){
		std::cout << "socket() failed: " << errno << std::endl;
		return EXIT_FAILURE;
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	port = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		std::cout << "bind() failed: " << errno << std::endl;
		return EXIT_FAILURE;
	}
	listen(sock, 1);
	clen = sizeof(cli_addr);

	char *wait_info_message = "No other clients now. Wait for partners\n";

	/* This is infinfty cycle for server */
	do  {
		if (clients_qty < MAX_CLIENTS_QTY) {
			pthread_mutex_lock(&sock_number_mutex);

			int sock_number = accept(sock, (struct sockaddr *)&cli_addr, &clen);
			if (sock_number < 0) {
				std::cout << "accept() failed: " << errno <<  std::endl;
				return EXIT_FAILURE;
			}

			clients_qty ++;

			if (clients_qty == 1){
				write(sock_number, wait_info_message, strlen(wait_info_message));
			}

			pthread_create(&threads, NULL, thread_func, &sock_number);
		}
		else
			sleep(1);

	} while (clients_qty > 0);

	return 0;
}