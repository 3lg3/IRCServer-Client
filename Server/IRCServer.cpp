
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include<sstream>

#include "IRCServer.h"

int QueueLength = 5;

struct User {
	const char * username;
	const char * password;
	struct User * next;
};

typedef struct User User;

struct UserList {
	User * head;
};

typedef struct UserList UserList;

struct Message {
	const char * text;
	const char * username;
	struct Message * next;
};

typedef struct Message Message;

struct MessageList {
	Message * head;
	int MessageNum;
};

typedef struct MessageList MessageList;

struct Room {
	UserList * user;
	const char * roomname;
	MessageList * msg;
	struct Room * next;
};

typedef struct Room Room;

struct RoomList {
	Room * head;
};

typedef struct RoomList RoomList;
	
UserList users;
RoomList rooms;
MessageList msgs;


//test

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

	//printf("The commandLine has the following format:\n");
	//printf("COMMAND <user> <password> <arguments>. See below.\n");
	//printf("You need to separate the commandLine into those components\n");
	//printf("For now, command, user, and password are hardwired.\n");

	//const char * command = "ADD-USER";
	//const char * user = "peter";
	//const char * password = "spider";
	//const char * args = "";
	const char * command = strtok(commandLine, " ");
	const char *user = strtok(NULL, " ");
	const char *password = strtok(NULL, " ");
	const char *args = strtok(NULL,""); 

	
	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

void
IRCServer::initialize()
{
	// Open password file
	FILE * f = fopen("password.txt","w");
 
	// Initialize users in room
	users.head = NULL;
	// Initalize message list
	msgs.head = NULL;
	// Initialize room list
	rooms.head = NULL;

	fclose(f);
}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password
	char * user1 = strdup(user);
        char * password1 = strdup(password);
	User * u = users.head;
	while (u != NULL) {
		if (strcmp(u->username,user1) == 0) {
			if (strcmp(u->password,password1) == 0) {
				return true;
			}
			else return false;
		}
		u = u->next;
	}
	return false;
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	// Here add a new user. For now always return OK.
	//char * user1 = strdup(user);
	//char * password1 = strdup(password);
	// Check if username already exists.
	User * u = users.head;	
	while (u != NULL) {
		if (strcmp(user,u->username) == 0) {
			const char * msg =  "DENIED\r\n";
        		write(fd, msg, strlen(msg));
			return;
		}
		u = u->next;
	}
	// add a new user to the list "users".
	User * v;
	v = (User *) malloc(sizeof(User));
	v->username = strdup(user);
	v->password = strdup(password);
	v->next = users.head;
	users.head = v;
	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
	// write the username and password into the file "password.txt"
	FILE * f = fopen("password.txt","a");
	fprintf(f,"%s\n",user);
	fprintf(f,"%s\n",password);
	fprintf(f,"\n");
	fclose(f);
	return;		
}
void
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args) {
        //Password check.
	if (checkPassword(fd,user,password) == false) {
                const char * msg =  "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	// check if roomname already exists.
	Room * r = rooms.head;
	while (r != NULL) {
		if (strcmp(args,r->roomname) == 0) {
			const char * msg2 =  "Room already exists!\r\n";
                        write(fd, msg2, strlen(msg2));
                        return;
		}
		r = r->next;
	}
	// add a new room to the list "rooms".
	Room * s;
	s = (Room *) malloc(sizeof(Room));
	s->roomname = strdup(args);
	s->next = rooms.head;
	rooms.head = s;
	const char * msg =  "OK\r\n";
        write(fd, msg, strlen(msg));
}
void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	//Password check
	if (checkPassword(fd,user,password) == false) {
		const char * msg =  "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
		return;
	}	
	// find the room by roomname(args).
	Room * r;
	r = rooms.head;
	while (r != NULL) {
		if (strcmp(r->roomname, args) == 0) {
			if (r->user == NULL) {
				r->user = (UserList*) malloc(sizeof(UserList));
				r->user->head = NULL;
			}
			//check if the user is already in the room.
			User * u = r->user->head;
			while (u != NULL) {
				if (strcmp(u->username,user) == 0) { 
					const char * msg =  "OK\r\n";
                                	write(fd, msg, strlen(msg));
					return;
				}
				u = u->next;
			}
			if (u == NULL) {
			// add this new user to the list "user" of Room 
				User * v = (User *) malloc(sizeof(User));
				v->username = strdup(user);	
				v->password = strdup(password);
				v->next = r->user->head;
				r->user->head = v;
				const char * msg =  "OK\r\n";
        			write(fd, msg, strlen(msg));
				return;
			}
		break;
		}
	r = r->next;
	}	
	//write "DENIED" if room not found.
	if (r == NULL) {
		const char * msg =  "ERROR (No room)\r\n";
                write(fd, msg, strlen(msg));
                return;
	} 	
	
}

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	//Password check.
	if (checkPassword(fd,user,password) == false) {
		const char * msg =  "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	// find the room by roomname(args).
        Room * r;
        r = rooms.head;
        while (r != NULL) {
                if (strcmp(r->roomname, args) == 0) {
			//find the user in the room by username and remove.
				User * u = r->user->head;
				if (strcmp(u->username, user) == 0) {
					r->user->head = u->next;
					const char * msg =  "OK\r\n";
                                	write(fd, msg, strlen(msg));	
					return;
				}
				while (u->next != NULL) {
					if (strcmp(u->next->username, user) == 0) {
						u->next = u->next->next;
						const char * msg =  "OK\r\n";
                                        	write(fd, msg, strlen(msg));
						return;
					}
					u = u->next;
				}
				const char * error = "ERROR (No user in room)\r\n";
				write(fd, error, strlen(error));
				break;
		}
		r = r->next;
	}
	//write "DENIED" if room not found.
        if (r == NULL) {
                const char * msg =  "Error(No room)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }




}
void 
IRCServer::listRooms(int fd, const char * user, const char * password, const char * args) {
	// Password check.
	if (checkPassword(fd,user,password) == false) {
                const char * msg =  "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	Room * r = rooms.head;
	if (r == NULL) {
		const char * msg =  "DENIED";
                write(fd, msg, strlen(msg));
                return;
	}
	while (r != NULL) {
		const char * msg1 = strdup(r->roomname);
                const char * msg2 = " ";
                write(fd,msg1,strlen(msg1));
                if (r->next != NULL) {
			write(fd,msg2,strlen(msg2));
		}
                r = r->next;
	}
	//const char * msg3 = "\r\n";
       // write(fd,msg3,strlen(msg3));
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	// Password check.
	if (checkPassword(fd,user,password) == false) {
		const char * msg =  "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
                return;
        }
	char * args1 = strdup(args);
	// Parse args into room and message.
	char * roomx = strtok(args1, " ");
	char * message = strtok(NULL, "");
	// Find the room by roomname.
	Room * r  = rooms.head;
	while (r != NULL) {
		if (strcmp(r->roomname, roomx) == 0) {
			//Check if this user is in the Room "roomx".
			if (r->user == NULL) {
				const char * error = "ERROR (user not in room)\r\n";
				write(fd, error, strlen(error));
				return;
			}
			User * u = r->user->head;
			while (u != NULL) {
				if (strcmp(user, u->username) == 0) break;
				u = u->next;
			}
			if (u == NULL) {
				const char * error = "ERROR (user not in room)\r\n";
                                write(fd, error, strlen(error));
                                return;
			}
			if (r->msg == NULL) {
				r->msg = (MessageList *) malloc(sizeof(MessageList));
				r->msg->head = NULL;
			}
			Message * n = r->msg->head;
			// add the message to the MessageList msg.
			Message * m = (Message *) malloc(sizeof(Message));
			m->text = strdup(message);
			m->username = strdup(user);
			m->next = NULL;
			if (n == NULL) {
				r->msg->head = m;
				const char * msg = "OK\r\n";
                        	write(fd, msg, strlen(msg));
                        	return;
			}
			while (n->next != NULL) {
				n = n->next;
			}
			n->next = m;
			const char * msg = "OK\r\n";
                        write(fd, msg, strlen(msg));
                        return;
		}
	r = r->next;
	}
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	// Password check.
        if (checkPassword(fd,user,password) == false) {
                const char * msg =  "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	//Parse the room into num and roomname.
	char * args1 = strdup(args);
	char * n = strtok(args1, " ");
	int num = atoi(n);
	char * roomx = strtok(NULL, "");
	// find the room by roomname.
	Room * r  = rooms.head;
        while (r != NULL) {
        	if (strcmp(r->roomname, roomx) == 0) { 
			// Check if the user is in the room.
			if (r->user == NULL) {
                                const char * error = "ERROR (User not in room)\r\n";
                                write(fd, error, strlen(error));
                                return;
                        }
                        User * u = r->user->head;
                        while (u != NULL) {
                                if (strcmp(user, u->username) == 0) break;
                                u = u->next;
                        }
                        if (u == NULL) {
                                const char * error = "ERROR (User not in room)\r\n";
                                write(fd, error, strlen(error));
                                return;
                        }
			// pick the message from index 'num'
			if (r->msg == NULL) {
				const char *error = "NO-NEW-MESSAGES\r\n";
                                write(fd,error,strlen(error));
                                return;
			}
			Message * m = r->msg->head;
			if (m == NULL) {
				const char *error = "NO-NEW-MESSAGES\r\n";
                                write(fd,error,strlen(error));
                                return;
			}
			int i;
			for (i = 0; i <= num; i++) {
				m = m->next;
				if (m == NULL) break;
			}
			if (m == NULL) {
				const char *error = "NO-NEW-MESSAGES\r\n";
				write(fd,error,strlen(error));
				return;
			}
			// print the messages out.
			while (m != NULL) {
				std::stringstream index;
				index << i;
				const char * space = " ";
				const char * msg1 = index.str().c_str();
				const char * msg2 = strdup(m->username);
                		const char * msg3 = strdup(m->text);
				const char * msg4 = "\r\n";
                		write(fd,msg1,strlen(msg1));
				write(fd,space,strlen(space));
                		write(fd,msg2,strlen(msg2));
				write(fd,space,strlen(space));
				write(fd,msg3,strlen(msg3));
				write(fd,msg4,strlen(msg4));
				m = m->next;
				i++;
			}
			const char * msg5 = "\r\n";
        		write(fd,msg5,strlen(msg5));
			break;
		}
		r = r->next;
	}		
	//write "DENIED" if room not found.
        if (r == NULL) {
                const char * msg =  "Error(No room)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }			
				
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	//Password check.
	if (checkPassword(fd,user,password) == false) {
                const char * msg =  "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	//find the room by roomname(args);
	Room * r;
	r = rooms.head;
	while (r != NULL) {
		if (strcmp(r->roomname,args) == 0) {
			break;	
		}
	r = r->next;
	}
	// if room not found
	if (r == NULL) {
		const char * msg =  "ERROR (No room)\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	//else room found
	//check if there are users in this room.
	if (r->user == NULL) {
		const char * msg3 = "\r\n";
        	write(fd,msg3,strlen(msg3));
		return;
	}
	//Sort the username in the room by an alphabetical order
	User * u;
        User * v;
        u = r->user->head;
        v = r->user->head;
        char * temp1;
        char *temp2;
        while (u->next != NULL) {
                while(v->next != NULL) {
                        if (strcmp(v->username,v->next->username) > 0) {
                                temp1 = strdup(v->username);
                                temp2 = strdup(v->password);
                                v->username = v->next->username;
                                v->password = v->next->password;
                                v->next->username = temp1;
                                v->next->password = temp2;
                        }
                v = v->next;
                }
        v = r->user->head;
        u = u->next;
        }
	//	
	// Print all users in room  with username and password;
        User * w;
        w = r->user->head;
	if (w == NULL) {
		const char * msgempty = "\r\n";
        	write(fd,msgempty,strlen(msgempty));
		return;
	}
        while (w != NULL) {
                const char * msg1 = strdup(w->username);
                const char * msg2 = "\r\n";
                write(fd,msg1,strlen(msg1));
                write(fd,msg2,strlen(msg2));
                w = w->next;
        }
        const char * msg3 = "\r\n";
        write(fd,msg3,strlen(msg3));
}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	// Password check.
	if (checkPassword(fd,user,password) == false) {
		const char * msg =  "ERROR (Wrong password)\r\n";
        	write(fd, msg, strlen(msg));
		return;
	}
	// Sort the username in an alphabetical order.
	User * u;
	User * v;
	u = users.head;
	v = users.head;
	char * temp1;
	char *temp2;
	while (u->next != NULL) {
		while(v->next != NULL) {
			if (strcmp(v->username,v->next->username) > 0) {
				temp1 = strdup(v->username);
				temp2 = strdup(v->password);
				v->username = v->next->username;
				v->password = v->next->password;
				v->next->username = temp1;
				v->next->password = temp2;
			}
		v = v->next;
		}
	v = users.head;
	u = u->next;	
	}
	// Print all users with username and password;
	User * w;
	w = users.head;
	while (w != NULL) {
		const char * msg1 = strdup(w->username);
		const char * msg2 = "\r\n";
		write(fd,msg1,strlen(msg1));
		write(fd,msg2,strlen(msg2));
		w = w->next;
	}	
	const char * msg3 = "\r\n";
	write(fd,msg3,strlen(msg3));		
}
