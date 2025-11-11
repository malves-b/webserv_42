#ifndef ROUTE_TYPE_HPP
#define ROUTE_TYPE_HPP

/**
 * @struct RouteType
 * @brief Defines the different types of routes handled by the Webservinho server.
 *
 * This struct contains an enumeration of all possible route categories
 * that the Router can assign to an HTTP request based on the configuration
 * and resource type.
 *
 * Route types:
 * - StaticPage: Serves a static file or HTML page from disk.
 * - CGI: Executes a CGI script and returns its generated output.
 * - AutoIndex: Generates a dynamic directory listing for a folder.
 * - Upload: Handles file upload requests via POST or PUT methods.
 * - Redirect: Sends an HTTP redirection response (3xx) to the client.
 * - Delete: Processes DELETE requests to remove existing resources.
 * - Error: Represents an invalid, forbidden, or not found route.
 */
struct RouteType
{
	/// @brief Enumerates possible route categories for request handling.
	enum route
	{
		StaticPage = 0, ///< Serves a static file or HTML page.
		CGI,            ///< Executes a CGI script and returns its output.
		AutoIndex,      ///< Generates a dynamic directory listing.
		Upload,         ///< Handles file upload requests.
		Redirect,       ///< Issues an HTTP redirection response.
		Delete,         ///< Processes HTTP DELETE requests to remove resources.
		Error           ///< Represents an error or invalid route configuration.
	};
};

#endif // ROUTE_TYPE_HPP
