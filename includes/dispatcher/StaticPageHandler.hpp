#ifndef STATIC_PAGE_HANDLER_HPP
# define STATIC_PAGE_HANDLER_HPP

#include <string>

//webserv
#include <request/HttpRequest.hpp>
#include <response/HttpResponse.hpp>

class StaticPageHandler
{
	private:
		StaticPageHandler(); //blocked
		~StaticPageHandler(); //blocked
		StaticPageHandler& operator=(const StaticPageHandler& rhs); //blocked
		StaticPageHandler(const StaticPageHandler& rhs); //blocked

		static const std::string	detectMimeType(const std::string& resolvedPath);

	public:
		static void	handle(HttpRequest& req, HttpResponse& res);
};

#endif //STATIC_PAGE_HANDLER_HPP
