#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <csse2310a3.h>  

#define NOT_ENOUGH_ARGS_EXIT 1
#define NAME_POSITION 2
#define INVALID_NAME_EXIT 2
#define INVALID_TOPIC_EXIT 2
#define INVALID_PORT_EXIT 3
#define FIRST_TOPIC_POSITION 3
#define PORT_POSITION 1
#define MIN_ARGS 3
#define BUFFER_SIZE 255
#define DEFAULT_PROTOCOL 0
#define SUCCESSFUL_EXIT 0    
#define CONNECTION_CLOSED_EXIT 4

//Struct holds IO file streams that connect it with server
typedef struct {
    FILE* out;
    FILE* in;
} InOut;

void validate_args(int argc, char** argv);
bool is_invalid_name_topic(char* name);
void initial_communication(int argc, char** argv, FILE* out);
void setup_connection(char* port, InOut* inOut);
void* handle_out(void* arg);
void setup_connection(char* port, InOut* inOut);
void connection_error(char* port);
 
int main(int argc, char** argv) {
    InOut inOut;
    validate_args(argc, argv);
    setup_connection(argv[PORT_POSITION], &inOut);
    initial_communication(argc, argv, inOut.out);

    //Listen on stdin and send to server
    pthread_t tid;
    pthread_create(&tid, NULL, handle_out, &inOut);  
    pthread_detach(tid);
    
    //Listen to socket and send to stdin 
    char* buffer;
    while ((buffer = read_line(inOut.in)) != NULL) {
        printf(buffer);
        printf("\n");
        fflush(stdout);
        free(buffer);
    } 

    fprintf(stderr, "psclient: server connection terminated\n");
    fclose(inOut.in);
    return CONNECTION_CLOSED_EXIT;
}

/* validate_args()
 * ---------------
 * Validates the command line arguements
 *
 * argc: the number of arguements from the command line
 *
 * argv: the command line arguements
 *
 * Errors: exits if not enough arguements with NOT_ENOUGH_ARGS_EXIT (1),
 * exits with INVALID_NAME_EXIT (2) if invalid name, or INVALID_TOPIC_EXIT (2)
 */
void validate_args(int argc, char** argv) {
    //Validate arg count
    if (argc < MIN_ARGS) {
        fprintf(stderr, "Usage: psclient portnum name [topic] ...\n");
        exit(NOT_ENOUGH_ARGS_EXIT);
    }

    //Validate name
    if (is_invalid_name_topic(argv[NAME_POSITION])) {
        fprintf(stderr, "psclient: invalid name\n");
        exit(INVALID_NAME_EXIT);
    }

    //Validate listed topics
    for (int i = FIRST_TOPIC_POSITION; i < argc; i++) {
        if (is_invalid_name_topic(argv[i])) {
            fprintf(stderr, "psclient: invalid topic\n");
            exit(INVALID_TOPIC_EXIT);
        }
    }
}

/* setup_connetions()
 * ------------------
 * Establishes a connection with the server
 *
 * port: the port that the server is listening on
 *
 * inOurt: a pointer to the InOut struct that will be populated
 *
 * Errors: exits with INVALID_PORT_EXIT (3) if the connection fails.
 * References: CSSE2310 week 10 lecture code - net2.c
 */
void setup_connection(char* port, InOut* inOut) {
    struct addrinfo* results = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int err;
    if ((err = getaddrinfo("localhost", port, &hints, &results))) {
        freeaddrinfo(results);
        connection_error(port);
    }

    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL)) < 0) {
        connection_error(port);
    }
    
    if (connect(fd, (struct sockaddr*) results->ai_addr, 
            sizeof(struct sockaddr))) {
        connection_error(port);
    }
    
    //Seperate into one read and one write FILE*
    int fdCopy = dup(fd);
    inOut->out = fdopen(fd, "w");
    inOut->in = fdopen(fdCopy, "r");
}

/* connection_error()
 * ------------------
 * Performs required procedure for connection error
 *
 * port: the port trying to be connected to
 *
 * Errors: exits with INVALID_PORT_EXIT (3)
 */
void connection_error(char* port) {
    fprintf(stderr, "psclient: unable to connect to port %s\n", port);
    exit(INVALID_PORT_EXIT);
}

/* in_invalid_name_topic()
 * -----------------------
 * Checks if name argument is an invalid name or topic
 *
 * line: the string to be checked
 *
 * Returns: true if the input argument is valid and false otherwise
 */
bool is_invalid_name_topic(char* line) {
    return (strlen(line) == 0 || strchr(line, ' ') || strchr(line, ':') || 
            strchr(line, '\n') || !strcmp(line, ""));
}

/* initial_communication()
 * -----------------------
 * Sends the inital communication information to the server i.e., the name and
 * the sub topics
 *
 * argc: the number of command line arguments
 *
 * argv: the command line arguements
 */
void initial_communication(int argc, char** argv, FILE* out) {
    fprintf(out, "name %s\n", argv[NAME_POSITION]);
    fflush(out);

    //Subscribe to prelisted topics
    for (int i = FIRST_TOPIC_POSITION; i < argc; i++) {
        fprintf(out, "sub %s\n", argv[i]);
        fflush(out);
    }
}

/* handle_out()
 * ------------
 * Handles the output that the client sends to the server
 *
 * arg: a pointer to an InOut struct holds connections to the server
 */
void* handle_out(void* arg) {
    InOut* inOut = (InOut*) arg;
    FILE* out = inOut->out;
    char* buffer;

    while((buffer = read_line(stdin)) != NULL) {
        fprintf(out, buffer);
        fprintf(out, "\n");
        fflush(out);
        free(buffer);
    }
    fclose(out);
    exit(SUCCESSFUL_EXIT);
}
