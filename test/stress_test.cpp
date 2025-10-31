#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <signal.h>
#include <sstream>

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

int connect_to_server(const char* host, int port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return -1;

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
		close(sock);
		return -1;
	}

	if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		close(sock);
		return -1;
	}

	return sock;
}

void send_simple_request(int sock, int client_id, int req_num) {
	std::ostringstream request;
	request << "GET /client" << client_id << "/req" << req_num << " HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";
	request << "Connection: keep-alive\r\n";
	request << "\r\n";

	std::string req_str = request.str();
	send(sock, req_str.c_str(), req_str.size(), 0);

	// Read response
	char buffer[4096];
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	int total_read = 0;
	while (true) {
		ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (n <= 0) break;
		total_read += n;

		// Simple check: if we have complete headers and Content-Length: 0
		if (total_read > 0 && strstr(buffer, "\r\n\r\n") && strstr(buffer, "Content-Length: 0"))
			break;
	}
}

void client_worker(int client_id, int num_requests) {
	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		std::cerr << RED << "Client " << client_id << ": Connection failed" << RESET << std::endl;
		exit(1);
	}

	for (int i = 0; i < num_requests; i++) {
		send_simple_request(sock, client_id, i);
		usleep(10000); // 10ms between requests
	}

	close(sock);
	exit(0); // Success
}

int main(int argc, char** argv) {
	int num_clients = 10;
	int requests_per_client = 5;

	if (argc > 1) num_clients = atoi(argv[1]);
	if (argc > 2) requests_per_client = atoi(argv[2]);

	std::cout << BLUE << "==========================================================" << RESET << std::endl;
	std::cout << BLUE << "  CONCURRENT CLIENT STRESS TEST" << RESET << std::endl;
	std::cout << BLUE << "==========================================================" << RESET << std::endl;
	std::cout << YELLOW << "Clients:            " << RESET << num_clients << std::endl;
	std::cout << YELLOW << "Requests per client:" << RESET << requests_per_client << std::endl;
	std::cout << YELLOW << "Total requests:     " << RESET << (num_clients * requests_per_client) << std::endl;
	std::cout << BLUE << "==========================================================" << RESET << std::endl;

	std::cout << "\n" << YELLOW << "Spawning " << num_clients << " concurrent client processes..." << RESET << std::endl;

	pid_t* pids = new pid_t[num_clients];

	// Fork all clients at once
	for (int i = 0; i < num_clients; i++) {
		pid_t pid = fork();

		if (pid < 0) {
			std::cerr << RED << "Fork failed!" << RESET << std::endl;
			// Kill any children we already created
			for (int j = 0; j < i; j++) {
				kill(pids[j], SIGTERM);
			}
			delete[] pids;
			return 1;
		} else if (pid == 0) {
			// Child process
			client_worker(i, requests_per_client);
			exit(1); // Should not reach
		} else {
			// Parent - save PID
			pids[i] = pid;
			std::cout << GREEN << "  Spawned client " << i << " (PID: " << pid << ")" << RESET << std::endl;
		}
	}

	std::cout << "\n" << YELLOW << "All clients spawned. Waiting for completion..." << RESET << std::endl;

	// Wait for all children
	int success = 0;
	int failed = 0;

	for (int i = 0; i < num_clients; i++) {
		int status;
		waitpid(pids[i], &status, 0);

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			success++;
			std::cout << GREEN << "  ✓ Client " << i << " completed successfully" << RESET << std::endl;
		} else {
			failed++;
			std::cout << RED << "  ✗ Client " << i << " failed" << RESET << std::endl;
		}
	}

	delete[] pids;

	// Summary
	std::cout << "\n" << BLUE << "==========================================================" << RESET << std::endl;
	std::cout << BLUE << "  RESULTS" << RESET << std::endl;
	std::cout << BLUE << "==========================================================" << RESET << std::endl;
	std::cout << GREEN << "Successful clients: " << success << "/" << num_clients << RESET << std::endl;
	std::cout << RED << "Failed clients:     " << failed << "/" << num_clients << RESET << std::endl;
	std::cout << YELLOW << "Total requests:     " << (success * requests_per_client) << RESET << std::endl;

	if (failed == 0) {
		std::cout << "\n" << GREEN << "  ✓✓✓ ALL CLIENTS SUCCEEDED! ✓✓✓" << RESET << std::endl;
		return 0;
	} else {
		std::cout << "\n" << RED << "  ✗✗✗ SOME CLIENTS FAILED ✗✗✗" << RESET << std::endl;
		return 1;
	}
}
