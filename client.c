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
#define PLAYER_ONE 1
#define PLAYER_TWO 2
#define DRAW 3

typedef struct {
    int map[3][3];
} Game;

char* itoa(int num, char* str) {
    char digits[] = "0123456789";
    char temp[16];
    int i = 0, j = 0;
    do {
        temp[i++] = digits[num % 10];
        num /= 10;
    } while (num > 0);
    
    while (--i >= 0)
        str[j++] = temp[i];
    str[j] = '\0';

    return str;
}

int someone_wins(Game* game) {
    for (int i=0 ; i < 3 ; i++) {
        //checks all the columns
        if(game -> map[i][0] == game -> map[i][1] && game -> map[i][1] == game -> map[i][2] && game -> map[i][0] != 0) {
            return game -> map[i][0];
        }
        //checks all the rows
        if(game -> map[0][i] == game -> map[1][i] && game -> map[1][i] == game -> map[2][i] && game -> map[0][i] != 0) {
            return game -> map[0][i];
        }
    }
    //checks the diagonal
    if(game -> map[0][0] == game -> map[1][1] && game -> map[1][1] == game -> map[2][2] && game -> map[1][1] != 0) {
        return game -> map[0][0];
    }
    //checks the diagonal
    if(game -> map[0][2] == game -> map[1][1] && game -> map[1][1] == game -> map[2][0] && game -> map[1][1] != 0) {
        return game -> map[1][1];
    }
    return 0;
}

bool is_move_valid(Game* game, char* input) {
    int x, y;
    sscanf(input, "%d %d\n", &x, &y);

    if(game -> map[x][y] != 0) {
        return false;
    }
    return true;
}

void draw_map(Game* game) {
    for (int i = 0 ; i < 3 ; i++) {
        for (int j = 0 ; j < 3 ; j++) {
            if (game -> map[i][j] != 0) {
                printf("%d ", game -> map[i][j]);
            } else {
                printf("# ");
            }
        }
        printf("\n");
    }
}

void process_input(Game* game, char* input) {
    int x, y, id;
    sscanf(input, "%d %d\n%d", &x, &y, &id);
    game -> map[x][y] = id;
}

int* play(int port, char* _id) {
    int winner;
    int result[2] = {0};
    int id = atoi(_id);
    Game game = {0};
    char id_str[128] = {0};
    int rc;
    char input_buffer[BUFFER_SIZE] = {0};
    char turn_info[BUFFER_SIZE] = {0};
    int turn = 0; 
    int broadcast = 1, opt = 1;
    int broadcast_socket;
    struct sockaddr_in bc_address;
    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(broadcast_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(port); 
    bc_address.sin_addr.s_addr = INADDR_ANY;

    int bc_len = sizeof(bc_address);

    rc = bind(broadcast_socket, (struct sockaddr *)&bc_address, (socklen_t) sizeof(bc_address));
    if (rc < 0) {
        perror("bind() failed.\n");
        exit(-1);
    }

    while(!(winner = someone_wins(&game))) {
        if (turn % 2 == (id - 1)) {
            printf("It's your turn to play. Please enter the X and Y of your next move: \n");
            read(0, input_buffer, sizeof(input_buffer));
            while(!is_move_valid(&game, input_buffer)) {
                memset(input_buffer, 0, BUFFER_SIZE);
                read(0, input_buffer, sizeof(input_buffer)); 
            }
            strcpy(turn_info, input_buffer);
            strcat(turn_info, _id);
            
            
            rc = sendto(broadcast_socket, turn_info, sizeof(turn_info), 0, (struct sockaddr *)&bc_address, bc_len);
            if (rc < 0) {
                perror("sendto() failed!\n");
                exit(-1);
            }
            turn++;
        } else {
            char ans[1024] = {0};
            printf("It's your opponents turn. Please wait for their move.\n");
            rc = recvfrom(broadcast_socket, ans, 1024, 0, (struct sockaddr *) &bc_address, (socklen_t*) &bc_len);

            process_input(&game, ans);
            draw_map(&game);
            if (rc < 0) {
                perror("recvfrom() failed.\n");
                exit(-1);
            }
            turn++;
        }
    }
    result[0] = winner;
    result[1] = port;

    return result;
}

void see_ports_list(int* ports) {
    printf("Available Ports: \n");
    for (int i = 0 ; i < 100 ; i++) {
        if (ports[i] != 0) {
            printf("%d\n", ports[i]);
        }
    }
}

int choose_port(int* ports_list) {
    char chosen_port[128] = {0};
    read(0, chosen_port, sizeof(chosen_port));
    for (int i = 0 ; i < 100 ; i++) {
        if(atoi(chosen_port) == ports_list[i]) {
            return atoi(chosen_port);
        }
    }
}

int main(int argc, char const *argv[]) 
{
    int valread;
    int rc;
    int port;
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    int ports_list[100] = {0};
    char answer[BUFFER_SIZE] = {0};

    port = argc > 1 ? atoi(argv[1]) : DEFAULT_PORT;
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

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
        int rc;
        char* room_port;
        char* id;
        int* result;
        recv(client_fd, info, sizeof(info), 0);
        room_port = strtok(info, "#");
        id = strtok(NULL, "#");
        printf("Port that you're going to play on is : %s\n", room_port);
        result = play(atoi(room_port), id);
        rc = send(client_fd, result, sizeof(result), 0);
        if (rc < 0) {
            perror("send result failed()!\n");
            exit(-1);
        }
        close(client_fd);
    } else if (strcmp(VIEWER, answer) == 0) {
        recv(client_fd, ports_list, sizeof(ports_list), 0);
        see_ports_list(ports_list);
        int chosen_port = choose_port(ports_list);
        printf("You're watching on port: %d\n", chosen_port);
    }

    
    
    

    
}