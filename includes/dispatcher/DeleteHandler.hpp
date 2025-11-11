#ifndef DELETE_HANDLER_HPP
# define DELETE_HANDLER_HPP

#include <string>
#include <unistd.h>
#include <cerrno>
#include <cstring>

//webserv
#include <response/ResponseStatus.hpp>
#include <response/HttpResponse.hpp>
#include <request/HttpRequest.hpp>
#include <utils/Logger.hpp>

class DeleteHandler
{
	public:
		static void	handle(HttpRequest& req, HttpResponse& res);
};

#endif