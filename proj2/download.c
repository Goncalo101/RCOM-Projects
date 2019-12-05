#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
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
#define RETR_MSG "retr"

#define NEED_PASS 331
#define LOGIN_OK 230
#define PASV_OK 227

// ./download ftp://anonymous:1@ftp.up.pt/pub/CentOS/2.1/readme.txt

typedef struct hostent hostent_t;

typedef struct User {
    char *username;
    char *password;
    char *host;
    char *url_path;
    char *filename;
    int sockfd_client;
} user_t;

typedef enum cmd
{
    LOGIN_CMD,
    PASV_CMD,
    RETR_CMD
} cmd_t;


user_t user_info;
typedef int (*cmd_handler)(int sockfd, va_list args);

hostent_t *getip(char *host, char *readable_addr)
{
    hostent_t *h = gethostbyname(host);

    if (h == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    char *server_address = inet_ntoa(*((struct in_addr *)h->h_addr));
    strcpy(readable_addr, server_address);

    return h;
}

//int parse_user_info(char *username, char *password, char *host, char *url_path, char *info)
int parse_user_info(char *info)
{
    user_info.username = calloc(MAX_STR_SIZE, sizeof(char));
    user_info.host = calloc(MAX_STR_SIZE, sizeof(char));
    user_info.password = calloc(MAX_STR_SIZE, sizeof(char));
    user_info.url_path = calloc(MAX_STR_SIZE, sizeof(char));
    user_info.filename = calloc(MAX_STR_SIZE, sizeof(char));

    // find colon, at and slash characters
    char *colon = strchr(info, ':');
    char *at = strchr(info, '@');
    char *slash = strchr(info, '/');
    char *last_slash = strrchr(info, '/');

    // fail if any of those doesnt exist
    if (colon == NULL || at == NULL || slash == NULL)
    {
        return -1;
    }

    // compute indices for the characters
    unsigned colon_index = colon - info;
    unsigned at_index = at - info;
    unsigned slash_index = slash - info;
    unsigned last_slash_index = last_slash - info;

    // copy username, password, host and path
    strncpy(user_info.username, info, colon_index);
    strncpy(user_info.password, &info[colon_index + 1], at_index - colon_index - 1);
    strncpy(user_info.host, &info[at_index + 1], slash_index - at_index - 1);
    strncpy(user_info.url_path, &info[slash_index + 1], strlen(info) - slash_index - 1);
    strncpy(user_info.filename, &info[last_slash_index +1], strlen(info) - last_slash_index - 1);

    return 0;
}

int get_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        exit(-1);
    }

    return sockfd;
}

void establish_connection(int sockfd, const char *server_addr, int server_port)
{
    struct sockaddr_in sock_addr;

    /*server address handling*/
    bzero((char *)&sock_addr, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(server_addr); /*32 bit Internet address network byte ordered*/
    sock_addr.sin_port = htons(server_port);            /*server TCP port must be network byte ordered */

    /*connect to the server*/
    int connection_status = connect(sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));

    if (connection_status < 0)
    {
        perror("connect");
        exit(-1);
    }
}

void server_connect(int sockfd, const char *server_addr)
{
    establish_connection(sockfd, server_addr, SERVER_PORT);

    char *response = calloc(1024, sizeof(char));
    int bytes_read = 0, total_read = 0;

    do
    {
        bytes_read = read(sockfd, &response[total_read], 1);
        if (bytes_read < 0)
        {
            perror("read");
            exit(-1);
        }

        total_read++;
    } while (strstr(response, "220 ") == NULL);

    printf("%s\n", response);

    free(response);
}

int socket_send(int sockfd, char *buffer)
{
    int bytes_written = write(sockfd, buffer, strlen(buffer));

    if (bytes_written < 0)
    {
        perror("write");
        exit(-1);
    }

    return bytes_written;
}

int socket_recv(int sockfd, char *buffer, int length)
{
    int bytes_read = read(sockfd, buffer, length);

    if (bytes_read < 0)
    {
        perror("read");
        exit(-1);
    }

    return bytes_read;
}

char *build_cmd(char *cmd_type, char *cmd_arg)
{
    char *command = calloc(strlen(cmd_type) + strlen(cmd_arg) + 2, sizeof(char));
    sprintf(command, "%s %s\n", cmd_type, cmd_arg);
    printf("> %s\n", command);

    return command;
}

void create_file(int sockfd)
{
    int fd = open(user_info.filename, O_WRONLY | O_CREAT, 0666);

    int bytes_written;
    char buf[1000];

    while((bytes_written = read(user_info.sockfd_client, buf, 1)) > 0)
        write(fd, buf, 1);
    
    puts("> File downloaded succesfully.");
}

int handle_retr(int sockfd, va_list args)
{
    va_end(args);

    char *response = malloc(1024);

    write(sockfd, "retr ", strlen("retr "));
    write(sockfd, user_info.url_path, strlen(user_info.url_path));
    write(sockfd, "\n", 1);

    read(sockfd, response, 1024);
    puts(response);
    create_file(sockfd);

    return 0;
}

int handle_login(int sockfd, va_list args)
{
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

    if (status_code == NEED_PASS)
    {
        char *pass_command = build_cmd(PASS_MSG, password);
        socket_send(sockfd, pass_command);

        bzero(response, 1024);
        socket_recv(sockfd, response, 1024);
        printf("%s\n", response);

        free(pass_command);

        status_code = atoi(response);
        if (status_code != LOGIN_OK)
            login_ret = -1;
    }

    free(response);
    return login_ret;
}

int handle_pasv(int sockfd, va_list args)
{
    va_end(args);
    int port = 0;

    char *pasv_command = calloc(strlen(PASV_MSG) + 2, sizeof(char));
    sprintf(pasv_command, "%s\n", PASV_MSG);
    printf("> %s\n", pasv_command);

    socket_send(sockfd, pasv_command);

    char *response = calloc(1024, sizeof(char));
    socket_recv(sockfd, response, 1024);

    int status_code = atoi(response);
    printf("%s\n", response);

    if (status_code != PASV_OK)
        return -1;

    // Get the last two bytes to calculate port number
    char *tok = strtok(strchr(response, '('), ",)");
    int i = 0;
    while (tok != NULL)
    {
        tok = strtok(NULL, ",)");
        if (i == 3)
            port = 256 * atoi(tok);
        else if (i == 4)
            port += atoi(tok);
        ++i;
    }

    free(response);
    free(tok);

    return port;
}

static cmd_handler handlers[] = {handle_login, handle_pasv, handle_retr};

int send_cmd(int sockfd, cmd_t cmd, ...)
{
    va_list args;
    va_start(args, cmd);

    return handlers[cmd](sockfd, args);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./download ftp://[<username>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    if (parse_user_info(&argv[1][6]) == -1)
    {
        printf("Usage: ./download ftp://[<username>:<password>@]<host>/<url-path>");
        exit(-1);
    }

    char *server_addr = calloc(16, sizeof(char));

    getip(user_info.host, server_addr);

    printf("username: %s\npassword: %s\nhost: %s\nurl_path: %s\nfilename: %s\nip_addr: %s\n", user_info.username, user_info.password, user_info.host, user_info.url_path, user_info.filename, server_addr);

    int sockfd = get_socket();
    server_connect(sockfd, server_addr);
    int pasv_port = 0;
    if (send_cmd(sockfd, LOGIN_CMD, user_info.username, user_info.password) == -1 ||
        (pasv_port = send_cmd(sockfd, PASV_CMD)) == -1)
    {
        printf("> Invalid login and/or password\n");
        exit(-1);
    }
    
    if ((user_info.sockfd_client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(0);
    }

    establish_connection(user_info.sockfd_client, server_addr, pasv_port);


    if(send_cmd(sockfd, RETR_CMD) == -1)
    {
        printf("> Error retrieving file");
        exit(-1);
    }

    free(user_info.username);
    free(user_info.host);
    free(user_info.password);
    free(user_info.url_path);
    free(user_info.filename);
    free(server_addr);

    return 0;
}
