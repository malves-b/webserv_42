#ifndef CGI_HANDLER_HPP
# define CGI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <unistd.h>

//webserv
#include <request/HttpRequest.hpp>
#include <response/HttpResponse.hpp>
#include <init/ClientConnection.hpp>

struct CgiProcess
{
	pid_t		pid;
	int			in_fd;      // stdin do CGI (-1 se não usado)
	int			out_fd;     // stdout do CGI
	int			err_fd;     // stderr (-1 se não usado)
	std::string	hdrBuf;   // cabeçalhos brutos do CGI
	std::string	outBuf;   // corpo da resposta
	bool		headersParsed;
	bool		finished;
	bool		chunked;
	time_t		startAt;
	time_t		deadline;
	int			client_fd;         // fd do cliente associado
};

class CgiHandler
{
	private:
		CgiHandler(const CgiHandler&);
		CgiHandler& operator=(const CgiHandler&);

	public:
		CgiHandler(void) {}
		~CgiHandler(void) {}

		// modo síncrono (para testes ou fallback)
		static void			handle(HttpRequest& request, HttpResponse& response);

		// modo assíncrono: cria processo CGI e retorna estrutura com FDs
		static CgiProcess	startAsync(HttpRequest& request, int clientFd);

		// helpers
		static std::string	extractScriptName(const std::string& resolvedPath);
		static std::string	extractPathInfo(const std::string& uri, const std::string& scriptName);

		static char**		buildEnvp(HttpRequest& request);
		static void			freeEnvp(char** envp);
		static void			setupRedirection(int* stdinPipe, int* stdoutPipe);
};

#endif