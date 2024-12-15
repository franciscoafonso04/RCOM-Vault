#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAX_SIZE 256

// Default login
#define DEFAULT_USERNAME "anonymous"
#define DEFAULT_PASSWORD "anonymous"

/**
 * @brief Structure representing a URL and its components.
 */
struct URL
{
    char user[MAX_SIZE]; ///< Username for authentication.
    char password[MAX_SIZE]; ///< Password for authentication.
    char host[MAX_SIZE]; ///< Hostname of the server.
    char resource_path[MAX_SIZE]; ///< Path to the resource on the server.
    char file[MAX_SIZE]; ///< Filename of the resource.
    char ip[MAX_SIZE]; ///< IP address of the server.
};

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


int parse_url(const char *url_str, struct URL *url) {
    char temp[MAX_SIZE];
    char *at_ptr, *colon_ptr, *slash_ptr;

    // Initialize URL structure
    memset(url, 0, sizeof(struct URL));

    // Copy URL string to temporary buffer
    strncpy(temp, url_str, MAX_SIZE - 1);
    temp[MAX_SIZE - 1] = '\0';

    // Skip protocol if present
    char *protocol_ptr = strstr(temp, "://");
    if (protocol_ptr != NULL) {
        protocol_ptr += 3;
    } else {
        protocol_ptr = temp;
    }

    // Extract user and password
    at_ptr = strchr(protocol_ptr, '@');
    if (at_ptr != NULL) {
        *at_ptr = '\0';
        char *user_pass_ptr = protocol_ptr;
        protocol_ptr = at_ptr + 1;

        colon_ptr = strchr(user_pass_ptr, ':');
        if (colon_ptr != NULL) {
            *colon_ptr = '\0';
            strncpy(url->user, user_pass_ptr, MAX_SIZE - 1);
            strncpy(url->password, colon_ptr + 1, MAX_SIZE - 1);
        } else {
            strncpy(url->user, user_pass_ptr, MAX_SIZE - 1);
            strncpy(url->password, DEFAULT_PASSWORD, MAX_SIZE - 1);
        }
    } else {
        strncpy(url->user, DEFAULT_USERNAME, MAX_SIZE - 1);
        strncpy(url->password, DEFAULT_PASSWORD, MAX_SIZE - 1);
    }

    // Extract host and resource path
    slash_ptr = strchr(protocol_ptr, '/');
    if (slash_ptr != NULL) {
        *slash_ptr = '\0';
        strncpy(url->host, protocol_ptr, MAX_SIZE - 1);
        strncpy(url->resource_path, slash_ptr + 1, MAX_SIZE - 1); // Skip leading '/'
    } else {
        strncpy(url->host, protocol_ptr, MAX_SIZE - 1);
        url->resource_path[0] = '\0'; // No resource path
    }

    // Extract file name from resource path
    if (strlen(url->resource_path) > 0) {
        char *last_slash_ptr = strrchr(url->resource_path, '/');
        if (last_slash_ptr != NULL) {
            strncpy(url->file, last_slash_ptr + 1, MAX_SIZE - 1);
        } else {
            strncpy(url->file, url->resource_path, MAX_SIZE - 1);
        }
    } else {
        url->file[0] = '\0'; // No file specified
    }

    // Resolve IP address
    struct hostent *h;
    if ((h = gethostbyname(url->host)) == NULL) {
        perror("gethostbyname()");
        return -1;
    }
    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

    return 0;
}

// test 
// ./bin/download ftp://anonymous:anonymous@ftp.bit.nl/speedtest/100mb.bin
// ./bin/download ftp://ftp.up.pt/pub/gnu/emacs/elisp-manual-21-2.8.tar.gz
// ./bin/download ftp://demo:password@test.rebex.net/readme.txt

int use_colors = 1; // Enable color output

/**
 * @brief Creates a socket connection to the specified host and port.
 * 
 * @param host The hostname to connect to.
 * @param port The port number to connect to.
 * @return int The socket file descriptor on success, exits on failure.
 */
int create_socket(char *host, char *port) {
    COLOR_PRINT(COLOR_BLUE, "Creating socket to %s:%s\n", host, port);

    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // Obtanin server IP
    if ((server = gethostbyname(host)) == NULL)
    {
        perror("gethostbyname()");
        exit(-1);
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(atoi(port));            

    // Open a TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(-1);
    }
    
    // Connect to the server
    if (connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
    {
        perror("connect()");
        exit(-1);
    }

    COLOR_PRINT(COLOR_GREEN, "Socket created successfully.\n");
    return sockfd;
}

/**
 * @brief Reads the response from the server.
 * 
 * @param socket The control socket file descriptor.
 * @param response The buffer to store the server response.
 * @return int 0 on success, -1 on failure.
 */
int read_response(int socket, char *response)
{
    int total_bytes = 0;
    ssize_t bytes;
    char code[4] = {0};
    int is_multiline = 0;

    memset(response, 0, MAX_RESPONSE_SIZE);

    while (total_bytes < MAX_RESPONSE_SIZE - 1)
    {
        bytes = recv(socket, response + total_bytes, MAX_RESPONSE_SIZE - 1 - total_bytes, 0);
        if (bytes == 0)
        {
            break; 
        }
        else if (bytes < 0)
        {
            perror("Error reading response");
            return -1;
        }

        total_bytes += bytes;
        response[total_bytes] = '\0';

        // Debug print to show what is being received
        COLOR_PRINT(COLOR_YELLOW, "%s\n", response + total_bytes - bytes);

        // Extract response code 
        if (total_bytes >= 4 && code[0] == '\0')
        {
            strncpy(code, response, 3); 
            code[3] = '\0';
            if (response[3] == '-') // Multi-line response
            {
                is_multiline = 1;
            }
        }

        // Parse response 
        char *line_start = response;
        while (line_start)
        {
            char *line_end = strstr(line_start, "\r\n");
            if (line_end)
            {
                *line_end = '\0'; // Null-terminate the line

                // Check if this line ends the response
                if (strncmp(line_start, code, 3) == 0 && line_start[3] == ' ')
                {
                    return 0; 
                }

                *line_end = '\r'; // Restore the line ending
                line_start = line_end + 2; // Move to the next line
            }
            else
            {
                break; // Incomplete line, wait for more data
            }
        }

        // For single-line responses, check the termination condition
        if (!is_multiline && total_bytes >= 4 &&
            response[total_bytes - 2] == '\r' && response[total_bytes - 1] == '\n')
        {
            return 0; // End of single-line response
        }
    }

    response[total_bytes] = '\0';
    return 0; // Complete response read
}


/**
 * @brief Reads the response code from the server.
 * 
 * @param socket The control socket file descriptor.
 * @param response The buffer to store the server response.
 * @return int The response code.
 */
int read_response_code(int socket, char *response)
{
    COLOR_PRINT(COLOR_BLUE, "Reading response code.\n");

    char response_code[4] = {0};

    if (read_response(socket, response) < 0)
    {
        fprintf(stderr, "Error reading response.\n");
        exit(EXIT_FAILURE);
    }

    // Extract the response code
    strncpy(response_code, response, 3);
    response_code[3] = '\0';

    return atoi(response_code);
}


/**
 * @brief Logs in to the FTP server using the provided username and password.
 * 
 * @param socket The control socket file descriptor.
 * @param username The username for login.
 * @param password The password for login.
 * @return int 0 on success, exits on failure.
 */
int login(int socket, char *username, char *password)
{
    COLOR_PRINT(COLOR_BLUE, "Logging in with username: %s\n", username);

    char buffer[BUFFER_SIZE];
    int status;

    // Send username
    snprintf(buffer, sizeof(buffer), "USER %s\r\n", username);
    if (send(socket, buffer, strlen(buffer), 0) < 0)
    {
        perror("Error sending username");
        exit(EXIT_FAILURE);
    }

    // Read response
    char response[MAX_RESPONSE_SIZE];
    status = read_response_code(socket, response);
    if (status != READY_PASS)
    {
        fprintf(stderr, "Error: Server not ready for password.\n");
        exit(EXIT_FAILURE);
    }

    COLOR_PRINT(COLOR_BLUE, "Logging in with password: %s\n", password);

    // Send password
    snprintf(buffer, sizeof(buffer), "PASS %s\r\n", password);
    if (send(socket, buffer, strlen(buffer), 0) < 0)
    {
        perror("Error sending password");
        exit(EXIT_FAILURE);
    }

    // Check if login was successful
    status = read_response_code(socket, response);
    if (status != LOGIN_SUCCESSFULL)
    {
        fprintf(stderr, COLOR_RED "Error: Login failed.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    COLOR_PRINT(COLOR_GREEN, "Login successful.\n");
    return 0;
}

/**
 * @brief Changes the transfer mode to binary.
 * 
 * @param socket The control socket file descriptor.
 * @return int 0 on success, exits on failure.
 */
int changeToBinaryMode(int socket)
{
    COLOR_PRINT(COLOR_BLUE, "Changing to binary mode.\n");

    char buffer[BUFFER_SIZE];
    int status;

    // Send binary mode
    snprintf(buffer, sizeof(buffer), "TYPE I\r\n");
    if (send(socket, buffer, strlen(buffer), 0) < 0)
    {
        perror("Error sending binary mode");
        exit(EXIT_FAILURE);
    }

    // Check if binary mode was successful
    char response[MAX_RESPONSE_SIZE];
    status = read_response_code(socket, response);
    if (status != BYTE_MODE_SUCCESSFULL)
    {
        fprintf(stderr, COLOR_RED "Error: Binary mode change failed.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    COLOR_PRINT(COLOR_GREEN, "Binary mode set successfully.\n");
    return 0;
}

/**
 * @brief Enables passive mode and extracts the IP and port for data connection.
 * 
 * @param socket The control socket file descriptor.
 * @param ip The buffer to store the extracted IP address.
 * @param port The buffer to store the extracted port number.
 * @return int 0 on success, -1 on failure.
 */
int setPassiveMode(const int socket, char *ip, char *port)
{
    COLOR_PRINT(COLOR_BLUE, "Setting passive mode.\n");

    char buffer[BUFFER_SIZE], response[BUFFER_SIZE];
    int h1, h2, h3, h4, p1, p2;

    // Send PASV command
    snprintf(buffer, sizeof(buffer), "PASV\r\n");
    if (send(socket, buffer, strlen(buffer), 0) < 0)
    {
        perror("Error sending PASV command");
        exit(EXIT_FAILURE);
    }

    // Check if passive mode was successful
    int status = read_response_code(socket, response);
    if (status != PASSIVE_MODE)
    {
        fprintf(stderr, COLOR_RED "Error: Passive mode failed.\n" COLOR_RESET);
        return -1;
    }

    // Extract IP and port from response
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) < 0)
    {
        fprintf(stderr, "Error: Failed to parse IP and port.\n");
        return -1;
    }

    // Construct IP address
    snprintf(ip, BUFFER_SIZE, "%d.%d.%d.%d", h1, h2, h3, h4);

    // Construct port
    snprintf(port, BUFFER_SIZE, "%d", p1 * 256 + p2);

    COLOR_PRINT(COLOR_GREEN, "Passive mode set successfully. IP: %s, Port: %s\n", ip, port);
    return 0;
}

/**
 * @brief Sends a request to transfer a file.
 * 
 * @param socket The control socket file descriptor.
 * @param path The path of the file to transfer.
 * @return int 0 on success, -1 on failure.
 */
int transfer_request(int socket, char *path)
{
    COLOR_PRINT(COLOR_BLUE, "Sending transfer request for path: %s\n", path);

    char buffer[BUFFER_SIZE];

    // Send RETR command
    snprintf(buffer, sizeof(buffer), "RETR %s\r\n", path);
    if (send(socket, buffer, strlen(buffer), 0) < 0)
    {
        perror("Error sending RETR command");
        exit(EXIT_FAILURE);
    }

    // Check if server is ready to transfer
    char response[MAX_RESPONSE_SIZE];
    int status = read_response_code(socket, response);
    if (status != READY_TRANSFER)
    {
        fprintf(stderr, COLOR_RED "Error: File transfer not ready.\n" COLOR_RESET);
        return -1;
    }

    COLOR_PRINT(COLOR_GREEN, "Transfer request sent successfully.\n");
    return 0;
}

/**
 * @brief Retrieves a file from the data socket and saves it locally.
 * 
 * @param data_socket The data socket file descriptor.
 * @param control_socket The control socket file descriptor.
 * @param filename The name of the file to save.
 * @return int 0 on success, -1 on failure.
 */
int get_file(int data_socket, int control_socket, char *filename)
{
    COLOR_PRINT(COLOR_BLUE, "Retrieving file: %s\n", filename);

    char filepath[BUFFER_SIZE];

    // coment this if the folder "received" is not created
    snprintf(filepath, sizeof(filepath), "received/%s", filename);

    FILE *file = fopen(filepath, "wb");
    if (file == NULL)
    {
        perror("Error opening file for writing");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    // Read data from data socket and write to file
    while ((bytes = recv(data_socket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, bytes, file);
    }

    fclose(file);

    // Check if transfer is complete
    char response[MAX_RESPONSE_SIZE];
    int status = read_response_code(control_socket, response);
    if (status != TRANSFER_COMPLETED)
    {
        fprintf(stderr, COLOR_RED "Error: File transfer not complete. Response code: %d\n" COLOR_RESET, status);
        return -1;
    }

    COLOR_PRINT(COLOR_GREEN, "File retrieved successfully.\n");
    return 0;
}

/**
 * @brief Closes the connection to the FTP server.
 * 
 * @param socket The control socket file descriptor.
 * @return int 0 on success, -1 on failure.
 */
int closeConection(int socket)
{
    COLOR_PRINT(COLOR_BLUE, "Closing connection.\n");

    char buffer[BUFFER_SIZE];
    int status;

    // Send QUIT command
    snprintf(buffer, sizeof(buffer), "QUIT\r\n");
    if (send(socket, buffer, strlen(buffer), 0) < 0)
    {
        perror("Error sending QUIT command");
        exit(EXIT_FAILURE);
    }

    // Check if server is ready to transfer
    char response[MAX_RESPONSE_SIZE];
    status = read_response_code(socket, response);
    if (status != GOODBYE)
    {
        fprintf(stderr, COLOR_RED "Error: Server did not close connection.\n" COLOR_RESET);
        return -1;
    }

    COLOR_PRINT(COLOR_GREEN, "Connection closed successfully.\n");
    return 0;
}

/**
 * @brief The main function to start the download process.
 * 
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return int 0 on success, 1 on failure.
 */
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        COLOR_PRINT(COLOR_YELLOW, "Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        return 1;
    }

    COLOR_PRINT(COLOR_BLUE, "Starting download process.\n");
    struct URL url;

    // Parse URL
    if (parse_url(argv[1], &url) < 0)
    {
        fprintf(stderr, "Error parsing URL.\n");
        return 1;
    }

    // Display URL details
    int control_socket = create_socket(url.host, "21");

    // Read server FIRST response
    char response[MAX_RESPONSE_SIZE];
    if (read_response_code(control_socket, response) != READY_AUTH)
    {
        close(control_socket);
        fprintf(stderr, COLOR_RED "Error: Server not ready for authentication.\n" COLOR_RESET);
        return 1;
    }

    // Login to the server
    if (login(control_socket, url.user, url.password) < 0)
    {
        close(control_socket);
        fprintf(stderr, COLOR_RED "Error: Login failed using -> USER: '%s' PASS: '%s .\n" COLOR_RESET, url.user, url.password);
        return 1;
    }

    // Change to binary mode
    if (changeToBinaryMode(control_socket) < 0)
    {
        close(control_socket);
        fprintf(stderr, COLOR_RED "Error: Failed to change to binary mode.\n" COLOR_RESET);
        return 1;
    }

    // Enable passive mode
    char ip[BUFFER_SIZE], port[BUFFER_SIZE];
    if (setPassiveMode(control_socket, ip, port) < 0)
    {
        close(control_socket);
        fprintf(stderr, COLOR_RED "Error: Failed to set passive mode.\n" COLOR_RESET);
        return 1;
    }

    // Create data socket
    int data_socket = create_socket(ip, port);

    // Send request to transfer file
    if (transfer_request(control_socket, url.resource_path) < 0)
    {
        close(control_socket);
        close(data_socket);
        fprintf(stderr, COLOR_RED "Error: Failed to transfer file.\n" COLOR_RESET);
        return 1;
    }

    // Retrieve file from data socket
    if (get_file(data_socket, control_socket, url.file) < 0)
    {
        close(control_socket);
        close(data_socket);
        fprintf(stderr, COLOR_RED "Error: Failed to retrieve file.\n" COLOR_RESET);
        return 1;
    }

    // Close connection
    if (closeConection(control_socket) < 0)
    {
        close(data_socket);
        close(control_socket);
        fprintf(stderr, COLOR_RED "Error: Failed to close connection.\n" COLOR_RESET);
        return 1;
    }

    // Close sockets
    close(data_socket);
    close(control_socket);

    COLOR_PRINT(COLOR_GREEN, "Download process completed.\n");
    return 0;
}