#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h> // For sbrk
#include <sys/queue.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1000000
#define PORT 9000

int server_sock = 0;
int file_fd = 0;
volatile sig_atomic_t exit_requested = 0;

//Global thread complete flag

typedef struct ThreadInfo {
    pthread_t thread_id;
    int client_socket;
    int thread_complete_flag;
    struct sockaddr_storage client_thread_addr;
    socklen_t sin_thread_size;
    SLIST_ENTRY(ThreadInfo) next_node;
};

/*
SLIST_HEAD(ThreadList, ThreadInfo): 
This macro declares a structure that will serve as the head of a singly-linked list.
The list will contain elements of type ThreadInfo. The head structure is named ThreadList.
Here, ThreadList is the name of the structure that will serve as the head of the list, and ThreadInfo is the type of the elements that will be stored in the list. The thread_list variable is an instance of this ThreadList structure.
*/
SLIST_HEAD(ThreadList, ThreadInfo) thread_list = SLIST_HEAD_INITIALIZER(thread_list);
/*
Static Mutexes:
Lifetime: Static mutexes have a lifetime that matches the duration of the program. They are usually global or static variables.

Initialization: They can be initialized at compile-time using the PTHREAD_MUTEX_INITIALIZER macro. This is useful because it allows the mutex to be used immediately upon program start-up.

Use Case: These are often used for resources that are shared across the entire application and need to be available for the entire duration of the program.
*/
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;


void sigint_handler(int signum) {
    syslog(LOG_INFO, "Caught signal, exiting");
    close(file_fd);
    close(server_sock);
    exit_requested = 1;
    unlink("/var/tmp/aesdsocketdata"); // Delete the file
    closelog();
    
    SLIST_FOREACH(info, &thread_list, next_node) {
    pthread_join(info->thread_id, NULL);
    }
    exit(EXIT_SUCCESS);
}

// Calculate available heap size
size_t get_available_heap_size() {
    struct mallinfo mi = mallinfo();
    size_t available_heap = mi.fordblks;

    return available_heap;
}

void add_timestamp_to_file(FILE *file) 
{
    time_t current_time;
    struct tm *time_info;
    char timestamp[128];

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z", time_info);

    pthread_mutex_lock(&file_mutex);
    fprintf(file, "%s\n", timestamp);
    pthread_mutex_unlock(&file_mutex);
    printf("%s\n", timestamp);  
}

void *timer_thread_function(void *arg) {
    FILE* file = (FILE *)arg;

    while(1) {
        add_timestamp_to_file(file);
        sleep(10);
    }
    return NULL;
}

void *handle_connection(void *arg) 
{
    struct ThreadInfo *info = (struct ThreadInfo *)arg;
    int client_socket = info->client_socket;
    struct sockaddr_storage client_thread_address_local = info->client_thread_addr;
    socklen_t sin_thread_size_local = info->sin_thread_size;	
    char *ptr = NULL;
    size_t available_heap = get_available_heap_size();
    char *buffer = (char*)malloc(BUFFER_SIZE*sizeof(char));
    memset(buffer, 0, BUFFER_SIZE*sizeof(char));


        // Receive data and append to file
        file_fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (file_fd == -1) 
        {
            perror("open");
            close(client_socket);
            return;
        }

	
        ssize_t total_received = 0;
        ssize_t bytes_received;
        pthread_mutex_lock(&file_mutex);
        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE*sizeof(char) - 1, 0)) > 0) 
        {
            if ((total_received > available_heap))
            {
                syslog(LOG_INFO, "Packet too large for available heap, closing connection");
                printf("Packet too large for available heap, closing connection");
                close(file_fd);
                close(client_socket);
                pthread_mutex_unlock(&file_mutex);
                break;
            }
            if (exit_requested) {
            pthread_exit(NULL);
            }
            ptr = strchr(buffer, '\n');
            if (ptr != NULL)
            {
            	break;
            }
            total_received += bytes_received;
        }
        pthread_mutex_unlock(&file_mutex);
        write(file_fd, buffer, bytes_received);
        if (close(fd) != 0) {
        perror("Failed to close the file");
    	}
        // Echo back the received data to the client
        file_fd = open("/var/tmp/aesdsocketdata", O_RDONLY);
        if (file_fd == -1) 
        {
            perror("open");
            close(client_socket);
            return;
        }

        while ((bytes_received = read(file_fd, buffer, sizeof(buffer))) > 0) 
            {
            ssize_t bytes_sent = send(client_socket, buffer, bytes_received, 0);
            }
	     memset(buffer, 0, BUFFER_SIZE*sizeof(char));
	free(buffer);
        close(file_fd);
    // Cleanup
    //free (contents_to_send);

    // Lets the cleanup process in main know that the thread is done
    info->thread_complete_flag = 1;
    pthread_exit(NULL);
}

void daemonize() {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS); // Parent process exits
    }

    umask(0); // Set file permissions

    sid = setsid(); // Create a new session
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    int client_sock_main = 0;
    openlog("aesdsocket_server", LOG_CONS | LOG_PID, LOG_USER);
    bool daemon_mode = false;
    struct sockaddr_in server_addr, client_addr;
    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                daemon_mode = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Daemonize if in daemon mode
    if (daemon_mode) {
        daemonize();
    }


    // Create a socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

	int enable = 1;
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    		perror("setsockopt(SO_REUSEADDR) failed");
	}


    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) == -1) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }


    socklen_t client_addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN];

    // Set up the signal handler for SIGINT (Ctrl+C) and SIGTERM
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
	
    pthread_t timer_thread;
    if(pthread_create(&timer_thread, NULL, timer_thread_function, global_file)) {
        perror("Timer thread create");
        goto cleanup_timer;
    }	
	
    while (1) 
    {
        // Accept a connection
        client_sock_main = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
         // Log the accepted connection to syslog with client IP address
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        if (client_sock_main == -1) 
        {
            perror("accept");
            continue;
        }
	else
	{
		ThreadInfo *info = (struct ThreadInfo *)malloc(sizeof(struct ThreadInfo));
		/*The SLIST_INIT macro initializes the list head, 
		setting its slh_first pointer to NULL, which marks 			
		the list as empty. */
		if (info == NULL) 
		{
            		perror("Thread memory allocation error");
            		close(client_sock_main);
            		continue;
        	}
        	info->client_socket = client_sock_main;
        	info->sin_thread_size = sizeof(client_addr);
        	info->client_thread_addr = client_addr;
        	info->thread_complete_flag = 0;
        	
		pthread_t thread_id;
		// Create the thread that handles reading and writing
        	// packets for the client connetion
        	pthread_create(&info->thread_id, NULL, handle_connection, info);
		// Insert the thread's info into the linked list
        	pthread_mutex_lock(&thread_list_mutex);
        	SLIST_INSERT_HEAD(&thread_list, info, next_node);
        	// Log closed connection to syslog
        	syslog(LOG_INFO, "Closed connection from %s", client_ip);
        	close(client_socket);
        	pthread_mutex_unlock(&thread_list_mutex);
        	
	}
        
    }
    SLIST_FOREACH(info, &thread_list, next_node) {
    pthread_join(info->thread_id, NULL);
    }
    cleanup_threads(timer_thread);

    return 0;
}
