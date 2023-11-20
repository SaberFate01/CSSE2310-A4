//Include all the libraries 
#include <netdb.h> 
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*check_server
 * ---------------
 * This function checks the connection with the server, and print errors if
 * there exist any errors in the connection
 *
 * FILE* jobfile: The jobfile 
 * FILE* to: The write end
 * FILE* from: The write end
 *
 * Return: none, as it is void
 * Error: Exit by signal 4 if the connection is terminated
 */
void check_server(FILE* jobfile, FILE* to, FILE* from) {
    char buffer[80];
    while(fgets(buffer, 79, jobfile) != NULL) {
        if (buffer[0] != '\n' && buffer[0] != '#') {
            fprintf(to, "%s", buffer);
            fflush(to);
            if (fgets(buffer, 79, from) == NULL) {
                fprintf(stderr, "crackclient: server connection terminated\n");
                exit(4);
            }
            if (strcmp(buffer, ":invalid\n") == 0) {
                printf("Error in command\n");
            } else if (strcmp(buffer, ":failed\n") == 0) {
                printf("Unable to decrypt\n");
            } else {
                printf("%s", buffer);
            }
        }
    }
}

/* main
 * ----------------
 * argc: the argc
 * argv: the argv
 *
 * The client for the crack server, basically checking all the requirements
 * of the input arguments and connecting to the server via the provided
 * port number, and test if it can open the jobfile.
 *
 * Return: 0 if nothing goes wrong
 * Error: If invalid input format, exit by signal 1
 * If unable to connect to port, exit by signal 3
 * If unable to open job file, exit by signal 2
 * If server terminated, exit by signal 4
 * REF: Most of the code here came from CSSE2310 week 10 lecture, net2.c
 */
int main(int argc, char** argv){
    if (argc < 2 || argc > 3) {//check num of arguments
	fprintf(stderr, "Usage: crackclient portnum [jobfile]\n");
	exit(1); //Exit by 1
    }
    FILE* jobfile;
    if (argc == 3) { //if has jobfile
	jobfile = fopen(argv[2], "r");
	if (jobfile == NULL) {//jobfile cant be openned
	    fprintf(stderr, "crackclient: unable to open job file \"%s\"\n",
		    argv[2]);
	    exit(2);//exit by 2
	}
    } else {
	jobfile = stdin;//temporarily set jobfile to stdin
    }
    const char* port = argv[1]; //get port num
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;        // IPv4, for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    int err;
    if ((err = getaddrinfo("localhost", port, &hints, &ai))) {//check localhost
	freeaddrinfo(ai);
	fprintf(stderr, "crackclient: unable to connect to port %s\n", port);
	exit(3); //exit by 3
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0); // 0 == use default protocol
    if (connect(fd, ai->ai_addr, ai->ai_addrlen)) {
	fprintf(stderr, "crackclient: unable to connect to port %s\n", port);
	exit(3);//exit by 3, cant connect to server
    }
    int fd2 = dup(fd);
    FILE* to = fdopen(fd, "w");//write end
    FILE* from = fdopen(fd2, "r");//read end
    check_server(jobfile, to, from);// runs check_server
    fclose(to); //close the two ends
    fclose(from);
    if (argc == 3) {
	fclose(jobfile);
    }//close jobfile
    return 0;
}
