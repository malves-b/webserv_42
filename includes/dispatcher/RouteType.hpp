#ifndef ROUTE_TYPE_HPP
#define ROUTE_TYPE_HPP

/**
 * @struct RouteType
 * @brief Represents the routing type determined after request parsing.
 *
 * This struct defines the possible destinations or handling strategies
 * for an HTTP request once it has been parsed and resolved.
 *
 * Routes:
 * - StaticPage: The request targets a static file (e.g., HTML, CSS, image).
 * - CGI: The request targets a CGI script for dynamic content generation.
 * - Error: The request could not be resolved properly (e.g., invalid path).
 */
struct RouteType
{
	enum route
	{
		StaticPage = 0,
		CGI,
		AutoIndex,
		Error
	};
};

#endif //ROUTE_TYPE_HPP