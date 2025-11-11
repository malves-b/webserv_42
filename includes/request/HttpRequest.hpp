#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>

//webserv
#include <request/RequestMethod.hpp>
#include <request/RequestMeta.hpp>
#include <request/RequestState.hpp>
#include <response/ResponseStatus.hpp>
#include <dispatcher/RouteType.hpp>

//Data Transfer Object
class HttpRequest
{
	private:
		HttpRequest& operator=(const HttpRequest& rhs); //blocked
		HttpRequest(const HttpRequest& rhs); //blocked

		RequestMethod::Method				_method;
		std::string							_uri;
		std::string							_queryString;
		int									_major;
		int									_minor;
		std::map<std::string, std::string>	_headers;
		RequestMeta							_meta;
		std::string							_body;
		ResponseStatus::code				_parseError;
		RequestState::state					_state;
		RouteType::route					_route;
		std::string							_rawRequest;
		std::string							_buffer;
		std::string							_chunkBuffer;
		int									_currentChunkSize;
		bool								_parsingChunkSize;
		bool								_expectingChunkSeparator;
		std::string							_resolvedPath;

	public:
		HttpRequest();
		~HttpRequest();

		//setters
		void	setMethod(const RequestMethod::Method& method);
		void	setUri(const std::string& uri);
		void	setQueryString(const std::string queryString);
		void	setMajor(int major);
		void	setMinor(int minor);
		void	appendBody(const std::string& body);
		void	appendBody(char c);
		void	setParseError(ResponseStatus::code reason);
		void	addHeader(const std::string& name, const std::string& value);
		void	setRequestState(RequestState::state state);
		void	setRouteType(RouteType::route route);
		void	appendRaw(const std::string& chunk);
		void	clearBuffer(void);
		void	clearChunkBuffer(void);
		void	setCurrentChunkSize(int size);
		void	setParsingChunkSize(bool value);
		void	setExpectingChunkSeparator(bool value);
		void	setResolvedPath(const std::string path);
		void	reset(void);

		//getters
		RequestMethod::Method		getMethod(void) const;
		const std::string			methodToString(void) const;
		const std::string&			getUri(void) const;
		const std::string&			getQueryString(void) const;
		const std::vector<int>		getHttpVersion(void) const;
		const RequestMeta&			getMeta(void) const;
		RequestMeta&				getMeta(void);
		const std::string&			getBody(void) const;
		ResponseStatus::code		getParseError(void) const;
		const std::string&			getHeader(const std::string& name) const;
		const std::map<std::string, std::string>&	getAllHeaders(void) const;
		RequestState::state			getState(void) const;
		RouteType::route			getRouteType(void) const;
		std::string&				getRaw(void);
		std::string&				getBuffer(void);
		std::string&				getChunkBuffer(void);
		int							getCurrentChunkSize(void) const;
		bool						isParsingChunkSize() const;
		bool						isExpectingChunkSeparator() const;
		const std::string			getResolvedPath(void) const;

		bool						hasHeader(const std::string& key) const;
		void						removeHeader(const std::string& key);
};

#endif //HTTP_REQUEST_HPP