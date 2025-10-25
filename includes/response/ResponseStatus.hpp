#ifndef RESPONSE_STATUS_HPP
#define RESPONSE_STATUS_HPP

/**
 * @struct ResponseStatus
 * @brief Contains standard HTTP status codes grouped by category.
 *
 * This struct defines an enumeration of commonly used HTTP response
 * status codes, based on RFC 2616 and related specifications.
 *
 * Categories:
 * - 1xx Informational: Request received, continuing process.
 * - 2xx Successful: Request successfully received and understood.
 * - 3xx Redirection: Further action must be taken to complete the request.
 * - 4xx Client Error: The request contains bad syntax or cannot be fulfilled.
 * - 5xx Server Error: The server failed to fulfill a valid request.
 */
struct ResponseStatus
{
	enum code
	{
		/* Informational - 1xx */
		Continue = 100,
		SwitchingProtocols = 101,

		/* Successful - 2xx */
		OK = 200,
		Created = 201,
		Accepted = 202,
		NoContent = 204,
		PartialContent = 206,

		/* Redirection - 3xx */
		MovedPermanently = 301,
		Found = 302,
		SeeOther = 303,
		NotModified = 304,
		TemporaryRedirect = 307,

		/* Client Error - 4xx */
		BadRequest = 400,
		Unauthorized = 401,
		Forbidden = 403,
		NotFound = 404,
		MethodNotAllowed = 405,
		RequestTimeout = 408,
		Conflict = 409,
		Gone = 410,
		PayloadTooLarge = 413,
		UriTooLong = 414,
		UnsupportedMediaType = 415,
		ExpectationFailed = 417,

		/* Server Error - 5xx */
		InternalServerError = 500,
		NotImplemented = 501,
		BadGateway = 502,
		ServiceUnavailable = 503,
		GatewayTimeout = 504,
		HttpVersionNotSupported = 505
	};
};

#endif //RESPONSE_STATUS_HPP