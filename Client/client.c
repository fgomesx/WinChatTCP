#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 2048

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
void AddControls(HWND);
void ReceiveMessages(void *param);

SOCKET client_socket;
char buffer[BUFFER_SIZE];
char username[50];
HWND hEditIn, hEditOut, hUsername, hServerIP, hServerPort;

void init_winsock() {
    WSADATA wsa;
    printf("Starting Winshock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Initialization failed. Error code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Winsock initialized.\n");
}

void connect_to_server(HWND hWnd) {
    struct sockaddr_in server;
    char server_ip[20], server_port_str[10];
    int server_port;

    GetWindowText(hServerIP, server_ip, sizeof(server_ip));
    GetWindowText(hServerPort, server_port_str, sizeof(server_port_str));
    server_port = atoi(server_port_str);

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        MessageBox(hWnd, "Unable to create socket.", "Error", MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }
    printf("Socket created.\n");

    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);

    if (connect(client_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        MessageBox(hWnd, "Unable to connect to server.\nCheck if the server is available.", "Connection Error!", MB_OK | MB_ICONERROR);
        closesocket(client_socket);
        return;
    }
    printf("Connected to the server.\n");

    _beginthread(ReceiveMessages, 0, NULL);

    EnableWindow(hEditIn, TRUE);
    EnableWindow(hUsername, TRUE);
    EnableWindow(hWnd, TRUE);
    
    MessageBox(hWnd, "Successfully connected to the server!", "Connection established", MB_OK | MB_ICONINFORMATION);
}

void send_message(const char *message) {
    char full_message[BUFFER_SIZE];
    sprintf(full_message, "%s: %s", username, message);

    if (send(client_socket, full_message, strlen(full_message), 0) < 0) {
        printf("Failed to send message\n");
    } else {
        int len = GetWindowTextLength(hEditOut);
        SendMessage(hEditOut, EM_SETSEL, len, len);
        SendMessage(hEditOut, EM_REPLACESEL, 0, (LPARAM)full_message);
        SendMessage(hEditOut, EM_REPLACESEL, 0, (LPARAM)"\r\n");
    }
}

void ReceiveMessages(void *param) {
    int recv_size;
    while ((recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) != SOCKET_ERROR) {
        buffer[recv_size] = '\0';
        int len = GetWindowTextLength(hEditOut);
        SendMessage(hEditOut, EM_SETSEL, len, len);
        SendMessage(hEditOut, EM_REPLACESEL, 0, (LPARAM)buffer);
        SendMessage(hEditOut, EM_REPLACESEL, 0, (LPARAM)"\r\n");
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
    WNDCLASS wc = {0};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = "myWindowClass";
    wc.lpfnWndProc = WindowProcedure;

    if (!RegisterClass(&wc)) return -1;

    CreateWindow("myWindowClass", "WinChatTCP", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 600, 520, NULL, NULL, hInst, NULL);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, NULL, NULL)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_COMMAND:
            if (wp == 1) {
                char message[BUFFER_SIZE];
                GetWindowText(hUsername, username, 50);
                
                // Verifica se o nome de usuário está vazio
                if (strlen(username) == 0) {
                    MessageBox(hWnd, "Please enter a username.", "Warning!", MB_OK | MB_ICONWARNING);
                    return 0;  // Does not send the message if the username is empty
                }
                
                // Verifica se a mensagem está vazia
                GetWindowText(hEditIn, message, BUFFER_SIZE);
                if (strlen(message) > 0) {
                    send_message(message);
                    SetWindowText(hEditIn, "");
                }
            } else if (wp == 6) {
                connect_to_server(hWnd);
            }
            break;
        case WM_CREATE:
            AddControls(hWnd);
            init_winsock();
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, msg, wp, lp);
    }
    return 0;
}

void AddControls(HWND hWnd) {
    CreateWindow("static", "Server IP:", WS_VISIBLE | WS_CHILD, 50, 20, 100, 20, hWnd, NULL, NULL, NULL);
    hServerIP = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 150, 20, 200, 20, hWnd, (HMENU)4, NULL, NULL);

    CreateWindow("static", "Server Port:", WS_VISIBLE | WS_CHILD, 50, 60, 100, 20, hWnd, NULL, NULL, NULL);
    hServerPort = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 150, 60, 200, 20, hWnd, (HMENU)5, NULL, NULL);

    CreateWindow("button", "Connect", WS_VISIBLE | WS_CHILD, 380, 20, 75, 60, hWnd, (HMENU)6, NULL, NULL);

    CreateWindow("static", "Username:", WS_VISIBLE | WS_CHILD, 50, 110, 100, 20, hWnd, NULL, NULL, NULL);
    hUsername = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 150, 110, 200, 20, hWnd, (HMENU)2, NULL, NULL);
    EnableWindow(hUsername, FALSE);

    CreateWindow("static", "Chat:", WS_VISIBLE | WS_CHILD, 50, 160, 100, 20, hWnd, NULL, NULL, NULL);
    hEditOut = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, 50, 180, 500, 200, hWnd, (HMENU)3, NULL, NULL);

	CreateWindow("static", "Message:", WS_VISIBLE | WS_CHILD, 50, 400, 100, 20, hWnd, NULL, NULL, NULL);
    hEditIn = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 130, 400, 300, 50, hWnd, (HMENU)1, NULL, NULL);
    EnableWindow(hEditIn, FALSE);
    CreateWindow("button", "Send", WS_VISIBLE | WS_CHILD, 450, 400, 75, 50, hWnd, (HMENU)1, NULL, NULL);

    HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Arial"));
    SendMessage(hServerIP, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hServerPort, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hUsername, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hEditIn, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hEditOut, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetFocus(hServerIP);
}