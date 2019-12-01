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
#include <unistd.h>

#define MAX_STR_SIZE 50

#define SERVER_PORT 21
#define USER_MSG "user"
#define PASS_MSG "pass"
#define PASV_MSG "pasv"

#define NEED_PASS 331
#define LOGIN_OK 230

typedef struct hostent hostent_t;

typedef enum cmd {
    LOGIN_CMD,
    PASV_CMD
} cmd_t;

typedef int (*cmd_handler)(int sockfd, va_list args);

hostent_t *getip(char *host, char *readable_addr) {
    hostent_t *h = gethostbyname(host);

    if (h == NULL) {
        herror("gethostbyname");
        exit(1);
    }

    char *server_address = inet_ntoa(*((struct in_addr *)h->h_addr));
    strcpy(readable_addr, server_address);

    return h;
}

int parse_user_info(char *username, char *password, char *host, char *url_path, char *info) {
    // find colon, at and slash characters
    char *colon = strchr(info, ':');
    char *at = strchr(info, '@');
    char *slash = strchr(info, '/');

    // fail if any of those doesnt exist
    if (colon == NULL || at == NULL || slash == NULL) {
        return -1;
    }

    // compute indices for the characters
    unsigned colon_index = colon - info;
    unsigned at_index = at - info;
    unsigned slash_index = slash - info;

    // copy username, password, host and path
    strncpy(username, info, colon_index);
    strncpy(password, &info[colon_index + 1], at_index - colon_index - 1);
    strncpy(host, &info[at_index + 1], slash_index - at_index - 1);
    strncpy(url_path, &info[slash_index + 1], strlen(info) - slash_index - 1);

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

void server_connect(int sockfd, const char *server_addr) {
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

    char *response = calloc(1024, sizeof(char));
    int bytes_read = 0, total_read = 0;

    do {
        bytes_read = read(sockfd, &response[total_read], 1);
        if (bytes_read < 0) {
            perror("read");
            exit(-1);
        }

        total_read++;
    } while (strstr(response, "220 ") == NULL);
    
    printf("%s\n", response);

    free(response);
}

int socket_send(int sockfd, char *buffer) {
    int bytes_written = write(sockfd, buffer, strlen(buffer));

    if (bytes_written < 0) {
        perror("write");
        exit(-1);
    }

    return bytes_written;
}

int socket_recv(int sockfd, char *buffer, int length) {
    int bytes_read = read(sockfd, buffer, length);

    if (bytes_read < 0) {
        perror("read");
        exit(-1);
    }

    return bytes_read;
}

char *build_cmd(char *cmd_type, char *cmd_arg) {
    char *command = calloc(strlen(cmd_type) + strlen(cmd_arg) + 2, sizeof(char));
    sprintf(command, "%s %s\n", cmd_type, cmd_arg);
    printf("> %s\n", command);

    return command;
}

int handle_login(int sockfd, va_list args) {
    int login_ret = 0;
    char *username = va_arg(args, char *);
    char *password = va_arg(args, char *);
    va_end(args);

    char *user_command = build_cmd(USER_MSG, username);
    socket_send(sockfd, user_command);

    char *response = calloc(1024, sizeof(char));
    read(sockfd, response, 2);
    socket_recv(sockfd, response, 1024);

    free(user_command);

    int status_code = atoi(response);
    printf("%s\n", response);

    if (status_code == NEED_PASS) {
        char *pass_command = build_cmd(PASS_MSG, password);
        socket_send(sockfd, pass_command);

        bzero(response, 1024);
        socket_recv(sockfd, response, 1024);
        printf("%s\n", response);

        free(pass_command);

        status_code = atoi(response);
        if (status_code != LOGIN_OK) login_ret = -1;
    }
    
    free(response);
    return login_ret;
}

int handle_pasv(int sockfd, va_list args) {
    va_end(args);

    char *pasv_command = calloc(strlen(PASV_MSG) + 2, sizeof(char));
    sprintf(pasv_command, "%s\n", PASV_MSG);
    printf("> %s\n", pasv_command);
    
    socket_send(sockfd, pasv_command);

    char *response = calloc(1024, sizeof(char));
    socket_recv(sockfd, response, 1024);

    int status_code = atoi(response);
    printf("%s\n", response);

    free(response);
    return 0;
}

static cmd_handler handlers[] =  {handle_login, handle_pasv};

int send_cmd(int sockfd, cmd_t cmd, ...) {
    va_list args;
    va_start(args, cmd);

    return handlers[cmd](sockfd, args);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./download ftp://[<username>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    char *username = calloc(MAX_STR_SIZE, sizeof(char));
    char *host = calloc(MAX_STR_SIZE, sizeof(char));
    char *password = calloc(MAX_STR_SIZE, sizeof(char));
    char *url_path = calloc(MAX_STR_SIZE, sizeof(char));

    if (parse_user_info(username, password, host, url_path, &argv[1][6]) == -1) {
        printf("Usage: ./download ftp://[<username>:<password>@]<host>/<url-path>");
        exit(-1);
    }

    char *server_addr = calloc(16, sizeof(char));

    getip(host, server_addr);

    printf("username: %s\npassword: %s\nhost: %s\nurl_path: %s\nip_addr: %s\n", username, password, host, url_path, server_addr);

    int sockfd = get_socket();
    server_connect(sockfd, server_addr);

    send_cmd(sockfd, LOGIN_CMD, username, password);
    send_cmd(sockfd, PASV_CMD);

    free(username);
    free(host);
    free(password);
    free(url_path);
    free(server_addr);

    return 0;
}
