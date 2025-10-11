#ifndef REQUEST_PARSE_HPP
#define REQUEST_PARSE_HPP

//webserv
#include <request/RequestMethod.hpp>
#include <request/HttpRequest.hpp>

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

		static void	requestLine(const std::string& str, HttpRequest& request);
		static void	method(const std::string& method, HttpRequest& request);
		static void	uri(const std::string str, HttpRequest& request);
		static void	headers(const std::string& header, HttpRequest& request);
		static void	body(char c, HttpRequest& request);
		static void	bodyChunked(char c, HttpRequest& request);
		static std::string	extractQueryString(const std::string uri);
		static bool	isGreaterThanMaxBodySize(std::size_t size);
		static void	checkMethod(HttpRequest& request);

	public:
		static void	handleRawRequest(const std::string& rawRequest, HttpRequest& request);
};

#endif //REQUEST_PARSE_HPP