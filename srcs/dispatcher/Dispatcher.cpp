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

/**
 * @brief Central dispatch routine that delegates HTTP requests to the appropriate handler.
 *
 * The Dispatcher acts as the coordinator between Router and the various handlers.
 * It interprets the RouteType defined by Router and calls the corresponding component:
 *  - StaticPageHandler for static content
 *  - CgiHandler for dynamic scripts
 *  - UploadHandler for file uploads
 *  - AutoIndexHandler for directory listings
 *  - DeleteHandler for DELETE requests
 *
 * Additionally, it builds the final HTTP response unless the request triggers
 * an asynchronous CGI process.
 */
void	Dispatcher::dispatch(ClientConnection& client)
{
	Logger::instance().log(DEBUG, "[Started] Dispatcher::dispatch");

	HttpRequest& req = client.getRequest();
	HttpResponse& res = client.getResponse();

	const ServerConfig& config = client.getServerConfig();
	const LocationConfig& location = config.matchLocation(req.getUri());

	// Determine route type based on URI and configuration
	Router::resolve(req, res, config);

	Logger::instance().log(DEBUG,
		"Dispatcher: RouteType -> " + toString(req.getRouteType()));
	Logger::instance().log(DEBUG,
		"Dispatcher: Resolved path -> " + req.getResolvedPath());

	// Dispatch based on the resolved route type
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
				// Starts asynchronous CGI execution
				CgiProcess proc = CgiHandler::startAsync(req, client.getFD());

				client.setCgiActive(true);
				client.setCgiFd(proc.out_fd);
				client.setCgiPid(proc.pid);
				client.setCgiStart(proc.startAt);
				client.cgiBuffer().clear();

				// Response will be built later by the WebServer main loop
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

	// Build and queue response only for non-CGI routes.
	if (!client.hasCgi())
	{
		ResponseBuilder::build(client, req, res);

		// Manage connection persistence (Keep-Alive)
		if (req.getMeta().shouldClose())
			client.setKeepAlive(false);
		else
			client.setKeepAlive(true);

		// Serialize full HTTP response into buffer
		client.setResponseBuffer(ResponseBuilder::responseWriter(res));

		// Optional debug log for HTML responses
		if (res.getHeader("Content-Type") == "text/html")
			Logger::instance().log(DEBUG, "Dispatcher: HTML response -> " + client.getResponseBuffer());
	}

	// Reset request/response for next cycle
	req.reset();
	res.reset();

	Logger::instance().log(DEBUG, "[Finished] Dispatcher::dispatch");
}
