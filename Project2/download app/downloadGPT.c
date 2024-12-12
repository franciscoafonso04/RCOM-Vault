#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define MAX_LENGTH  500
#define FTP_PORT    21

/* Server responses */
#define SV_READY4AUTH           220
#define SV_READY4PASS           331
#define SV_LOGINSUCCESS         230
#define SV_PASSIVE              227
#define SV_READY4TRANSFER       150
#define SV_TRANSFER_COMPLETE    226
#define SV_GOODBYE              221

/* Parser regular expressions */
#define AT              "@"
#define BAR             "/"
#define HOST_REGEX      "%*[^/]//%[^/]"
#define HOST_AT_REGEX   "%*[^/]//%*[^@]@%[^/]"
#define RESOURCE_REGEX  "%*[^/]//%*[^/]/%s"
#define USER_REGEX      "%*[^/]//%[^:/]"
#define PASS_REGEX      "%*[^/]//%*[^:]:%[^@\n$]"
#define RESPCODE_REGEX  "%d"
#define PASSIVE_REGEX   "%*[^(](%d,%d,%d,%d,%d,%d)%*[\^\n$)]"

/* Default login for anonymous FTP */
#define DEFAULT_USER        "anonymous"
#define DEFAULT_PASSWORD    "password"

struct URL {
    char host[MAX_LENGTH];      // Hostname
    char resource[MAX_LENGTH];  // Resource path
    char file[MAX_LENGTH];      // File name
    char user[MAX_LENGTH];      // Username
    char password[MAX_LENGTH];  // Password
    char ip[MAX_LENGTH];        // IP address
};

typedef enum {
    START,
    SINGLE,
    MULTIPLE,
    END
} ResponseState;

/* Function prototypes */
int parseURL(char *input, struct URL *url);
int createSocket(char *ip, int port);
int authenticate(int socket, const char *user, const char *password);
int enterPassiveMode(int socket, char *ip, int *port);
int requestFile(int socket, const char *resource);
int downloadFile(int controlSocket, int dataSocket, const char *filename);
int closeConnections(int controlSocket, int dataSocket);
int readResponse(int socket, char *buffer);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct URL url;
    memset(&url, 0, sizeof(url));

    if (parseURL(argv[1], &url) != 0) {
        fprintf(stderr, "Failed to parse URL.\n");
        exit(EXIT_FAILURE);
    }

    printf("Host: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n",
           url.host, url.resource, url.file, url.user, url.password, url.ip);

    char response[MAX_LENGTH];
    int controlSocket = createSocket(url.ip, FTP_PORT);

    if (controlSocket < 0 || readResponse(controlSocket, response) != SV_READY4AUTH) {
        fprintf(stderr, "Failed to connect to server.\n");
        exit(EXIT_FAILURE);
    }

    if (authenticate(controlSocket, url.user, url.password) != SV_LOGINSUCCESS) {
        fprintf(stderr, "Authentication failed.\n");
        close(controlSocket);
        exit(EXIT_FAILURE);
    }

    char dataIP[MAX_LENGTH];
    int dataPort;
    if (enterPassiveMode(controlSocket, dataIP, &dataPort) != SV_PASSIVE) {
        fprintf(stderr, "Failed to enter passive mode.\n");
        close(controlSocket);
        exit(EXIT_FAILURE);
    }

    int dataSocket = createSocket(dataIP, dataPort);
    if (dataSocket < 0) {
        fprintf(stderr, "Failed to establish data connection.\n");
        close(controlSocket);
        exit(EXIT_FAILURE);
    }

    if (requestFile(controlSocket, url.resource) != SV_READY4TRANSFER) {
        fprintf(stderr, "Failed to request file.\n");
        closeConnections(controlSocket, dataSocket);
        exit(EXIT_FAILURE);
    }

    if (downloadFile(controlSocket, dataSocket, url.file) != SV_TRANSFER_COMPLETE) {
        fprintf(stderr, "Failed to download file.\n");
        closeConnections(controlSocket, dataSocket);
        exit(EXIT_FAILURE);
    }

    if (closeConnections(controlSocket, dataSocket) != 0) {
        fprintf(stderr, "Failed to close connections.\n");
        exit(EXIT_FAILURE);
    }

    printf("File downloaded successfully.\n");
    return 0;
}

int parseURL(char *input, struct URL *url) {
    regex_t regex;
    regcomp(&regex, BAR, 0);
    if (regexec(&regex, input, 0, NULL, 0)) return -1;

    regcomp(&regex, AT, 0);
    if (regexec(&regex, input, 0, NULL, 0) != 0) {
        sscanf(input, HOST_REGEX, url->host);
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
    } else {
        sscanf(input, HOST_AT_REGEX, url->host);
        sscanf(input, USER_REGEX, url->user);
        sscanf(input, PASS_REGEX, url->password);
    }

    sscanf(input, RESOURCE_REGEX, url->resource);
    strcpy(url->file, strrchr(input, '/') + 1);

    struct hostent *h;
    if ((h = gethostbyname(url->host)) == NULL) {
        fprintf(stderr, "Invalid hostname '%s'.\n", url->host);
        exit(EXIT_FAILURE);
    }

    strcpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr_list[0])));
    return 0;
}

int createSocket(char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return -1;
    }

    return sockfd;
}

int authenticate(int socket, const char *user, const char *password) {
    char userCommand[MAX_LENGTH], passCommand[MAX_LENGTH], response[MAX_LENGTH];

    sprintf(userCommand, "USER %s\r\n", user);
    write(socket, userCommand, strlen(userCommand));
    if (readResponse(socket, response) != SV_READY4PASS) {
        fprintf(stderr, "Invalid username.\n");
        return -1;
    }

    sprintf(passCommand, "PASS %s\r\n", password);
    write(socket, passCommand, strlen(passCommand));
    return readResponse(socket, response);
}

int enterPassiveMode(int socket, char *ip, int *port) {
    char response[MAX_LENGTH];
    int ip1, ip2, ip3, ip4, port1, port2;

    write(socket, "PASV\r\n", 6);
    if (readResponse(socket, response) != SV_PASSIVE) return -1;

    sscanf(response, PASSIVE_REGEX, &ip1, &ip2, &ip3, &ip4, &port1, &port2);
    *port = port1 * 256 + port2;
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

    return SV_PASSIVE;
}

int requestFile(int socket, const char *resource) {
    char command[MAX_LENGTH], response[MAX_LENGTH];

    sprintf(command, "RETR %s\r\n", resource);
    write(socket, command, strlen(command));
    return readResponse(socket, response);
}

int downloadFile(int controlSocket, int dataSocket, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    char buffer[MAX_LENGTH];
    int bytes;
    while ((bytes = read(dataSocket, buffer, MAX_LENGTH)) > 0) {
        fwrite(buffer, 1, bytes, file);
    }

    fclose(file);

    char response[MAX_LENGTH];
    return readResponse(controlSocket, response);
}

int closeConnections(int controlSocket, int dataSocket) {
    char response[MAX_LENGTH];
    write(controlSocket, "QUIT\r\n", 6);
    readResponse(controlSocket, response);
    return close(controlSocket) || close(dataSocket);
}

int readResponse(int socket, char *buffer) {
    memset(buffer, 0, MAX_LENGTH);
    int index = 0, responseCode;
    ResponseState state = START;
    char byte;

    while (state != END) {
        read(socket, &byte, 1);
        switch (state) {
            case START:
                if (byte == ' ') state = SINGLE;
                else if (byte == '-') state = MULTIPLE;
                else if (byte == '\n') state = END;
                buffer[index++] = byte;
                break;
            case SINGLE:
                if (byte == '\n') state = END;
                buffer[index++] = byte;
                break;
            case MULTIPLE:
                if (byte == '\n') {
                    memset(buffer, 0, MAX_LENGTH);
                    state = START;
                    index = 0;
                } else {
                    buffer[index++] = byte;
                }
                break;
            case END:
                break;
        }
    }

    sscanf(buffer, RESPCODE_REGEX, &responseCode);
    return responseCode;
}
