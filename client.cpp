#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include "Board.h"
#include "Status.h"

#define PORT "50002"
#define BUF_SIZE 1024

using namespace std;

int init_client(char* server_name);
void serverDisconnected();
bool makeMove(Board board);

int client_socket = 0;
Status st;

int main(int argc, char* argv[]) {
    char buffer[1024];
    bool game_over = false;
    int row, col;

    StatusCode statusCode;
    Board board;

    if (argc != 3) {
        cout << "error, enter server address" << endl;
        exit(1);
    }

    client_socket = init_client(argv[1]);

    snprintf(buffer, BUF_SIZE, "register %s\n", argv[2]);
    write(client_socket, buffer, strlen(buffer));

    while (!game_over) {
        if (!st.receiveStatus(client_socket, &statusCode)) {
            serverDisconnected();
        }

        switch (statusCode) {
            case CREATED: {
                cout << "Game created! Waiting your opponent to connect..." << endl;
                board.setType(CROSS);
                break;
            }
            case PLAYER_JOINED: {
                cout << "You successfull joined in game" << endl;
                board.setType(CIRCLE);
                break;
            }
            case SECOND_PLAYER_JOINED: {
                cout << "Second player to joined. Game start!" << endl;
                makeMove(board);
                break;
            }
        }
    }
}

int init_client(char* server_name) {
    int client_socket = 0;
    struct addrinfo *aip;
    struct addrinfo hint{};
    int err = 0;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    if ((err = getaddrinfo(server_name, PORT, &hint, &aip)) != 0) {
        cout << "Failed to get server address info" << gai_strerror(err) << endl;
        exit(1);
    }

    if ((client_socket = socket(aip->ai_family, aip->ai_socktype, 0)) < 0) {
        cout << "Failed to create socket" << endl;
        exit(1);
    }

    if (connect(client_socket, aip->ai_addr, aip->ai_addrlen) < 0) {
        cout << "Failed to connect to server" << endl;
        exit(1);
    }

    cout << "Connected to server successfully" << endl;

    return client_socket;
}

bool makeMove(Board board) {
    int row = 0, col = 0;
    bool correct_input = false;

    int sizeBoard = board.getSize() - 1;

    board.DrawBoard();

    while (!correct_input) {
        cout << "Enter move(row col): ";

        cin >> row >> col;

        if (row < 0 || row > sizeBoard) {
            cout << "Invalid row input. Try again" << endl;
        } else if (col < 0 || col > sizeBoard) {
            cout << "Invalid col input. Try again" << endl;
        } else if (!board.isBlank(row, col)) {
            cout << "This cell is don't blank. Try again" << endl;
        } else {
            correct_input = true;
        }
    }

    board.playerMakeMove(row, col);

    cout << "data send to server" << endl;

    if (!st.sendStatus(client_socket, MOVE)) {
        serverDisconnected();
    }

}

void serverDisconnected() {
    cout << "You've been disconnected from the server" << endl;
    close(client_socket);
    exit(1);
}