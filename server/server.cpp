#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <list>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

#define LENGTH 1000
#define LENGTH_SEND 2000
#define LENGTH_DATA 12
#define NEW 1
#define REPEAT 2
#define OLD 3

using namespace std;
int listener;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
list<string> threadsID;
list<thread> threads;
int sockfd;
string locked_str;

struct data_client {
    int last_code;
    int state;
    string last_result;
};
map<int, data_client> map_data;

void send_data(const string &text, sockaddr_in address) {
    string result_for_send;
    char buffer[LENGTH_SEND];
    bzero(buffer, LENGTH_SEND);
    string num_char;
    int port;
    port = ntohs(address.sin_port);
    auto num = to_string(map_data[port].last_code);

    int size = num.length();
    int i = 0;
    num_char.append(num);
    for (i = size; i < LENGTH_DATA; i++) {
        num_char.append(" ");
    }
    result_for_send.append(num_char);
    result_for_send.append(text);
//    if (map_data[port].last_code != 2)
    if (sendto(sockfd, (const char *) result_for_send.c_str(), strlen(result_for_send.c_str()),
               MSG_CONFIRM, (const struct sockaddr *) &address,
               sizeof(address)) < 0) {
        perror("send error");
    }
}

void update_state(int number, unsigned int port) {
    auto map_value = map_data.find(port);
    if (map_value != map_data.end()) {
        data_client value = map_data[port];

        if (value.last_code + 1 == number) {
            value.last_code = number;
            value.state = NEW;
        } else if (value.last_code == number) {
            value.state = REPEAT;
        } else {
            value.state = OLD;
        }
        map_data[port] = value;
    } else {
        data_client dataClient = {};
        dataClient.last_code = number;
        dataClient.state = NEW;
        map_data[port] = dataClient;
    }

}

string read_data(sockaddr_in &address) {
    string answer;
    char buffer[LENGTH];
    bzero(buffer, LENGTH);
    int i, n, len, number;
    len = sizeof(address);
    n = recvfrom(sockfd, buffer, LENGTH,
                 MSG_WAITALL, (sockaddr *) &address, reinterpret_cast<socklen_t *>(&len));
    buffer[n] = '\0';
    printf("req - %s\n", buffer);

    char num_recv[LENGTH_DATA];
    bzero(num_recv, LENGTH_DATA);
    string text_recv;

    for (i = 0; i < LENGTH_DATA; i++) {
        num_recv[i] = buffer[i];
    }
    unsigned int port;
    port = ntohs(address.sin_port);
    number = atoi(num_recv);
    update_state(number, port);

    for (i = LENGTH_DATA; i < strlen(buffer); i++) {
        text_recv += buffer[i];
    }
    answer = text_recv;
    return answer;
}

string toStr(thread::id id) {
    stringstream ss;
    ss << id;
    return ss.str();
}

string listToString() {
    string listThreads = "list threadsID: ";
    for (const string &v : threadsID) {
        listThreads += v;
        listThreads += " ";
    }
    listThreads += "\n";
    for (thread &v : threads) {
        listThreads += toStr(v.get_id());
        listThreads += " ";
    }
    listThreads += "\n";
    return listThreads;
}

list<string> split(const string &s, char delimiter) {
    list<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void exit() {
    shutdown(listener, 2);
    close(listener);
}

bool containsThread(const string &id) {
    for (const string &elem : threadsID) {
        if (strcmp(elem.c_str(), id.c_str()) == 0) {
            return true;
        }
    }
    return false;

}

void kill(const string &id, string &pString) {
    if (containsThread(id)) {
        threadsID.remove(id);
        pString = "killed ";
        pString += id;
    } else {
        pString = "Can not kill this";
    }
    pString += "\n";
}

void commandLine() {
    while (true) {
        string message;
        getline(cin, message);
        list<string> listSplit = split(message, ' ');
        if (strcmp(message.c_str(), string("list").c_str()) == 0) {
            cout << listToString();
        } else if (strcmp(message.c_str(), string("exit").c_str()) == 0) {
            cout << "Exit";
            exit();
            break;
        } else {
            listSplit = split(message, ' ');
            if (strcmp(listSplit.front().c_str(), string("kill").c_str()) == 0) {
                listSplit.pop_front();
                kill(listSplit.front(), message);
                cout << message;
            }
        }
    }
}

void fun(sockaddr_in address) {
    string buf = locked_str;
    printf("req - %s\n", buf.c_str());
    pthread_mutex_unlock(&mutex);

    int port;
    port = ntohs(address.sin_port);

    data_client value = map_data[port];

    string message;
    if (value.state == OLD) {
        pthread_mutex_lock(&mutex2);
        threadsID.remove(toStr(this_thread::get_id()));
        pthread_mutex_unlock(&mutex2);
        return;
    }
    if (value.state == REPEAT) {
        message = value.last_result;
    } else {
        if (strcmp(buf.c_str(), string("hello").c_str()) == 0) {
            message = "hello User";
        } else if (strcmp(buf.c_str(), string("hi").c_str()) == 0) {
            message = "hi User";
        } else {
            message = "Good day";
        }
    }

    value.last_result = message;
    map_data[port] = value;
    send_data(message, address);
    if (strcmp(buf.c_str(), "exit") == 0) {
        map_data.erase(port);
    }
    pthread_mutex_lock(&mutex2);
    threadsID.remove(toStr(this_thread::get_id()));
    pthread_mutex_unlock(&mutex2);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in servaddr{}, cliaddr{};

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(atoi(argv[1]));

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *) &servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    thread cl(commandLine);

    while (true) {
        string request = read_data(cliaddr);
        if (request.length() == 0) { break; }

        pthread_mutex_lock(&mutex);
        locked_str = request;
        threads.emplace_back([&] { fun(cliaddr); });
        pthread_mutex_lock(&mutex2);
        threadsID.emplace_back(toStr(threads.back().get_id()));
        pthread_mutex_unlock(&mutex2);
    }

    cl.join();

    for (thread &thread : threads) {
        if (containsThread(toStr(thread.get_id()))) {
            thread.join();
        }
    }

    printf("\nServer exit\n");
    return 0;

}