#include "test_framework.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>

// Forward declarations of test category runners
void run_basic_tests(const TestConfig &config, TestStats &stats);
void run_directory_tests(const TestConfig &config, TestStats &stats);
void run_error_tests(const TestConfig &config, TestStats &stats);
void run_keepalive_tests(const TestConfig &config, TestStats &stats);
void run_multiport_tests(const TestConfig &config, TestStats &stats);
void run_chunked_tests(const TestConfig &config, TestStats &stats);
void run_pipelining_tests(const TestConfig &config, TestStats &stats);
void run_post_delete(const TestConfig &config, TestStats &stats);
void run_cgi_tests(const TestConfig &config, TestStats &stats);
void run_redirect_test(const TestConfig &config, TestStats &stats);

// ============================================================================
// CONFIGURATION & COMMAND LINE PARSING
// ============================================================================

void print_usage(const char *program_name)
{
	std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
	std::cout << "Options:\n";
	std::cout << "  --host <host>        Server hostname (default: 127.0.0.1)\n";
	std::cout << "  --port <port>        Server port (default: 8080)\n";
	std::cout << "  --verbose            Enable verbose output\n";
	std::cout << "  --stop-on-fail       Stop testing on first failure\n";
	std::cout << "  --only <category>    Run only specific category\n";
	std::cout << "  --skip <category>    Skip specific category\n";
	std::cout << "  --list               List available test categories\n";
	std::cout << "  --help               Show this help message\n\n";
	std::cout << "Categories:\n";
	std::cout << "  basic                Basic connectivity and requests\n";
	std::cout << "  error                Error codes & pages\n";
	std::cout << "  directory            Directory rules and file mapping\n";
	std::cout << "  keepalive            Connection persistence (keep-alive)\n";
	std::cout << "  multiport            Connection from various ports\n";
	std::cout << "  chunked              Chunked transfer encoding\n";
	std::cout << "  pipeline             Pipelined requests\n";
	std::cout << "  post-delete          Post and Delete functionalities\n";
	std::cout << "  cgi                  CGI script execution\n";
	std::cout << "  redirect             redirection\n";
	std::cout << "  all                  All categories (default)\n\n";
	std::cout << "Examples:\n";
	std::cout << "  " << program_name << "                    # Run all tests\n";
	std::cout << "  " << program_name << " --only basic       # Run only basic tests\n";
	std::cout << "  " << program_name << " --skip pipeline    # Skip pipeline tests\n";
	std::cout << "  " << program_name << " --verbose          # Verbose output\n";
}

TestConfig parse_args(int argc, char **argv)
{
	TestConfig config;
	config.server_host = "127.0.0.1";
	config.server_port = 8080;
	config.verbose = false;
	config.stop_on_fail = false;

	// Enable all categories by default
	config.enabled_categories["basic"] = true;
	config.enabled_categories["directory"] = true;
	config.enabled_categories["error"] = true;
	config.enabled_categories["keepalive"] = true;
	config.enabled_categories["multiport"] = true;
	config.enabled_categories["chunked"] = true;
	config.enabled_categories["pipeline"] = true;
	config.enabled_categories["post-delete"] = true;
	config.enabled_categories["cgi"] = true;
	config.enabled_categories["redirect"] = true;

	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];

		if (arg == "--help" || arg == "-h")
		{
			print_usage(argv[0]);
			exit(0);
		}
		else if (arg == "--list")
		{
			std::cout << "Available test categories:\n";
			std::cout << "  - basic\n";
			std::cout << "  - directory\n";
			std::cout << "  - error\n";
			std::cout << "  - keepalive\n";
			std::cout << "  - multiport\n";
			std::cout << "  - chunked\n";
			std::cout << "  - pipeline\n";
			std::cout << "  - post-delete\n";
			std::cout << "  - redirect\n";
			exit(0);
		}
		else if (arg == "--host" && i + 1 < argc)
		{
			config.server_host = argv[++i];
		}
		else if (arg == "--port" && i + 1 < argc)
		{
			config.server_port = atoi(argv[++i]);
		}
		else if (arg == "--verbose" || arg == "-v")
		{
			config.verbose = true;
		}
		else if (arg == "--stop-on-fail")
		{
			config.stop_on_fail = true;
		}
		else if (arg == "--only" && i + 1 < argc)
		{
			// Disable all, then enable specified
			config.enabled_categories["basic"] = false;
			config.enabled_categories["directory"] = false;
			config.enabled_categories["error"] = false;
			config.enabled_categories["keepalive"] = false;
			config.enabled_categories["multiport"] = false;
			config.enabled_categories["chunked"] = false;
			config.enabled_categories["pipeline"] = false;
			config.enabled_categories["post-delete"] = false;
			config.enabled_categories["cgi"] = false;
			config.enabled_categories["redirect"] = false;
			config.enabled_categories[argv[++i]] = true;
		}
		else if (arg == "--skip" && i + 1 < argc)
		{
			config.enabled_categories[argv[++i]] = false;
		}
		else
		{
			std::cerr << "Unknown option: " << arg << std::endl;
			print_usage(argv[0]);
			exit(1);
		}
	}

	return config;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

void print_summary(const TestStats &stats)
{
	std::cout << "\n"
			  << Color::CYAN << Color::BOLD;
	std::cout << "========================================\n";
	std::cout << "  TEST SUMMARY\n";
	std::cout << "========================================" << Color::RESET << "\n";

	std::cout << "Total:   " << stats.total << "\n";
	std::cout << Color::GREEN << "Passed:  " << stats.passed << Color::RESET << "\n";
	std::cout << Color::RED << "Failed:  " << stats.failed << Color::RESET << "\n";
	std::cout << Color::YELLOW << "Skipped: " << stats.skipped << Color::RESET << "\n";

	if (stats.total > 0)
	{
		std::cout << "\nPass rate: " << Color::BOLD;
		if (stats.pass_rate() >= 90.0)
		{
			std::cout << Color::GREEN;
		}
		else if (stats.pass_rate() >= 70.0)
		{
			std::cout << Color::YELLOW;
		}
		else
		{
			std::cout << Color::RED;
		}
		std::cout << stats.pass_rate() << "%" << Color::RESET << "\n";
	}

	std::cout << "\n";
}

int main(int argc, char **argv)
{
	TestConfig config = parse_args(argc, argv);
	TestStats stats;

	print_header("WEBSERV TEST SUITE");
	std::cout << "Testing server at " << config.server_host << ":" << config.server_port << "\n";

	// Test server connectivity first
	print_info("Checking server connectivity...");
	int test_sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (test_sock < 0)
	{
		std::cerr << Color::RED << "\n✗ ERROR: Cannot connect to server at "
				  << config.server_host << ":" << config.server_port << Color::RESET << "\n";
		std::cerr << "Make sure your server is running!\n\n";
		return 1;
	}
	close(test_sock);
	std::cout << Color::GREEN << "✓ Server is reachable" << Color::RESET << "\n";

	// Run test categories
	if (config.enabled_categories["basic"])
	{
		run_basic_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["directory"])
	{
		run_directory_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["error"])
	{
		run_error_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["keepalive"])
	{
		run_keepalive_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["chunked"])
	{
		run_chunked_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["pipeline"])
	{
		run_pipelining_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["post-delete"])
	{
		run_post_delete(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["cgi"])
	{
		run_cgi_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}
	if (config.enabled_categories["multiport"])
	{
		run_multiport_tests(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	if (config.enabled_categories["redirect"])
	{
		run_redirect_test(config, stats);
		if (config.stop_on_fail && stats.failed > 0)
		{
			print_summary(stats);
			return 1;
		}
	}

	// Print summary
	print_summary(stats);

	// Return exit code based on test results
	return (stats.failed > 0) ? 1 : 0;
}
