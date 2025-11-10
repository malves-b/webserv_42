#include <dispatcher/Dispatcher.hpp>
#include <dispatcher/Router.hpp>
#include <dispatcher/StaticPageHandler.hpp>
#include <dispatcher/CgiHandler.hpp>
#include <dispatcher/AutoIndexHandler.hpp>
#include <dispatcher/UploadHandler.hpp>
#include <dispatcher/DeleteHandler.hpp>
#include <response/ResponseBuilder.hpp>
#include <config/LocationConfig.hpp>
#include <config/ServerConfig.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <utils/Signals.hpp>

void Dispatcher::dispatch(ClientConnection& client)
{
	Logger::instance().log(DEBUG, "[Started] Dispatcher::dispatch");

	HttpRequest& req = client.getRequest();
	HttpResponse& res = client.getResponse();

	const ServerConfig& config = client.getServerConfig();
	const LocationConfig& location = config.matchLocation(req.getUri());

	Router::resolve(req, res, config);

	Logger::instance().log(DEBUG,
		"Dispatcher: RouteType -> " + toString(req.getRouteType()));
	Logger::instance().log(DEBUG,
		"Dispatcher: Resolved path -> " + req.getResolvedPath());

	switch (req.getRouteType())
	{
		case RouteType::Redirect:
			Logger::instance().log(INFO, "Dispatcher: Handling Redirect");
			break ;

		case RouteType::Upload:
			Logger::instance().log(INFO, "Dispatcher: Handling Upload");
			UploadHandler::handle(req, res, location.getUploadPath(), config.getRoot());
			break ;

		case RouteType::StaticPage:
			Logger::instance().log(INFO, "Dispatcher: Handling Static Page");
			StaticPageHandler::handle(req, res);
			break ;

		case RouteType::CGI:
		{
			Logger::instance().log(INFO, "Dispatcher: Handling CGI Execution");

			try
			{
				// inicia CGI não bloqueante
				CgiProcess proc = CgiHandler::startAsync(req, client.getFD());

				client.setCgiActive(true);
				client.setCgiFd(proc.out_fd);
				client.setCgiPid(proc.pid);
				client.setCgiStart(proc.startAt);
				client.cgiBuffer().clear();

				// não construir resposta ainda — o WebServer fará depois
				Logger::instance().log(DEBUG, "Dispatcher: async CGI started");
			}
			catch (const std::exception& e)
			{
				Logger::instance().log(ERROR, "Dispatcher: CGI start failed -> " + std::string(e.what()));
				res.setStatusCode(ResponseStatus::InternalServerError);
			}
			break ;
		}

		case RouteType::AutoIndex:
			Logger::instance().log(INFO, "Dispatcher: Handling AutoIndex");
			AutoIndexHandler::handle(req, res);
			break ;

		case RouteType::Delete:
			Logger::instance().log(INFO, "Dispatcher: Handling Delete");
			DeleteHandler::handle(req, res);
			break ;

		case RouteType::Error:
		default:
			Logger::instance().log(WARNING, "Dispatcher: Handling Error Response");
			break ;
	}

	// 🟡 IMPORTANTE:
	// Só monta resposta se NÃO for CGI assíncrono
	if (!client.hasCgi())
	{
		ResponseBuilder::build(client, req, res);

		if (req.getMeta().shouldClose())
			client.setKeepAlive(false);
		else
			client.setKeepAlive(true);

		client.setResponseBuffer(ResponseBuilder::responseWriter(res));

		if (res.getHeader("Content-Type") == "text/html")
			Logger::instance().log(DEBUG, "Dispatcher: HTML response -> " + client.getResponseBuffer());
	}

	req.reset();
	res.reset();

	Logger::instance().log(DEBUG, "[Finished] Dispatcher::dispatch");
}
