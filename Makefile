NAME = webserv

CPPC = c++

RM = rm -f

CPPFLAGS += -Wall -Wextra -Werror -std=c++98 -MMD -MP

SRCDIR := srcs

# SRCS = $(addprefix $(SRCDIR)/, \
# 		client.cpp \ main.cpp  parserConfig.cpp request.cpp response.cpp route.cpp server.cpp service.cpp \
# 	)

SRCS =	srcs/main.cpp  \
		srcs/Core/service.cpp \
		srcs/Functions/method.cpp \
		srcs/Functions/parserConfig.cpp \
		srcs/Functions/requestUtils.cpp \
		srcs/Functions/workCGI.cpp \
		srcs/Models/client.cpp \
		srcs/Models/request.cpp \
		srcs/Models/response.cpp \
		srcs/Models/route.cpp \
		srcs/Models/server.cpp \
		srcs/Models/CGIProcess.cpp

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
