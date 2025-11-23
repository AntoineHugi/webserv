#include "test_framework.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

// ============================================================================
// CATEGORY: CHUNKED TRANSFER ENCODING
// ============================================================================

void test_chunked_request_simple(const TestConfig &config, TestStats &stats)
{
	print_test("Chunked Encoding: Multipart POST (hello.txt)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	const std::string boundary = "----BOUNDARYXYZ";

	std::string multipart;
	multipart += "--" + boundary + "\r\n";
	multipart += "Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n";
	multipart += "Content-Type: text/plain\r\n";
	multipart += "\r\n";
	multipart += "hello\r\n"; // file content
	multipart += "--" + boundary + "--\r\n";

	std::string request;
	request += "POST /upload HTTP/1.1\r\n";
	request += "Host: localhost:8080\r\n";
	request += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
	request += "Transfer-Encoding: chunked\r\n";
	request += "Connection: close\r\n";
	request += "\r\n";

	std::stringstream chunk_header;
	chunk_header << std::hex << multipart.size();

	request += chunk_header.str() + "\r\n";
	request += multipart;
	request += "\r\n0\r\n\r\n";

	send(sock, request.c_str(), request.size(), 0);

	std::string response;
	char buffer[4096];

	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	while (true)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0)
			break;
		buffer[received] = '\0';
		response.append(buffer, received);

		if (response.find("\r\n\r\n") != std::string::npos)
			break;
	}

	HttpResponse resp = parse_response(response);
	assert_status(resp, 204, stats);

	close(sock);
}

void test_chunked_request_multiple_chunks(const TestConfig &config, TestStats &stats)
{
	print_test("Chunked Encoding: Multiple Chunks");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string request = "POST /upload HTTP/1.1\r\n";
	request += "Host: localhost:8080\r\n";
	request += "Transfer-Encoding: chunked\r\n";
	request += "Connection: close\r\n";
	request += "\r\n";
	request += "5\r\n";
	request += "Hello\r\n";
	request += "7\r\n";
	request += " World!\r\n";
	request += "3\r\n";
	request += "End\r\n";
	request += "0\r\n";
	request += "\r\n";

	send(sock, request.c_str(), request.size(), 0);

	std::string response;
	char buffer[4096];
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	while (true)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0)
			break;
		buffer[received] = '\0';
		response.append(buffer, received);
		if (response.find("\r\n\r\n") != std::string::npos)
			break;
	}

	HttpResponse resp = parse_response(response);
	assert_status(resp, 200, stats);

	close(sock);
}

void test_chunked_with_keepalive(const TestConfig &config, TestStats &stats)
{
	print_test("Chunked Encoding: With Keep-Alive");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string request = "POST /upload HTTP/1.1\r\n";
	request += "Host: localhost:8080\r\n";
	request += "Transfer-Encoding: chunked\r\n";
	request += "Connection: keep-alive\r\n";
	request += "\r\n";
	request += "A\r\n";
	request += "0123456789\r\n";
	request += "0\r\n";
	request += "\r\n";

	send(sock, request.c_str(), request.size(), 0);

	std::string response;
	char buffer[4096];
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	size_t header_end_pos = std::string::npos;
	while (true)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			break;
		}
		if (received == 0)
			break;

		buffer[received] = '\0';
		response.append(buffer, received);

		if (header_end_pos == std::string::npos)
		{
			header_end_pos = response.find("\r\n\r\n");
			if (header_end_pos != std::string::npos &&
			    response.find("Content-Length: 0") != std::string::npos)
			{
				break;
			}
		}
	}

	HttpResponse resp = parse_response(response);
	assert_status(resp, 200, stats);
	assert_header(resp, "Connection", "keep-alive", stats);

	// Try second request on same connection
	usleep(100000);
	std::map<std::string, std::string> headers;
	headers["Connection"] = "close";
	std::string response2 = send_request(sock, "GET", "/", headers);
	HttpResponse resp2 = parse_response(response2);

	if (resp2.status_code == 200)
	{
		stats.add_pass();
		print_pass("Second request succeeded on same connection");
	}
	else
	{
		stats.add_fail();
		print_fail("Second request failed");
	}

	close(sock);
}

// ============================================================================
// CATEGORY RUNNER
// ============================================================================

void run_chunked_tests(const TestConfig &config, TestStats &stats)
{
	print_category("CHUNKED TRANSFER ENCODING");

	test_chunked_request_simple(config, stats);
	test_chunked_request_multiple_chunks(config, stats);
	test_chunked_with_keepalive(config, stats);
}
