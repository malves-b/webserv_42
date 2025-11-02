#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

//webserv
#include <response/HttpResponse.hpp>
#include <response/ResponseStatus.hpp>
#include <init/ClientConnection.hpp>
#include <config/ServerConfig.hpp>
#include <config/LocationConfig.hpp>

class ResponseBuilder
{
	private:
		ResponseBuilder(); //blocked
		~ResponseBuilder(); //blocked
		ResponseBuilder(const ResponseBuilder& rhs); //blocked
		ResponseBuilder& operator=(const ResponseBuilder& rhs); //blocked

		static const std::string	fmtTimestamp(void);
		static void					setMinimumHeaders(HttpResponse& response);
		static std::string			errorPageGenerator(ResponseStatus::code code);
		static bool					errorPageConfig(const std::string& root, HttpResponse& res, ServerConfig const& config);
		static bool					shouldCloseConnection(int statusCode);

	public:
		static const std::string	responseWriter(HttpResponse& response);
		static void					build(ClientConnection& client, HttpRequest& req, HttpResponse& res);
		static void					handleCgiOutput(HttpResponse& response, const std::string& output);
		static void					handleStaticPageOutput(HttpResponse& response,
										const std::string output,
										const std::string& mimeType);
};

#endif //RESPONSE_BUILDER_HPP