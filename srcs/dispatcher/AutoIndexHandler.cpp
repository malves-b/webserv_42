#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <dispatcher/AutoIndexHandler.hpp>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>
#include <response/ResponseBuilder.hpp>

void AutoIndexHandler::handle(HttpRequest& req, HttpResponse& res)
{
    Logger::instance().log(DEBUG, "[Started] AutoIndexHandler::handle");

    std::string resolvedPath = req.getResolvedPath();
    std::string uri = req.getUri();

    if (uri.empty() || uri[uri.size() - 1] != '/')
        uri += '/';

    DIR* dir = opendir(resolvedPath.c_str());
    if (!dir){
        res.setStatusCode(ResponseStatus::InternalServerError);
        return;}

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
            continue;

        bool isDir = S_ISDIR(fileStat.st_mode);
        char dateBuf[100];
        time_t modTime = fileStat.st_mtime;
        struct tm* timeInfo = localtime(&modTime);
        strftime(dateBuf, sizeof(dateBuf), "%d-%b-%Y %H:%M", timeInfo);

        std::string sizeStr = isDir ? "-" : formatSize(fileStat.st_size);

        // Create line of the table
        content += "<tr>";
        content += "<td><a href=\"" + uri + name + (isDir ? "/" : "") + "\">" + name + (isDir ? "/" : "") + "</a></td>";
        content += "<td>" + std::string(dateBuf) + "</td>";
        content += "<td class=\"size\">" + sizeStr + "</td>";
        content += "</tr>\n";
    }
    closedir(dir);

    std::string html = getTemplate();    
    size_t pos;
    pos = html.find("{PATH}");
    if (pos != std::string::npos){
        html.replace(pos, 6, uri);}

	pos = html.find("{PATH}");
    if (pos != std::string::npos)
        html.replace(pos, 6, uri);
        
    pos = html.find("{CONTENT}");
    if (pos != std::string::npos)
        html.replace(pos, 9, content);
        
    pos = html.find("{SERVER_INFO}");
    if (pos != std::string::npos)
        html.replace(pos, 13, "WebServinho/1.0");

    res.appendBody(html);
    res.addHeader("Content-Type", "text/html");
    res.addHeader("Content-Length", toString(html.size()));
    res.setStatusCode(ResponseStatus::OK);

    Logger::instance().log(DEBUG, "[Finished] AutoIndexHandler::handle");
}

std::string AutoIndexHandler::formatSize(size_t size)
{
    std::stringstream ss;
    if (size < 1024)
        ss << size << " B";
    else if (size < 1024 * 1024)
        ss << (size / 1024) << " KB";
    else
        ss << (size / (1024 * 1024)) << " MB";
    return ss.str();
}

std::string AutoIndexHandler::getTemplate()
{
    return "<!DOCTYPE html>\n"
           "<html>\n"
           "<head>\n"
           "<title>Index of {PATH}</title>\n"
           "<style>\n"
           "    body {\n"
           "        font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;\n"
           "        font-size: 13px;\n"
           "        margin: 0;\n"
           "        padding: 20px;\n"
           "        background-color: #f5f5f5;\n"
           "    }\n"
           "    h1 {\n"
           "        color: #333;\n"
           "        font-size: 18px;\n"
           "        margin-bottom: 20px;\n"
           "        padding-bottom: 10px;\n"
           "        border-bottom: 1px solid #ddd;\n"
           "    }\n"
           "    table {\n"
           "        width: 100%;\n"
           "        border-collapse: collapse;\n"
           "        background-color: white;\n"
           "        box-shadow: 0 1px 3px rgba(0,0,0,0.1);\n"
           "    }\n"
           "    th {\n"
           "        text-align: left;\n"
           "        padding: 12px 15px;\n"
           "        background-color: #f8f8f8;\n"
           "        border-bottom: 2px solid #ddd;\n"
           "        font-weight: bold;\n"
           "        color: #555;\n"
           "    }\n"
           "    td {\n"
           "        padding: 10px 15px;\n"
           "        border-bottom: 1px solid #eee;\n"
           "    }\n"
           "    tr:hover {\n"
           "        background-color: #f9f9f9;\n"
           "    }\n"
           "    a {\n"
           "        text-decoration: none;\n"
           "        color: #0366d6;\n"
           "    }\n"
           "    a:hover {\n"
           "        text-decoration: underline;\n"
           "    }\n"
           "    .size {\n"
           "        text-align: right;\n"
           "        font-family: monospace;\n"
           "        color: #666;\n"
           "    }\n"
           "    .date {\n"
           "        color: #666;\n"
           "        font-family: monospace;\n"
           "    }\n"
           "    hr {\n"
           "        border: none;\n"
           "        border-top: 1px solid #ddd;\n"
           "        margin: 20px 0;\n"
           "    }\n"
           "    address {\n"
           "        font-style: normal;\n"
           "        color: #888;\n"
           "        font-size: 11px;\n"
           "    }\n"
           "</style>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Index of {PATH}</h1>\n"
           "<table>\n"
           "    <thead>\n"
           "        <tr>\n"
           "            <th>Name</th>\n"
           "            <th>Last Modified</th>\n"
           "            <th>Size</th>\n"
           "        </tr>\n"
           "    </thead>\n"
           "    <tbody>\n"
           "        {CONTENT}\n"
           "    </tbody>\n"
           "</table>\n"
           "<hr>\n"
           "<address>{SERVER_INFO}</address>\n"
           "</body>\n"
           "</html>";
}