#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048

void init_winsock() {
    WSADATA wsa;
    printf("Starting Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Initialization failed. Error code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Winsock initialized.\n");
}

int main() {
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    int c, i;
    SOCKET client_sockets[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    // Inicializar o Winsock
    init_winsock();
    
	// Solicitar porta ao administrador
	int server_port;
	printf("Port: ");
	scanf("%d", &server_port);

    // Criar socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Unable to create socket. Error code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Socket created.\n");

    // Preparar a estrutura sockaddr_in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(server_port);

    // Vincular
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind error. Error code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Bind done.\n");

    // Listen
    listen(server_socket, 3);

    printf("Waiting for connections...\n");

    c = sizeof(struct sockaddr_in);

    // Inicializar clientes
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    while (1) {
        // Limpar o conjunto de sockets
        FD_ZERO(&readfds);

        // Adicionar o socket principal ao conjunto
        FD_SET(server_socket, &readfds);
        int max_sd = server_socket;

        // Adicionar sockets filhos ao conjunto
        for (i = 0; i < MAX_CLIENTS; i++) {
            SOCKET s = client_sockets[i];

            if (s > 0)
                FD_SET(s, &readfds);

            if (s > max_sd)
                max_sd = s;
        }

        // Esperar por atividade em um dos sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            printf("select error: %d\n", WSAGetLastError());
        }

        // Se algo acontecer no socket principal, então é uma conexão nova
        if (FD_ISSET(server_socket, &readfds)) {
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &c)) < 0) {
                printf("accept failed with error: %d\n", WSAGetLastError());
                exit(EXIT_FAILURE);
            }

            printf("\nNew connection, socket fd: %d, ip: %s, port: %d \n", client_socket, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            // Adicionar novo socket ao array de sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_socket;
                    printf("Adding the list of sockets at position %d\n", i);

                    break;
                }
            }
        }

        // Checar outros sockets
        for (i = 0; i < MAX_CLIENTS; i++) {
            SOCKET s = client_sockets[i];

            if (FD_ISSET(s, &readfds)) {
                int valread;
                if ((valread = recv(s, buffer, BUFFER_SIZE, 0)) == SOCKET_ERROR) {
                    int error_code = WSAGetLastError();
                    if (error_code == WSAECONNRESET) {
                        printf("\nHost disconnected, ip %s, port %d \n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                        closesocket(s);
                        client_sockets[i] = 0;
                    } else {
                        printf("\nrecv failed with error code: %d\n", error_code);
                    }
                } else if (valread == 0) {
                    printf("\nClient disconnected, ip %s, port %d \n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                    closesocket(s);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != 0 && client_sockets[j] != s) {
                            send(client_sockets[j], buffer, strlen(buffer), 0);
                        }
                    }
                }
            }
        }
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}