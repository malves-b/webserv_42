#include <dispatcher/Dispatcher.hpp>
#include <dispatcher/Router.hpp>
#include <dispatcher/StaticPageHandler.hpp>
#include <dispatcher/CgiHandler.hpp>
#include <dispatcher/AutoIndexHandler.hpp>
#include <dispatcher/UploadHandler.hpp>
#include <response/ResponseBuilder.hpp>
#include <config/LocationConfig.hpp>
#include <config/ServerConfig.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>

void	Dispatcher::dispatch(ClientConnection& client)
{
	Logger::instance().log(DEBUG, "[Started] Dispatcher::dispatch");

	HttpRequest& req = client.getRequest();
	HttpResponse& res = client.getResponse();

	const ServerConfig& config = client.getServerConfig();
	const LocationConfig& location = config.mathLocation(req.getUri());

	Router::resolve(req, res, config);

	Logger::instance().log(DEBUG,
		"StaticPageHandler::handle Route -> " + toString(req.getRouteType()));
	Logger::instance().log(DEBUG,
		"Dispatcher::dispatch [Path -> " + req.getResolvedPath() + "]");

	switch (req.getRouteType())
	{
		case RouteType::Redirect:
			break ;
		case RouteType::Upload:
			UploadHandler::handle(req, res, location.getUploadPath());
			break ;
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

	ResponseBuilder::build(req, res, config);

	if (req.getMeta().shouldClose()) //TODO
		client._keepAlive = false;
	else
		client._keepAlive = true;

	client.setResponseBuffer(ResponseBuilder::responseWriter(res)); //TODO
	if (res.getHeader("Content-Type") == "text/html")
		Logger::instance().log(DEBUG, "Dispatcher::dispatch response->" + client.getResponseBuffer());
	req.reset();
	res.reset();
	Logger::instance().log(DEBUG, "[Finished] Dispatcher::dispatch");
}
