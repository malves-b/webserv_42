#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <utils/Logger.hpp>
#include <dispatcher/DeleteHandler.hpp>

/**
 * @brief Handles HTTP DELETE requests to remove files from the server.
 *
 * This handler verifies file existence, access permissions, and ensures
 * directories cannot be deleted via HTTP.  
 * It maps common POSIX errors to appropriate HTTP status codes:
 *  - ENOENT → 404 Not Found  
 *  - EACCES → 403 Forbidden  
 *  - Others → 500 Internal Server Error  
 *
 * On success, responds with 204 No Content.
 */
void	DeleteHandler::handle(HttpRequest& req, HttpResponse& res)
{
	std::string path = req.getResolvedPath();

	// Verify that the file exists
	if (access(path.c_str(), F_OK) != 0)
	{
		Logger::instance().log(WARNING, "DeleteHandler: File not found -> " + path);
		res.setStatusCode(ResponseStatus::NotFound);
		return ;
	}

	// Prevent deletion of directories for safety
	struct stat s;
	if (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		Logger::instance().log(WARNING, "DeleteHandler: Cannot delete directory -> " + path);
		res.setStatusCode(ResponseStatus::Forbidden);
		return ;
	}

	// Attempt to unlink (delete) the file
	if (unlink(path.c_str()) == 0)
	{
		Logger::instance().log(INFO, "DeleteHandler: Successfully deleted -> " + path);
		res.setStatusCode(ResponseStatus::NoContent);
		return ;
	}

	// Handle deletion failure and map to HTTP status
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
