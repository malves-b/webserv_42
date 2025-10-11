#include <iostream>
#include <sys/wait.h>
#include <dispatcher/CgiHandler.hpp>
#include <utils/Logger.hpp>
#include <response/ResponseBuilder.hpp>

std::string	CgiHandler::extractScriptName(const std::string& resolvedPath)
{
	std::string::size_type lastSlash = resolvedPath.find_last_of("/");
	if (lastSlash != std::string::npos)
	{
		return resolvedPath.substr(lastSlash + 1);
	}
	return resolvedPath;
}

std::string	CgiHandler::extractPathInfo(const std::string &uri, const std::string &scriptName)
{
	std::string::size_type script_pos = uri.find(scriptName);
	if (script_pos != std::string::npos)
	{
		std::string::size_type pathInfoStart = script_pos + scriptName.length();
		if (pathInfoStart < uri.length())
		{
			return uri.substr(pathInfoStart);
		}
	}
	return "";
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

	// SERVER
	std::string resolved = request.getResolvedPath();
	env.push_back("SCRIPT_FILENAME=" + resolved);
	env.push_back("SCRIPT_NAME=" + extractScriptName(resolved));
	env.push_back("PATH_INFO=" + extractPathInfo(request.getUri(), extractScriptName(resolved)));
	env.push_back("PATH_TRANSLATED=" + resolved);

	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_SOFTWARE=webservinho/1.0");

	// Host SERVER_NAME / SERVER_PORT
	std::string host = request.getHeader("Host");
	if (host != "Host")
	{
		size_t colon = host.find(":");
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

	// HTTP_* headers
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

	for (size_t i = 0; i < env.size(); ++i)
	{
		std::cout << env[i] << std::endl;
	}

	// char **
	char** envp = new char*[env.size() + 1];
	for (size_t i = 0; i < env.size(); ++i) {
		envp[i] = new char[env[i].size() + 1];
		std::strcpy(envp[i], env[i].c_str());
	}
	envp[env.size()] = NULL;

	return envp;
}

void	CgiHandler::freeEnvp(char** envp)
{
	for (int i = 0; envp[i] != NULL; ++i) {
		delete[] envp[i];
	}
	delete[] envp;
}

void	CgiHandler::setupRedirection(int *stdinPipe, int *stdoutPipe)
{
	close(stdinPipe[1]);
	close(stdoutPipe[0]);

	// redirect stdin
	dup2(stdinPipe[0], STDIN_FILENO);
	close(stdinPipe[0]);

	// redirect stdout
	dup2(stdoutPipe[1], STDOUT_FILENO);
	close(stdoutPipe[1]);
}

void	CgiHandler::checkForFailure(pid_t pid, HttpResponse& response)
{
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status))
	{
		int exitCode = WEXITSTATUS(status);
		if (exitCode != 0)
		{
			Logger::instance().log(ERROR, "CgiHandler CGI exited with code " + toString(exitCode));
			response.setStatusCode(ResponseStatus::InternalServerError);
		}
	}
	else if (WIFSIGNALED(status))
	{
		int signal = WTERMSIG(status);
		Logger::instance().log(ERROR, "CgiHandler CGI terminated by signal " + toString(signal));
		response.setStatusCode(ResponseStatus::InternalServerError);
	}
}

void	CgiHandler::handle(HttpRequest &request, HttpResponse& response)
{
	Logger::instance().log(DEBUG, "[Started] CgiHandler::handle");
	pid_t pid;
	int stdinPipe[2];
	int stdoutPipe[2];

	if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1)
	{
		Logger::instance().log(ERROR, "CgiHandler::handle pipe()");
		response.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}
	
	pid = fork();
	if (pid == -1)
	{
		Logger::instance().log(ERROR, "CgiHandler::handle fork()");
		response.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}
	
	//child process
	if (pid == 0)
	{
		setupRedirection(stdinPipe, stdoutPipe);

		std::string resolvedpath = request.getResolvedPath();
		std::string rootDir = resolvedpath.substr(0, resolvedpath.find_last_of('/'));
		char* argv[] = {&resolvedpath[0], NULL};
		char **envp = buildEnvp(request);
		if (chdir(rootDir.c_str()) == -1)
		{
			Logger::instance().log(ERROR, "CgiHandler::handle chdir() failed");
			exit(EXIT_FAILURE);
		}
		execve(resolvedpath.c_str(), argv, envp);
		freeEnvp(envp);

		Logger::instance().log(ERROR, "CgiHandler::handle execve()");
		exit(EXIT_FAILURE);
	}
	else
	{
		close(stdinPipe[0]);
		checkForFailure(pid, response);
		close(stdoutPipe[1]);

		// write body
		std::string body = request.getBody();
		if (!body.empty())
			write(stdinPipe[1], body.c_str(), body.size());
		close(stdinPipe[1]); // EOF to CGI

		// read output
		char buffer[4096];
		std::string output;
		ssize_t bytesRead;
		while ((bytesRead = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0)
		{
			output.append(buffer, bytesRead);
		}
		close(stdoutPipe[0]);

		ResponseBuilder::handleCgiOutput(response, output);
	}
	Logger::instance().log(DEBUG, "[Finished] CgiHandler::handle");
}
