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

struct terminal
{
    int id;
    string name;
    int socket;
    thread th;
    bool active;

    terminal() : active(true) {}
};

vector<terminal> clients;
string def_col = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};
int seed = 0;
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

    if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
    {
        perror("bind error: ");
        exit(-1);
    }

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
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
        {
            perror("accept error: ");
            exit(-1);
        }
        seed++;
        thread t(handle_client, client_socket, seed);
        lock_guard<mutex> guard(clients_mtx);
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

void end_connection(int id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            lock_guard<mutex> guard(clients_mtx);
            clients[i].th.detach();
            clients[i].active = false;
            clients.erase(clients.begin() + i);
            close(clients[i].socket);
            break;
        }
    }
}

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