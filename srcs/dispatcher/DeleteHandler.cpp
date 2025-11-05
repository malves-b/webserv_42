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

	if (unlink(path.c_str()) == 0)
	{
		Logger::instance().log(INFO, "DeleteHandler: Successfully deleted -> " + path);
		res.setStatusCode(ResponseStatus::NotFound);
		return ;
	}

	Logger::instance().log(ERROR,
		"DeleteHandler: Failed to delete -> " + path + " (" + std::string(strerror(errno)) + ")");

	res.setStatusCode(ResponseStatus::NotFound);
}
