#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

typedef struct hostent hostent_t;
typedef int (*cmd_handler)(int sockfd, va_list args);

typedef enum cmd {
    USER_CMD,
    PASS_CMD,
    PASV_CMD
} cmd_t;

#define MAX_STR_SIZE 50

#define SERVER_PORT 21
#define USER_MSG "user "

hostent_t *getip(char *host, char* readable_addr)
{
    hostent_t *h = gethostbyname(host);

    if (h == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    char *server_address = inet_ntoa(*((struct in_addr *)h->h_addr));

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", server_address);
    
    strcpy(readable_addr, server_address);

    return h;
}

int parse_user_info(char *username, char *password, char *host, char *url_path, char *info)
{
    char *colon = strchr(info, ':');
    char *at = strchr(info, '@');
    char *slash = strchr(info, '/');

    if (colon == NULL || at == NULL || slash == NULL)
    {
        return -1;
    }

    unsigned colon_index = colon - info;
    unsigned at_index = at - info;
    unsigned slash_index = slash - info;

    memcpy(username, info, colon_index);
    puts(username);

    memcpy(password, &info[colon_index + 1], at_index - colon_index - 1);
    puts(password);

    memcpy(host, &info[at_index + 1], slash_index - at_index - 1);
    puts(host);

    memcpy(url_path, &info[slash_index + 1], strlen(info) - slash_index - 1);

    puts(url_path);
    return 0;
}

int get_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket");
        exit(-1);
    }

    return sockfd;
}

void server_connect(int sockfd, const char* server_addr) {
	struct sockaddr_in sock_addr;

    /*server address handling*/
	bzero((char*) &sock_addr, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(server_addr);	/*32 bit Internet address network byte ordered*/
	sock_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

	/*connect to the server*/
    int connection_status = connect(sockfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr));

    if (connection_status < 0) {
    	perror("connect");
		exit(-1);
	}

    char response[65535];
    int bytes_read = 1;
    
    do {
        bytes_read = read(sockfd, response, 65535);
        printf("< %s\n", response);
    } while (strcmp(response, "220 \n") != 0);
}

int handle_user(int sockfd, va_list args) {
    char *username = va_arg(args, char*);
    char *cmd = malloc(strlen(USER_MSG) + strlen(username) + 1);
    sprintf(cmd, "%s%s", USER_MSG, username);

    printf("> %s\n", cmd);

    char response[10000];
    int bytes_written = write(sockfd, cmd, strlen(cmd));
    int bytes_read = read(sockfd, response, 10000);
    printf("< %s\n", response);
    return 0;
}

static cmd_handler handlers[] =  {handle_user};

int send_cmd(int sockfd, cmd_t cmd, ...) {
    va_list args;
    va_start(args, cmd);

    printf("cmd: %d\n", cmd);

    handlers[cmd](sockfd, args);
    va_end(args);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./download ftp://[<username>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    char *username = malloc(MAX_STR_SIZE);
    char *host = malloc(MAX_STR_SIZE);
    char *password = malloc(MAX_STR_SIZE);
    char *url_path = malloc(MAX_STR_SIZE);

    if (parse_user_info(username, password, host, url_path, &argv[1][6]) == -1)
    {
        printf("Usage: ./download ftp://[<username>:<password>@]<host>/<url-path>");
        exit(-1);
    }

    char server_addr[12];

    hostent_t *h = getip(host, server_addr);

    printf("username: %s\n password: %s\n host: %s\n url_path: %s\n ip_addr: %s\n", username, password, host, url_path, server_addr);

    int sockfd = get_socket();
    server_connect(sockfd, server_addr);

    send_cmd(sockfd, USER_CMD, username);

    free(username);
    free(host);
    free(password);
    free(url_path);

    return 0;
}
