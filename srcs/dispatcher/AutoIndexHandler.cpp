#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <dispatcher/AutoIndexHandler.hpp>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>
#include <response/ResponseBuilder.hpp>

void	AutoIndexHandler::handle(HttpRequest& req, HttpResponse& res)
{
	Logger::instance().log(DEBUG, "[Started] AutoIndexHandler::handle");

	std::string resolvedPath = req.getResolvedPath();
	std::string uri = req.getUri();

	if (uri.empty() || uri[uri.size() - 1] != '/')
		uri += '/';

	// Try to open directory
	DIR* dir = opendir(resolvedPath.c_str());
	if (dir == NULL)
	{
		Logger::instance().log(ERROR, "AutoIndexHandler: Failed to open directory: " + resolvedPath);
		res.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}

	std::string content;
	struct dirent* entry;

	// Build directory listing rows
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;

		if (name == "." || name == "..")
			continue;

		std::string fullPath = resolvedPath + "/" + name;
		struct stat fileStat;

		if (stat(fullPath.c_str(), &fileStat) != 0)
		{
			Logger::instance().log(WARNING, "AutoIndexHandler: Unable to stat file: " + fullPath);
			continue;
		}

		bool isDir = S_ISDIR(fileStat.st_mode);

		char dateBuf[100];
		time_t modTime = fileStat.st_mtime;
		struct tm* timeInfo = localtime(&modTime);
		strftime(dateBuf, sizeof(dateBuf), "%d-%b-%Y %H:%M", timeInfo);

		std::string sizeStr = isDir ? "-" : formatSize(fileStat.st_size);

		content += "<tr>";
		content += "<td><a href=\"" + uri + name + (isDir ? "/" : "") + "\">" + name + (isDir ? "/" : "") + "</a></td>";
		content += "<td>" + std::string(dateBuf) + "</td>";
		content += "<td class=\"size\">" + sizeStr + "</td>";
		content += "</tr>\n";
	}

	closedir(dir);

	// Load external template file
	std::string html = loadTemplate(getTemplatePath());

	if (html.empty())
	{
		Logger::instance().log(ERROR, "AutoIndexHandler: Template not found or empty");
		res.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}

	// Replace placeholders
	replacePlaceholder(html, "{PATH}", uri);
	replacePlaceholder(html, "{PATH}", uri);
	replacePlaceholder(html, "{CONTENT}", content);
	replacePlaceholder(html, "{SERVER_INFO}", "WebServinho/1.0");

	res.appendBody(html);
	res.addHeader("Content-Type", "text/html");
	res.addHeader("Content-Length", toString(html.size()));
	res.setStatusCode(ResponseStatus::OK);

	Logger::instance().log(DEBUG, "[Finished] AutoIndexHandler::handle");
}

std::string	AutoIndexHandler::formatSize(size_t size)
{
	std::stringstream ss;

	if (size < 1024)
		ss << size << " B";
	else if (size < 1024 * 1024)
		ss << (size / 1024) << " KB";
	else
		ss << (size / (1024 * 1024)) << " MB";

	return (ss.str());
}

void	AutoIndexHandler::replacePlaceholder(std::string& html, const std::string& tag, const std::string& value)
{
	size_t pos = html.find(tag);

	if (pos != std::string::npos)
		html.replace(pos, tag.size(), value);
}

std::string	AutoIndexHandler::loadTemplate(const std::string& path)
{
	Logger::instance().log(DEBUG, "AutoIndexHandler: Loading template file -> " + path);

	std::ifstream file(path.c_str());
	if (!file.is_open())
	{
		Logger::instance().log(ERROR, "AutoIndexHandler: Failed to open template file: " + path);
		return ("");
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	file.close();

	Logger::instance().log(DEBUG, "AutoIndexHandler: Template successfully loaded (" + toString(buffer.str().size()) + " bytes)");
	return (buffer.str());
}

std::string	AutoIndexHandler::getTemplatePath()
{
	char exePath[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);

	if (len == -1)
	{
		Logger::instance().log(WARNING, "AutoIndexHandler: Failed to resolve /proc/self/exe, using default path");
		return ("/var/www/templates/autoindex.html");
	}

	exePath[len] = '\0';
	std::string path(exePath);

	// Remove executable name (after last '/')
	size_t lastSlash = path.find_last_of('/');
	if (lastSlash != std::string::npos)
		path = path.substr(0, lastSlash);

	// Go up one directory (from /bin to project root)
	path += "/assets/autoindex.html";

	Logger::instance().log(DEBUG, "AutoIndexHandler: Resolved template path → " + path);
	return (path);
}
