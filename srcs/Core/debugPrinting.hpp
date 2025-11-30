#ifndef DEBUGPRINTING_HPP
#define DEBUGPRINTING_HPP

#include <string>
#include <iostream>
#include <sstream>

// Function declarations (tell compiler these exist)
void print_header(std::string header_name);
void print_white(std::string message, int is_debug);
void print_red(std::string message, int is_debug);
void print_green(std::string message, int is_debug);
void print_yellow(std::string message, int is_debug);
void print_blue(std::string message, int is_debug);
void print_magenta(std::string message, int is_debug);
void print_cyan(std::string message, int is_debug);
std::string convert_to_string(int number);

#endif
