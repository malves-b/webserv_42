NAME = webserv

REQUEST_PATH = srcs/request
RESPONSE_PATH = srcs/response
UTILS_PATH = srcs/utils
INIT_PATH = srcs/init
CONFIG_PATH = srcs/config

SRCS = srcs/main.cpp \
	$(REQUEST_PATH)/HttpRequest.cpp \
	$(REQUEST_PATH)/RequestMeta.cpp \
	$(REQUEST_PATH)/RequestParse.cpp \
	$(RESPONSE_PATH)/HttpResponse.cpp \
	$(RESPONSE_PATH)/ResponseBuilder.cpp \
	$(UTILS_PATH)/Logger.cpp \
	$(INIT_PATH)/WebServer.cpp \
	$(INIT_PATH)/ServerSocket.cpp \
	$(INIT_PATH)/ClientConnection.cpp \
	$(CONFIG_PATH)/Config.cpp \
	$(CONFIG_PATH)/ConfigParser.cpp \
	$(CONFIG_PATH)/LocationConfig.cpp \
	$(CONFIG_PATH)/ServerConfig.cpp \

OBJS_DIR = objs
OBJS = $(SRCS:srcs/%.cpp=$(OBJS_DIR)/%.o)

#add logs folder

CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -Iincludes -g 

RM = rm -rf

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -DDEV=1 $(OBJS) -o $(NAME)

all: $(NAME)

$(OBJS_DIR)/%.o: srcs/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS_DIR) 

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re