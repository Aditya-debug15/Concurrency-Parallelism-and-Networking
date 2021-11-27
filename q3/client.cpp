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
#include <fcntl.h>

/////////////////////////////
#include <pthread.h>
#include <iostream>
#include <semaphore.h>
#include <assert.h>
#include <queue>
#include <vector>
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
const LL MOD = 1000000007;
#define part cout << "-----------------------------------" << endl;
#define pb push_back
#define debug(x) cout << #x << " : " << x << endl

///////////////////////////////
#define SERVER_PORT 8001
////////////////////////////////////

const LL buff_sz = 1048576;
///////////////////////////////////////////////////

struct client_info_struct
{
    pthread_t thread_obj;
    int reach_time;
    string command;
    int id; // serve dual purpose, will be given to thread and will act as request id
    int thr_id;
    int key;
    int key2;
    string value;
};
pthread_mutex_t printing;
vector<client_info_struct> client_info;

pair<string, int> read_string_from_socket(int fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    // debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. Seems server has closed socket\n";
        // return "
        exit(-1);
    }

    // debug(output);
    output[bytes_received] = 0;
    output.resize(bytes_received);

    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // cout << "We are sending " << s << endl;
    int bytes_sent = write(fd, s.c_str(), s.length());
    // debug(bytes_sent);
    // debug(s);
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA on socket.\n";
        // return "
        exit(-1);
    }

    return bytes_sent;
}

int get_socket_fd(struct sockaddr_in *ptr)
{
    struct sockaddr_in server_obj = *ptr;

    // socket() creates an endpoint for communication and returns a file
    //        descriptor that refers to that endpoint.  The file descriptor
    //        returned by a successful call will be the lowest-numbered file
    //        descriptor not currently open for the process.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Error in socket creation for CLIENT");
        exit(-1);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    int port_num = SERVER_PORT;

    memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
    server_obj.sin_family = AF_INET;
    server_obj.sin_port = htons(port_num); // convert to big-endian order

    // Converts an IP address in numbers-and-dots notation into either a
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    // https://stackoverflow.com/a/20778887/6427607

    /////////////////////////////////////////////////////////////////////////////////////////
    /* connect to server */

    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
    {
        perror("Problem in connecting to the server");
        exit(-1);
    }

    // part;
    //  printf(BGRN "Connected to server\n" ANSI_RESET);
    //  part;
    return socket_fd;
}
////////////////////////////////////////////////////////

void begin_process(int socket_fd, string t_jo_mila)
{
    send_string_on_socket(socket_fd, t_jo_mila);
    int num_bytes_read;
    string output_msg;
    tie(output_msg, num_bytes_read) = read_string_from_socket(socket_fd, buff_sz);
    cout << "Received: " << output_msg << endl;
    cout << "====" << endl;
}
void *init_clients(void *ptr)
{
    int id = *((int *)ptr);
    // sleep before arrival
    sleep(client_info[id].reach_time);
    struct sockaddr_in server_obj;
    int socket_fd = get_socket_fd(&server_obj);
    string string_to_send;
    int num_bytes_read;
    string output_msg;
    string_to_send = client_info[id].command + ";" +to_string(client_info[id].key) + ";";
    if (client_info[id].command == "insert")
    {
        string_to_send += client_info[id].value;
    }
    else if (client_info[id].command == "update")
    {
        string_to_send += client_info[id].value;
    }
    else if (client_info[id].command == "concat")
    {
        string_to_send += to_string(client_info[id].key2);
    }
    send_string_on_socket(socket_fd, string_to_send);
    tie(output_msg, num_bytes_read) = read_string_from_socket(socket_fd, buff_sz);
    pthread_mutex_lock(&printing);
    //cout<<id<<":"<<gettid()<<":"<<output_msg<<endl;
    printf("%d:%d:%s\n",id,gettid(),output_msg.c_str());
    pthread_mutex_unlock(&printing);
    return NULL;
}
int main(int argc, char *argv[])
{
    pthread_mutex_init(&printing,NULL);
    int number_of_times_to_loop;
    cin >> number_of_times_to_loop;
    for (int i = 0; i < number_of_times_to_loop; i++)
    {
        // need to initalize a thread
        // make struct and all that shit
        client_info_struct temporary_struct;
        cin >> temporary_struct.reach_time >> temporary_struct.command >> temporary_struct.key;
        if (temporary_struct.command == "insert")
        {
            cin >> temporary_struct.value;
        }
        else if (temporary_struct.command == "update")
        {
            cin >> temporary_struct.value;
        }
        else if (temporary_struct.command == "concat")
        {
            cin >> temporary_struct.key2;
        }
        temporary_struct.id = i;
        client_info.push_back(temporary_struct);
    }
    for (int i = 0; i < number_of_times_to_loop; i++)
    {
        client_info[i].thr_id = pthread_create(&(client_info[i].thread_obj), NULL, init_clients, (void *)(&(client_info[i].id)));
    }
    // begin_process();
    // thread join
    for (int i = 0; i < number_of_times_to_loop; i++)
    {
        pthread_join(client_info[i].thread_obj, NULL);
    }
    return 0;
}
