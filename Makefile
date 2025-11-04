NAME = webserv

CPPC = c++

RM = rm -f

CPPFLAGS += -Wall -Wextra -Werror -std=c++98 -MMD -MP

SRCDIR := srcs

SRCS = $(addprefix $(SRCDIR)/, \
		client.cpp main.cpp method.cpp parserConfig.cpp request.cpp response.cpp route.cpp server.cpp service.cpp \
	)

OBJDIR := build

OBJS := $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))

DEPS := $(OBJS:.o=.d)

all:  $(NAME)

$(NAME): $(OBJS)
	@$(CPPC) $(CPPFLAGS) $(OBJS) -o $(NAME)
	@echo "Created executable:" $(NAME)
	@echo "\nUsage : run the webserv executable followed by the config file.\n"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CPPC) $(CPPFLAGS) -c $< -o $@
	@echo "Compiling $< -> $@"

-include $(DEPS)

clean:
	@$(RM) $(OBJS) $(DEPS)
	@rm -rf $(OBJDIR)
	@echo "Cleaned object and dependancie files"

fclean: clean
	@$(RM) $(NAME)
	@echo "Removed Executable"

re:	fclean all

cleanly : all clean

.PHONY: all clean fclean re
