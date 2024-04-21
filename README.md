# pub-sub-server
Client and server following publish/subscribe architecture. Completed for the course CSSE2310 (a4).

Written to specification "CSSE2310-7231 A4 specification - 2022 sem2 v1.1.pdf" which was supplied by the CSSE2310 course. The full spec is provided in the repository. A summary of the spec is provided below.

## Introduction
You are to create two programs which together implement a distributed communication architecture known as publish/subscribe. One program – psserver – is a network server which accepts connections from clients (including psclient, which you will implement). Clients connect, and tell the server that they wish to subscribe to notifications on certain topics. Clients can also publish values (strings) on certain topics. The server will relay a message to all subscribers of a particular topic. Communication between the psclient and psserver is over TCP using a newline-terminated text command protocol. 

## psclient
The psclient program provides a commandline interface that allows you to participate in the publish/subscribe system as a client, connecting to the server, naming the client, subscribing to and publishing topics. psclient will also output notifications when topics to which that client is subscribed, get published.

## psserver
psserver is a networked publish/subscribe server, allowing clients to connect, name themselves, subscribe to and unsubscribe from topics, and publish messages setting values for topics. All communication between clients and the server is over TCP using a simple command protocol that will be described in a later section.
