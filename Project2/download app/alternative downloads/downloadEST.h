#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "url.h"

#define SERVER_PORT 21

#define MAX_RESPONSE_SIZE 512
#define BUFFER_SIZE 1024

#define TRUE 1
#define FALSE 0

// Response codes
#define READY_AUTH              220
#define READY_PASS              331
#define LOGIN_SUCCESSFULL       230
#define BYTE_MODE_SUCCESSFULL   200
#define PASSIVE_MODE            227
#define READY_TRANSFER          150
#define TRANSFER_COMPLETED      226
#define GOODBYE                 221

// Color definitions
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"

// Macro for color printing
#define COLOR_PRINT(color, format, ...) \
    if (use_colors) { \
        printf(color format COLOR_RESET, ##__VA_ARGS__); \
    } else { \
        printf(format, ##__VA_ARGS__); \
    }

/**
 * @brief Creates a socket connection to the specified host and port.
 * 
 * @param host The hostname to connect to.
 * @param port The port number to connect to.
 * @return int The socket file descriptor on success, exits on failure.
 */
int create_socket(char *host, char *port);

/**
 * @brief Logs in to the FTP server using the provided username and password.
 * 
 * @param socket The control socket file descriptor.
 * @param username The username for login.
 * @param password The password for login.
 * @return int 0 on success, exits on failure.
 */
int login(int socket, char *username, char *password);

/**
 * @brief Enables passive mode and extracts the IP and port for data connection.
 * 
 * @param socket The control socket file descriptor.
 * @param ip The buffer to store the extracted IP address.
 * @param port The buffer to store the extracted port number.
 * @return int 0 on success, -1 on failure.
 */
int setPassiveMode(const int socket, char *ip, char *port);

/**
 * @brief Sends a request to transfer a file.
 * 
 * @param socket The control socket file descriptor.
 * @param path The path of the file to transfer.
 * @return int 0 on success, -1 on failure.
 */
int transfer_request(int socket, char *path);

/**
 * @brief Retrieves a file from the data socket and saves it locally.
 * 
 * @param data_socket The data socket file descriptor.
 * @param control_socket The control socket file descriptor.
 * @param filename The name of the file to save.
 * @return int 0 on success, -1 on failure.
 */
int get_file(int data_socket, int control_socket, char *filename);

/**
 * @brief Reads the response code from the server.
 * 
 * @param socket The control socket file descriptor.
 * @param response The buffer to store the server response.
 * @return int The response code.
 */
int read_response_code(int socket, char *response);

/**
 * @brief Reads the response from the server.
 * 
 * @param socket The control socket file descriptor.
 * @param response The buffer to store the server response.
 * @return int 0 on success, -1 on failure.
 */
int read_response(int socket, char *response);

/**
 * @brief Changes the transfer mode to binary.
 * 
 * @param control_socket The control socket file descriptor.
 * @return int 0 on success, exits on failure.
 */
int changeToBinaryMode(int control_socket);

/**
 * @brief Closes the connection to the FTP server.
 * 
 * @param socket The control socket file descriptor.
 * @return int 0 on success, -1 on failure.
 */
int closeConection(int socket);

#endif