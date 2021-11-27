#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <queue>
#include <map>
#include <vector>
/////////////////////////////
#include <iostream>
#include <assert.h>
#include <tuple>
using namespace std;
/////////////////////////////

// Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

///////////////////////////////
#define MAX_CLIENTS 100
#define MAX_THREADS 100
#define PORT_ARG 8001

const int initial_msg_len = 256;

////////////////////////////////////

const LL buff_sz = 1048576;
///////////////////////////////////////////////////

vector<pair<int, string>> dictionary;

queue<int> clients_queue;
// initialise a thread pool or better declare

pthread_t thread_pool[MAX_THREADS];

// need 2 mutex lock and 1 cond variable
// 2 mutex -- 1 for queue and other for dictionary
// cond variable so that worker threads don't do busy waiting

pthread_mutex_t queue_mutex;
pthread_mutex_t printing_mutex;
pthread_mutex_t dictionary_mutex;
pthread_cond_t queue_cond;

void mutex_initializer()
{
    pthread_mutex_init(&(queue_mutex), NULL);
    pthread_mutex_init(&(dictionary_mutex), NULL);
    pthread_cond_init(&(queue_cond), NULL);
    pthread_mutex_init(&printing_mutex, NULL);
}
pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);
    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

///////////////////////////////
int find_in_dict(int key_given)
{
    for (int i = 0; i < dictionary.size(); i++)
    {
        if (dictionary[i].first == key_given)
        {
            return i;
        }
    }
    return -1;
}
int last_to_add()
{
    for (int i = 0; i < dictionary.size(); i++)
    {
        if (dictionary[i].first == -1)
        {
            return i;
        }
    }
    return 0;
}

void handle_connection(int client_socket_fd)
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    //####################################################

    int received_num, sent_num, received_num2;

    /* read message from client */
    int ret_val = 1;

    while (true)
    {
        string cmd;
        pthread_mutex_lock(&printing_mutex);
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        pthread_mutex_unlock(&printing_mutex);
        if (received_num <= 0)
        {
            // perror("Error read()");
            printf("Server could not read msg sent from client\n");
            goto close_client_socket_ceremony;
        }
        printf("Client sent : %s\n", cmd.c_str());
        if (cmd == "exit")
        {
            printf("Exit pressed by client");
            goto close_client_socket_ceremony;
        }
        string msg_to_send_back = "Ack: ";
        if (cmd[0] == 'i')
        {
            string key_given;
            string value_given;
            // insert wala hai
            // 7 - lekar ; tak key given hogi
            int starter = 7;
            while (cmd[starter] != ';')
            {
                key_given.push_back(cmd[starter]);
                starter++;
            }
            starter++;
            while (starter < cmd.length())
            {
                value_given.push_back(cmd[starter]);
                starter++;
            }
            printf("reached here ");
            printf("%s %s\n", key_given.c_str(), value_given.c_str());
            int key_given_actuall = stoi(key_given);
            printf("%d %s\n", key_given_actuall, value_given.c_str());
            pthread_mutex_lock(&dictionary_mutex);
            printf("One more check\n");
            int cheker = find_in_dict(key_given_actuall);
            if (cheker == -1)
            {
                dictionary.push_back({key_given_actuall, value_given});
                msg_to_send_back = "Insertion successful";
            }
            else
            {
                printf("check\n");
                msg_to_send_back = "Key already exists";
            }
            pthread_mutex_unlock(&dictionary_mutex);
        }
        else if (cmd[0] == 'c')
        {
            // insert wala hai
            // 7 - lekar ; tak key given hogi
            string key_given;
            string value_given;
            int starter = 7;
            while (cmd[starter] != ';')
            {
                key_given.push_back(cmd[starter]);
                starter++;
            }
            starter++;
            while (starter < cmd.length())
            {
                value_given.push_back(cmd[starter]);
                starter++;
            }
            printf("reached here ");
            printf("%s %s\n", key_given.c_str(), value_given.c_str());
            int key_given_1 = stoi(key_given);
            int key_given_2 = stoi(value_given);
            printf("%d %d\n", key_given_1, key_given_2);
            pthread_mutex_lock(&dictionary_mutex);
            printf("One more check\n");
            int cheker_1 = find_in_dict(key_given_1);
            int cheker_2 = find_in_dict(key_given_2);
            if (cheker_1 == -1 || cheker_2 == -1)
            {
                msg_to_send_back = "Concat failed as at least one of the keys does not exist";
            }
            else
            {
                string a = dictionary[cheker_1].second;
                string b = dictionary[cheker_2].second;
                dictionary[cheker_1].second = a+b;
                dictionary[cheker_2].second = b+a;
                msg_to_send_back = dictionary[cheker_2].second;
            }
            pthread_mutex_unlock(&dictionary_mutex);
        }
        else if (cmd[0] == 'u')
        {
            // insert wala hai
            // 7 - lekar ; tak key given hogi
            string key_given;
            string value_given;
            int starter = 7;
            while (cmd[starter] != ';')
            {
                key_given.push_back(cmd[starter]);
                starter++;
            }
            starter++;
            while (starter < cmd.length())
            {
                value_given.push_back(cmd[starter]);
                starter++;
            }
            printf("reached here ");
            printf("%s %s\n", key_given.c_str(), value_given.c_str());
            int key_given_1 = stoi(key_given);
            pthread_mutex_lock(&dictionary_mutex);
            printf("One more check\n");
            int cheker_1 = find_in_dict(key_given_1);
            if (cheker_1 == -1)
            {
                msg_to_send_back = "Key does not exist";
            }
            else
            {
                dictionary[cheker_1].second = value_given;
                msg_to_send_back = dictionary[cheker_1].second;
            }
            pthread_mutex_unlock(&dictionary_mutex);
        }
        else if (cmd[0] == 'd')
        {
            // insert wala hai
            // 7 - lekar ; tak key given hogi
            string key_given;
            int starter = 7;
            while (cmd[starter] != ';')
            {
                key_given.push_back(cmd[starter]);
                starter++;
            }
            starter++;
            printf("reached here ");
            int key_given_1 = stoi(key_given);
            pthread_mutex_lock(&dictionary_mutex);
            printf("One more check\n");
            int cheker_1 = find_in_dict(key_given_1);
            if (cheker_1 == -1)
            {
                msg_to_send_back = "No such key exists";
            }
            else
            {
                dictionary[cheker_1].first = -1;
                dictionary[cheker_1].second = ";";
                msg_to_send_back = "Deletion successful";
            }
            pthread_mutex_unlock(&dictionary_mutex);
        }
        else if (cmd[0] == 'f')
        {
            // insert wala hai
            // 7 - lekar ; tak key given hogi
            string key_given;
            int starter = 6;
            while (cmd[starter] != ';')
            {
                key_given.push_back(cmd[starter]);
                starter++;
            }
            starter++;
            printf("reached here ");
            int key_given_1 = stoi(key_given);
            pthread_mutex_lock(&dictionary_mutex);
            printf("One more check\n");
            int cheker_1 = find_in_dict(key_given_1);
            if (cheker_1 == -1)
            {
                msg_to_send_back = "Key does not exist";
            }
            else
            {
                msg_to_send_back = dictionary[cheker_1].second;
            }
            pthread_mutex_unlock(&dictionary_mutex);
        }
        // if (!strcmp("insert", extra_token[0]))
        // {
        //     printf("reached 149\n");
        //     msg_to_send_back = insert_ka_kaam(string(extra_token[1]),string(extra_token[2]));
        // }
        // else if (!strcmp("concat", extra_token[0]))
        // {
        //     printf("reached 175\n");
        //     int key_given = atoi(extra_token[1]);
        //     int key_given_2 = atoi(extra_token[2]);
        //     pthread_mutex_lock(&dictionary_mutex);
        //     if (dictionary.find(key_given) != dictionary.end() && dictionary.find(key_given_2) != dictionary.end())
        //     {
        //         printf("one more check");
        //         std::map<int, string>::iterator itr = dictionary.find(key_given);
        //         std::map<int, string>::iterator itr2 = dictionary.find(key_given);
        //         string a = itr->second;
        //         string b = itr2->second;
        //         itr->second = (a + b);
        //         itr2->second = (b + a);
        //         msg_to_send_back += itr2->second;
        //     }
        //     else
        //     {
        //         msg_to_send_back += "Concat failed as at least one of the keys does not exist";
        //     }
        //     pthread_mutex_unlock(&dictionary_mutex);
        // }
        pthread_mutex_lock(&printing_mutex);
        send_string_on_socket(client_socket_fd, msg_to_send_back);
        pthread_mutex_unlock(&printing_mutex);
        goto close_client_socket_ceremony;
    }
close_client_socket_ceremony:
    close(client_socket_fd);
    printf(BRED "Disconnected from client" ANSI_RESET "\n");
}

// return NULL;

void *init_server_thread(void *ptr)
{
    while (true)
    {
        int client_socked_to_pass;
        // acquire a lock for queue
        pthread_mutex_lock(&queue_mutex);
        printf("acquired lock line 265\n");
        while (clients_queue.empty())
        {
            // do nothing
            // and wait for signal causing no busy waiting
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        client_socked_to_pass = clients_queue.front();
        clients_queue.pop();
        printf(BGRN "New client connected \n" ANSI_RESET);
        // free the lock
        pthread_mutex_unlock(&queue_mutex);
        handle_connection(client_socked_to_pass);
        printf("Done task by %d\n", gettid());
    }
}

int main(int argc, char *argv[])
{

    mutex_initializer();
    int number_of_threads = atoi(argv[1]);
    for (int i = 0; i < number_of_threads; i++)
    {
        pthread_create(&thread_pool[i], NULL, init_server_thread, NULL);
    }
    // call initializer function

    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;
    struct sockaddr_in serv_addr_obj, client_addr_obj;
    /////////////////////////////////////////////////////////////////////////
    /* create socket */
    /*
    The server program must have a special door—more precisely,
    a special socket—that welcomes some initial contact
    from a client process running on an arbitrary host
    */
    // get welcoming socket
    // get ip,port
    /////////////////////////
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }

    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    // On the server side I understand that INADDR_ANY will bind the port to all available interfaces,
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); // process specifies port

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* bind socket to this port number on this machine */
    /*When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */

    // CHECK WHY THE CASTING IS REQUIRED
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    //////////////////////////////////////////////////////////////////////////////////////

    /* listen for incoming connection requests */

    listen(wel_socket_fd, MAX_CLIENTS);
    printf("Server has started listening on the LISTEN PORT\n");
    clilen = sizeof(client_addr_obj);

    while (1)
    {
        /* accept a new request, create a client_socket_fd */
        /*
        During the three-way handshake, the client process knocks on the welcoming door
of the server process. When the server “hears” the knocking, it creates a new door—
more precisely, a new socket that is dedicated to that particular client.
        */
        // accept is a blocking call
        // printf("Waiting for a new client to request for a connection\n");
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }
        pthread_mutex_lock(&queue_mutex);
        clients_queue.push(client_socket_fd);
        pthread_mutex_unlock(&queue_mutex);
        pthread_cond_signal(&queue_cond);

        // printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));
        // handle_connection(client_socket_fd);
    }

    close(wel_socket_fd);
    return 0;
}