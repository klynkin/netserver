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

#define MAX_CLIENTS_QTY 10000
#define BUF_SIZE 256

static int clients_qty = 0;

static int sock;

static pthread_mutex_t sock_number_mutex;

/* Container with user names */
static std::set<std::string> user_names;

/* Matches socket numbers and user names */
static std::set <int> sockets;

/* Removes specials symbols */
static std::string remove_specials(std::string str) {

	/* Add here characters to remove from string */
	std::string chars = "\r\n \t";
 
    for (auto c: chars)
        str.erase(std::remove(str.begin(), str.end(), c), str.end());

	return str;
}

/* Linux special signal handler */
static void term_handler(int){

	std::cout << "Terminating\n" << std::endl;
	
	for (auto socket: sockets)
		close(socket);

	close(sock);
	exit(EXIT_SUCCESS);
}

static void send_broadcast (const int own_sock_number, const std::string message) {
	for (auto socket: sockets)
		if (socket != own_sock_number)
			write(socket, message.c_str(), strlen(message.c_str()));
}

/* Function for client threads */
static void * client_thread(void *arg)
{
	int sock_number = *((int *)arg);

	/* Release here mutex with the sock number after copy
	it to local variable */
	pthread_mutex_unlock(&sock_number_mutex);

	char buf[BUF_SIZE];
	std::string answer;
	std::string name;

	/* Register new user. Check in cycle whether name is correct*/
	do {
		write(sock_number, std::string("Write your name. Use numbers and characters\n").c_str(),
							std::string("Write your name. Use numbers and characters\n").length());

		read(sock_number, buf, BUF_SIZE-1);

		/* Remove all special symbols from string (\r\n \t) */
		name = remove_specials(std::string(buf));

		if (name.length() == 0)
			write(sock_number, std::string("Invalid name\n\n").c_str(),
								std::string("Invalid name\n\n").length());

		else if( user_names.find(name) != user_names.end())
			write(sock_number, std::string("This name is already used\n\n").c_str(),
								std::string("This name is already used\n\n").length());

	} while (name.length() == 0 || user_names.find(name) != user_names.end());

	/* If there are no users in chat */
	if (user_names.size() != 0) {
		std::string hello = "Welcome, " + name + "!\nUsers in chat:\n";
		write(sock_number, hello.c_str(), hello.length());

		/* Write all user names from chat to the new client */
		for (auto item: user_names) {
			item += "\n";
			write(sock_number, item.c_str(), item.length());
		}

	} else {
		std::string hello = "Welcome, " + name + "!\nChat is empty now\n\n";
		write(sock_number, hello.c_str(), hello.length());
	}

	/* Check if sock_number is not already used */
	if (sockets.count(sock_number) == 0) {

		/* Add new sock_number and user_name to the appropriate containers */
		sockets.insert(sock_number);
		user_names.insert(name);

		std::cout << sockets.size() << std::endl;
	} else {
		std::cout << "Socket is already used!" << std::endl;
		return NULL;
	}

	std::string server_message = std::string("Server: new client ") + name + std::string(" has connected\n\n");

	/* Send broadcast message to all clinets, that new user has just connected */
	send_broadcast(sock_number, server_message);

	while(1) {
		
		/* Wait for a message from another client */
		read(sock_number, buf, BUF_SIZE-1);

		/* Check if shutdown command */
		if (strcmp(buf, "shutdown\r\n") == 0) {

			/* In case of shutdown command close socket and free all resoures */
			close(sock_number);

			std::cout << "Client " << name <<  " terminated" << std::endl;
			std::cout << "clients_qty " << clients_qty << std::endl;

			sockets.erase(sock_number);
			user_names.erase(name);
			clients_qty--;
			return NULL;
		}

		/* Prepare for retransmitting it to another client */
		answer = name + std::string(": ") + std::string(buf);
		std::cout << answer << std::endl;

		/* Send broadcast message to all clinets with answer */
		send_broadcast(sock_number, answer);

		/* Clear buffer */
		for (int i=0; i<BUF_SIZE; i++) 
			buf[i]=0;
	}

	return NULL;
}

int main(int argc, char ** argv) {

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

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << "<port_number>" << std::endl;
		return EXIT_FAILURE;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0) {
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

			pthread_create(&threads, NULL, client_thread
		, &sock_number);
		} else
			sleep(1);

	} while (clients_qty > 0);

	return 0;
}