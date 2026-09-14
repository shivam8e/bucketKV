#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define PORT 7890
#define BUFFER_SIZE 1048576
#define TABLE_SIZE 4000

// ############################## GENERAL FUNCTION #####################

char *sliceString(const char *str, int start, int end)
{
    if (str == NULL || start < 0 || end < start || end > strlen(str))
    {
        return NULL; // Handle invalid input
    }

    int length = end - start;
    char *result = (char *)malloc((length + 1) * sizeof(char));
    if (result == NULL)
    {
        return NULL; // Handle memory allocation failure
    }

    strncpy(result, str + start, length);
    result[length] = '\0'; // Null-terminate the new string

    return result;
}

// ############################# GENERAL FUNCTION ENDS

// ###################################### HASHMAP IMPLEMENTATION ############3333

typedef struct Node
{
    char *key;
    char *value;
    struct Node *next;

    double endAfterSecond;

} Node;

typedef struct HashMap
{
    Node *table[TABLE_SIZE + 1];
    pthread_mutex_t locks[TABLE_SIZE + 1];
    pthread_mutex_t stats_lock;

    long long unsigned int totalNodes;
    long long unsigned int totalKeyCharacterSize;
    long long unsigned int totalValueCharacterSize;
} HashMap;

unsigned int hashFunction(const char *key)
{
    if (key == NULL)
    {
        return 0; // Handle NULL keys
    }

    unsigned long hash = 5381;
    int c;

    while ((c = *key++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    // printf("value is %ld :\n", hash % TABLE_SIZE);
    return hash % TABLE_SIZE;
}

void initHashMap(HashMap *map)
{
    map->totalNodes = 0;
    map->totalKeyCharacterSize = 0;
    map->totalValueCharacterSize = 0;
    pthread_mutex_init(&map->stats_lock, NULL);
    for (int i = 0; i < TABLE_SIZE + 1; i++)
    {
        // map->table[i]->next = NULL;
        map->table[i] = NULL;
        pthread_mutex_init(&map->locks[i], NULL);
    }
}

void insertInMap(HashMap *map, const char *key, char *value)
{
    printf("%s \n", key);
    int hashIndex = hashFunction(key);
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->key = strdup(key);
    newNode->value = strdup(value);
    newNode->next = NULL;

    newNode->endAfterSecond = -1;

    pthread_mutex_lock(&map->locks[hashIndex]);

    if (map->table[hashIndex] == NULL)
    {
        pthread_mutex_lock(&map->stats_lock);
        map->totalNodes = map->totalNodes + 1;
        map->totalKeyCharacterSize = map->totalKeyCharacterSize + strlen(key) * sizeof(char);
        map->totalValueCharacterSize = map->totalValueCharacterSize + strlen(value) * sizeof(char);
        pthread_mutex_unlock(&map->stats_lock);
        ((map))->table[hashIndex] = newNode;
    }
    else
    {
        Node *ptr = ((map))->table[hashIndex];
        while (1)
        {
            if (strcmp(ptr->key, key) == 0)
            {
                pthread_mutex_lock(&map->stats_lock);
                map->totalValueCharacterSize = (map->totalValueCharacterSize - (strlen(ptr->value)) * sizeof(char)) + (strlen(value) * sizeof(char));
                pthread_mutex_unlock(&map->stats_lock);
                
                free(ptr->value); // Free the old value to prevent memory leak
                ptr->value = strdup(value); // Safely duplicate the new value
                
                // We didn't end up needing the newNode we allocated at the top, so free it!
                free(newNode->key);
                free(newNode->value);
                free(newNode);
                
                pthread_mutex_unlock(&map->locks[hashIndex]);
                return;
            }
            
            if (ptr->next == NULL) {
                break; // Reached the end of the chain, key not found
            }
            ptr = ptr->next;
        }
        
        pthread_mutex_lock(&map->stats_lock);
        map->totalNodes = map->totalNodes + 1;
        map->totalKeyCharacterSize = map->totalKeyCharacterSize + strlen(key) * sizeof(char);
        map->totalValueCharacterSize = map->totalValueCharacterSize + strlen(value) * sizeof(char);
        pthread_mutex_unlock(&map->stats_lock);

        ptr->next = newNode;
    }
    
    pthread_mutex_unlock(&map->locks[hashIndex]);
}

void displayStringMapSize(HashMap *map)
{
    // printf("%lld , %lld , %lld", map->totalNodes, map->totalKeyCharacterSize, map->totalValueCharacterSize);
}

void deleteInHashMap(HashMap *map, char *key)
{
    int hashIndex = hashFunction(key);
    // printf("HASH INDEX IS %d \n", hashIndex);
    
    pthread_mutex_lock(&map->locks[hashIndex]);
    
    Node *curr = map->table[hashIndex];
    Node *prev = NULL;

    while (curr != NULL)
    {
        if (strcmp(curr->key, key) == 0)
        {
            // Update stats
            pthread_mutex_lock(&map->stats_lock);
            map->totalNodes = map->totalNodes - 1;
            map->totalKeyCharacterSize = map->totalKeyCharacterSize - strlen(curr->key);
            map->totalValueCharacterSize = map->totalValueCharacterSize - strlen(curr->value);
            pthread_mutex_unlock(&map->stats_lock);

            // Unlink the node
            if (prev == NULL)
            {
                map->table[hashIndex] = curr->next; // Deleting the head node
            }
            else
            {
                prev->next = curr->next; // Deleting a middle or tail node
            }

            // Free the memory to prevent leaks
            free(curr->key);
            free(curr->value);
            free(curr);

            break; // Exit the loop after successful deletion
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&map->locks[hashIndex]);
}

char ***getALLStringMapKeys(HashMap *map)
{
    char **keysArray = (char **)malloc((map->totalNodes) * sizeof(char *));
    char **valuesArray = (char **)malloc((map->totalNodes) * sizeof(char *));
    if (keysArray == NULL)
    {
        return NULL;
    }

    int index = 0;
    for (int i = 0; (i < TABLE_SIZE + 1); i++)
    {
        pthread_mutex_lock(&map->locks[i]);
        Node *ptr = map->table[i];
        while (ptr != NULL)
        {
            char *key = ptr->key;
            char *value = ptr->value;
            if (strcmp(key, "") == 0)
            {
                ptr = ptr->next;
                continue;
            }
            // printf("key is %s and value is %s \n", key, value);
            keysArray[index] = (char *)malloc((strlen(ptr->key) + 1) * sizeof(char));
            valuesArray[index] = (char *)malloc((strlen(ptr->value) + 1) * sizeof(char));

            strcpy(keysArray[index], ptr->key);
            strcpy(valuesArray[index], ptr->value);

            ptr = ptr->next;
            index++;
        }
        pthread_mutex_unlock(&map->locks[i]);
    }

    char ***keypairs = (char ***)malloc(sizeof(char **) * 2);
    keypairs[0] = keysArray;
    keypairs[1] = valuesArray;
    return keypairs;
}

char *searchInMap(HashMap *map, const char *key)
{
    int hashIndex = hashFunction(key);
    
    pthread_mutex_lock(&map->locks[hashIndex]);
    
    Node *curr = map->table[hashIndex];
    char *data = NULL;
    
    while (curr != NULL)
    {
        if (strcmp(curr->key, key) == 0)
        {
            data = strdup(curr->value); // strdup perfectly allocates exactly strlen + 1 bytes safely
            break;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&map->locks[hashIndex]);
    
    if (data == NULL) {
        data = strdup("-1"); // Return "-1" as heap memory so caller can safely call free() unconditionally
    }
    
    return data;
}

char *allStringToJson(HashMap *map)
{
    char ***AllData = getALLStringMapKeys(map);
    char **keyData = AllData[0];
    char **valueData = AllData[1];
    int size = (map)->totalNodes;
    
    // Calculate total buffer size and allocate memory
    size_t bufferSize = (map->totalKeyCharacterSize + map->totalValueCharacterSize) + (100 * map->totalNodes) + 10;
    char *jsonResponse = (char *)malloc(bufferSize);
    
    // 1. Fix the Garbage Memory Bug: Initialize the string safely!
    jsonResponse[0] = '[';
    jsonResponse[1] = '\0';
    
    // Keep track of our current length to avoid Schlemiel the Painter's Algorithm
    size_t currentLen = 1;

    for (int i = 0; i < size; i++)
    {
        char *key = keyData[i];
        char *value = valueData[i];
        
        // 2. Fix the Stack Smashing Bug: Adding +3 for quotes AND null terminator
        char insertKey[strlen(key) + 3];
        char insertValue[strlen(value) + 3];
        sprintf(insertKey, "\"%s\"", key);
        sprintf(insertValue, "\"%s\"", value);

        // 3. Fix the O(N^2) strcat Bug: Use snprintf with an offset for O(1) appending
        char *comma = (i != size - 1) ? "," : "";
        
        // We append safely at the exact end of the current string without rescanning
        int added = snprintf(jsonResponse + currentLen, bufferSize - currentLen, "{%s:%s}%s", insertKey, insertValue, comma);
        currentLen += added;
        
        // Bonus Fix: Free the memory allocated by getALLStringMapKeys to stop massive leaks
        free(key);
        free(value);
    }
    
    // Append the closing bracket safely
    snprintf(jsonResponse + currentLen, bufferSize - currentLen, "]");
    
    // Free the array pointers themselves
    free(keyData);
    free(valueData);
    free(AllData);

    return jsonResponse;
}

HashMap mapForString;

// ###########################################3 ALL FOR EXECUTION ##########################3333
typedef struct
{
    double endTime;
    char *key;
} execution_struct;

void *delayed_function(void *args)
{
    execution_struct *execution = (execution_struct *)args;
    sleep((int)execution->endTime);
    deleteInHashMap(&mapForString, execution->key);
    free(execution);
    return NULL;
}

void setExecutionTime(HashMap *map, char *key, double executionTime)
{
    int hashIndex = hashFunction(key);
    Node *ptr = ((map))->table[hashIndex];
    if (ptr == NULL)
    {
        return;
    }
    while (ptr != NULL)
    {
        if (strcmp(ptr->key, key) == 0)
        {
            ptr->endAfterSecond = executionTime;
            pthread_t thread_id;
            execution_struct *execution = (execution_struct *)malloc(sizeof(execution_struct));
            execution->endTime = executionTime;
            execution->key = (char *)malloc(sizeof(char) * (strlen(ptr->key)+2));
            strcpy(execution->key, ptr->key);

            if ((pthread_create(&thread_id, NULL, delayed_function, (void *)execution)) == 0)
            {
                printf("thread started \n");
                pthread_detach(thread_id);
            }
            else
            {
                perror("Thread not started");
            }
        }

        ptr = ptr->next;
    }
}

// #############################3 End OF EXECUTION FUNCTION ##############################3

// hashmap implementation ends

void read_all_data(int client_socket)
{
    char buffer[1024];
    int bytes_read;

    // Read data in chunks
    while ((bytes_read = read(client_socket, buffer, 1024)) > 0)
    {
        // Process the data
        // Here you can append the data to a larger buffer or handle it as needed
        // printf("%.*s", bytes_read, buffer);
    }

    if (bytes_read < 0)
    {
        perror("read");
    }
}

void display(char *c, int size)
{
    for (int i = 0; i < size; i++)
    {
        // printf("%c", c[i]);
    }
}

// Route structure
typedef struct
{
    char *pattern;
    void (*handler)(int client_socket, char *id);
} route_t;

// Route handler function
void handle_user(int client_socket, char *id);
void handle_not_found(int client_socket, char *id);
void handle_string_route(int client_socket, char *id);
void handle_string_get_route(int client_socket, char *id);
void handle_string_delete_route(int client_socket, char *id);
void handle_get_all_data_in_json(int client_socket, char *id);
void handle_time_executer_for_string(int client_socket, char *id);
void memoryToDiskFileWriting(int client_socket, char *id);

// Array of routes and their handler

route_t routes[] = {
    {"/user", handle_user},
    {"/string", handle_string_route},         // /string/name/&value
    {"/get/string", handle_string_get_route}, // get vlaue /get/string/key
    {"/delete/string", handle_string_delete_route},
    {"/jsonString", handle_get_all_data_in_json},
    {"/time/string", handle_time_executer_for_string}, // handle executer for string  same like /time/string/key/&endtime
    {"/memoryString", memoryToDiskFileWriting},
    {NULL, handle_not_found}};

void handle_user(int client_socket, char *id)
{
    // response[BUFFER_SIZE]; snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"message\": \"User Page\", \"id\": \"%s\"}",id);
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nUser Page for ID: %s", id);
    send(client_socket, response, strlen(response), 0);
}

void handle_string_route(int client_socket, char *id)
{
    char *value;
    value = sliceString(id, 1, strlen(id));
    // handling response like /key/&value
    int forward = 0; // for /
    int back = 0;    // for &
    for (int i = 0; i < strlen(value) - 1; i++)
    {
        if (value[i] == '/' && value[i + 1] == '&')
        {
            forward = i;
            back = i + 1;
            break;
        }
    }
    char *finalKey = sliceString(value, 0, forward);
    char *finalValue = sliceString(value, back + 1, strlen(value));

    int integerKey = hashFunction(finalKey);

    insertInMap(&mapForString, finalKey, finalValue);

    char response[strlen(finalKey) + strlen(finalValue) + 1024];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nUser Page for ID: %s and %s", finalKey, finalValue);
    free(finalKey);
    free(finalValue);
    free(value);
    send(client_socket, response, strlen(response), 0);
}

// Retrive data from string
void handle_string_get_route(int client_socket, char *request)
{

    char *searchKey;
    searchKey = sliceString(request, 1, strlen(request));
    char *result = searchInMap(&mapForString, searchKey);
    char response[1024 + strlen(result)];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n %s", result);
    send(client_socket, response, strlen(response), 0);
    
    // Free the dynamically allocated strings to prevent Memory Leaks!
    free(searchKey);
    free(result);
}

void memoryToDiskFileWriting(int client_socket, char *request)
{
    char *searchKey;
    // Extract the requested filename from the route (e.g., /backup1 -> backup1)
    searchKey = sliceString(request, 1, strlen(request));
    
    // Create the filename securely (length of name + ".json" + Null Terminator)
    char *fileName = (char *)malloc(strlen(searchKey) + 6);
    sprintf(fileName, "%s.json", searchKey);
    
    // Utilize our newly-fixed, lightning-fast JSON exporter!
    char *jsonString = allStringToJson(&mapForString);
    
    // Write everything to a SINGLE file (fixes the Inode exhaustion issue!)
    FILE *file = fopen(fileName, "w");
    if (file == NULL)
    {
        printf("ERROR OPENING FILE\n");
        perror("file not created");
        
        char response[1024];
        snprintf(response, sizeof(response), "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to write backup to disk.");
        send(client_socket, response, strlen(response), 0);
    }
    else
    {
        fprintf(file, "%s", jsonString);
        fclose(file);
        
        char response[1024];
        snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nSuccessfully backed up database to %s", fileName);
        send(client_socket, response, strlen(response), 0);
    }
    
    // Clean up all dynamically allocated memory to stop leaks!
    free(searchKey);
    free(fileName);
    free(jsonString);
}

void handle_string_delete_route(int client_socket, char *request)
{

    char *searchKey;
    searchKey = sliceString(request, 1, strlen(request));
    deleteInHashMap(&mapForString, searchKey);
    char response[1024];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n OK");
    send(client_socket, response, strlen(response), 0);
    
    // Fix memory leak
    free(searchKey);
}

void handle_time_executer_for_string(int client_socket, char *id)
{
    char *value;
    value = sliceString(id, 1, strlen(id));
    // handling response like /key/&value
    int forward = 0; // for /
    int back = 0;    // for &
    for (int i = 0; i < strlen(value) - 1; i++)
    {
        if (value[i] == '/' && value[i + 1] == '&')
        {
            forward = i;
            back = i + 1;
            break;
        }
    }
    char *finalKey = sliceString(value, 0, forward);
    char *finalValue = sliceString(value, back + 1, strlen(value));
    int finalDelay = 0;
    for (int i = 0; i < strlen(finalValue); i++)
    {
        finalDelay = (finalDelay * 10) + (finalValue[i] - '0');
    }
    printf("the final delay is %d", finalDelay);
    setExecutionTime(&mapForString, finalKey, (int)finalDelay);
    char response[1024];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n %s %s", finalKey, finalValue);
    send(client_socket, response, strlen(response), 0);
    
    // Fix memory leaks
    free(value);
    free(finalKey);
    free(finalValue);
}

void handle_not_found(int client_socket, char *id)
{
    char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nPage Not Found!";
    send(client_socket, response, strlen(response), 0);
}

void handle_request(int client_socket, char *request)
{
    char route[BUFFER_SIZE] = {0};
    sscanf(request, "GET %s HTTP/1.1", route);
    // display(route, BUFFER_SIZE);
    for (int i = 0; routes[i].pattern != NULL; i++)
    {
        if (strncmp(routes[i].pattern, route, strlen(routes[i].pattern)) == 0)
        {
            ;
            char *id = route + strlen(routes[i].pattern);
            // printf(" displaying id --- %s\n", id);
            routes[i].handler(client_socket, id);
            return;
        }
    }

    handle_not_found(client_socket, NULL);
}

void handle_get_all_data_in_json(int client_socket, char *id)
{
    char *jsonstringData = allStringToJson(&mapForString);
    char response[BUFFER_SIZE + strlen(jsonstringData)];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", jsonstringData);
    send(client_socket, response, strlen(response), 0);
    
    // Free the dynamically allocated JSON string
    free(jsonstringData);
}

// Use this function instead of the single read call

// ###################### POST HANDLER ROUTE##################

void handle_post_request(int new_socket, const char *path, const char *body)
{
    char response[BUFFER_SIZE];

    // Handle different routes
    // printf("\n\n\n\n\n\n %s \n\n\n\n\n\n\n\n\\",body);
    if (strcmp(path, "/") == 0)
    {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\nHello, you've POSTed to the root!\nData: %s",
                 strlen(body) + 33, body);
    }
    else if (strcmp(path, "/string") == 0)
    {
        long long unsigned int front = 0, back = 0;
        for (int i = 0; i < strlen(body); i++)
        {
            if (body[i] == '=')
            {
                front = i;
                break;
            }
        }
        char *key = sliceString(body, 0, front);
        char *value = sliceString(body, front + 1, strlen(body));
        insertInMap(&mapForString, key, value);
        // printf("hello");
        // printf("key %s value %s \n", key, value);
        free(key);
        free(value);
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\nHello, you've POSTed to the root!\nData: %s",
                 strlen(body) + 33, body);
    }
    else
    {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: 13\r\n"
                 "\r\nRoute not found");
    }

    // printf("\n%s\n",response);
    // Send the response
    send(new_socket, response, strlen(response), 0);
}

// POST HANDLER EXIT

int main(int argc, char const *argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address, client_address;
    int addrlen = sizeof(address);
    char *buffer = malloc(sizeof(char) * BUFFER_SIZE);
    char client_ip[INET_ADDRSTRLEN];
    struct timeval timeout;
    int opt = 1;

    initHashMap(&mapForString);
    // struct timeval timeout;

    // SOCK_STREAM: This specifies the type of socket. SOCK_STREAM is used for TCP,
    // 0: This specifies the protocol. When set to 0, it automatically selects the appropriate protocol for the given socket type. For SOCK_STREAM, it will choose TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // int opt = 1;
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    // {
    //     perror("setsockopt");
    //     close(server_fd);
    //     exit(EXIT_FAILURE);
    // }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;         // ipv4
    address.sin_addr.s_addr = INADDR_ANY; // it tells the server to bind to all available IP addresses on the machine. This is particularly useful if your machine has multiple network interfaces (e.g., both a wired and a wireless connection).
    address.sin_port = htons(PORT);

    // server_fd: The file descriptor of the socket you're binding.
    //*(struct sockaddr )&address: The address to bind the socket to, which includes the IP address and port number.

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE); // ff 0 then program run succesfully and if non zero then it does not run succesfully
    }

    // server_fd: The file descriptor of the socket you want to mark as a listening socket.
    // 3: The backlog parameter, which specifies the maximum number of pending connections that can be queued up before the server starts rejecting new connections. In this case, up to 50 connections can be queued.

    if (listen(server_fd, 50) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    };

    //

    // Attach socket to the port

    // // Set the write timeout

    while (1)
    {
        // server_fd: The listening socket file descriptor.
        //*(struct sockaddr )&address: A pointer to a sockaddr structure where accept will store the address of the connecting client.
        //(socklen_t)&addrlen:* A pointer to a variable that stores the length of the address.

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accepting socket");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Set the read timeout
        timeout.tv_sec = 30; // Timeout in seconds
        timeout.tv_usec = 0; // Additional timeout in microseconds
        if (setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }

        // Convert client ip to the string
        inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET6_ADDRSTRLEN);
        // printf("CLIENT CONNECTED %s \n", client_ip);

        read(new_socket, buffer, BUFFER_SIZE);
        // printf("%s\n", buffer);
        if (strncmp(buffer, "POST", 4) == 0)
        {

            char method[10], path[100], version[10];
            sscanf(buffer, "%s %s %s", method, path, version);
            // printf("%s %s %s \n", method, path, version);
            char *body = strstr(buffer, "\r\n\r\n");
            if (body)
            {
                body += 4;

                char response[BUFFER_SIZE];
                handle_post_request(new_socket, path, body);
                // printf("the buffer %s \n", response);
            }
        }
        else
        {
            handle_request(new_socket, buffer);
        }

        // for (int i = 0; i < BUFFER_SIZE; i++)
        // {
        //     printf("%c", buffer[i]);
        // }

        close(new_socket);
    }

    return 0;
}
