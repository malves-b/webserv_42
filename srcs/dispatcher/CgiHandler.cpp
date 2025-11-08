#include <iostream>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <ctime>
#include <dispatcher/CgiHandler.hpp>
#include <response/ResponseBuilder.hpp>
#include <utils/Logger.hpp>
#include <utils/Signals.hpp>

static const int CGI_TIMEOUT_SEC = 30;

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
	env.push_back("SERVER_SOFTWARE=WebServinho/1.0");

	env.push_back("REDIRECT_STATUS=200"); //php-cgi

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

	// Add HTTP_ headers
	const std::map<std::string, std::string>& headers = request.getAllHeaders();
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it)
	{
		std::string key = it->first;
		std::string val = it->second;

		std::string envKey = "HTTP_" + key;
		std::replace(envKey.begin(), envKey.end(), '-', '_');
		envKey = toUpper(envKey);
		env.push_back(envKey + "=" + val);
	}

	// Allocate envp
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
		exit(EXIT_FAILURE);
	}
	close(stdinPipe[0]);

	if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1)
	{
		Logger::instance().log(ERROR, "CgiHandler: dup2(stdout) failed: " + std::string(strerror(errno)));
		exit(EXIT_FAILURE);
	}
	close(stdoutPipe[1]);
}

// void	CgiHandler::checkForFailure(pid_t pid, HttpResponse& response)
// {
// 	int status = 0;
// 	pid_t r = waitpid(pid, &status, 0);

// 	if (r == 0)
// 	{
// 		// still running; caller decide (timeout, etc.)
// 		return ;
// 	}
// 	if (r == -1)
// 	{
// 		if (errno == ECHILD)
// 		{
// 			Logger::instance().log(DEBUG, "CgiHandler: no child process remains (CGI finished)");
// 			return ;
// 		}
// 		Logger::instance().log(WARNING, "CgiHandler: waitpid(WNOHANG) failed: " + std::string(strerror(errno)));
// 		response.setStatusCode(ResponseStatus::InternalServerError);
// 		return ;
// 	}
// 	if (WIFEXITED(status))
// 	{
// 		int exitCode = WEXITSTATUS(status);
// 		if (exitCode != 0)
// 		{
// 			Logger::instance().log(ERROR, "CgiHandler: CGI exited with code " + toString(exitCode));
// 			response.setStatusCode(ResponseStatus::InternalServerError);
// 		}
// 		return ;
// 	}
// 	if (WIFSIGNALED(status))
// 	{
// 		int sig = WTERMSIG(status);
// 		Logger::instance().log(ERROR, "CgiHandler: CGI terminated by signal " + toString(sig));
// 		response.setStatusCode(ResponseStatus::InternalServerError);
// 		return ;
// 	}
// }

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
		{
			Logger::instance().log(ERROR, "CgiHandler: chdir() failed: " + std::string(strerror(errno)));
			exit(EXIT_FAILURE);
		}

		char* argv[] = { &resolvedPath[0], NULL };
		char** envp = buildEnvp(request);

		execve(resolvedPath.c_str(), argv, envp);

		Logger::instance().log(ERROR, "CgiHandler: execve() failed: " + std::string(strerror(errno)));
		freeEnvp(envp);
		exit(EXIT_FAILURE);
	}

	// PARENT
	Signals::registerCgiProcess(pid);
	close(stdinPipe[0]);
	close(stdoutPipe[1]);

	{
		const std::string& body = request.getBody();
		if (!body.empty())
		{
			const char* p = body.c_str();
			size_t left = body.size();
			while (left > 0)
			{
				ssize_t w = write(stdinPipe[1], p, left);
				if (w < 0)
				{
					if (errno == EINTR)
						continue ;
					Logger::instance().log(WARNING, "CgiHandler: write() failed: " + std::string(strerror(errno)));
					break ;
				}
				left -= (size_t)w;
				p += w;
			}
		}
	}
	close(stdinPipe[1]);

	if (fcntl(stdoutPipe[0], F_SETFL, O_NONBLOCK) == -1)
		Logger::instance().log(WARNING, "CgiHandler: fcntl(O_NONBLOCK) failed on CGI stdout");

	std::string output;
	char buffer[4096];
	std::time_t start = std::time(NULL);
	bool done = false;

	while (!done)
	{
		std::time_t now = std::time(NULL);
		double elapsed = difftime(now, start);
		if (elapsed >= CGI_TIMEOUT_SEC)
		{
			Logger::instance().log(WARNING, "CgiHandler: CGI timeout exceeded (" + toString(CGI_TIMEOUT_SEC) + "s); killing pid=" + toString(pid));
			kill(pid, SIGKILL);
			int st;
			waitpid(pid, &st, 0);
			Signals::unregisterCgiProcess(pid);

			response.setStatusCode(ResponseStatus::GatewayTimeout); // 504
			close(stdoutPipe[0]);
			Logger::instance().log(DEBUG, "[Finished] CgiHandler::handle (timeout)");
			return ;
		}

		struct pollfd pfd;
		pfd.fd = stdoutPipe[0];
		pfd.events = POLLIN;
		pfd.revents = 0;

		int pr = poll(&pfd, 1, 100);
		if (pr > 0 && (pfd.revents & POLLIN))
		{
			for (;;)
			{
				ssize_t n = read(stdoutPipe[0], buffer, sizeof(buffer));
				if (n > 0)
				{
					output.append(buffer, n);
					continue ;
				}
				if (n == -1 && errno == EAGAIN)
					break ;
				if (n == -1 && errno == EINTR)
					continue ;
				if (n == 0)
				{
					done = true;
					break ;
				}
				Logger::instance().log(WARNING, "CgiHandler: read() failed: " + std::string(strerror(errno)));
				done = true;
				break ;
			}
		}
		else if (pr == 0)
		{
			;
		}
		else if (pr < 0)
		{
			if (errno == EINTR)
				continue ;
			Logger::instance().log(WARNING, "CgiHandler: poll() failed: " + std::string(strerror(errno)));
			done = true;
		}

		int st;
		pid_t res = waitpid(pid, &st, WNOHANG);
		if (res > 0)
			done = true;
	}

	// for (;;)
	// {
	// 	ssize_t n = read(stdoutPipe[0], buffer, sizeof(buffer));
	// 	if (n <= 0) break;
	// 	output.append(buffer, n);
	// }

	close(stdoutPipe[0]);
	int status = 0;
	waitpid(pid, &status, WNOHANG);

	Signals::unregisterCgiProcess(pid);
	ResponseBuilder::handleCgiOutput(response, output);

	Logger::instance().log(DEBUG, "[Finished] CgiHandler::handle");
}
