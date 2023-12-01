#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include <algorithm>

#define MAX_LEN 200
#define NUM_COLORS 6
using namespace std;
//to store information about connected clients
struct terminal
{
    int id;
    string name;
    int socket;
    thread th;
    bool active;

    terminal() : active(true) {}
};
//used to maintain a list of connected clients
vector<terminal> clients;
string def_col = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};
int seed = 0;
//is used to synchronize access to the standard output (cout). This ensures that multiple threads don't interfere with console printing
mutex cout_mtx, clients_mtx;
condition_variable cv;

string color(int code);
void set_name(int id, const string &name);
void shared_print(const string &str, bool endLine = true);
void broadcast_message(const string &message, int sender_id);
void broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);

int main()
{
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket: ");
        exit(-1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(0); // Use dynamic port allocation
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 0);
    // binds the socket to the address and port number specified
//It binds to an available port using the bind() function with a dynamic port allocation (0), allowing the operating system to assign a free port.
    if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
    {
        perror("bind error: ");
        exit(-1);
    }
//The server listens for incoming connections with a maximum queue length of 8 using listen().
    if ((listen(server_socket, 8)) == -1)
    {
        perror("listen error: ");
        exit(-1);
    }

    struct sockaddr_in client;
    int client_socket;
    unsigned int len = sizeof(sockaddr_in);

    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl
         << def_col;

    while (1)
    {
        //extracts the first connection request on the queue of pending connections for the listening socket, sockfd, creates a new connected socket, and returns a new file descriptor referring to that socket. At this point, the connection is established between client and server, and they are ready to transfer data.
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
        {
            perror("accept error: ");
            exit(-1);
        }
        seed++;
        thread t(handle_client, client_socket, seed);
        // Locks the mutex
    // Critical section: Access and modify shared resources
        lock_guard<mutex> guard(clients_mtx);
   // lock_guard goes out of scope and automatically unlocks the mutex
        clients.push_back({seed, "Anonymous", client_socket, move(t)});
    }

    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].th.joinable())
            clients[i].th.join();
    }

    close(server_socket);
    return 0;
}

string color(int code)
{
    return colors[code % NUM_COLORS];
}
//The set_name function sets the name for a client based on their unique ID.
void set_name(int id, const string &name)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            clients[i].name = name;
        }
    }
}

void shared_print(const string &str, bool endLine = true)
{
    lock_guard<mutex> guard(cout_mtx);
    cout << str;
    if (endLine)
        cout << endl;
}

void broadcast_message(const string &message, int sender_id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, message.c_str(), message.size(), 0);
        }
    }
}

// Two broadcast_message functions are used to send messages to all clients. One sends text messages, and the other sends integer values.
// These functions iterate through the clients vector and send messages to all clients except the sender.
void broadcast_message(int num, int sender_id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, &num, sizeof(num), 0);
        }
    }
}
//The end_connection function is responsible for disconnecting a client, stopping their thread, and updating the clients vector.
void end_connection(int id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            //Throughout the code, lock_guard is used with mutexes to ensure thread safety. This ensures that only one thread can access shared resources or critical sections at a time.
            lock_guard<mutex> guard(clients_mtx);
            clients[i].th.detach();
            clients[i].active = false;
            clients.erase(clients.begin() + i);
            close(clients[i].socket);
            break;
        }
    }
}

// The handle_client function is executed in a separate thread for each connected client. It manages communication with the client.
// The function starts by receiving the client's name, displays a welcome message to all clients, and handles incoming and outgoing messages.
// Messages are broadcasted to all connected clients except the sender.

void handle_client(int client_socket, int id)
{
    char name[MAX_LEN], str[MAX_LEN];
    int bytes_received;
    {
        lock_guard<mutex> guard(cout_mtx);
        cout << "New client connected with ID: " << id << endl;
    }
    bytes_received = recv(client_socket, name, sizeof(name), 0);
    if (bytes_received <= 0)
    {
        end_connection(id);
        return;
    }
    name[bytes_received] = '\0';
    set_name(id, name);

    // Display welcome message
    string welcome_message = name + string(" has joined");
    {
        lock_guard<mutex> guard(clients_mtx);
        for (const auto &client : clients)
        {
            if (client.id != id && client.active)
            {
                string msg = "#NULL";
                send(client.socket, msg.c_str(), msg.size(), 0);
                broadcast_message(client.socket, id);
                send(client.socket, welcome_message.c_str(), welcome_message.size(), 0);
            }
        }
    }
    shared_print(color(id) + welcome_message + def_col);

    while (1)
    {
        bytes_received = recv(client_socket, str, sizeof(str), 0);
        if (bytes_received <= 0)
        {
            break;
        }
        str[bytes_received] = '\0';

        if (strcmp(str, "#exit") == 0)
        {
            // Display leaving message
            string message = name + string(" has left");
            {
                lock_guard<mutex> guard(clients_mtx);
                for (const auto &client : clients)
                {
                    if (client.id != id && client.active)
                    {
                        string msg = "#NULL";
                        send(client.socket, msg.c_str(), msg.size(), 0);
                        broadcast_message(client.socket, id);
                        send(client.socket, message.c_str(), message.size(), 0);
                    }
                }
            }
            shared_print(color(id) + message + def_col);
            end_connection(id);
            return;
        }

        lock_guard<mutex> guard(clients_mtx);
        for (const auto &client : clients)
        {
            if (client.id != id && client.active)
            {
                string msg = "#NULL";
                send(client.socket, msg.c_str(), msg.size(), 0);
                broadcast_message(client.socket, id);
                send(client.socket, name, name, sizeof(name), 0);
                send(client.socket, str, sizeof(str), 0);
            }
        }
    }
