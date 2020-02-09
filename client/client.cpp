#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <arpa/inet.h>

#define LENGTH 1000
#define LENGTH_RECV 2000
#define LENGTH_DATA 12

using namespace std;

string message;

string send_data(const string &text, sockaddr_in servaddr, int sockfd, int &number) {
    string result_for_send;
    string answer;
    bool is_sended = false;
    int current_step = 0;
    if (text.length() == 0) {
        printf("Введите текст\n");
        return "Введите текст\n";
    }
//
//    if (number > 4) {
//        number = 6;
//    }
//    if (number > 2) {
//        number = 2;
//    }
    if (number == 6) {
        number = 3;
    }
    if (number == 3) {
        number = 6;
    }

    while (!is_sended && current_step < 3) {
        answer.clear();
        result_for_send.clear();
        char buffer[LENGTH_RECV];
        bzero(buffer, LENGTH_RECV);
        string num_char;
        auto num = to_string(number);
        int size = num.length();
        int i = 0;
        num_char.append(num);
        for (i = size; i < LENGTH_DATA; i++) {
            num_char.append(" ");
        }
        result_for_send.append(num_char);
        result_for_send.append(text);

        if (sendto(sockfd, (const char *) result_for_send.c_str(), strlen(result_for_send.c_str()),
                   MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                   sizeof(servaddr)) < 0) {
            break;
        }
        int n, len;
        n = recvfrom(sockfd, (char *) buffer, LENGTH_RECV,
                     0, (sockaddr *) &servaddr, reinterpret_cast<socklen_t *>(&len));
        buffer[n] = '\0';
        if (n == 0) continue;
        char num_recv[LENGTH_DATA];
        bzero(num_recv, LENGTH_DATA);
        string text_recv;

        for (i = 0; i < LENGTH_DATA; i++) {
            num_recv[i] = buffer[i];
        }

        for (i = LENGTH_DATA; i < strlen(buffer); i++) {
            text_recv += buffer[i];
        }
//        printf("num - %s\n", num_char.c_str());
        if (atoi(num_recv) == atoi(num_char.c_str())) {
            answer = text_recv;
            is_sended = true;
        }
        current_step++;
    }
    if (!is_sended) {
        answer = "Ошибка сети\n";
    } else {
        number++;
    }
    printf("%s\n", answer.c_str());
    return answer;
}

void Event(int port) {
    int sock;
    struct sockaddr_in addr{};

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));

    // Filling server information
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int number = 0;
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    bool isWork = true;
    send_data("hello", addr, sock, number);
    while (isWork) {
        std::getline(std::cin, message);
        send_data(message, addr, sock, number);
        if (strcmp(message.c_str(), std::string("exit").c_str()) == 0) {
            isWork = false;
        }
    }
    shutdown(sock, 2);
    close(sock);
}

int main(int argc, char *argv[]) {
    Event(atoi(argv[1]));
    return 0;
}




