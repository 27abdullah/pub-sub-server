# Publish/Subscribe System

This program implements a client-server architecture for a publish/subscribe messaging system. The system allows multiple clients to connect to a server, subscribe to specific topics, and publish messages under those topics. Clients receive updates only for topics to which they are subscribed.

## psclient

### Overview

The `psclient` program provides a command-line interface for clients to participate in the publish/subscribe system. Clients can connect to the server, name themselves, subscribe to topics, and publish messages. Additionally, the client will automatically receive and output notifications when new messages are published to topics they are subscribed to.

### Command Line Usage

```bash
./psclient portnum name [topic] ...
```
 
- **portnum** : Mandatory argument specifying the localhost port the server is listening on. It can be either numerical or a named service.
 
- **name** : Mandatory argument specifying the client's name.
 
- **[topic]** : Optional arguments to subscribe the client to specific topics upon connection (e.g., `news`, `weather`).

Example:


```Copy code
./psclient 49152 alice news weather
```

### Behaviour 

- If insufficient arguments are provided, the program will print an error message and exit with status code 1.
 
- The `name` argument must not contain spaces, colons, or newlines. If invalid, the program will emit an error and exit with status code 2.
 
- Any provided `topic` arguments must also not contain spaces, colons, or newlines. If invalid, the program will emit an error and exit with status code 2.

- Duplicated topic strings are allowed but will be ignored by the server if already subscribed.

### Runtime Behaviour 
 
1. **Connect to the server** : The client connects to the server on the specified port.
 
2. **Send client name** : The client sends its name to the server using the `name` command.
 
3. **Subscribe to topics** : If topics are specified on the command line, the client sends subscription requests to the server.
The client then listens for messages from the server and outputs any received messages to `stdout` without additional processing.If the connection to the server is closed, the client prints a message to `stderr` and exits with status code 4.The client also reads input from `stdin` for user commands, sending them to the server as follows: 
- `pub <topic> <value>`: Publish a message `<value>` under the topic `<topic>`.
 
- `sub <topic>`: Subscribe to the topic `<topic>`.
 
- `unsub <topic>`: Unsubscribe from the topic `<topic>`.

### Example Interaction 


```Copy code
$ ./psclient 49152 alice news
sub news
pub news "Breaking news: Market crash"
alice:news:Breaking news: Market crash
unsub news
pub news "Second wave of news"
# No output as alice unsubscribed from news
```

### Error Codes 
 
- **Status 1** : Insufficient command line arguments.
 
- **Status 2** : Invalid name or topic arguments.
 
- **Status 3** : Unable to connect to the server on the specified port.
 
- **Status 4** : Server connection terminated.

## psserver 

### Overview 
The `psserver` program implements the publish/subscribe server. It listens for client connections, processes subscriptions, unsubscriptions, and message publications. The server relays published messages to subscribed clients using TCP.
### Command Line Usage 


```Copy code
./psserver connections [portnum]
```
 
- **connections** : Mandatory argument indicating the maximum number of simultaneous clients. If `0`, there is no limit.
 
- **portnum** : Optional argument specifying the port the server listens on. If omitted or set to `0`, an ephemeral port will be used.

Example:


```Copy code
./psserver 10 49152
```

### Behaviour 

- If the command line arguments are invalid, the program prints an error message and exits with status code 1.

- The server prints the port number it is listening on, then starts accepting client connections.

- For each connected client, the server spawns a new thread to handle that client’s subscription, unsubscription, and message publishing requests.

- The server supports concurrent connections, ensuring mutual exclusion on shared data structures to prevent data corruption.

### Client Commands 
 
- **name <client_name>** : Registers the client with a specific name.
 
- **sub <topic>** : Subscribes the client to the specified topic.
 
- **unsub <topic>** : Unsubscribes the client from the specified topic.
 
- **pub <topic> <value>** : Publishes a message `<value>` under the topic `<topic>`. All clients subscribed to this topic will receive the message.

### Error Handling 

- If the server cannot open the socket for listening, it will print an error message and exit with status code 2.

### Example Interaction 


```Copy code
$ ./psserver 10 49152
Listening on port 49152
# Server starts listening for client connections
```

### Error Codes 
 
- **Status 1** : Invalid command line arguments.
 
- **Status 2** : Unable to open socket for listening.
