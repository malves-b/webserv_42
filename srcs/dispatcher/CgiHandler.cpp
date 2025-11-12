#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <dispatcher/CgiHandler.hpp>
#include <response/ResponseBuilder.hpp>
#include <utils/Logger.hpp>
#include <utils/Signals.hpp>
#include <utils/string_utils.hpp>

/**
 * @brief Extracts the script name from a resolved filesystem path.
 *
 * Example:
 * "/var/www/cgi-bin/echo.py" → "echo.py"
 */
std::string	CgiHandler::extractScriptName(const std::string& resolvedPath)
{
	std::string::size_type lastSlash = resolvedPath.find_last_of('/');
	if (lastSlash != std::string::npos)
		return (resolvedPath.substr(lastSlash + 1));
	return (resolvedPath);
}

/**
 * @brief Extracts PATH_INFO from the original URI following the script name.
 *
 * Example:
 * URI "/cgi-bin/echo.py/foo/bar" → PATH_INFO="/foo/bar"
 */
std::string	CgiHandler::extractPathInfo(const std::string& uri, const std::string& scriptName)
{
	std::string::size_type scriptPos = uri.find(scriptName);
	if (scriptPos != std::string::npos)
	{
		std::string::size_type start = scriptPos + scriptName.length();
		if (start < uri.length())
			return (uri.substr(start));
	}
	return ("");
}

/**
 * @brief Builds the CGI environment variables array (envp).
 *
 * Converts HTTP request headers and metadata into key-value pairs
 * according to CGI/1.1 specification. Allocates memory dynamically.
 *
 * @return A NULL-terminated array of C-strings suitable for execve().
 */
char**	CgiHandler::buildEnvp(HttpRequest& request)
{
	std::vector<std::string> env;

	env.push_back("REQUEST_METHOD=" + request.methodToString());
	env.push_back("QUERY_STRING=" + request.getQueryString());

	std::string contentType = request.getHeader("Content-Type");
	if (contentType != "Content-Type")
		env.push_back("CONTENT_TYPE=" + contentType);

	std::string contentLength = request.getHeader("Content-Length");
	if (contentLength != "Content-Length")
		env.push_back("CONTENT_LENGTH=" + contentLength);

	std::string resolved = request.getResolvedPath();
	env.push_back("SCRIPT_FILENAME=" + resolved);
	env.push_back("SCRIPT_NAME=" + extractScriptName(resolved));
	env.push_back("PATH_INFO=" + extractPathInfo(request.getUri(), extractScriptName(resolved)));
	env.push_back("PATH_TRANSLATED=" + resolved);

	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_SOFTWARE=Webservinho/1.0");
	env.push_back("REDIRECT_STATUS=200");

	std::string host = request.getHeader("Host");
	if (host != "Host")
	{
		size_t colon = host.find(':');
		if (colon != std::string::npos)
		{
			env.push_back("SERVER_NAME=" + host.substr(0, colon));
			env.push_back("SERVER_PORT=" + host.substr(colon + 1));
		}
		else
		{
			env.push_back("SERVER_NAME=" + host);
			env.push_back("SERVER_PORT=80");
		}
	}
	else
	{
		env.push_back("SERVER_NAME=localhost");
		env.push_back("SERVER_PORT=80");
	}

	// Convert HTTP headers to CGI-style environment variables (HTTP_HEADER_NAME)
	const std::map<std::string, std::string>& headers = request.getAllHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		std::string key = it->first;
		std::string val = it->second;
		std::string envKey = "HTTP_" + key;
		std::replace(envKey.begin(), envKey.end(), '-', '_');
		std::transform(envKey.begin(), envKey.end(), envKey.begin(), ::toupper);
		env.push_back(envKey + "=" + val);
	}

	char** envp = new char*[env.size() + 1];
	for (size_t i = 0; i < env.size(); ++i)
	{
		envp[i] = new char[env[i].size() + 1];
		std::strcpy(envp[i], env[i].c_str());
	}
	envp[env.size()] = NULL;

	Logger::instance().log(DEBUG, "CgiHandler: Environment built with " + toString(env.size()) + " variables");
	return (envp);
}

/**
 * @brief Frees memory allocated for the environment variable array.
 */
void	CgiHandler::freeEnvp(char** envp)
{
	for (int i = 0; envp[i] != NULL; ++i)
		delete[] envp[i];
	delete[] envp;
}

/**
 * @brief Starts an asynchronous CGI execution process.
 *
 * Creates non-blocking pipes, forks a child process, and registers it
 * for monitoring. Used by the event-driven dispatcher for non-blocking CGI.
 *
 * @callgraph
 * @param request The HTTP request associated with the CGI.
 * @param clientFd The client socket file descriptor for reference.
 * @return A populated CgiProcess structure with process metadata.
 */
CgiProcess	CgiHandler::startAsync(HttpRequest& request, int clientFd)
{
	int pipeIn[2];
	int pipeOut[2];
	if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0)
		throw std::runtime_error("CgiHandler: pipe() failed");

	fcntl(pipeIn[1],  F_SETFL, O_NONBLOCK);
	fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);

	pid_t pid = fork();
	if (pid < 0)
		throw std::runtime_error("CgiHandler: fork() failed");

	if (pid == 0)
	{
		dup2(pipeIn[0], STDIN_FILENO);
		dup2(pipeOut[1], STDOUT_FILENO);
		close(pipeIn[1]);
		close(pipeOut[0]);

		std::string resolvedPath = request.getResolvedPath();
		std::string rootDir = resolvedPath.substr(0, resolvedPath.find_last_of('/'));
		if (chdir(rootDir.c_str()) == -1)
			_exit(EXIT_FAILURE);

		char** envp = buildEnvp(request);
		char* argv[] = { &resolvedPath[0], NULL };
		execve(resolvedPath.c_str(), argv, envp);

		freeEnvp(envp);
		_exit(EXIT_FAILURE);
	}

	Signals::registerCgiProcess(pid);
	close(pipeIn[0]);
	close(pipeOut[1]);

	const std::string& body = request.getBody();
	if (!body.empty())
		write(pipeIn[1], body.c_str(), body.size());
	close(pipeIn[1]);

	CgiProcess proc;
	proc.pid = pid;
	proc.in_fd = -1;
	proc.out_fd = pipeOut[0];
	proc.err_fd = -1;
	proc.headersParsed = false;
	proc.finished = false;
	proc.chunked = false;
	proc.startAt = time(NULL);
	proc.deadline = proc.startAt + Signals::CGI_TIMEOUT_SEC;
	proc.client_fd = clientFd;

	fcntl(proc.out_fd, F_SETFL, O_NONBLOCK);

	Logger::instance().log(DEBUG, "CGI: started async pid=" + toString(pid) +
		" fd=" + toString(proc.out_fd));

	return proc;
}
