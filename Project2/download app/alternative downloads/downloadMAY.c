#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define PORT 21
#define MAX_BUFFER_SIZE 1024
#define MAX_URL_LENGTH 500

typedef struct {
    char hostname[MAX_URL_LENGTH];
    char resource_path[MAX_URL_LENGTH];
    char filename[MAX_URL_LENGTH];
    char ip_address[MAX_URL_LENGTH];
    char username[MAX_URL_LENGTH];
    char password[MAX_URL_LENGTH];
} ftpURL;

void rushed_exit(int control_socket, int data_socket, const char *message) {
    if (message) fprintf(stderr, "Error: %s - %s\n", message, strerror(errno));

    if (control_socket >= 0) close(control_socket);
    if (data_socket >= 0) close(data_socket);

    exit(EXIT_FAILURE);
}

void graceful_exit(int control_socket) {
    if (write(control_socket, "QUIT\r\n", 6) < 0) {
        fprintf(stderr, "Failed to send QUIT command\n");
    }
    close(control_socket);
}


// Improved URL parsing with more robust checks
bool parse_ftp_url(const char *url, ftpURL *parsed_url) {
    // Reset all fields
    memset(parsed_url, 0, sizeof(ftpURL));

    // Check for valid FTP prefix
    if (strncmp(url, "ftp://", 6) != 0) {
        rushed_exit(-1, -1, "Invalid URL: Must start with ftp://");
    }

    // Try to parse URL with credentials
    int credential_parse = sscanf(url, "ftp://%[^:]:%[^@]@%[^/]/%s", parsed_url->username, parsed_url->password, parsed_url->hostname, parsed_url->resource_path);

    // If no credentials, use anonymous login
    if (credential_parse != 4) {
        credential_parse = sscanf(url, "ftp://%[^/]/%s", parsed_url->hostname, parsed_url->resource_path);
        
        if (credential_parse != 2) {
            rushed_exit(-1, -1, "Invalid FTP URL format");
        }

        // Set default anonymous credentials
        strcpy(parsed_url->username, "anonymous");
        strcpy(parsed_url->password, "anonymous");
    }

    // Extract filename (last part of resource path)
    char *last_slash = strrchr(url, '/');
    if (last_slash) {
        strcpy(parsed_url->filename, last_slash + 1);
    } else {
        strcpy(parsed_url->filename, "downloaded_file");
    }

    // Resolve hostname to IP
    struct hostent *host = gethostbyname(parsed_url->hostname);
    if (host == NULL) {
        herror("Hostname resolution failed");
        return false;
    }

    // Convert IP to string
    strcpy(parsed_url->ip_address, inet_ntoa(*((struct in_addr *) host->h_addr_list[0])));

    return true;
}


int create_socket(const char *ip, int port) {
    struct sockaddr_in server_addr;
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);   
    server_addr.sin_port = htons(port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        rushed_exit(-1, -1, "Socket creation failed");
    }

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        rushed_exit(-1, -1, "Connection failed");
    }

    return sockfd;
}


// Improved login function with better error checking
void ftp_login(int control_socket, const char* username, const char* password) {
    char buffer[MAX_BUFFER_SIZE];
    int response_code;

    // Send username
    snprintf(buffer, sizeof(buffer), "USER %s\r\n", username);
    if (write(control_socket, buffer, strlen(buffer)) < 0) {
        rushed_exit(control_socket, -1, "Failed to send username");
    }

    // Read username response
    while (read(control_socket, buffer, sizeof(buffer)) > 0) {
        if (sscanf(buffer, "%d", &response_code) == 1 && response_code == 331) {
            break;
        }
    }

    // Send password
    snprintf(buffer, sizeof(buffer), "PASS %s\r\n", password);
    if (write(control_socket, buffer, strlen(buffer)) < 0) {
        rushed_exit(control_socket, -1, "Failed to send password");
    }

    // Read password response
    while (read(control_socket, buffer, sizeof(buffer)) > 0) {
        if (sscanf(buffer, "%d", &response_code) == 1 && response_code == 230) {
            break;
        }
    }
}


// More robust passive mode activation
void activate_passive_mode(int control_socket, char *passive_ip, int *passive_port) {
    char buffer[MAX_BUFFER_SIZE];
    int response_code, byte1, byte2, byte3, byte4, byte5, byte6;

    for (int attempt = 0; attempt < 10; attempt++) {
        if (write(control_socket, "PASV\r\n", 6) < 0) {
            rushed_exit(control_socket, -1, "Failed to send PASV command");
        }

        // Read the response line by line
        while (read(control_socket, buffer, sizeof(buffer)) > 0) {
            char *pasv_response = strstr(buffer, "227 Entering Passive Mode");
            if (pasv_response && sscanf(pasv_response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &byte1, &byte2, &byte3, &byte4, &byte5, &byte6) == 6) {
                snprintf(passive_ip, MAX_URL_LENGTH, "%d.%d.%d.%d", byte1, byte2, byte3, byte4);
                *passive_port = (byte5 * 256) + byte6;
                printf("Passive IP: %s, Passive Port: %d\n", passive_ip, *passive_port);
                return;
            }
        }

        fprintf(stderr, "Retrying PASV due to parsing failure: %s\n", buffer);
    }

    rushed_exit(control_socket, -1, "Failed to activate passive mode after retries");
}


// Main download function with improved error handling
void download_file(int control_socket, int data_socket, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        rushed_exit(control_socket, data_socket, "Unable to create local file");
    }

    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;

    // Download file in chunks
    while ((bytes_read = read(data_socket, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, bytes_read, file) != (size_t)bytes_read) {
            fclose(file);
            rushed_exit(control_socket, data_socket, "Failed to write to the file properly");
        }
    }
    if (bytes_read < 0) {
        fclose(file);
        rushed_exit(control_socket, data_socket, "read() function returned an error");
    }

    fclose(file);
    close(data_socket);  // Close data socket after file transfer

    // Verify download completion
    char response[MAX_BUFFER_SIZE];
    int response_code;

    while (read(control_socket, response, sizeof(response)) > 0) {
        if (sscanf(response, "%d", &response_code) == 1 && response_code == 226) {
            return;
        }
    }

    // If we exit the loop without success
    rushed_exit(control_socket, -1, response);
}


int main(int argc, char *argv[]) {
    // Validate command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[user:pass@]host/path/to/file\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse URL
    ftpURL url;
    if (!parse_ftp_url(argv[1], &url)) {
        return EXIT_FAILURE;
    }

    // Print parsed URL details
    printf("Downloading file:\n");
    printf("Host: %s\n", url.hostname);
    printf("Resource: %s\n", url.resource_path);
    printf("Filename: %s\n", url.filename);
    printf("IP: %s\n", url.ip_address);

    // Control socket connection
    int control_socket = create_socket(url.ip_address, PORT);
    char response[MAX_BUFFER_SIZE];
    int response_code;

    // Initial server connection
    while (read(control_socket, response, sizeof(response)) > 0) {
        if (sscanf(response, "%d", &response_code) == 1 && response_code == 220) {
            break;
        }
    }

    // Login
    ftp_login(control_socket, url.username, url.password);

    // Passive mode
    char passive_ip[MAX_URL_LENGTH];
    int passive_port;
    activate_passive_mode(control_socket, passive_ip, &passive_port);

    // Data socket connection
    int data_socket = create_socket(passive_ip, passive_port);

    // Request resource
    char request[MAX_BUFFER_SIZE];
    snprintf(request, sizeof(request), "RETR %s\r\n", url.resource_path);
    if (write(control_socket, request, strlen(request)) < 0) {
        rushed_exit(control_socket, data_socket, "Failed to send RETR command");
    }

    // Verify resource request
    while (read(control_socket, response, sizeof(response)) > 0) {
        if (sscanf(response, "%d", &response_code) == 1 && (response_code == 150 || response_code == 125)) {
            break;
        }
    }

    // Download file
    download_file(control_socket, data_socket, url.filename);

    // Close connections
    graceful_exit(control_socket); 

    printf("File downloaded successfully: %s\n", url.filename);
    return EXIT_SUCCESS;
}