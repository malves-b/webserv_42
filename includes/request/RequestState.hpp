#ifndef REQUEST_STATE_HPP
#define REQUEST_STATE_HPP

/**
 * @struct RequestState
 * @brief Represents the parsing state of an HTTP request.
 *
 * This struct defines the different stages involved in the
 * parsing of an HTTP request.
 *
 * States:
 * - RequestLine: Currently parsing the request line (method, URI, version).
 * - Headers: Currently parsing the headers.
 * - Body: Currently reading the message body (if present).
 * - Complete: The request has been fully parsed.
 */
struct RequestState
{
	enum state
	{
		RequestLine = 0,
		Headers,
		Body,
		Complete
	};
};

#endif //REQUEST_STATE_HPP