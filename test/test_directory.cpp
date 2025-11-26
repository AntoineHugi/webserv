#include "test_framework.hpp"

void test_directory_default_index(const TestConfig &config, TestStats &stats)
{
	print_test("Directory Default Index");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string raw = send_request(sock, "GET", "/");
	HttpResponse resp = parse_response(raw);

	assert_status(resp, 200, stats);
	assert_body_contains(resp, "<html", stats);
	assert_header_exists(resp, "Content-Type", stats);

	close(sock);
}

void test_autoindex_on(const TestConfig &config, TestStats &stats)
{
	print_test("Autoindex: Listing Enabled");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}
	std::string raw = send_request(sock, "GET", "/upload-list/");
	HttpResponse resp = parse_response(raw);

	assert_status(resp, 200, stats);
	assert_body_contains(resp, "<a href=", stats);

	close(sock);
}

void test_url_to_fs_mapping(const TestConfig &config, TestStats &stats)
{
	print_test("URL-to-FS Mapping");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string raw = send_request(sock, "GET", "/about");
	HttpResponse resp = parse_response(raw);

	assert_status(resp, 200, stats);
	assert_body_contains(resp, "About", stats);

	close(sock);
}

void test_path_traversal(const TestConfig &config, TestStats &stats)
{
	print_test("Path Traversal Security");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string raw = send_request(sock, "GET", "/../secret.txt");
	HttpResponse resp = parse_response(raw);

	if (resp.status_code == 403 || resp.status_code == 404)
	{
		stats.add_pass();
		print_pass("Traversal blocked");
	}
	else
	{
		stats.add_fail();
		print_fail("Traversal allowed");
	}

	close(sock);
}

// ============================================================================
// CATEGORY RUNNER
// ============================================================================

void run_directory_tests(const TestConfig &config, TestStats &stats)
{
	print_category("DIRECTORY RULES, AUTOINDEX & FILE MAPPING");

	test_directory_default_index(config, stats);
	test_autoindex_on(config, stats);
	test_url_to_fs_mapping(config, stats);
	test_path_traversal(config, stats);
}