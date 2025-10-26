#ifndef UPLOAD_HANDLER_HPP
#define UPLOAD_HANDLER_HPP

#include <string>
#include <request/HttpRequest.hpp>
#include <response/HttpResponse.hpp>

class UploadHandler
{
	private:
		static std::string extractBoundary(const std::string& contentType);
		static bool parseMultipart(const std::string& body, const std::string& contentType, const std::string& uploadPath);
		static bool parsePart(const std::string& part, const std::string& uploadPath);
		static void saveFile(const std::string& filename, const std::string& uploadPath, const std::string& data);

	public:
		static void handle(HttpRequest& request, HttpResponse& response, std::string uploadPath);
};

#endif //UPLOAD_HANDLER_HPP