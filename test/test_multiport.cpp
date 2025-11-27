#include "test_framework.hpp"

void test_multi_port_behavior(const TestConfig &config, TestStats &stats)
{
	print_test("Multiple Ports: Different Servers");

	int sock1 = connect_to_server(config.server_host.c_str(), 8080);
	if (sock1 < 0)
	{
		stats.add_skip();
		print_skip("Connection to port 8080 failed");
		return;
	}

	std::string response1 = send_request(sock1, "GET", "/");
	HttpResponse resp1 = parse_response(response1);
	close(sock1);

	int sock2 = connect_to_server(config.server_host.c_str(), 8081);
	if (sock2 < 0)
	{
		stats.add_skip();
		print_skip("Connection to port 8081 failed");
		return;
	}

	std::string response2 = send_request(sock2, "GET", "/");
	HttpResponse resp2 = parse_response(response2);
	close(sock2);

	if (resp1.body != resp2.body)
	{
		stats.add_pass();
		print_pass("Server blocks serve different content");
	}
	else
	{
		stats.add_fail();
		print_fail("Both ports return identical root content");
	}
}

// ============================================================================
// CATEGORY RUNNER
// ============================================================================

void run_multiport_tests(const TestConfig &config, TestStats &stats)
{
	print_category("MULTIPORT");

	test_multi_port_behavior(config, stats);
}