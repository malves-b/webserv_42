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

/**
 * @brief Generates and serves a fully hardcoded HTML directory listing.
 *
 * This version of autoindex embeds the entire HTML template directly in the
 * binary, removing the need to load external files. It enumerates all directory
 * entries inside the resolved filesystem path, formats each entry into a table
 * row, and injects the content into the HTML structure.
 *
 * Links are generated using the normalized request path (not the raw URI),
 * ensuring correct behavior with nested directories, trailing slashes and
 * interactions with `root` mappings. Each regular file includes a delete button
 * that triggers a JavaScript DELETE request.
 *
 * The resulting HTML page is fully compliant, styled and self-contained.
 *
 * @param req Reference to the parsed HttpRequest object.
 * @param res Reference to the HttpResponse to be populated and returned.
 * @callgraph
 */
void	AutoIndexHandler::handle(HttpRequest& req, HttpResponse& res)
{
    Logger::instance().log(DEBUG, "[Started] AutoIndexHandler::handle");

    std::string resolvedPath = req.getResolvedPath();
    std::string uri = req.getUri();   // isto já é o request path puro

	if (uri.empty() || uri[uri.size() - 1] != '/')
		uri += '/';


    DIR* dir = opendir(resolvedPath.c_str());
    if (dir == NULL)
    {
        Logger::instance().log(ERROR, "AutoIndexHandler: Failed to open directory: " + resolvedPath);
        res.setStatusCode(ResponseStatus::InternalServerError);
        return;
    }

    std::string content;
    struct dirent* entry;

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
        content += "<td><a href=\"" + uri + name + (isDir ? "/" : "") + "\">"
                   + name + (isDir ? "/" : "") + "</a></td>";
        content += "<td>" + std::string(dateBuf) + "</td>";
        content += "<td class=\"size\">" + sizeStr + "</td>";

        if (!isDir)
        {
            std::string encoded = uriEncode(name);
            content += "<td><button onclick=\"deleteFile('" + uri + encoded + "')\">Delete</button></td>";
        }
        else
        {
            content += "<td>-</td>";
        }

        content += "</tr>\n";
    }

    closedir(dir);

    std::string html =
        "<!doctype html>"
        "<html lang=\"en\">"
        "<head>"
        "  <meta charset=\"utf-8\">"
        "  <title>Index of {PATH}</title>"
        "  <style>"
        "    body { font-family: 'Comic Sans MS', Arial, sans-serif; background: #b7d1f8; text-align: center; }"
        "    .wrapper { background: #fff; padding: 20px; margin: 20px auto; width: 900px;"
        "               box-shadow: 0 0 0 4px #000, 0 0 0 8px #ff00ff; }"
        "    table { width: 100%; border-collapse: collapse; margin-top: 20px; background: #fafafa; }"
        "    th, td { border: 2px dashed #999; padding: 8px; }"
        "    th { background: #ffff00; font-weight: bold; }"
        "    tr:nth-child(even) { background: #f0f0f0; }"
        "    a { text-decoration: none; color: #000; font-weight: bold; }"
        "    a:hover { color: #ff00ff; }"
        "    button { background: #ff00ff; color: #fff; border: none; padding: 5px 10px; cursor: pointer; border-radius: 4px; }"
        "    button:hover { background: #cc00cc; }"
        "    hr { border: 0; border-top: 2px dashed #999; margin: 20px 0; }"
        "    address { font-size: 12px; margin-top: 10px; }"
        "  </style>"
        "</head>"
        "<body>"
        "  <div class=\"wrapper\">"
        "    <h1>Index of {PATH}</h1>"
        "    <img src=\"/img/webservinho_logo.png\" alt=\"webservinho logo\" width=\"200\">"
        "    <table>"
        "      <thead>"
        "        <tr><th>Name</th><th>Last Modified</th><th>Size</th><th>Actions</th></tr>"
        "      </thead>"
        "      <tbody>"
        "        {CONTENT}"
        "      </tbody>"
        "    </table>"
        "    <hr>"
        "    <address>{SERVER_INFO}</address>"
        "  </div>"
        ""
        "  <script>"
        "    async function deleteFile(path) {"
        "      if (!confirm('Delete ' + path + ' ?')) return;"
        "      try {"
        "        const res = await fetch(path, { method: 'DELETE' });"
        "        if (res.ok) { alert('Deleted successfully!'); location.reload(); }"
        "        else alert('Failed (' + res.status + ')');"
        "      } catch (e) { alert('Error: ' + e); }"
        "    }"
        "  </script>"
        "</body>"
        "</html>";

    replacePlaceholder(html, "{PATH}", uri);
    replacePlaceholder(html, "{CONTENT}", content);
    replacePlaceholder(html, "{SERVER_INFO}", "WebServinho/1.0");

    res.appendBody(html);
    res.addHeader("Content-Type", "text/html");
    res.addHeader("Content-Length", toString(html.size()));
    res.setStatusCode(ResponseStatus::OK);

    Logger::instance().log(DEBUG, "[Finished] AutoIndexHandler::handle");
}

/**
 * @brief Replaces a single placeholder tag in the HTML template.
 *
 * Only replaces the first occurrence found.
 */
void	AutoIndexHandler::replacePlaceholder(std::string& html, const std::string& tag, const std::string& value)
{
	size_t pos = html.find(tag);
	if (pos != std::string::npos)
		html.replace(pos, tag.size(), value);
}

/**
 * @brief Converts a byte size into a human-readable format (B, KB, MB).
 */
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
