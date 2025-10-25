#ifndef REQUEST_META_HPP
#define REQUEST_META_HPP

#include <string>

class RequestMeta
{
	private:
		std::size_t	_contentLength; // -1 if there is no
		bool		_chunked; // transfer encoding
		bool		_connectionClose;
		bool		_expectContinue; //expect 100-continue //before send body
		bool		_isRedirect;
		std::string	_host;

		RequestMeta(const RequestMeta& rhs); //blocked
		RequestMeta& operator=(const RequestMeta& rhs); //blocked
	
	public:
		RequestMeta();
		~RequestMeta();

		//setters
		void	setContentLength(std::size_t content_length);
		void	setChunked(bool chunked);
		void	setConnectionClose(bool connection_close);
		void	setExpectContinue(bool expect_continue);
		void	setRedirect(bool redirect);
		void	setHost(const std::string& host);
		void	resetMeta(void);

		//getters
		std::size_t	getContentLength(void) const;
		bool		isChunked(void) const;
		bool		shouldClose(void) const;
		bool		getExpectContinue(void) const;
		bool		isRedirect(void) const;
		std::string	getHost(void) const;
};

#endif //REQUEST_META_HPP