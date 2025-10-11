#ifndef ROUTE_TYPE_HPP
#define ROUTE_TYPE_HPP

struct RouteType
{
	enum route
	{
		StaticPage = 0,
		CGI,
		Upload,
		Redirect,
		Error
	};
};

#endif //ROUTE_TYPE_HPP