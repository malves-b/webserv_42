#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <utils/Logger.hpp>
#include <dispatcher/DeleteHandler.hpp>

void	DeleteHandler::handle(HttpRequest& req, HttpResponse& res)
{
	std::string path = req.getResolvedPath();

	if (access(path.c_str(), F_OK) != 0)
	{
		Logger::instance().log(WARNING, "DeleteHandler: File not found -> " + path);
		res.setStatusCode(ResponseStatus::NotFound);
		return ;
	}

	struct stat s;
	if (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		Logger::instance().log(WARNING, "DeleteHandler: Cannot delete directory -> " + path);
		res.setStatusCode(ResponseStatus::Forbidden);
		return ;
	}

	if (unlink(path.c_str()) == 0)
	{
		Logger::instance().log(INFO, "DeleteHandler: Successfully deleted -> " + path);
		res.setStatusCode(ResponseStatus::NoContent);
		return ;
	}

	int err = errno;
	Logger::instance().log(ERROR,
		"DeleteHandler: Failed to delete -> " + path + " (" + std::string(strerror(err)) + ")");

	if (err == EACCES)
		res.setStatusCode(ResponseStatus::Forbidden);
	else if (err == ENOENT)
		res.setStatusCode(ResponseStatus::NotFound);
	else
		res.setStatusCode(ResponseStatus::InternalServerError);
}
