#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define HS110_PORT  (9999)
#define HS110_KEY   (171)


static char *encrypt(char *message, size_t message_len)
{
    char key = HS110_KEY;
    for (size_t i = 0u; i < message_len; i++) {
        message[i] ^= key;
        key = message[i];
    }
    return message;
}

static char *decrypt(char *message, size_t message_len)
{
    char key = HS110_KEY;
    for (size_t i = 0u; i < message_len; i++) {
        char temp = message[i];
        message[i] ^= key;
        key = temp;
    }
    return message;
}

// Note: message buffer will be rewritten, response will be printed to stdout
static int send_message(int sockfd, char *message)
{
    // encrypt message
    ssize_t message_len = strlen(message);
    message = encrypt(message, message_len);
    assert(message != NULL);

    // send message and message size
    uint32_t encryped_size = htonl(message_len);
    if (send(sockfd, &encryped_size, sizeof(encryped_size), 0) < 0) {
        fprintf(stderr, "Error: failed to send message size(%d): %s\n", errno, strerror(errno));
        return -errno;
    }
    if (send(sockfd, message, message_len, 0) < 0) {
        fprintf(stderr, "Error: failed to send message(%d): %s\n", errno, strerror(errno));
        return -errno;
    }

    // receive response and response size
    if (recv(sockfd, &encryped_size, sizeof(encryped_size), MSG_WAITALL) < 0) {
        fprintf(stderr, "Error: failed to receive message size(%d): %s\n", errno, strerror(errno));
        return -errno;
    }
    message_len = ntohl(encryped_size);
    char buffer[message_len + 1];
    if (recv(sockfd, buffer, sizeof(buffer), MSG_WAITALL) < 0) {
        fprintf(stderr, "Error: failed to receive message(%d): %s\n", errno, strerror(errno));
        return -errno;
    }

    // decrypt message
    buffer[message_len] = '\0';
    message = decrypt(buffer, message_len);
    assert(message != NULL);

    fprintf(stdout, "%s\n", message);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s IP [command]\n"
                        "Example: %s 192.168.1.2\n"
                        "List of commands: https://github.com/kettenbach-it/FHEM-TPLink-HS110/blob/master/tplink-smarthome-commands.txt\n",
                        argv[0], argv[0]);
        return -EINVAL;
    }

    // Address resolving
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HS110_PORT);
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        const struct hostent *host_ip = gethostbyname(argv[1]);
        if (host_ip == NULL) {
            fprintf(stderr, "Error: failed to resolve host(%d): \"%s\"\n", errno, argv[1]);
            return -errno;
        }
        memcpy(&addr.sin_addr, host_ip->h_addr_list[0], host_ip->h_length);
    }

    // Socket creation
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: failed to create socket(%d): %s\n", errno, strerror(errno));
        return -errno;
    }

    // Establish connection
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: failed to establish connection(%d): %s\n", errno, strerror(errno));
        close(sockfd);
        return -errno;
    }

    if (argc < 3) {
        char buffer[256];
        while (!feof(stdin)) {
            char *message = fgets(buffer, sizeof(buffer), stdin);
            if (message == NULL) break;
            if (message[strlen(message) - 1] == '\n') {
                message[strlen(message) - 1] = '\0';
            }

            if (send_message(sockfd, message) < 0) {
                close(sockfd);
                return -errno;
            }
        }
    } else {
        char message[strlen(argv[2]) + 1];
        strcpy(message, argv[2]);
        if (send_message(sockfd, message) < 0) {
            close(sockfd);
            return -errno;
        }
    }

    close(sockfd);
    return 0;
}
