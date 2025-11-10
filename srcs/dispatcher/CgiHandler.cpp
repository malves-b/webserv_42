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

std::string	CgiHandler::extractScriptName(const std::string& resolvedPath)
{
	std::string::size_type lastSlash = resolvedPath.find_last_of('/');
	if (lastSlash != std::string::npos)
		return (resolvedPath.substr(lastSlash + 1));
	return (resolvedPath);
}

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

	// add HTTP_ headers
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

void	CgiHandler::freeEnvp(char** envp)
{
	for (int i = 0; envp[i] != NULL; ++i)
		delete[] envp[i];
	delete[] envp;
}

void	CgiHandler::setupRedirection(int* stdinPipe, int* stdoutPipe)
{
	close(stdinPipe[1]);
	close(stdoutPipe[0]);

	if (dup2(stdinPipe[0], STDIN_FILENO) == -1)
	{
		Logger::instance().log(ERROR, "CgiHandler: dup2(stdin) failed: " + std::string(strerror(errno)));
		_exit(EXIT_FAILURE);
	}
	close(stdinPipe[0]);

	if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1)
	{
		Logger::instance().log(ERROR, "CgiHandler: dup2(stdout) failed: " + std::string(strerror(errno)));
		_exit(EXIT_FAILURE);
	}
	close(stdoutPipe[1]);
}

/* ==========================================================
**  BLOQUEANTE (modo compatível com o atual Dispatcher)
** ========================================================== */
void	CgiHandler::handle(HttpRequest& request, HttpResponse& response)
{
	Logger::instance().log(DEBUG, "[Started] CgiHandler::handle");

	int stdinPipe[2];
	int stdoutPipe[2];

	if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1)
	{
		Logger::instance().log(ERROR, "CgiHandler: pipe() failed: " + std::string(strerror(errno)));
		response.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		Logger::instance().log(ERROR, "CgiHandler: fork() failed: " + std::string(strerror(errno)));
		response.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}

	// CHILD
	if (pid == 0)
	{
		setupRedirection(stdinPipe, stdoutPipe);

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

	// PARENT
	Signals::registerCgiProcess(pid);
	close(stdinPipe[0]);
	close(stdoutPipe[1]);

	const std::string& body = request.getBody();
	if (!body.empty())
		write(stdinPipe[1], body.c_str(), body.size());
	close(stdinPipe[1]);

	std::string output;
	char buffer[4096];
	ssize_t n;
	while ((n = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0)
		output.append(buffer, n);

	close(stdoutPipe[0]);
	waitpid(pid, NULL, 0);
	Signals::unregisterCgiProcess(pid);

	ResponseBuilder::handleCgiOutput(response, output);
	Logger::instance().log(DEBUG, "[Finished] CgiHandler::handle");
}

/* ==========================================================
**  ASSÍNCRONO (usado pelo WebServer com poll)
** ========================================================== */
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
		// CHILD
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

	// PARENT
	Signals::registerCgiProcess(pid);
	close(pipeIn[0]);
	close(pipeOut[1]);

	// envia corpo se houver
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
