#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <csse2310a3.h>
#include <csse2310a4.h>
#include <string.h>
#include <stringmap.h>
#include "clientList.h"
#include <semaphore.h>
#include <signal.h>

#define INITIAL_CLIENTS_SIZE 5
#define SUBSCRIBE 0
#define UNSUBSCRIBE 1
#define PUBLISH 2
#define INVALID_COMMAND -1
#define INITIAL_LIST_SIZE 1
#define INVALID_FORMAT_EXIT 1
#define CONNECTION_ERROR_EXIT 2
#define MIN_ARG_COUNT 2
#define MAX_ARG_COUNT 3
#define CONNECTIONS_POS 1
#define PORT_POS 2
#define MIN_PORT_LIMIT 1024
#define MAX_PORT_LIMIT 65535
#define NAME_POS 1
#define TOPIC_POS 1
#define IGNORE 3

//Struct stores command line argument information
typedef struct {
    int connections;
    char* port;
} Params;

//Struct stores the stats of the psserver
typedef struct {
    int currentClientCount;
    int completedClients;
    int pubCount;
    int subCount;
    int unsubCount;
    int maxClients;
    sem_t* guard;
    sigset_t* set;
    pthread_mutex_t* lockStat;
} Stats;

//Stores the data required for one client thread
typedef struct {
    Client* client;
    StringMap* map;
    pthread_mutex_t* lock;
    Stats* stats;
} ClientThreadInfo;

void init_stats(Stats* stats, int maxClients);
void validate_commands(int argc, char** argv, Params* params);
bool is_valid_port(char* port);
bool is_non_neg_int(char* value);
void invalid_format();
int open_listen(Params* params);
void process_connections(int fdServer, Stats* stats, StringMap* map, 
        pthread_mutex_t* lock);
void* client_thread(void* arg);
void subscribe(ClientThreadInfo* cti, char* topic);
void unsubscribe(ClientThreadInfo* cti, char* topic);
void publish(ClientThreadInfo* cti, char* buffer);
int validate_cmd(char* cmd);
int count_args(char** args);
bool is_name(char* line);
void connection_error();
void clean_up_client(ClientThreadInfo* cti);
void* signal_handler(void* arg);

int main(int argc, char** argv) {
    Params params;
    validate_commands(argc, argv, &params);
    int fdServer = open_listen(&params);

    //Setup lock to protect client list
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_t lockStat;
    pthread_mutex_init(&lockStat, NULL);

    StringMap* map = stringmap_init();

    Stats stats;
    init_stats(&stats, params.connections);

    //Create thread that handles signal
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    stats.set = &set;
    stats.lockStat = &lockStat;

    pthread_t threadId;
    pthread_create(&threadId, NULL, signal_handler, &stats);
    pthread_detach(threadId);
    
    //Semaphore to limit max clients
    if (stats.maxClients != 0) {
        sem_t guard;
        sem_init(&guard, 0, stats.maxClients);
        stats.guard = &guard;
    }

    process_connections(fdServer, &stats, map, &lock); 
    pthread_exit(0);
}

/* signal_handler()
 * ----------------
 * Prints statistics upon SIGHUP about server
 */
void* signal_handler(void* arg) {
    Stats* stats = arg;
    int sig;
    while (true) {
        sigwait(stats->set, &sig);
        pthread_mutex_lock(stats->lockStat);
        fprintf(stderr, "Connected clients:%d\n", stats->currentClientCount);
        fprintf(stderr, "Completed clients:%d\n", stats->completedClients);
        fprintf(stderr, "pub operations:%d\n", stats->pubCount);
        fprintf(stderr, "sub operations:%d\n", stats->subCount);
        fprintf(stderr, "unsub operations:%d\n", stats->unsubCount);
        fflush(stderr);
        pthread_mutex_unlock(stats->lockStat);
    }
}

/* init_stats()
 * ------------
 * Sets up the requred variable values for a Stats struct
 *
 * stats: a pointer to the Stats struct to set up
 *
 * maxClient: the maximum number of clients the server can have a connection
 * with
 */
void init_stats(Stats* stats, int maxClients) {
    stats->currentClientCount = 0;
    stats->completedClients = 0;
    stats->pubCount = 0;
    stats->subCount = 0;
    stats->unsubCount = 0;
    stats->maxClients = maxClients;
}

/* validate_commands()
 * -------------------
 * Checks whether the command line arguments are valid for the server. 
 * Populates a Params struct with command line information if successful.
 *
 * argc: the number of command line arguments
 *
 * argv: the command line arguements
 *
 * params: a pointer to the Params struct to populate
 *
 * Errors: will exit if the command line arguments are in an invalid format
 * with exit status 1
 */
void validate_commands(int argc, char** argv, Params* params) {
    //Validate arguments
    if (argc < MIN_ARG_COUNT || argc > MAX_ARG_COUNT || 
            !is_non_neg_int(argv[CONNECTIONS_POS]) || 
            (argc == MAX_ARG_COUNT && !is_valid_port(argv[PORT_POS]))) {
        invalid_format();
    } 
    
    //Populate params
    if (argc == MAX_ARG_COUNT) {
        params->port = argv[PORT_POS];
    } else {
        params->port = "0";
    }
    params->connections = atoi(argv[CONNECTIONS_POS]);
}

/* is_valid_port()
 * ---------------
 * Returns true if the input is a valid port number i.e., a number that is
 * 0, or between 1024 and 65535. False otherwise.
 *
 * port: the port number
 *  
 * Returns: true if a valid port and false otherwise.
 */
bool is_valid_port(char* port) {
    int portValue = atoi(port);
    return is_non_neg_int(port) && (portValue == 0 || 
            (portValue >= MIN_PORT_LIMIT && portValue <= MAX_PORT_LIMIT));
}

/* is_non_neg_int()
 * ---------------
 * Returns true if input argument is a non negative integer and returns false
 * otherwise.
 *
 * value: the value to be checked
 *
 * Returns: true if the value arguement is a non-negative interger. False 
 * otherwise.
 */
bool is_non_neg_int(char* value) {
    int integer = atoi(value);
    if (integer == 0) {
        return !strcmp(value, "0");
    }
    return integer >= 0;
}

/* invalid_format()
 * ----------------
 * Performs required procedure if an invalid format is encountered
 *
 * Errors: returns with exit status INVALID_FORMAT_ERROR (1)
 */
void invalid_format() {
    fprintf(stderr, "Usage: psserver connections [portnum]\n");
    exit(INVALID_FORMAT_EXIT);
}

/* open_listen()
 * -------------
 * Gets server to listen on the required port with the correct IPv4 
 * configuration
 *
 * params: the command line argument Param struct
 *
 * Errors: if the port cannot be listened on, the system will exit with 
 * a connection error i.e., CONNECTION_ERROR_EXIT (2);
 *
 * Reference: CSSE2310 week 10 lecture code - server-multithreaded.c
 *
 * Returns: the integer of the socket that the server will be listening on
 */
int open_listen(Params* params) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err;
    if ((err = getaddrinfo(NULL, params->port, &hints, &ai))) {
        freeaddrinfo(ai);
        connection_error();
    }

    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    int optVal = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &optVal, 
            sizeof(int)) < 0) {
        connection_error();
    }
    
    if (bind(listenFd, (struct sockaddr*) ai->ai_addr, sizeof(struct sockaddr))
            < 0) {
        connection_error();
    }

    if (listen(listenFd, params->connections) < 0) {
        connection_error();
    }
    
    //Get port used if unknown and print it ;
    if (!strcmp(params->port, "0")) {
        struct sockaddr_in ad;
        memset(&ad, 0, sizeof(struct sockaddr_in));
        socklen_t len = sizeof(struct sockaddr_in);
        if (getsockname(listenFd, (struct sockaddr*) &ad, &len)) {
            connection_error();
        }
        int port = ad.sin_port;
        fprintf(stderr, "%d\n", ntohs(port));
    } else {
        fprintf(stderr, "%s\n", params->port);
    }
    fflush(stderr);
    return listenFd;
}

/* connection_error()
 * ------------------
 * Performs the required procedure for a connection error
 *
 * Errors: exits with the CONNECTION_ERROR_EXIT code (2)
 */
void connection_error() {
    fprintf(stderr, "psserver: unable to open socket for listening\n");
    exit(CONNECTION_ERROR_EXIT);
}

/* process_connections()
 * ---------------------
 * Waits on clients to connect and then spawns a client thread to deal
 * with them
 *
 * fdServer: the socket the server is accepting from
 * 
 * stats: a pointer to the Stats struct for this server
 *
 * map: a pointer to a string map that holds informatino on which client is 
 * subscribed to what topic
 *
 * lock: the mutex lock for the string map
 */
void process_connections(int fdServer, Stats* stats, StringMap* map, 
        pthread_mutex_t* lock) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;

    while(true) {
        //Wait on client
        fromAddrSize = sizeof(struct sockaddr_in);
        
        //Limit max clients if necessary
        if (stats->maxClients != 0) {
            sem_wait(stats->guard);
        }

        fd = accept(fdServer, (struct sockaddr*) &fromAddr, &fromAddrSize);
        if (fd < 0) {
            continue;
        }

        //Update stats
        pthread_mutex_lock(stats->lockStat);
        stats->currentClientCount++;
        pthread_mutex_unlock(stats->lockStat);

        //Setup Client struct for new client
        Client* client = malloc(sizeof(Client));
        int fdCopy = dup(fd);
        client->in = fdopen(fd, "r");
        client->out = fdopen(fdCopy, "w");
        client->hasName = false;

        //Setup ClientThreadInfo for new client
        ClientThreadInfo* cti = malloc(sizeof(ClientThreadInfo));
        cti->client = client;
        cti->map = map;
        cti->lock = lock;
        cti->stats = stats;
        
        pthread_t threadId;
        pthread_create(&threadId, NULL, client_thread, cti);
        pthread_detach(threadId);
    }
}

/* client_thread()
 * ---------------
 * The thread that handles the client for its life span. Will call clean up 
 * function for client when it disconnects.
 *
 * arg: a pointer to a ClientThreadInfo struct
 */
void* client_thread(void* arg) {
    ClientThreadInfo* cti = arg;
    Client* client = cti->client;
    char* buffer;
    while ((buffer = read_line(client->in)) != NULL) {
        //Get name, if not name reject and wait for name
        if (!client->hasName) {
            if (is_name(buffer)) {
                char* name = split_by_char(buffer, ' ', 0)[NAME_POS];
                client->name = strdup(name);
                client->hasName = true;
                continue;
            } else {
                free(buffer);
                continue;
            }
        }

        //Handle commands
        pthread_mutex_lock(cti->lock);
        pthread_mutex_lock(cti->stats->lockStat);
        switch (validate_cmd(buffer)) {
            case SUBSCRIBE:
                cti->stats->subCount++;
                subscribe(cti, split_by_char(buffer, ' ', 0)[TOPIC_POS]);
                break;
            case UNSUBSCRIBE:
                unsubscribe(cti, split_by_char(buffer, ' ', 0)[TOPIC_POS]);
                break;
            case PUBLISH:
                cti->stats->pubCount++;
                publish(cti, buffer);
                break;
            case IGNORE:
                break;
            default:
                fprintf(client->out, ":invalid\n");
                fflush(client->out);
        }
        pthread_mutex_unlock(cti->stats->lockStat);
        pthread_mutex_unlock(cti->lock);
        free(buffer);
    }

    if (client->hasName) {
        clean_up_client(cti);
    }
    return NULL;
}

/* clean_up_client()
 * -----------------
 * Performs freeing, and closing of IO streams for the client. Also 
 * unsubscribes them from all their topics
 * 
 * cti: a pointer to the ClientThreadInfo struct that holds info on
 * client to be cleaned up
 */
void clean_up_client(ClientThreadInfo* cti) {
    pthread_mutex_lock(cti->lock);

    //Create list to unsubscribe from
    StringMapItem* itemMap = NULL;
    char** unsubList = malloc(sizeof(char*) * INITIAL_LIST_SIZE);
    int count = 0;
    int size = INITIAL_LIST_SIZE;

    while ((itemMap = stringmap_iterate(cti->map, itemMap))) {
        if (count == size) {
            size *= 2;
            unsubList = realloc(unsubList, size * sizeof(char*));
        }
        unsubList[count] = strdup(itemMap->key);
        count++;
    }

    //Unsubscribe from list
    for (int i = 0; i < count; i++) {
        unsubscribe(cti, unsubList[i]);
        free(unsubList[i]);
    }
    free(unsubList);
    
    //Clean up client struct
    free(cti->client->name);
    fclose(cti->client->in);
    fclose(cti->client->out);
    free(cti->client);
    free(cti);

    //Update stats and client allowance
    pthread_mutex_lock(cti->stats->lockStat);
    cti->stats->currentClientCount--;
    cti->stats->completedClients++;
    if (cti->stats->maxClients != 0) {
        sem_post(cti->stats->guard);
    }
    pthread_mutex_unlock(cti->stats->lockStat);
    pthread_mutex_unlock(cti->lock);
}

/* count_args()
 * ------------
 * Counts the number of arguments in a string array.
 *
 * args: the string array to be counted
 *
 * Returns: the number of arguments in args
 */
int count_args(char** args) {
    int count = 0;
    while (args[count]) {
        count++;
    }
    return count;
}

/* is_name()
 * ---------
 * Verifies if the input line argument is in the correct form of a name
 *
 * line: the line to be checked for whether it is a name
 *
 * Returns: true if the line argument is a name and false otherwise.
 */
bool is_name(char* line) {
    char* lineCopy = strdup(line); 
    char** args = split_by_char(lineCopy, ' ', 0);
    int count = count_args(args);
    bool result = (count == 2) && !strcmp(args[0], "name") && 
        strlen(args[1]) != 0 && !strchr(args[1], ':');
    free(lineCopy);
    return result;
}

/* validate_cmd()
 * --------------
 * Validates whether the cmd argument is a valid command from a client
 *
 * cmd: the command to be checked
 *
 * Returns: -1 if invalid and UNSUBSCRIBE (1) for unsub, SUBSCRIBE (0) for sub,
 * PUBLISH (2) for pub, and IGNORE (-2) for a name;
 */
int validate_cmd(char* cmd) {
    char* cmdCopy = strdup(cmd);
    char** args = split_by_char(cmdCopy, ' ', 0);
    int count = count_args(args);
    int result = INVALID_COMMAND;

    //Name
    if (count == 2 && !strcmp(args[0], "name") && is_name(args[1])) {
        result = IGNORE;
    }

    //Subscribe, unsubscribe
    if (count == 2 && strcmp(args[1], "") && !strchr(args[1], ':')) {
        if (!strcmp(args[0], "sub")) {
            result = SUBSCRIBE;
        } else if (!strcmp(args[0], "unsub")) {
            result = UNSUBSCRIBE;
        } 
    } 

    //Publish
    if (count > 2 && !strcmp(args[0], "pub") && !strchr(args[1], ':') && 
            !strchr(args[2], ':') && strcmp(args[1], "") && 
            strcmp(args[2], "")) {
        result = PUBLISH;
    }
    free(cmdCopy);
    return result;
}

/* subscribe()
 * -----------
 * Performs a subcribe for the client on the given topic
 * 
 * cti: pointer to ClientThreadInfo struct that desccribes client doing the 
 * sub
 *
 * topic: topic being subscribed to
 */
void subscribe(ClientThreadInfo* cti, char* topic) {
    Node* node = stringmap_search(cti->map, topic);
    if (!node) {
        //Topic not in map - create list and add to map
        node = init_client_list(cti->client);
        stringmap_add(cti->map, topic, node);
    } else if (!in_list(node, cti->client)) {
        //Topic in map and client not in list - add name to list
        add_client(node, cti->client);
    } 
}

/* unsubscribe()
 * -------------
 * Unsubcribes client from the given topic
 * 
 * cti: a pointer to a ClientThreadStruct describing the client who is 
 * unsubscribing
 *
 * topic: the topic being unsubscribed to
 */
void unsubscribe(ClientThreadInfo* cti, char* topic) {
    Node* node = stringmap_search(cti->map, topic); 
    //Do nothing if topic does not exist or client not subbed to topic
    if (!node || !in_list(node, cti->client)) {
        return;
    }

    bool deleteTopic = is_last_client(node);
    
    //If deleting first node, set cti->map to point to new first node
    Node* newFirstNode = delete_client(node, cti->client);
    if (newFirstNode) {
        stringmap_remove(cti->map, topic);
        stringmap_add(cti->map, topic, newFirstNode);
    }
    
    //If last client in clientList, remove topic from map 
    if (deleteTopic) {
        stringmap_remove(cti->map, topic);
    }

    //Update stats
    cti->stats->unsubCount++;
}

/* publish()
 * ---------
 * Publishes the text that the client sends for a specific topic
 *
 * cti: a pointer to a ClientThreadInfo struct that describes the client
 * sending the text
 *
 * buffer: the string containing the text that the client sent
 */
void publish(ClientThreadInfo* cti, char* buffer) {
    char** args = split_by_char(buffer, ' ', 3);
    char* topic = args[1];
    char* value = args[2];
    
    Node* node = stringmap_search(cti->map, topic);
    if (node) {
        do {
            fprintf(node->client->out, "%s:%s:%s\n", cti->client->name, 
                    topic, value);    
            fflush(node->client->out);
        } while (node = node->next, node);
    }
}

