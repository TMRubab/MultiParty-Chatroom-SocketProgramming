#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t *listOfUsers[MAX_USERS] = {0}; // list of users


/* Add user to userList */
void user_add(user_info_t user);
/* Delete user from userList */
void user_delete(int sockfd);
/* Get user name from userList */
char * get_username(int sockfd);
/* Get user sockfd by name */
int get_sockfd(char *name);

/* Add user to userList */
void user_add(user_info_t user){
	/***************************/
	/* add the user to the list */
	/**************************/
	++users_count;
	listOfUsers[users_count-1] = malloc(sizeof(user_info_t));
	if(listOfUsers[users_count-1] == NULL){
		printf("Memory allocation for new user failed\n");
		exit(0);
	}
	listOfUsers[users_count-1]->sockfd = user.sockfd;
	strcpy(listOfUsers[users_count-1]->username, user.username);
}

/* Delete user from userList */
void user_delete(int ss){//ss is the sockfd of the user
	int i;
	int b = 0;
	for(i=0;i<users_count;i++){
		/***************************/
		/* delete the user from the list */
		/**************************/
		if(listOfUsers[i]->sockfd == ss) b = 1;
		if(b == 1) listOfUsers[i] = listOfUsers[i+1]; //shifting information of each user after the deleted user to the left

	}
	if(b == 1){ //a user was deleted i.e. the input was a valid sockfd
		--users_count;
		free(listOfUsers[users_count]);
	}
	
}

/* Get user name from userList */
char * get_username(int ss){
	int i;
	static char uname[MAX];
	bzero(uname, sizeof(uname)); //in case ss is not valid sockfd or this user is not in the list, this function will return empty string
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	for(int i = 0; i < users_count; ++i)
		if(listOfUsers[i]->sockfd == ss) strcpy(uname, listOfUsers[i]->username);

	printf("get user name: %s\n", uname);
	return uname;
}

/* Get user sockfd by name */
int get_sockfd(char *name){
	int i;
	int sock = -1; //if no such name exists, it will return -1
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	for(i = 0; i < users_count; ++i)
		if(strcmp(listOfUsers[i]->username, name) == 0)
			sock = listOfUsers[i]->sockfd ;
	return sock;
}

int main(){
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number

	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    	int i, rv;
	
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
	// set us a socket and bind it
  	listener = socket(AF_INET, SOCK_STREAM, 0);
  	if (listener == -1) {
    		printf("Socket creation failed...\n");
    		exit(1);
  	}
  	else
    		printf("Socket successfully created..\n");
  
  	// lose the pesky "address already in use" error message
  	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  
  	bzero(&server_addr, sizeof(server_addr));
  
  	// asign IP, PORT
  	server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	server_addr.sin_port = htons(PORT);
  
  	// Binding newly created socket to given IP and verification
  	if ((bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr))) != 0) {
    		printf("Socket bind failed...\n");
    		exit(2);
  	}
  	else
    		printf("Socket successfully binded..\n");



	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
		
	// add the listener to the master set
    	FD_SET(listener, &master);
    	
    	// keep track of the biggest file descriptor
    	fdmax = listener; // so far, it's this one
	
	// main loop

	for(;;) {
		/***************************************/
		/* use select function to get read_fds */
		/**************************************/
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
        	for(i = 0; i <= fdmax; i++) {
            	  	if (FD_ISSET(i, &read_fds)) { // we got one!!
                  		if (i == listener) {
                      			/**************************/
					/* we are the listener and we need to handle new connections from clients */
					/****************************/
					addr_size = sizeof(client_addr);
					newfd = accept(listener, (struct sockaddr*)&client_addr, &addr_size);
                      			if (newfd == -1){
                        			perror("accept\n");
                      			}
					else {
                          			FD_SET(newfd, &master); // add to master set
                          			if (newfd > fdmax) { // keep track of the max
                            				fdmax = newfd;
                          			}
                          			printf("selectserver: new connection from %s on socket %d\n",inet_ntoa(client_addr.sin_addr),newfd);
                      			}


					// send welcome message
					bzero(buffer, sizeof(buffer));
					strcpy(buffer, "Welcome to the chat room!\nPlease enter a nickname.");
					if (send(newfd, buffer, sizeof(buffer), 0) == -1)
						perror("send Welcome\n");
                     	 	}
				else {
                        		// handle data from a client
					bzero(buffer, sizeof(buffer));
					nbytes = recv(i, buffer, sizeof(buffer), 0);
                        		if (nbytes < 0) {
                         	 		// got error
                            			perror("recv\n");  
                        		}
					// user exits by ctrl+c or other connection issue
					else if(nbytes == 0){
						char curr_name[256];
						strcpy(curr_name, get_username(i));
						// Broadcast the leave message to the other users in the group if the user is registered
						if(strcmp(curr_name, "\0") != 0){ //the user is registered	
							// Prepare the welcome message
							bzero(buffer, sizeof(buffer));
                                			strcat(buffer, curr_name);
                                			strcat(buffer, " has left the chat room!");	
							int j;
							for(j = 0; j < users_count; ++j){
								if(listOfUsers[j]->sockfd!= i){
									if (send(listOfUsers[j]->sockfd, buffer, sizeof(buffer), 0) == -1)
										perror("send broadcast leave\n");
								}
							}
						}
						printf("user left\n");
						close(i);
						user_delete(i);
						FD_CLR(i, &master);
					}						
					else {
                            			// we got some data from a client
						if (strncmp(buffer, "REGISTER", 8)==0){
							printf("Got register message\n");
							//There are already 50 users
							if(users_count ==  MAX_USERS){
								printf("But the system is full\n");
								//Send the bad news to the client
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Sorry, the system is full. Please try again later.");
								if (send(i, buffer, sizeof(buffer), 0) == -1)
									perror("send system full news\n");
								continue;
							}

							/********************************/
							/* In case the system is not full, get the user name and add the user to the userlist*/
							/**********************************/
							char curr_name[MAX]; //current client's name
							strcpy(curr_name, &buffer[8]); //getting the name that is after the word "REGISTER"
							printf("username is %s\n", curr_name);
							//username taken by another user
							if(get_sockfd(curr_name) != -1){
								printf("username exists\n");
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Nickname taken :(\nPlease enter a new nickname.");
								if (send(i, buffer, sizeof(buffer), 0) == -1)
									perror("send nickname taken\n");
								continue;
							}
							//Now that the username is unique, add it to the list
							user_info_t curr_client;
							curr_client.sockfd = i;
							strcpy(curr_client.username, curr_name);
							user_add(curr_client);
							// Prepare the welcome message
							printf("about to broadcast\n");
							bzero(buffer, sizeof(buffer));
							strcpy(buffer, "Welcome ");
                                			strcat(buffer, curr_name);
                                			strcat(buffer, " to join the chat room!");
	
							/*****************************/
							/* Broadcast the welcome message*/
							/*****************************/
							int j;
							for(j = 0; j < users_count; ++j){
								if (send(listOfUsers[j]->sockfd, buffer, sizeof(buffer), 0) == -1)
									perror("send broadcast join\n");
							}
						}
						else if (strncmp(buffer, "EXIT", 4)==0){
							printf("Got exit message. Removing user from system\n");
							// prepare the leave message
                                			bzero(buffer, sizeof(buffer));
							strcpy(buffer, get_username(i));
							strcat(buffer, " has left the chatroom");
	
							/*********************************/
							/* Broadcast the leave message to the other users in the group*/
							/**********************************/
							int j;
							for(j = 0; j < users_count; ++j){
								if(listOfUsers[j]->sockfd!= i){
									if (send(listOfUsers[j]->sockfd, buffer, sizeof(buffer), 0) == -1)
										perror("send broadcast leave\n");
								}
							}
	
							/***********************************/
							/* close the socket, delete the user and remove the socket from the fd array*/
							/***********************************/
							close(i);
							user_delete(i);
							FD_CLR(i, &master);
	
						}
						else if (strncmp(buffer, "WHO", 3)==0){
							printf("Got WHO message from client.\n");
							/***************************************/
							/* Concatenate all the user names except the sender's name into the tab-separated char ToClient and send it to the requesting client*/	
							/***************************************/
							char ToClient[MAX];
							bzero(ToClient, sizeof(ToClient));
							int j;
							for(j = 0; j < users_count; ++j){
								if(listOfUsers[j]->sockfd!= i){
									strcat(ToClient, listOfUsers[j]->username);
									strcat(ToClient, "\t");
								}
							}
							if (send(i, ToClient, sizeof(ToClient), 0) == -1)
								perror("send WHO\n");
						}
						else if ((strncmp(buffer, "#", 1)==0) && (strchr(buffer, ':') != NULL)){ //checks if the message starts with # and contains :
							// send direct message 
							printf("Got direct message: %s\n", buffer);
	
							/**************************************/
							/* Get the source name xx, and the target sockfd*/
							/*************************************/
							// get which client sent the message
							char from_name[MAX];
							strcpy(from_name, get_username(i));
							// get which client to send the message
							char to_name[MAX];
							bzero(to_name, sizeof(to_name));
							int j = 1;
							while(buffer[j] != ':')
								j++;
							memcpy(to_name, &buffer[1], (j-1)*sizeof(*to_name));
							// get dest sock
							int destsock = get_sockfd(to_name);
							//on failure
							if(destsock == -1){
								printf("user does not exist\n");
								char msg_unsuccess[MAX];
								strcpy(msg_unsuccess, "No user named ");
								strcat(msg_unsuccess, to_name);
								strcat(msg_unsuccess, ". Please check your input or type WHO to find the list of active users");
								printf("Sending unsuccess status message to %s\n", from_name);
								if (send(i, msg_unsuccess, sizeof(msg_unsuccess), 0) == -1)
									perror("send unsuccess status message\n");
							}
							//on success						
							else{		
								// concatenate the message in the form "xx to you: msg"
								char sendmsg[MAX];
								strcpy(sendmsg, from_name);
								strcat(sendmsg, " to you: ");
								strcat(sendmsg, &buffer[j+2]);
								printf("Sending from %s to %s\n", from_name, to_name);
								if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1)
									perror("send private message\n");
							}
						}
						else{
							printf("Got broadcast message from user: %s\n", buffer);
							/*********************************************/
							/* Broadcast the message to all users except the one who sent the message*/
							/*********************************************/
							// get which client sent the message
							char from_name[MAX];
							strcpy(from_name, get_username(i));
							//get the message
							char sendmsg[MAX];
							strcpy(sendmsg, from_name);
							strcat(sendmsg, " to everyone: ");
							strcat(sendmsg, buffer);
							printf("Sending from %s to everyone\n", from_name);
							int j;
							for(j = 0; j < users_count; ++j){
								if(listOfUsers[j]->sockfd != i){
									if (send(listOfUsers[j]->sockfd, sendmsg, sizeof(sendmsg), 0) == -1)
										perror("send");
								}
							}
									
						} 
	
                        		}
                    		} // end handle data from client
        		} // end got new incoming connection
        	} // end looping through file descriptors
        } // end for(;;) 

	return 0;
}
