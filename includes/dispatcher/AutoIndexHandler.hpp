#ifndef AUTO_INDEX_HANDLER_HPP
#define AUTO_INDEX_HANDLER_HPP

//webserv
#include <request/HttpRequest.hpp>
#include <response/HttpResponse.hpp>

class AutoIndexHandler
{
	private:
		static std::string	getTemplate();
		static std::string	formatSize(size_t size);
		static void			replacePlaceholder(std::string& html, const std::string& tag, const std::string& value);
		static std::string	loadTemplate(const std::string& path);
		static std::string	getTemplatePath();

	public:
		static void			handle(HttpRequest& req, HttpResponse& res);
};

#endif // AUTO_INDEX_HANDLER_HPP