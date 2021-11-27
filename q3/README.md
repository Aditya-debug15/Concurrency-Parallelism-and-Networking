# MULTITHREADED CLIENT AND SERVER
We have a client that has m strings that they want to send to server for processing, these m strings have to send at a particular instance so it is seen as multithreaded client (having m threads) that connect with a server room<br>
The server room has n threads to send back the data<br>
# RUNNING A PROGRAM
do<br>
```sh
make
```
Then use a split terminal on one type<br>
`server n` <br>
and on other type<br>
`client`
<br>
To remove the executable use :<br>
```sh
make clean
```

# CLIENT PROGRAM

The struct to store the data is<br>
```cpp
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
```
I have used a vector to store the info about all the client threads and i have used a mutex printing so that only 1 thread print {to the stdout} at a given instance.
<br>
Each thread passes a string to the server

# SERVER PROGRAM
I have declared a thread pool as a global variable but it is initialised in the main function only<br>
The conditional variable and locks used are<br>
```cpp
pthread_mutex_t queue_mutex;
pthread_mutex_t printing_mutex;
pthread_mutex_t dictionary_mutex;
pthread_cond_t queue_cond;
```
Printing mutex is for printing purpose though not of much use.
<br>
To maintain the incoming request from the client i have used a queue, whenever we accept an input from the client i acquire the lock, enqueue the output of the accept function and then release the lock.
<br>
The workers (which are server threads) when they finish a job wait untill they queue is empty, for that purpose a conditional variable queue_cond is used.<br>
This thing is taking place in `void *init_server_thread(void *ptr)`<br>

```cpp
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
```
After that this **client_socket_to_pass** is passed to **handle_connection** function.<br>
For dictionary i have used `vector<pair<int,string>>` since this variable is shared among different server threads, so updating, cheking the value in the vector belongs to critical section and only one thread is allowed to go in critical section while the other must wait, that is why i have used mutex lock `dictionary_mutex`.<br>
After processing of the string it is sent back to client.<br>
The threads and main function both are using while(true) so they won't exit untill an error occurs or we enter 
`Ctrl+C`