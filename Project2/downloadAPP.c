#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#define PORT 21

struct URL {
    char host[500];
    char resource[500];
    char file[500];
    char ip[500];
    char username[500];
    char password[500];
};

int parse(const char *url, struct URL *res) {
    int user = (strstr(url, "@") == NULL);
    int pass = (strstr(url, ":") == NULL);
    if (user && pass) {
        int params_assigned = sscanf(url, "ftp://%[^:]:%[^@]@%[^/]/%s", res->username, res->password, res->host, res->resource);
        if (params_assigned != 4) {
            perror("The FTP URL is invalid\n");
            return 1;
        }
    }
    else {
        strcpy(res->username, "anonymous");
        strcpy(res->password, "anonymous");

        int params_assigned = sscanf(url, "ftp://%[^/]/%s", res->host, res->resource);
        if (params_assigned != 2) {
            perror("The FTP URL is invalid\n");
            return 1;
        }
    }

    strcpy(res->file, strrchr(url, '/') + 1);
    struct hostent *host;
    if ((host = gethostbyname(res->host)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    strcpy(res->ip, inet_ntoa(*((struct in_addr *) host->h_addr_list[0])));

    return 0;
}

    
void openSocket(int *sockfd, const char *ip, int port){
    struct sockaddr_in server_addr;

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);   
    server_addr.sin_port = htons(port);        

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }

    if (connect(*sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
}

void login(int sockfd, const char* username, const char* password) {
    int user_len = 6+strlen(username);
    char user[user_len];
    strcpy(user, "USER ");
    strcat(user, username);
    strcat(user, "\n");  
    write(sockfd, user, user_len);

    char response[1024];

    if (read(sockfd, response, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    int code;
    sscanf(response, "%d", &code);
    printf("%d", code);
    if (code != 331){
        perror("User unknown");
        exit(-1);
    }


    int pass_len = 6+strlen(password);
    char pass[pass_len];
    strcpy(pass, "PASS ");
    strcat(pass, password);
    strcat(pass, "\n");  
    write(sockfd, pass, pass_len);

    if (read(sockfd, response, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    sscanf(response, "%d", &code);
    if (code != 230){
        perror("Wrong password");
        exit(-1);
    }

}

void activatePassive(int sockfd, char *ip, int *port){
    write(sockfd, "pasv\n", 5);
    
    char response[1024];
    if (read(sockfd, response, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    int code;
        sscanf(response, "%d", &code);
    if (code != 227){
        perror("Passive mode inactive");
        exit(-1);
    }

    int byte1, byte2, byte3, byte4, byte5, byte6;
    sscanf(response, "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]", &byte1, &byte2, &byte3, &byte4, &byte5, &byte6);
    sprintf(ip, "%d.%d.%d.%d", byte1, byte2, byte3, byte4);
    *port = (byte5*256)+byte6;   
}

void requestResource(const int sockfd, const char *resource) {
    int user_len = 6+strlen(resource);
    char file[user_len];
    strcpy(file, "retr ");
    strcat(file, resource);  
    strcat(file, "\n");  
    write(sockfd, file, user_len);
    
    char response[1024];
    if (read(sockfd, response, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    int code;
    sscanf(response, "%d", &code);
    if (code != 150){
        perror("Unknown resource");
        exit(-1);
    }
}

void resourceDownload(const int controlSocket, const int dataSocket, char *filename) {
    char buf[1024];
    FILE *fptr; 
    fptr = fopen(filename, "wb");

    if (fptr == NULL) {
        perror("This file wasn't found\n");
        exit(EXIT_FAILURE);
    }

    int bytes_to_read = read(dataSocket, buf, 1024);

    while (bytes_to_read > 0) {

        if (fwrite(buf, bytes_to_read, 1, fptr) < 0) {
            perror("write()");
            exit(-1);
        }

        bytes_to_read = read(dataSocket, buf, 1024);
    }

    fclose(fptr);
    
    char response[1024];
    if (read(controlSocket, response, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    int code;
    sscanf(response, "%d", &code);
    if (code != 226){
        perror("Error while downloading file");
        exit(-1);
    }
}

void closeSocket(const int controlSocket, const int dataSocket){
    write(controlSocket, "quit\n", 5);
    
    char response[1024];
    if (read(controlSocket, response, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    int code;
    sscanf(response, "%d", &code);
    if (code != 221){
        perror("Error while quitting");
        exit(-1);
    }
    close(controlSocket);
    close(dataSocket);  
}

int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        perror("Include a URL");
        exit(-1);
    }

    struct URL url; 

    if (parse(argv[1], &url)){
        perror("Error while parsing the URL");
        exit(-1);
    }
    
    printf("Host: %s\n", url.host);
    printf("Resource: %s\n", url.resource);
    printf("Username: %s\n", url.username);
    printf("Password: %s\n", url.password);
    printf("File: %s\n", url.file);
    printf("IP: %s\n", url.ip);
    
    int sockfd;
    openSocket(&sockfd, url.ip, PORT);
    sleep(1);
    char response[1024];

    if (read(sockfd, response, 1024) < 0) {
        perror("read()");
        exit(-1);
    }

    int code;
    sscanf(response, "%d", &code);
    printf("%d", code);
    if (code != 220){
        perror("Failed to connect to the service");
        exit(-1);
    }

    login(sockfd, url.username, url.password);

    char ip[500];
    int port = 0;
    activatePassive(sockfd, ip, &port);
    
    int sockfdB;
    openSocket(&sockfdB, ip, port);
    
    requestResource(sockfd, url.resource);

    resourceDownload(sockfd, sockfdB, url.file);

    closeSocket(sockfd, sockfdB);
}