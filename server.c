#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define DEFAULT_PORT 8000

int run_socket(int port);
int generate_random_port();
int send_game_room_port();

int main() {
    int server_fd = run_socket(DEFAULT_PORT);

}

int run_socket(int port) {
    int fd;
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet("127.0.0.1");

    if(!(fd = socket(AF_INET, SOCK_STREAM, 0))) {
        printf("Failed to create socket on port %d!\nExiting..", port);
        exit(1);
    }
}