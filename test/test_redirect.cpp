#include "test_framework.hpp"

void test_redirection(const TestConfig &config, TestStats &stats)
{
	print_test("HTTP Redirection");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		return;
	}

	std::string response = send_request(sock, "GET", "/old_upload");
	HttpResponse resp = parse_response(response);

	if (resp.status_code == 301 && resp.headers.count("Location"))
	{
		stats.add_pass();
		print_pass("Redirection OK");
	}
	else
	{
		stats.add_fail();
		print_fail("Missing 3xx or Location");
	}

	close(sock);
}

void run_redirect_test(const TestConfig &config, TestStats &stats)
{
	print_category("REDIRECT");

	test_redirection(config, stats);
}