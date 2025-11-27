# ==============================================================================
# WEBSERV TEST SUITE MAKEFILE
# ==============================================================================

CXX := c++
CXXFLAGS := -std=c++98 -Wall -Wextra -Werror -I.
TARGET := test_suite

# Source files
FRAMEWORK_SRCS := test_framework.cpp
CATEGORY_SRCS := test_basic.cpp \
		test_directory.cpp \
		test_error_codes.cpp \
		test_keepalive.cpp \
		test_multiport.cpp \
		test_chunked.cpp \
		test_pipelining.cpp \
		test_post_delete.cpp \
		test_redirect.cpp \
		test_cgi.cpp
MAIN_SRC := test_suite.cpp

SRCS := $(FRAMEWORK_SRCS) $(CATEGORY_SRCS) $(MAIN_SRC)
OBJS := $(SRCS:.cpp=.o)

# Colors for pretty output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
NC := \033[0m  # No Color

# ==============================================================================
# TARGETS
# ==============================================================================

all: $(TARGET)
	@echo "$(GREEN)✓ Test suite built successfully!$(NC)"
	@echo "$(BLUE)Run with: ./$(TARGET) --help$(NC)"

$(TARGET): $(OBJS)
	@echo "$(YELLOW)Linking $(TARGET)...$(NC)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cpp
	@echo "$(YELLOW)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(YELLOW)Cleaning object files...$(NC)"
	@rm -f $(OBJS)

fclean: clean
	@echo "$(YELLOW)Removing $(TARGET)...$(NC)"
	@rm -f $(TARGET)

re: fclean all

# ==============================================================================
# TESTING TARGETS
# ==============================================================================

test: $(TARGET)
	@echo "$(BLUE)Running all tests...$(NC)"
	@./$(TARGET)

test-basic: $(TARGET)
	@echo "$(BLUE)Running basic tests only...$(NC)"
	@./$(TARGET) --only basic

test-keepalive: $(TARGET)
	@echo "$(BLUE)Running keep-alive tests only...$(NC)"
	@./$(TARGET) --only keepalive

test-chunked: $(TARGET)
	@echo "$(BLUE)Running chunked encoding tests only...$(NC)"
	@./$(TARGET) --only chunked

test-pipeline: $(TARGET)
	@echo "$(BLUE)Running pipeline tests only...$(NC)"
	@./$(TARGET) --only pipeline

test-cgi: $(TARGET)
	@echo "$(BLUE)Running CGI tests only...$(NC)"
	@./$(TARGET) --only cgi

test-verbose: $(TARGET)
	@echo "$(BLUE)Running all tests (verbose)...$(NC)"
	@./$(TARGET) --verbose

# Run with server auto-start (requires server binary in parent dir)
test-full: $(TARGET)
	@echo "$(BLUE)Starting server and running tests...$(NC)"
	@../webserv ../default.conf > /tmp/webserv_test.log 2>&1 & \
		SERVER_PID=$$!; \
		sleep 1; \
		./$(TARGET); \
		TEST_RESULT=$$?; \
		kill $$SERVER_PID 2>/dev/null; \
		exit $$TEST_RESULT

# ==============================================================================
# UTILITY TARGETS
# ==============================================================================

help:
	@echo "$(BLUE)Webserv Test Suite - Available targets:$(NC)"
	@echo "  $(GREEN)all$(NC)              Build test suite"
	@echo "  $(GREEN)test$(NC)             Run all tests"
	@echo "  $(GREEN)test-basic$(NC)       Run only basic tests"
	@echo "  $(GREEN)test-keepalive$(NC)   Run only keep-alive tests"
	@echo "  $(GREEN)test-chunked$(NC)     Run only chunked tests"
	@echo "  $(GREEN)test-pipeline$(NC)    Run only pipeline tests"
	@echo "  $(GREEN)test-cgi$(NC)         Run only CGI tests"
	@echo "  $(GREEN)test-verbose$(NC)     Run all tests with verbose output"
	@echo "  $(GREEN)test-full$(NC)        Auto-start server and run tests"
	@echo "  $(GREEN)clean$(NC)            Remove object files"
	@echo "  $(GREEN)fclean$(NC)           Remove object files and binary"
	@echo "  $(GREEN)re$(NC)               Rebuild from scratch"
	@echo ""
	@echo "$(BLUE)Test suite usage:$(NC)"
	@echo "  ./$(TARGET) --help       Show all command-line options"
	@echo "  ./$(TARGET) --list       List all test categories"

.PHONY: all clean fclean re test test-basic test-keepalive test-chunked test-pipeline test-cgi test-verbose test-full help
