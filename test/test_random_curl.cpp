#include <cstdio>
#include <cstdlib>

#include "test_framework.hpp"

#include <sstream>

std::string int_to_string(int n)
{
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

std::string exec_cmd(const std::string &cmd)
{
	std::string result;
	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe)
		return "";

	char buffer[512];
	while (fgets(buffer, sizeof(buffer), pipe))
		result += buffer;

	pclose(pipe);
	return result;
}

int run_curl_status(const std::string &cmd)
{
	std::string wrapped =
		"bash -c '" + cmd +
		" -o /dev/null -s -w \"%{http_code}\"'";

	std::string out = exec_cmd(wrapped);
	return atoi(out.c_str());
}

void test_curl_unknown_method(const TestConfig &config, TestStats &stats)
{
	print_test("Curl Unknown Method (BREW)");

	std::string cmd =
		"curl -X BREW http://" + config.server_host + ":" +
		int_to_string(config.server_port) + "/";

	int status = run_curl_status(cmd);

	assert_status_code(status, 405, stats);
}

void test_curl_bad_content_length(const TestConfig &config, TestStats &stats)
{
	print_test("Curl Bad Content-Length");

	std::string cmd =
		"curl -X POST "
		"-H \"Content-Length: -5\" "
		"http://" +
		config.server_host + ":" +
		int_to_string(config.server_port) + "/upload";

	int status = run_curl_status(cmd);

	assert_status_code(status, 400, stats);
}

void test_curl_invalid_http_version(const TestConfig &config, TestStats &stats)
{
	print_test("Curl HTTP/0.9");

	std::string cmd =
		"printf \"GET / HTTP/9.0\\r\\nHost: " +
		config.server_host + "\\r\\n\\r\\n\" | nc " +
		config.server_host + " " +
		int_to_string(config.server_port);

	std::string out = exec_cmd(cmd);

	bool ok = (out.find("505") != std::string::npos);

	if (ok)
	{
		stats.add_pass();
		print_pass("Status " + int_to_string(505));
	}
	else
	{
		stats.add_fail();
		print_fail("Expected " + int_to_string(505));
	}
}

void run_weird_curl_tests(const TestConfig &config, TestStats &stats)
{
	print_category("WEIRD CURL TESTS");

	test_curl_unknown_method(config, stats);
	test_curl_bad_content_length(config, stats);
	test_curl_invalid_http_version(config, stats);
}
