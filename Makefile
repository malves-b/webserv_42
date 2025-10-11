NAME = webserv

REQUEST_PATH = srcs/request
RESPONSE_PATH = srcs/response
DISPATCHER_PATH = srcs/dispatcher
UTILS_PATH = srcs/utils
INIT_PATH = srcs/init

SRCS = srcs/main.cpp \
	$(REQUEST_PATH)/HttpRequest.cpp \
	$(REQUEST_PATH)/RequestMeta.cpp \
	$(REQUEST_PATH)/RequestParse.cpp \
	$(RESPONSE_PATH)/HttpResponse.cpp \
	$(RESPONSE_PATH)/ResponseBuilder.cpp \
	$(DISPATCHER_PATH)/Router.cpp \
	$(DISPATCHER_PATH)/Dispatcher.cpp \
	$(DISPATCHER_PATH)/StaticPageHandler.cpp \
	$(DISPATCHER_PATH)/CgiHandler.cpp \
	$(DISPATCHER_PATH)/AutoIndexHandler.cpp \
	$(UTILS_PATH)/Logger.cpp \
	$(INIT_PATH)/WebServer.cpp \
	$(INIT_PATH)/ServerSocket.cpp \
	$(INIT_PATH)/ClientConnection.cpp \
	$(INIT_PATH)/ServerConfig.cpp \

OBJS_DIR = objs
OBJS = $(SRCS:srcs/%.cpp=$(OBJS_DIR)/%.o)

#add logs folder

CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -g -I./includes -DDEV=1

RM = rm -f

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

all: $(NAME)

$(OBJS_DIR)/%.o: srcs/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re