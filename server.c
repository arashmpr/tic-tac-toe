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

typedef struct {
    int player_one;
    int player_two;
} waiting_list;

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

int get_rand_port() {
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    int random_socket;

    address.sin_family = AF_INET; 
    address.sin_port = htons(0); 

    random_socket = socket(AF_INET, SOCK_DGRAM, 0);

    while (address.sin_port == 0) {
        bind(random_socket, (struct sockaddr *)&address, sizeof(address));
        getsockname(random_socket, (struct sockaddr *)&address, (socklen_t *) &addr_len);
    }

    close(random_socket);

    return address.sin_port;
}

void add_player(int sock, waiting_list* list) {
    if (list -> player_one == 0) {
        list -> player_one = sock;
    } else if (list -> player_two == 0) {
        list -> player_two = sock;
    }
}

bool players_ready_to_play(waiting_list* list) {
    if (list -> player_one != 0 && list -> player_two != 0) {
        return true;
    }
    return false;
}

int send_port(waiting_list* list) {
    int port = get_rand_port();
    printf("port %d has been activated.\n", port);
    char port_and_id_playe_one[BUFFER_SIZE] = {0};
    char port_and_id_playe_two[BUFFER_SIZE] = {0};

    itoa(port, port_and_id_playe_one);
    itoa(port, port_and_id_playe_two);

    strcat(port_and_id_playe_one, "#1#");
    strcat(port_and_id_playe_two, "#2#");

    send(list -> player_one, port_and_id_playe_one, strlen(port_and_id_playe_one), 0);
    send(list -> player_two, port_and_id_playe_two, strlen(port_and_id_playe_two), 0);

    close(list -> player_one);
    close(list -> player_two);

    return port;
}

void save_port(int port, int* ports_list) {
    for (int i=0 ; i < sizeof(ports_list) ; i++) {
        if (ports_list[i] != 0) {
            ports_list[i] = port;
        }
    }
}

void reset_waiting_list(waiting_list* players_list) {
    players_list -> player_one = 0;
    players_list -> player_two = 0;
}

void handle_player(int client_socket, waiting_list* players_list, int* ports_list) {
    add_player(client_socket, players_list);
    if(players_ready_to_play(players_list)) {
        int rand_port = send_port(players_list);
        save_port(rand_port, ports_list);
        reset_waiting_list(players_list);
    }
}
void handle_viewer(int client_socket, int* ports_list) {
    send(client_socket, ports_list, sizeof(ports_list), 0);
}

void handle_result(int* ports_list, char* result) {
    int winner, port;
    sscanf(result, "%d %d", &winner, &port);
    printf("port %d has been deactivated.\n", port);

    FILE *fp;
    fp = fopen("result.txt", "w+");
    fprintf(fp, "The winner of port %d is Player %d", port, winner);
    fclose(fp);

    //deleting the port from active ports
    for (int i=0 ; i < 100 ; i++) {
        if (ports_list[i] == port) {
            ports_list[i] = 0;
        }
    }
}

void handle_connection(int client_socket, waiting_list* players_list, int* ports_list) {
    char player_or_viewer_buffer[BUFFER_SIZE];
    recv(client_socket, player_or_viewer_buffer, sizeof(player_or_viewer_buffer), 0);

    if (atoi(player_or_viewer_buffer) == 1) { //wants to be a player
        handle_player(client_socket, players_list, ports_list);
    } else if (atoi(player_or_viewer_buffer) == 2) {
        handle_viewer(client_socket, ports_list);
    } else {
        handle_result(ports_list, player_or_viewer_buffer);
    }
}

int main(int argc, char const *argv[])
{
    int server_fd, new_player_fd;
    int rc, opt = 1;
    int port;
    int ports_list[100] = {0};
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE] = {0};
    fd_set master_set, working_set;
    int max_sd, valread;
    waiting_list players_list = {0};

    //creating the socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket creation failed.\n");
        exit(-1);
    }

    //allow socket to be reusable
    rc = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (rc < 0) {
        perror("setsockopt() failed.\n");
        exit(-1);
    }

    //set socket to be non-blocking
    opt = fcntl(server_fd, F_GETFL);
    opt = (opt | O_NONBLOCK);
    rc = fcntl(server_fd, F_SETFL, opt);
    if (rc < 0){
        perror("fcntl() failed\n");
        close(server_fd);
        exit(-1);
   }

    port = argc > 1 ? atoi(argv[1]) : DEFAULT_PORT;
    ports_list[0] = port;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(DEFAULT_PORT);

    int addr_len = sizeof(server_address);
    
    //bind the socket
    rc = bind(server_fd, (struct sockaddr *)&server_address, (socklen_t) addr_len);
    if (rc < 0) {
        perror("bind() failed.\n");
        close(server_fd);
        exit(-1);
    }

    //listen on socket
    rc = listen(server_fd, 4);
    if (rc < 0) {
        perror("listen() failed.\n");
        close(server_fd);
        exit(-1);
    }

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);

    printf("Server is listening...\n");

    while (SERVER_IS_RUNNING) {
        working_set = master_set;
        rc = select(FD_SETSIZE, &working_set, NULL, NULL, NULL);
        if (rc < 0) {
            perror("select() failed\n");
            close(server_fd);
            exit(-1);
        }
        for (int i = 0; i <= FD_SETSIZE; i++) {
            if (FD_ISSET(i, &working_set)) {
                if (i == server_fd) {
                    //there is a new connection that we can accept
                    new_player_fd = accept(server_fd, (struct sockaddr *)&server_address, (socklen_t *) &addr_len);
                    if (new_player_fd < 0) {
                        perror("accept() failed.\n");
                        close(server_fd);
                        exit(-1);
                    }
                    FD_SET(new_player_fd, &master_set);
                }
                else {
                    handle_connection(i, &players_list, ports_list);
                    FD_CLR(i, &master_set);
                }
            }
        }

    }

    return EXIT_SUCCESS; 
} 