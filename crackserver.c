/*CSSE2310 Assignment 4 crackServer.c
 * By S4716841 YUNFEI PEI
 * A little word: I know that this code is not done yet, but I had to focus on
 * the other assignments of other courses that are all squeezed into Week 13
 * Sorry for that. 
 * Thanks for your attention
 */

//Include all necessary
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <csse2310a4.h>
#include <pthread.h>

//a struct for storing client information
typedef struct {
    int clientFd;
} ClientInfo;

/* open_listen
 * ----------------
 *  This function tries to access and listen to the given, and returns the 
 *  listening socket
 *  
 *  const char* port: The port that the program is going to listen to
 *  Return: the listenfd (socket) if everything is well
 *  Error: Exit by signal 1 if address cannot be determined, 
 *         Exit by 1 if error setting socket option
 *         Exit by 4 if unable to open socket 
 *  REF: This code came from server-multiprocess.c from week 10 lecture
 */
int open_listen(const char* port) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // listen on all IP addresses
    int err;
    if ((err = getaddrinfo(NULL, port, &hints, &ai))) {
	freeaddrinfo(ai);
	fprintf(stderr, "%s\n", gai_strerror(err));
	return 1;   // Could not determine address
    }
    // Create a socket and bind it to a port
    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // 0=default protocol (TCP)
    // Allow address (port number) to be reused immediately
    int optVal = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
	    &optVal, sizeof(int)) < 0) {
	perror("Error setting socket option");
	exit(1);
    }
    if (bind(listenfd, ai->ai_addr, sizeof(struct sockaddr)) < 0) {
	fprintf(stderr, "crackserver: unable to open socket for listening\n");
	exit(4);
    }
    if (listen(listenfd, 10) < 0) {  // Up to 10 connection requests can queue
	perror("Listen");
	exit(4);
    }
    // Have listening socket - return it
    return listenfd;
}

/*capitalise
 *------------
 * This function capitalise all the given input
 *
 * char* buffer: The incoming buffer from client
 * int len: The length of the input
 *
 * Return: The capitalised buffer
 * REF: Code came from multi-server.c from week 10 lecture
 */
char* capitalise(char* buffer, int len) {
    int i;
    for (i = 0; i < len; i++) {
	buffer[i] = (char)toupper((int)buffer[i]);//capitalise the buffer
    }
    return buffer;
}

/*handle_clients
 * ---------------
 * This function handles the clients, handles the termination of clients
 * 
 * void* arg: A generic pointer
 * Returns: NULL
 */
void* handle_client(void* arg) {
    ClientInfo* ci = (ClientInfo*) arg;//cast arg to client info
    int clientFd = ci->clientFd;
    close(clientFd); //close client fd 
    free(ci); //free memory
    return NULL;
}

/* process_connections
 * -------------------
 * This function processes the incoming connections from clients and set
 * relative responses according to the requests
 * 
 * int fdServer: file descriptor of server socket
 * Return: nothing because its void
 * Error: exit by 1 if error accepting connections or client disconnected
 * REF: The code came from server-multiprocess.c of week 10 lecture
 */
void process_connections(int fdServer) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    char buffer[1024];
    ssize_t numBytesRead;
    // Repeatedly accept connections and process data (capitalise)
    while (1) {
	fromAddrSize = sizeof(struct sockaddr_in);
	// Block, waiting for a new connection. (fromAddr will be populated
	// with address of client)
	fd = accept(fdServer, (struct sockaddr*) &fromAddr, &fromAddrSize);
	if (fd < 0) {
	    perror("Error accepting connection");
	    exit(1);
	}
	// Turn our client address into a hostname and print out both
	// the address and hostname as well as the port number
	char hostname[NI_MAXHOST];
	int error = getnameinfo((struct sockaddr*)&fromAddr,
		fromAddrSize, hostname, NI_MAXHOST, NULL, 0, 0);
	if (error) {
	    fprintf(stderr, "Error getting hostname: %s\n",
		    gai_strerror(error));
	} else {
	    printf("Accepted connection from %s (%s), port %d\n",
		    inet_ntoa(fromAddr.sin_addr), hostname,
		    ntohs(fromAddr.sin_port));
	}

	//Send a welcome message to our client
	dprintf(fd, "Welcome\n");

	// Repeatedly read data arriving from client - turn it to upper case -
	// send it back to client
	while((numBytesRead = read(fd, buffer, 1024)) > 0) {
	    capitalise(buffer, numBytesRead);
	    write(fd, buffer, numBytesRead);
	}
	// error or EOF - client disconnected

	if (numBytesRead < 0) {
	    perror("Error reading from socket");
	    exit(1);
	}
	// Print a message to server's stdout
	printf("Done with client\n");
	fflush(stdout);
	close(fd);
    }
}

/* check_integer
 * ---------------
 * This function checks if the given string consists of only numbers 
 *
 * const char* string: The given string
 *
 * Returns: 0 if the string contains alphabets 
 *          1 if the string contains only numbers
 */
int check_integer(const char* string){
    while (*string) {
	if (!isdigit(*string)) {
	    return 0;
	}
	string++;
    }
    return 1;
}

/* check_dictionary
 * ----------------
 *  This function checks the dictionary file on whether it can be openned,
 *  or if it has short words
 *
 *  const char* dictionaryFile: The dictionary file to be checked
 *
 *  Return: 0 if everything is good
 *  Error: Exit by 2 if unable to open file or file is non existent
 *         Eit by 3 if there are no plain text words to read
 */
int check_dictionary(const char* dictionaryFile){
    if (strlen(dictionaryFile) == 0) {//if user keyed in empty file 
	fprintf(stderr, "crackserver: unable to open dictionary "
		"file \"%s\"\n", dictionaryFile);
	exit(2);
    }
    FILE* file = fopen(dictionaryFile, "r");//opens file to read
    if (file == NULL) {
	fprintf(stderr, "crackserver: unable to open dictionary "
		"file \"%s\"\n", dictionaryFile);
	exit(2);
    }
    // Check if there are words of length <= 8
    int hasShortWords = 0;
    char word[51];  // Words assumed to be less than 50 characters
    while (fgets(word, sizeof(word), file) != NULL) {
	if (strcspn(word, "\n") <= 8) {//get length of word with no \n
	    hasShortWords = 1;
	    break;
	}
    }
    fclose(file); //close the file
    if (!hasShortWords) { //if does not have short wrords
	fprintf(stderr, "crackserver: no plain text words to test\n");
	exit(3);
    }
    return 0;
}

/* check_command
 * --------------
 * This funcrion checks the given command given by the user and gives an 
 * error message if the given command is invalid
 *
 * int argc: argument count of the program
 * char* argv[]: argument vector of the program
 *
 * Return: 0 if everything goes well
 * Error: Exit by 1 if the user entered invalid commmand 
 *        Exit by 2 if unable to open dictionary file 
 *        Exit by 3 if no plain word to test
 */
int check_command(int argc, char* argv[]){
    // Default values
    int maxconn = 0;  // No limit by default
    const char* port = "0";  // Ephemeral port by default
    const char* dictionaryFile = "/usr/share/dict/words";//default path
    // Flags to check if arguments are repeated
    int maxconnFlag = 0, portFlag = 0, dictFlag = 0;
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "--maxconn") == 0) {//has maxconn
	    if (++maxconnFlag > 1 || ++i >= argc || !check_integer(argv[i]) ||
		    (maxconn = atoi(argv[i])) < 0) {
		fprintf(stderr, "Usage: crackserver [--maxconn connections] "
			"[--port portnum] [--dictionary filename]\n");
		exit(1);
	    }
	} else if (strcmp(argv[i], "--port") == 0) {//has port
	    if (++portFlag > 1 || ++i >= argc || !check_integer(argv[i]) ||
		    (atoi(argv[i]) < 1024 && atoi(argv[i]) != 0) || 
		    atoi(argv[i]) > 65535) {
		fprintf(stderr, "Usage: crackserver [--maxconn connections] "
			"[--port portnum] [--dictionary filename]\n");
		exit(1);
	    }
	    port = argv[i];
	} else if (strcmp(argv[i], "--dictionary") == 0) {//has dictionary
	    if (++dictFlag > 1 || ++i >= argc) {
		fprintf(stderr, "Usage: crackserver [--maxconn connections] "
			"[--port portnum] [--dictionary filename]\n");
		exit(1);
	    }
	    dictionaryFile = argv[i]; //checks dictionaryfile
	    check_dictionary(dictionaryFile);
	} else {
	    fprintf(stderr, "Usage: crackserver [--maxconn connections] "
		    "[--port portnum] [--dictionary filename]\n");
	    exit(1);
	}
    }
    int fdServer = open_listen(port); //listens to port
    process_connections(fdServer); //process the connection
    return 0;
}

/* main
 * --------
 * The main function, runs the check_command function
 *
 * int argc: The argument count 
 * char* argv[]: The argument vector
 * Return: 0 if everything is good
 */
int main(int argc, char* argv[]) {
    check_command(argc, argv);
    return 0;
}

