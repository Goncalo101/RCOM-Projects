#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

typedef struct hostent hostent_t;

#define MAX_STR_SIZE 50

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

hostent_t *getip(char *host)
{
    hostent_t *h;

    if ((h = gethostbyname(host)) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));

    return h;
}

int parse_user_info(char *user, char *password, char *host, char *url_path, char *info)
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

    memcpy(user, info, colon_index);
    puts(user);

    memcpy(password, &info[colon_index + 1], at_index - colon_index - 1);
    puts(password);

    memcpy(host, &info[at_index + 1], slash_index - at_index - 1);
    puts(host);

    memcpy(url_path, &info[slash_index + 1], strlen(info) - slash_index - 1);

    puts(url_path);
    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>");
        exit(-1);
    }

    char *user = malloc(MAX_STR_SIZE);
    char *host = malloc(MAX_STR_SIZE);
    char *password = malloc(MAX_STR_SIZE);
    char *url_path = malloc(MAX_STR_SIZE);

    if (parse_user_info(user, password, host, url_path, &argv[1][6]) == -1)
    {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>");
        exit(-1);
    }

    hostent_t *h = getip(host);

    printf("user: %s, password: %s, host: %s, url_path: %s\n", user, password, host, url_path);

    free(user);
    free(host);
    free(password);
    free(url_path);

    return 0;
}
