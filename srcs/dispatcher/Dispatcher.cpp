#include <dispatcher/Dispatcher.hpp>
#include <dispatcher/Router.hpp>
#include <dispatcher/StaticPageHandler.hpp>
#include <dispatcher/CgiHandler.hpp>
#include <dispatcher/AutoIndexHandler.hpp>
#include <response/ResponseBuilder.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>

void	Dispatcher::dispatch(ClientConnection& client)
{
	Logger::instance().log(DEBUG, "[Started] Dispatcher::dispatch");

	HttpRequest& req = client.getRequest();
	HttpResponse& res = client.getResponse();

	Router::resolve(req, res); //TODO query string

	Logger::instance().log(DEBUG,
		"StaticPageHandler::handle Route -> " + toString(req.getRouteType()));
	Logger::instance().log(DEBUG,
		"Dispatcher::dispatch [Path -> " + req.getResolvedPath() + "]");

	switch (req.getRouteType())
	{
		case RouteType::StaticPage:
			StaticPageHandler::handle(req, res);
			break ;
		case RouteType::CGI:
			CgiHandler::handle(req, res);
			break ;
		case RouteType::AutoIndex:
			AutoIndexHandler::handle(req, res);
			break;
		case RouteType::Error:
		default:
			break ;
	}

	ResponseBuilder::build(req, res);

	client.setResponseBuffer(ResponseBuilder::responseWriter(res)); //TODO
	Logger::instance().log(DEBUG, "Dispatcher::dispatch response->" + client.getResponseBuffer());
	req.reset();
	res.reset();
	Logger::instance().log(DEBUG, "[Finished] Dispatcher::dispatch");
}
