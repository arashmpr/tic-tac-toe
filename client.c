#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdbool.h>

#define DEFAULT_PORT 8090
#define OPTION_ONE 1
#define SERVER_IS_RUNNING 1
#define BUFFER_SIZE 1024
#define TWO 2
#define PLAYER "1\n"
#define VIEWER "2\n"

typedef struct {
    int map[3][3];
} Game;

bool someone_wins(Game* game) {
    return false;
}

void play(int port, int id) {
    Game* game = {0};
    int rc;
    char input_buffer[BUFFER_SIZE] = {0};
    int turn = 0; 
    int broadcast = 1, opt = 1;
    int broadcast_socket;
    struct sockaddr_in bc_address;
    int bc_len = sizeof(bc_address);
    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(broadcast_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(port); 
    bc_address.sin_addr.s_addr = inet_addr("172.20.10.15");

    rc = bind(broadcast_socket, (struct sockaddr *)&bc_address, (socklen_t) sizeof(bc_address));
    if (rc < 0) {
        perror("bind() failed.\n");
        exit(-1);
    }

    while(!someone_wins(game)) {
        if (turn % 2 == (id - 1)) {
            printf("It's your turn to play. Please enter the X and Y of your next move: \n");
            read(0, input_buffer, sizeof(input_buffer));
            printf("input buffer is : %s\n", input_buffer);

            rc = sendto(broadcast_socket, input_buffer, sizeof(input_buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
            if (rc < 0) {
                perror("sendto() failed!\n");
                exit(-1);
            }
            turn++;
        } else {
            char ans[1024] = {0};
            printf("It's your opponents turn. Please wait for their move.\n");
            rc = recvfrom(broadcast_socket, ans, 1024, 0, (struct sockaddr *) &bc_address, sizeof(bc_address));
            if (rc < 0) {
                perror("recvfrom() failed.\n");
                exit(-1);
            }
            printf("your opp move: %s\n", ans);
            turn++;
        }
    }
}

int main() 
{
    int valread;
    int rc;
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    int room_ports[1024] = {0};
    char answer[BUFFER_SIZE] = {0};

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(DEFAULT_PORT);

    int addr_len = sizeof(server_address);

    printf("If you want to be a player please enter 1\nIf you want to be a viewer please enter 2\n");
    read(0, answer, BUFFER_SIZE);

    rc = connect(client_fd, (struct sockaddr *)&server_address, (socklen_t) addr_len);
    if (rc < 0) {
        perror("connect() failed.\n");
        close(client_fd);
        exit(-1);
    }
    rc = send(client_fd, answer, sizeof(answer), 0);
    if (rc < 0) {
        perror("send() failed.\n");
        close(client_fd);
        exit(-1);
    }

    if(strcmp(PLAYER, answer) == 0) {
        char info[BUFFER_SIZE] = {0};
        char* room_port;
        char* id;
        recv(client_fd, info, sizeof(info), 0);
        room_port = strtok(info, "#");
        id = strtok(NULL, "#");
        printf("Port that you're going to play on is : %s\n", room_port);
        play(atoi(room_port), atoi(id));
        close(client_fd);
    } else if (strcmp(VIEWER, answer) == 0) {
        recv(client_fd, &room_ports, sizeof(room_ports), 0);
        printf("port you're watching%d\n", room_ports[0]);
    }

    
    
    

    
}