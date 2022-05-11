#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789  // port number

pthread_mutex_t lock; //for mutex lock
static int sockfd; //for socket

void generate_menu(){
	printf("Hello dear user pls select one of the following options:\n");
	printf("EXIT\t-\t Send exit message to server - unregister ourselves from server\n");
    	printf("WHO\t-\t Send WHO message to the server - get the list of current users except ourselves\n");
    	printf("#<user>: <msg>\t-\t Send <MSG>> message to the server for <user>\n");
    	printf("Or input messages sending to everyone in the chatroom.\n");
}

void* recv_server_msg_handler(){
    	/********************************/
	/* receive message from the server and desplay on the screen*/
	/**********************************/
	char buffer_thread[MAX];
	while(1){
		sleep(0.1); //to slow down the iterations since recv here is non-blocking
		bzero(buffer_thread, sizeof(buffer_thread));
		pthread_mutex_lock(&lock); //lock before accessing the socket
		int status = recv(sockfd, buffer_thread, sizeof(buffer_thread), MSG_DONTWAIT); //it's non-blocking as we need to let the main thread send through this socket in case user inputs something
		pthread_mutex_unlock(&lock);
		if(status == -1) //nothing to receive
			continue;
		else if(status == 0){ //server closed (e.g. with ctrl+c)
			puts("Ooops, server closed!");
			exit(0);
		}
    		puts(buffer_thread); //show the message received from server
	}
	pthread_exit(NULL);
	return NULL;
}


int main(){
	//setvbuf(stdout, NULL, _IONBF, 0);
	int n;
	int nbytes;
	static char buffer[MAX];
	struct sockaddr_in server_addr;
	char nickname[MAX];
	
	/******************************************************/
	/* create the client socket and connect to the server */
	/******************************************************/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		printf("Socket creation failed...\n");
    		exit(0);
  	}
  	else
    	printf("Socket successfully created...\n");
    
  	bzero(&server_addr, sizeof(server_addr));
  
  	// assign IP, PORT
  	server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  	server_addr.sin_port = htons(PORT);	
	
  	// connect the client socket to the server socket
  	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
    		printf("Connection with the server failed...\n");
    		exit(0);
  	}
  	else
    		printf("Connected to the server...\n");
    
	// recieve welcome message
    	bzero(buffer, sizeof(buffer));
	nbytes = recv(sockfd, buffer, sizeof(buffer), 0);
    	if (nbytes == -1){
        	perror("error with recv");
    	}
	else if(nbytes == 0){ //server closed (e.g. with ctrl+c)
		puts("Ooops, server closed!");
		exit(0);
	}
    	puts(buffer);

	/*************************************/
	/* Let the user receive the prompt from the server and enter the nickname to register. */
	/* Note that we concatenate "REGISTER" before the name to notify the server it is the register message*/
	/*******************************************/
	do{
		bzero(buffer, sizeof(buffer));
		n = 0;
		while((buffer[n++] = getchar()) != '\n');
		buffer[n-1] = '\0'; //removing the '\n' from the name
		if(n >= 26) //name longer than 24 characters
			printf("The maximum length of a name is 24. Please input again\n");
		else if(n == 1) //an empty string name
			printf("Empty string cannot be a name. Please input something\n");
		else{ //correct length
			//Preparing a register message
			strcpy(nickname, buffer);
			strcpy(buffer, "REGISTER");
			strcat(buffer, nickname);		
			if(send(sockfd, buffer, strlen(buffer),0) < 0){
				printf("%s%s", "Error with sending message: ", buffer);
				exit(0);
			}
			// receive welcome message "welcome xx to joint the chatroom." OR receive prompt again if username not unique
			bzero(buffer, sizeof(buffer));
			nbytes = recv(sockfd, buffer, sizeof(buffer), 0);
    			if (nbytes == -1){
        			perror("error with recv");
    			}
			else if(nbytes == 0){ //server crash
				puts("Ooops, server closed!");
				exit(0);
			}
    			puts(buffer);
		}
	}while(strncmp(buffer, "Welcome", 7) != 0); //if the name is not unique, user does not receive "Welcome" message and is prompted again to input another name
    

    	/*****************************************************/
	/* Create a thread to receive message from the server*/
	/*****************************************************/
	//initiate the mutex lock
	if(pthread_mutex_init(&lock, NULL) != 0){
		perror("mutex init failed");
		exit(0);
	}
	//create the thread
	pthread_t id;
	if(pthread_create(&id, NULL, recv_server_msg_handler, NULL) != 0){
		perror("thread creation failed");
		exit(0);
	}

	generate_menu();
	// chat with the server
	while (1) {
		int is_exit = 0;
		sleep(0.1); //so the user can't attack the server with a bot
		bzero(buffer, sizeof(buffer));
		n = 0;
		while ((buffer[n++] = getchar()) != '\n');

		if(strcmp(buffer, "\n") == 0){
			printf("Please type something!\n");
			continue;
		}

		else if ((strncmp(buffer, "EXIT", 4)) == 0){
			printf("Client Exit...\n");
			is_exit = 1;
		}

		else if (strncmp(buffer, "WHO", 3) == 0)
			printf("Getting user list, pls hold on...\n");

		buffer[n-1] = '\0'; //removing the '\n' from the message

		/*************************************/
		/* Sending EXIT/WHO/direct/broadcast message. Direct message format input by user is "#<send_name>: <message>". Broadcast message has no specific format. The server will check the user list to find this user's name.*/
		/**************************************/
		pthread_mutex_lock(&lock); //lock before accessing the socket
		if(send(sockfd, buffer, strlen(buffer),0) < 0){
			printf("%s%s\n", "Error with sending message: ", buffer);
			exit(0);
		}
		pthread_mutex_unlock(&lock);
		if(is_exit == 1) break;
			
	}
	pthread_mutex_destroy(&lock);
	return 0;
}

