#include "debugPrinting.hpp"

extern bool DEBUG;

void print_header(std::string header_name)
{
	if (DEBUG)
	{
		header_name = " " + header_name + " ";
		while(header_name.length() < 49)
			header_name = "#" + header_name + "#";
		std::cout << "\n###################################################" << std::endl;
		std::cout << header_name << std::endl;
		std::cout << "###################################################\n" << std::endl;
	}
}

void print_white(std::string message, int is_debug)
{
	if (is_debug)
		std::cout << message << std::endl;
}

void print_red(std::string message, int is_debug)
{
	if (is_debug)
		std::cout << "\033[31m" << message << "\033[0m" << std::endl;
}
void print_green(std::string message, int is_debug)
{
	if (is_debug)
		std::cout << "\033[32m" << message << "\033[0m" << std::endl;
}
void print_yellow(std::string message, int is_debug)
{
	if (is_debug)
		std::cout << "\033[33m" << message << "\033[0m" << std::endl;
}
void print_blue(std::string message, int is_debug)
{
	if (is_debug)
		std::cout << "\033[34m" << message << "\033[0m" << std::endl;
}
void print_magenta(std::string message, int is_debug)
{
	if (is_debug)
		std::cout << "\033[35m" << message << "\033[0m" << std::endl;
}
void print_cyan(std::string message, int is_debug)
{
	if (is_debug)
		std::cout << "\033[36m" << message << "\033[0m" << std::endl;
}

std::string convert_to_string(int number)
{
	std::ostringstream ss;
	ss << number;
	return ss.str();
}

// White	\033[37m
// Reset	\033[0m
// Bright Colors
// Color	ANSI Code
// Bright Black	\033[90m
// Bright Red	\033[91m
// Bright Green	\033[92m
// Bright Yellow	\033[93m
// Bright Blue	\033[94m
// Bright Magenta	\033[95m
// Bright Cyan	\033[96m
// Bright White	\033[97m
