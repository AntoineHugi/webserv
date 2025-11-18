#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

int connect_to_server(const char* host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

void test_pipelined_chunked() {
    std::cout << "\n" << YELLOW << "=====================================" << RESET << std::endl;
    std::cout << YELLOW << "TEST: Pipelined Chunked + GET" << RESET << std::endl;
    std::cout << YELLOW << "=====================================" << RESET << std::endl;

    int sock = connect_to_server("127.0.0.1", 8080);
    if (sock < 0) {
        std::cout << RED << "✗ Connection failed" << RESET << std::endl;
        return;
    }

    // Build pipelined request
    std::string request;
    request += "POST /upload HTTP/1.1\r\n";
    request += "Host: localhost:8080\r\n";
    request += "Transfer-Encoding: chunked\r\n";
    request += "Connection: keep-alive\r\n";
    request += "\r\n";
    request += "5\r\n";
    request += "Hello\r\n";
    request += "0\r\n";
    request += "\r\n";
    // Second request immediately after
    request += "GET /next HTTP/1.1\r\n";
    request += "Host: localhost:8080\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";

    std::cout << "Sending " << request.size() << " bytes (both requests in one send)..." << std::endl;
    ssize_t sent = send(sock, request.c_str(), request.size(), 0);
    std::cout << "Sent: " << sent << " bytes" << std::endl;

    // Receive ALL responses
    std::string all_responses;
    char buffer[4096];
    int recv_count = 0;

    std::cout << "\nReceiving responses:" << std::endl;
    while (true) {
        ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            std::cout << "recv() returned " << received << " (done or error)" << std::endl;
            break;
        }
        recv_count++;
        buffer[received] = '\0';
        all_responses.append(buffer, received);
        std::cout << "Recv #" << recv_count << ": " << received << " bytes" << std::endl;
    }

    std::cout << "\nTotal received: " << all_responses.size() << " bytes" << std::endl;

    // Count HTTP responses
    size_t count = 0;
    size_t pos = 0;
    while ((pos = all_responses.find("HTTP/", pos)) != std::string::npos) {
        count++;
        std::cout << "Found response #" << count << " at position " << pos << std::endl;
        pos += 5;
    }

    std::cout << "\n";
    if (count >= 2) {
        std::cout << GREEN << "✓ PASS: Got " << count << " responses (pipelined worked!)" << RESET << std::endl;
    } else {
        std::cout << RED << "✗ FAIL: Only got " << count << " response(s)" << RESET << std::endl;
        std::cout << "\nFull response data:" << std::endl;
        std::cout << "-----" << std::endl;
        std::cout << all_responses << std::endl;
        std::cout << "-----" << std::endl;
    }

    close(sock);
}

void test_pipelined_content_length() {
    std::cout << "\n" << YELLOW << "=====================================" << RESET << std::endl;
    std::cout << YELLOW << "TEST: Pipelined Content-Length + GET" << RESET << std::endl;
    std::cout << YELLOW << "=====================================" << RESET << std::endl;

    int sock = connect_to_server("127.0.0.1", 8080);
    if (sock < 0) {
        std::cout << RED << "✗ Connection failed" << RESET << std::endl;
        return;
    }

    std::string body = "Hello, World!";
    std::string request;
    request += "POST /submit HTTP/1.1\r\n";
    request += "Host: localhost:8080\r\n";
    request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    request += "Connection: keep-alive\r\n";
    request += "\r\n";
    request += body;
    // Second request
    request += "GET /after HTTP/1.1\r\n";
    request += "Host: localhost:8080\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";

    std::cout << "Sending " << request.size() << " bytes (both requests in one send)..." << std::endl;
    ssize_t sent = send(sock, request.c_str(), request.size(), 0);
    std::cout << "Sent: " << sent << " bytes" << std::endl;

    // Receive ALL responses
    std::string all_responses;
    char buffer[4096];
    int recv_count = 0;

    std::cout << "\nReceiving responses:" << std::endl;
    while (true) {
        ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            std::cout << "recv() returned " << received << " (done or error)" << std::endl;
            break;
        }
        recv_count++;
        buffer[received] = '\0';
        all_responses.append(buffer, received);
        std::cout << "Recv #" << recv_count << ": " << received << " bytes" << std::endl;
    }

    std::cout << "\nTotal received: " << all_responses.size() << " bytes" << std::endl;

    // Count HTTP responses
    size_t count = 0;
    size_t pos = 0;
    while ((pos = all_responses.find("HTTP/", pos)) != std::string::npos) {
        count++;
        std::cout << "Found response #" << count << " at position " << pos << std::endl;
        pos += 5;
    }

    std::cout << "\n";
    if (count >= 2) {
        std::cout << GREEN << "✓ PASS: Got " << count << " responses (pipelined worked!)" << RESET << std::endl;
    } else {
        std::cout << RED << "✗ FAIL: Only got " << count << " response(s)" << RESET << std::endl;
        std::cout << "\nFull response data:" << std::endl;
        std::cout << "-----" << std::endl;
        std::cout << all_responses << std::endl;
        std::cout << "-----" << std::endl;
    }

    close(sock);
}

int main() {
    std::cout << "\n" << GREEN << "========================================" << RESET << std::endl;
    std::cout << GREEN << "  PIPELINED REQUEST TESTS" << RESET << std::endl;
    std::cout << GREEN << "========================================" << RESET << std::endl;

    test_pipelined_chunked();
    test_pipelined_content_length();

    std::cout << "\n" << GREEN << "========================================" << RESET << std::endl;
    std::cout << GREEN << "  TESTS COMPLETE" << RESET << std::endl;
    std::cout << GREEN << "========================================" << RESET << std::endl;

    return 0;
}
