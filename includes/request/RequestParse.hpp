#ifndef REQUEST_PARSE_HPP
#define REQUEST_PARSE_HPP

//webserv
#include <request/RequestMethod.hpp>
#include <request/HttpRequest.hpp>
#include <config/ServerConfig.hpp>

const size_t MAX_URI = 2048;
const size_t MAX_HEADER_LINE = 8192;
const size_t MAX_TOTAL_HEADER_SIZE = 16384;

class RequestParse
{
	private:
		RequestParse(); //blocked
		~RequestParse(); //blocked
		RequestParse(const RequestParse& rhs); //blocked
		RequestParse& operator=(const RequestParse& rhs); //blocked

		static void	requestLine(const std::string& str, HttpRequest& request, ServerConfig const& config);
		static void	method(const std::string& method, HttpRequest& request, ServerConfig const& config);
		static void	uri(const std::string str, HttpRequest& request);
		static void	headers(const std::string& header, HttpRequest& request, std::size_t maxBodySize);
		static void	body(char c, HttpRequest& request, std::size_t maxBodySize);
		static void	bodyChunked(char c, HttpRequest& request, std::size_t maxBodySize);
		static std::string	extractQueryString(const std::string uri);
		static bool	isGreaterThanMaxBodySize(std::size_t size, std::size_t maxBodySize);
		static void	checkMethod(HttpRequest& request, ServerConfig const& config);

	public:
		static void	handleRawRequest(const std::string& rawRequest, HttpRequest& request, ServerConfig const& config);
};

#endif //REQUEST_PARSE_HPP