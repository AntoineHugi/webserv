#include "test_framework.hpp"
#include <unistd.h>

// ============================================================================
// CATEGORY: POST & DELETE
// ============================================================================
void test_simple_post(const TestConfig &config, TestStats &stats)
{
	print_test("Upload a file and GET it back");

	const char *filename = "simple.txt";
	const char *file_content = "This is a test file.";

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	// Multipart body
	const char *boundary = "----WebServBoundary";
	std::string body;
	body += "--";
	body += boundary;
	body += "\r\n";
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"";
	body += filename;
	body += "\"\r\n";
	body += "Content-Type: text/plain\r\n\r\n";
	body += file_content;
	body += "\r\n";
	body += "--";
	body += boundary;
	body += "--\r\n";

	char cl_buf[32];
	sprintf(cl_buf, "%d", (int)body.size());

	// POST request
	std::string post_request = "POST /upload-list HTTP/1.1\r\n";
	post_request += "Host: localhost:8080\r\n";
	post_request += "Content-Length: ";
	post_request += cl_buf;
	post_request += "\r\n";
	post_request += "Content-Type: multipart/form-data; boundary=";
	post_request += boundary;
	post_request += "\r\n";
	post_request += "Connection: keep-alive\r\n\r\n";
	post_request += body;

	// GET request
	std::string get_request = "GET /upload-list/";
	get_request += filename;
	get_request += " HTTP/1.1\r\nHost: localhost:8080\r\nConnection: close\r\n\r\n";

	std::string request = post_request + get_request;

	print_info("Sending pipelined multipart POST + GET for file");
	send(sock, request.c_str(), request.size(), 0);

	char buffer[4096];
	std::string all_responses;

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	while (1)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0)
			break;
		buffer[received] = '\0';
		all_responses += buffer;
	}

	bool post_ok = all_responses.find("HTTP/1.1 200") != std::string::npos;
	bool get_ok = all_responses.find(file_content) != std::string::npos;

	if (post_ok && get_ok)
	{
		stats.add_pass();
		print_pass("File uploaded and retrieved successfully");
	}
	else if (!post_ok)
	{
		stats.add_fail();
		print_fail("POST failed");
	}
	else
	{
		stats.add_fail();
		print_fail("GET failed");
	}

	close(sock);
}

void test_post_delete(const TestConfig &config, TestStats &stats)
{
	print_test("Upload a file, GET it, then DELETE it");

	const char *filename = "delete_me.txt";
	const char *file_content = "This file will be deleted.";

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	const char *boundary = "----WebServBoundary";
	std::string body;
	body += "--";
	body += boundary;
	body += "\r\n";
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"";
	body += filename;
	body += "\"\r\n";
	body += "Content-Type: text/plain\r\n\r\n";
	body += file_content;
	body += "\r\n";
	body += "--";
	body += boundary;
	body += "--\r\n";

	char cl_buf[32];
	sprintf(cl_buf, "%d", (int)body.size());

	// POST request
	std::string post_request = "POST /upload-list HTTP/1.1\r\n";
	post_request += "Host: localhost:8080\r\n";
	post_request += "Content-Length: ";
	post_request += cl_buf;
	post_request += "\r\n";
	post_request += "Content-Type: multipart/form-data; boundary=";
	post_request += boundary;
	post_request += "\r\n";
	post_request += "Connection: keep-alive\r\n\r\n";
	post_request += body;

	// GET request
	std::string get_request = "GET /upload-list/";
	get_request += filename;
	get_request += " HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n";

	// DELETE request
	std::string delete_request = "DELETE /upload-list/";
	delete_request += filename;
	delete_request += " HTTP/1.1\r\nHost: localhost:8080\r\nConnection: close\r\n\r\n";

	std::string request = post_request + get_request + delete_request;

	print_info("Sending pipelined multipart POST + GET + DELETE for file");
	send(sock, request.c_str(), request.size(), 0);

	char buffer[4096];
	std::string all_responses;

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	while (1)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0)
			break;
		buffer[received] = '\0';
		all_responses += buffer;
	}

	bool post_ok = all_responses.find("HTTP/1.1 200") != std::string::npos;
	bool get_ok = all_responses.find(file_content) != std::string::npos;
	bool delete_ok = all_responses.find("HTTP/1.1 204") != std::string::npos;

	if (post_ok && get_ok && delete_ok)
	{
		stats.add_pass();
		print_pass("Upload, GET, and DELETE succeeded");
	}
	else
	{
		stats.add_fail();
		print_fail("One of the operations failed");
	}

	close(sock);
}

void run_post_delete(const TestConfig &config, TestStats &stats)
{
	print_category("POST & DELETE");

	test_simple_post(config, stats);
	test_post_delete(config, stats);
}
