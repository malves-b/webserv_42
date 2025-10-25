#ifndef AUTO_INDEX_HANDLER_HPP
#define AUTO_INDEX_HANDLER_HPP

#include <request/HttpRequest.hpp>
#include <response/HttpResponse.hpp>

class AutoIndexHandler
{
public:
    static void handle(HttpRequest& req, HttpResponse& res);
private:
    static std::string getTemplate();
    static std::string formatSize(size_t size);
};

#endif // AUTO_INDEX_HANDLER_HPP