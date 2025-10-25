#ifndef REQUEST_METHOD_HPP
#define REQUEST_METHOD_HPP

/**
 * @struct RequestMethod
 * @brief Defines supported HTTP request methods.
 *
 * This struct contains an enumeration of HTTP methods,
 * as defined in RFC 2616 and its updates.
 *
 * Supported methods:
 * - OPTIONS: Request for communication options.
 * - GET: Retrieve data from the server.
 * - HEAD: Same as GET but returns only headers.
 * - POST: Submit data to the server (e.g., form submission).
 * - PUT: Upload or replace a resource.
 * - DELETE: Delete a resource.
 * - TRACE: Echo the received request.
 * - CONNECT: Establish a tunnel to the server.
 * - PATCH: Apply partial modifications to a resource.
 * - INVALID: Used to represent an unrecognized or unsupported method.
 */
struct RequestMethod
{
	enum Method
	{
		OPTIONS = 0,
		GET,
		HEAD,
		POST,
		PUT,
		DELETE,
		TRACE,
		CONNECT,
		PATCH,
		INVALID
	};
};

#endif //REQUEST_METHOD_HPP