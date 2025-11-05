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

		static void	computeResolvedPath(HttpRequest& req, const std::string& rawRoot);
		static bool	checkErrorStatus(HttpRequest& req, HttpResponse& res);
		static bool	isUpload(HttpRequest& req, HttpResponse& res, ServerConfig const& config);
		static bool	isStaticFile(const std::string& index, HttpRequest& req, HttpResponse& res);
		static bool	isCgi(const std::string& cgiPath, HttpRequest& req, HttpResponse& res);
		static bool isAutoIndex(const std::string& index, HttpRequest& req, ServerConfig const& config);
		static bool	hasCgiExtension(const std::string& path);
		static bool	isRedirect(HttpRequest& req, HttpResponse& res, ServerConfig const& config);

	public:
		static void	resolve(HttpRequest& req, HttpResponse& res, ServerConfig const& config);
};

#endif //ROUTER_HPP