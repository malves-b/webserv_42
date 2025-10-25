#include <fstream>
#include <sstream>
#include <algorithm>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <dispatcher/UploadHandler.hpp>
#include <response/ResponseStatus.hpp>
#include <config/ServerConfig.hpp>

static std::string	trim_copy(const std::string& s)
{
	size_t	a = 0;
	size_t	b = s.size();

	while (a < b && (s[a] == ' ' || s[a] == '\t'))
		a++;
	while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t'))
		b--;

	return (s.substr(a, b - a));
}

void	UploadHandler::handle(HttpRequest& request, HttpResponse& response)
{
	Logger::instance().log(DEBUG, "[Started] UploadHandler::handle");

	const std::string	uploadPath = ServerConfig::instance().upload_path;
	const std::string	contentType = request.getHeader("Content-Type");

	Logger::instance().log(DEBUG, "UploadHandler: Content-Type raw=[" + contentType + "]");

	if (uploadPath.empty())
	{
		Logger::instance().log(ERROR, "UploadHandler: upload path not configured");
		response.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}

	std::string	ctLower = contentType;
	std::transform(ctLower.begin(), ctLower.end(), ctLower.begin(), ::tolower);
	if (ctLower.find("multipart/form-data") == std::string::npos)
	{
		Logger::instance().log(ERROR, "UploadHandler: invalid Content-Type");
		response.setStatusCode(ResponseStatus::BadRequest);
		return ;
	}

	if (!parseMultipart(request.getBody(), contentType, uploadPath))
	{
		response.setStatusCode(ResponseStatus::BadRequest);
		Logger::instance().log(ERROR, "UploadHandler: failed to parse multipart body");
		return ;
	}

	response.setStatusCode(ResponseStatus::Created);
	response.addHeader("Content-Type", "text/html; charset=utf-8");
	response.appendBody("<html><body><h1>Upload successful!</h1></body></html>");
	response.addHeader("Content-Length", toString(response.getBody().size()));

	Logger::instance().log(DEBUG, "[Finished] UploadHandler::handle");
}

std::string	UploadHandler::extractBoundary(const std::string& contentType)
{
	std::string	ctLower = contentType;
	std::transform(ctLower.begin(), ctLower.end(), ctLower.begin(), ::tolower);

	const std::string	key = "boundary=";
	size_t	pos = ctLower.find(key);

	if (pos == std::string::npos)
	{
		return ("");
	}

	std::string	value = trim_copy(contentType.substr(pos + key.length()));
	size_t		semicolon = value.find(';');

	if (semicolon != std::string::npos)
	{
		value = trim_copy(value.substr(0, semicolon));
	}

	if (value.size() >= 2)
	{
		if ((value[0] == '"' && value[value.size() - 1] == '"')
			|| (value[0] == '\'' && value[value.size() - 1] == '\''))
		{
			value = value.substr(1, value.size() - 2);
		}
	}

	return (value);
}

bool	UploadHandler::parseMultipart(const std::string& body,
	const std::string& contentType, const std::string& uploadPath)
{
	std::string	boundaryValue = extractBoundary(contentType);
	if (boundaryValue.empty())
	{
		return (false);
	}

	std::string	delimiter = std::string("--") + boundaryValue;
	std::string	closingDelim = delimiter + "--";

	if (body.compare(0, delimiter.size(), delimiter) != 0)
	{
		return (false);
	}

	size_t	pos = delimiter.size();
	if (pos + 2 <= body.size() && body[pos] == '\r' && body[pos + 1] == '\n')
	{
		pos += 2;
	}

	while (pos < body.size())
	{
		size_t	next = body.find("\r\n" + delimiter, pos);
		if (next == std::string::npos)
		{
			break;
		}

		std::string	part = body.substr(pos, next - pos);
		if (!parsePart(part, uploadPath))
		{
			return (false);
		}

		// avançar até o próximo delimitador
		pos = next + 2 + delimiter.size();

		// se for o delimitador final (--boundary--), sai com sucesso
		if (body.compare(pos, 2, "--") == 0)
		{
			return (true);
		}

		// pular CRLF após o delimitador normal
		if (pos + 2 <= body.size() && body[pos] == '\r' && body[pos + 1] == '\n')
		{
			pos += 2;
		}
	}

	return (true);
}


bool	UploadHandler::parsePart(const std::string& part, const std::string& uploadPath)
{
	const std::string	sep = "\r\n\r\n";
	size_t				hEnd = part.find(sep);

	if (hEnd == std::string::npos)
	{
		return (false);
	}

	std::string	headers = part.substr(0, hEnd);
	std::string	data = part.substr(hEnd + sep.size());

	if (data.size() >= 2 && data[data.size() - 2] == '\r' && data[data.size() - 1] == '\n')
	{
		data.resize(data.size() - 2);
	}

	std::istringstream	hs(headers);
	std::string			line;
	std::string			filename;

	while (std::getline(hs, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
		{
			line.erase(line.size() - 1);
		}
		std::string	lowerLine = line;
		std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

		if (lowerLine.find("content-disposition:") == 0)
		{
			size_t	fn = lowerLine.find("filename=");
			if (fn != std::string::npos)
			{
				std::string	val = line.substr(fn + 9);
				val = trim_copy(val);

				if (!val.empty() && (val[0] == '"' || val[0] == '\''))
				{
					char	q = val[0];
					size_t	endq = val.find(q, 1);
					if (endq != std::string::npos)
					{
						val = val.substr(1, endq - 1);
					}
					else
					{
						val = val.substr(1);
					}
				}

				size_t	slash = val.find_last_of("/\\");
				if (slash != std::string::npos)
				{
					val = val.substr(slash + 1);
				}

				filename = val;
			}
		}
	}

	if (filename.empty())
	{
		return (true);
	}

	saveFile(filename, uploadPath, data);
	return (true);
}

void	UploadHandler::saveFile(const std::string& filename, const std::string& uploadPath, const std::string& data)
{
	std::string	path = uploadPath + "/" + filename;
	std::ofstream	out(path.c_str(), std::ios::binary);

	if (!out.is_open())
	{
		Logger::instance().log(ERROR, "UploadHandler: cannot open file for writing: " + path);
		return ;
	}

	out.write(data.c_str(), data.size());
	out.close();

	Logger::instance().log(DEBUG, "UploadHandler: saved file -> " + path);
}
