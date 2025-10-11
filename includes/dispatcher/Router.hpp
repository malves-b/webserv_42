#ifndef ROUTER_HPP
#define ROUTER_HPP

//webserv
#include <init/ClientConnection.hpp>

class Router
{
	private:
		Router(); //blocked
		~Router(); //blocked
		Router(const Router& rhs); //blocked
		Router& operator=(const Router& rhs); //blocked

		static void	computeResolvedPath(HttpRequest& request);
		static bool	checkErrorStatus(ResponseStatus::code status,
						HttpRequest& req,
						HttpResponse& res);
		static bool	isUpload(const std::string& uploadPath,	HttpRequest& req);
		static bool	isStaticFile(const std::string& index,
						ResponseStatus::code& status,
						HttpRequest& req);
		static bool	isCgi(const std::string& cgiPath,
						const std::string resolvedPath,
						ResponseStatus::code& status);
		static bool	hasCgiExtension(const std::string& path);
		static bool	isRedirect(HttpRequest& req, HttpResponse& res);

	public:
		static void	resolve(HttpRequest& request, HttpResponse& response);
};

#endif //ROUTER_HPP