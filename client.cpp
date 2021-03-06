#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <csignal>
#include "Board.h"
#include "Status.h"
#include "sockets.h"
#include "CellType.h"

using namespace std;

bool connect_to_socket(char* server_name, const string& port, char* client_name);
void serverDisconnected();
bool makeMove(int type);

int client_socket = 0;
char *server_name;
char *client_name;
string portList[3] = {"50001", "50002", "50003"};
int currentIndexPort = 0;

Board *board;

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    int type;
    int row, col;
    bool game_over = false;
    server_name = argv[1];
    client_name = argv[2];
    StatusCode statusCode;

    if (argc != 3) {
        cout << "error, enter server address" << endl;
        exit(1);
    }

    for (int i = 0; i < portList->length(); ++i) {
        bool socketIsConnected = connect_to_socket(server_name, portList[i], client_name);
        currentIndexPort++;
        if (socketIsConnected) {
            sendStatus(client_socket, MAIN_SERVER, true);
            break;
        }
    }

    while (!game_over) {
        if (!receiveStatus(client_socket, &statusCode)) {
            serverDisconnected();
            if (!receiveStatus(client_socket, &statusCode)) {
                serverDisconnected();
            }
        }

        switch (statusCode) {
            case CREATED: {
                cout << "Game created! Waiting your opponent to connect..." << endl;
                board = new Board();
                type = CROSS;
                board->setType(CROSS);
                break;
            }
            case PLAYER_JOINED: {
                cout << "You successfull joined in game" << endl;
                board = new Board();
                type = CIRCLE;
                board->setType(CIRCLE);
                break;
            }
            case SECOND_PLAYER_JOINED: {
                cout << "Second player to joined. Game start!" << endl;
                board->DrawBoard();
                makeMove(type);
                break;
            }
            case GAME_EXIST: {
                cout << "Game exist. Choose another game" << endl;
                break;
            }
            case MOVE: {
                if(!receiveInt(client_socket, &row))
                    serverDisconnected();

                if(!receiveInt(client_socket, &col))
                    serverDisconnected();

                board->otherMakeMove(row, col);
                board->DrawBoard();

                game_over = makeMove(type);

                break;
            }
            case INVALID_COMMAND: {
                cout << "Invalid command! Please try again" << endl;
                break;
            }
            case WIN: {
                cout << "You win!" << endl;
                cout << "Press Enter for continue...";
                int c = getchar();
                while ((c = getchar()) != '\n' && c != EOF) {}
                sendStatus(client_socket, CONNECTED, true);
                sendInt(client_socket, 0);
                char buffer[1024];
                snprintf(buffer, 1024, "%s\n", client_name);
                write(client_socket, buffer, strlen(buffer));
                break;
            }
            case LOSS: {
                cout << "You loss!" << endl;
                cout << "Press Enter for continue...";
                int c = getchar();
                while ((c = getchar()) != '\n' && c != EOF) {}
                sendStatus(client_socket, CONNECTED, true);
                sendInt(client_socket, 0);
                char buffer[1024];
                snprintf(buffer, 1024, "%s\n", client_name);
                write(client_socket, buffer, strlen(buffer));
                break;
            }
            case STATE: {
                for (int i = 0; i < MAX_ROWS; ++i) {
                    for (int j = 0; j < MAX_ROWS; ++j) {
                        int cell;
                        receiveInt(client_socket, &cell);
                        board->board[i][j] = cell;
                    }
                }

                board->DrawBoard();

                break;
            }
            case WRONG: {
                cout << "You wrong!" << endl;
                break;
            }
            default: {
                cout << "Unrecognized response from the server" << endl;
                exit(1);
            }
        }
    }
}

bool connect_to_socket(char* server_name, const string& port, char* client_name) {
    struct addrinfo *aip;
    struct addrinfo hint{};
    int err = 0;
    char buffer[1024];

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    if ((err = getaddrinfo(server_name, port.c_str(), &hint, &aip)) != 0) {
        cout << "Failed to get server address info: " << gai_strerror(err) << endl;
        return false;
    }

    if ((client_socket = socket(aip->ai_family, aip->ai_socktype, 0)) < 0) {
        cout << "Failed to create socket" << endl;
        return false;
    }

    if (connect(client_socket, aip->ai_addr, aip->ai_addrlen) < 0) {
        cout << "Failed to connect to server" << endl;
        return false;
    }

    sendStatus(client_socket, CONNECTED, true);
    sendInt(client_socket, 1);

    snprintf(buffer, 1024, "%s\n", client_name);
    write(client_socket, buffer, strlen(buffer));

    return true;
}

bool makeMove(int type) {
    bool game_over = false;
    int row = 0, col = 0;
    bool correct_input = false;

    while (!correct_input) {
        cout << "Enter move(row col): ";

        cin >> row >> col;

        if (row < 0 || row > MAX_ROWS) {
            cout << "Invalid row input. Try again" << endl;
        } else if (col < 0 || col > MAX_ROWS) {
            cout << "Invalid col input. Try again" << endl;
        } else if (!board->isBlank(row, col)) {
            cout << "This cell is don't blank. Try again" << endl;
        } else {
            correct_input = true;
        }
    }

    if (!sendStatus(client_socket, MOVE, true)) {
        serverDisconnected();
    }

    if (!sendInt(client_socket, type)) {
        serverDisconnected();
        sendStatus(client_socket, MOVE, true);
        sendInt(client_socket, type);
    }

    if (!sendInt(client_socket, row)) {
        serverDisconnected();
        //sendInt(client_socket, row);
    }

    if (!sendInt(client_socket, col)) {
        serverDisconnected();
        //sendInt(client_socket, col);
    }

    cout << "Wait opponent..." << endl;

    return game_over;
}

void serverDisconnected() {
    close(client_socket);

    for (int i = currentIndexPort; i < portList->length(); ++i) {
        bool socketIsConnected = connect_to_socket(server_name, portList[i], client_name);
        currentIndexPort++;
        if (socketIsConnected) {
            sendStatus(client_socket, MAIN_SERVER, true);
            break;
        }
    }
}