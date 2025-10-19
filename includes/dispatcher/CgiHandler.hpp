#ifndef CGI_HANDLER
# define CGI_HANDLER

#include <map>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

//webserv
#include <utils/string_utils.hpp>
#include <request/HttpRequest.hpp>
#include <response/HttpResponse.hpp>

class CgiHandler
{
	private:
		CgiHandler(); //blocked
		~CgiHandler(); //blocked
		CgiHandler(const CgiHandler& rhs); //blocked
		CgiHandler& operator=(const CgiHandler& rhs); //blocked

		static std::string	extractScriptName(const std::string& resolvedPath);
		static std::string	extractPathInfo(const std::string& uri, const std::string& scriptName);

		static char**		buildEnvp(HttpRequest& request);
		static void			freeEnvp(char **envp);

		static void			setupRedirection(int *stdinPipe, int *stdoutPipe);
		static void			checkForFailure(pid_t pid, HttpResponse& response);
		//static void			parentProcess(int *stdinPipe, int *stdoutPipe);
	
	public:
		static void	handle(HttpRequest& request, HttpResponse& response);
};

#endif // CGI_HANDLER