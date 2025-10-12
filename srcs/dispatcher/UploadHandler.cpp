#include <fstream>
#include <sstream>
#include <algorithm>
#include <utils/Logger.hpp>
#include <dispatcher/UploadHandler.hpp>
#include <response/ResponseStatus.hpp>
#include <init/ServerConfig.hpp>

void	UploadHandler::handle(HttpRequest& request, HttpResponse& response)
{
	Logger::instance().log(DEBUG, "[Started] UploadHandler::handle");

	const std::string& contentType = request.getHeader("Content-Type");
	const std::string& uploadPath = ServerConfig::instance().upload_path;

	if (contentType.find("multipart/form-data") == std::string::npos)
	{
		Logger::instance().log(ERROR, "UploadHandler: invalid Content-Type");
		response.setStatusCode(ResponseStatus::BadRequest);
		return;
	}

	if (uploadPath.empty())
	{
		Logger::instance().log(ERROR, "UploadHandler: upload path not configured");
		response.setStatusCode(ResponseStatus::InternalServerError);
		return;
	}

	if (!parseMultipart(request.getBody(), contentType, uploadPath))
	{
		response.setStatusCode(ResponseStatus::BadRequest);
		Logger::instance().log(ERROR, "UploadHandler: failed to parse multipart body");
		return;
	}

	response.setStatusCode(ResponseStatus::Created);
	response.addHeader("Content-Type", "text/html");
	response.appendBody("<html><body><h1>Upload successful!</h1></body></html>");

	Logger::instance().log(DEBUG, "[Finished] UploadHandler::handle");
}

std::string	UploadHandler::extractBoundary(const std::string& contentType)
{
	std::string key = "boundary=";
	size_t pos = contentType.find(key);
	if (pos == std::string::npos)
		return "";
	return ("--" + contentType.substr(pos + key.length()));
}

bool	UploadHandler::parseMultipart(const std::string& body, const std::string& contentType, const std::string& uploadPath)
{
	std::string boundary = extractBoundary(contentType);
	if (boundary.empty())
		return (false);

	size_t pos = 0;
	while (true)
	{
		size_t next = body.find(boundary, pos);
		if (next == std::string::npos)
			break;

		std::string part = body.substr(pos, next - pos);
		if (!parsePart(part, uploadPath))
			return (false);

		pos = next + boundary.size();
	}
	return (true);
}

bool	UploadHandler::parsePart(const std::string& part, const std::string& uploadPath)
{
	std::istringstream stream(part);
	std::string line, filename;
	std::ostringstream fileData;

	// Ler headers da parte
	while (std::getline(stream, line) && line != "\r")
	{
		if (line.find("Content-Disposition:") != std::string::npos)
		{
			size_t fn = line.find("filename=");
			if (fn != std::string::npos)
			{
				filename = line.substr(fn + 9);
				if (!filename.empty() && filename[0] == '\"')
					filename = filename.substr(1, filename.find('\"', 1) - 1);
			}
		}
	}

	// Ler conteúdo do arquivo
	while (std::getline(stream, line))
		fileData << line << "\n";

	if (filename.empty())
		return (true); // parte sem arquivo (ex: campo de formulário)

	saveFile(filename, uploadPath, fileData.str());
	return (true);
}

void	UploadHandler::saveFile(const std::string& filename, const std::string& uploadPath, const std::string& data)
{
	std::string path = uploadPath + "/" + filename;
	std::ofstream out(path.c_str(), std::ios::binary);
	if (!out.is_open())
	{
		Logger::instance().log(ERROR, "UploadHandler: cannot open file for writing: " + path);
		return ;
	}
	out.write(data.c_str(), data.size());
	out.close();

	Logger::instance().log(DEBUG, "UploadHandler: saved file -> " + path);
}
