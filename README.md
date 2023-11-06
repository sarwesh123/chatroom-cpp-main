# Chatroom Application

A chatroom built in C++ using the concepts of socket programming and multi-threading. It supports chatting among multiple clients.

1 Socket Programming

The code utilizes socket programming to establish communication between the server and multiple clients.
The server listens on a socket for incoming client connections using the socket, bind, and listen functions.
Clients connect to the server using the socket and connect functions.
Communication between the server and clients is established through sockets, allowing data to be sent and received.

2 Multi-threading

Multi-threading is used to handle multiple clients concurrently. Each client connection is handled in a separate thread.
The std::thread library is used to create and manage threads for sending and receiving messages.

3 Colors

ANSI escape codes for text color are used to differentiate between different users in the chatroom. Different colors are used for different clients.
The colors array is used to store ANSI escape codes for text color.

4 Signal Handling:

The code handles the Ctrl+C signal using the signal function. When a Ctrl+C is detected, the server and client threads are gracefully terminated, and the sockets are closed


5 Main Function (Server):

The server's main function sets up a socket, binds to a port, and listens for incoming client connections in an infinite loop.
When a new client connects, it is assigned a unique ID, and a new thread is created to handle its communication.

Handle Client Function (Server):

The handle_client function is executed in a separate thread for each connected client.
It manages communication with the client, including receiving and broadcasting messages, setting client names, and handling client disconnections.

Shared Printing:

The shared_print function ensures that messages are printed to the console in a thread-safe manner by using a mutex.
Broadcasting Messages:

The server broadcasts messages to all connected clients to ensure that messages are seen by everyone except the sender.
Messages are sent to clients using the send function with the client's socket.

Client (Client.cpp):

The client connects to the server and provides a name. It then enters a loop to send and receive messages.
If the client types #exit, it gracefully exits the chatroom.
Color Function (Both):

The color function is used to select a color for the client's messages based on its unique ID.

Erase Text Function (Client.cpp):

The eraseText function is used to clear text on the client's terminal, providing a cleaner chatroom interface.

Ctrl+C Handling (Client.cpp):

The catch_ctrl_c function is invoked when the client presses Ctrl+C. It sends an exit message to the server and gracefully terminates.

![](/screenshot.png)
## How to run

1. Clone this repository
2. Run the following commands in your terminal :
```
g++ server.cpp -lpthread -o server
g++ client.cpp -lpthread -o client
```
3. To run the server application, use this command in the terminal :
```
./server
```

4. Now, open another terminal and use this command to run the client application :
```
./client
```

5. For opening multiple client applications, repeat step 4.