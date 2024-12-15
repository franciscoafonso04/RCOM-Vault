#ifndef URL_H
#define URL_H

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

// Function declarations
/**
 * @brief Parses a URL string and fills the URL structure.
 * 
 * @param input_url The URL string to parse.
 * @param url The URL structure to fill.
 * @return int 0 on success, -1 on failure.
 */
int parse_url(const char *input_url, struct URL *url);

/**
 * @brief Displays the details of a URL structure.
 * 
 * @param url The URL structure to display.
 */
void display_url(const struct URL url);

#endif