#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>

//webserv
#include <response/ResponseStatus.hpp>

//Data Transfer Object
class HttpResponse
{
	private:
		ResponseStatus::code				_statusCode;
		std::string							_reasonPhrase;
		std::string							_version;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		bool								_chunked; // transfer encoding

		HttpResponse& operator=(const HttpResponse& rhs); //blocked
		HttpResponse(const HttpResponse& rhs); //blocked

		const std::string	statusCodeToString(const ResponseStatus::code& code);

	public:
		HttpResponse();
		~HttpResponse();

		//setters
		void	setStatusCode(const ResponseStatus::code& code);
		void	setReasonPhrase(const ResponseStatus::code& code);
		void	setVersion(const std::string& version);
		void	appendBody(const std::string& body);
		void	appendBody(char c);
		void	addHeader(const std::string& name, const std::string& value);
		void	setChunked(bool chunked);
		void	reset(void);

		//getters
		ResponseStatus::code		getStatusCode(void) const;
		const std::string&			getReasonPhrase(void) const;
		const std::string&			getHttpVersion(void) const;
		const std::string&			getBody(void) const;
		const std::string&			getHeader(const std::string& name) const;
		const std::map<std::string, std::string>&	getHeaders(void) const;
		bool						isChunked(void) const;
};

#endif //HTTP_RESPONSE_HPP