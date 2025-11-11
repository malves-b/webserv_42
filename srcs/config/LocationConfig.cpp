#include <config/LocationConfig.hpp>
#include <request/RequestMethod.hpp>

/**
 * @brief Constructs a new LocationConfig for a given path.
 *
 * Initializes defaults:
 * - autoindex disabled
 * - uploads disabled
 * - default allowed method: GET
 */
LocationConfig::LocationConfig(std::string newPath) 
	: _path(newPath),
	_autoindex(false),
	_uploadEnabled(false),
	_hasRoot(false),
	_hasIndexFiles(false),
	_hasAutoIndex(false)
{
	this->_methods.push_back(RequestMethod::GET);
}

/**
 * @brief Copy constructor — clones all configuration attributes.
 */
LocationConfig::LocationConfig(LocationConfig const& src)
	: _path(src._path),
	_root(src._root),
	_indexFiles(src._indexFiles),
	_autoindex(src._autoindex),
	_methods(src._methods),
	_return(src._return),
	_uploadPath(src._uploadPath),
	_uploadEnabled(src._uploadEnabled),
	_cgiPath(src._cgiPath),
	_cgiExtension(src._cgiExtension),
	_hasRoot(src._hasRoot),
	_hasIndexFiles(src._hasIndexFiles),
	_hasAutoIndex(src._hasAutoIndex)
{}

/**
 * @brief Destructor — no dynamic resources to release.
 */
LocationConfig::~LocationConfig(void) {}

/**
 * @return The route path pattern of the location block.
 */
std::string const& LocationConfig::getPath(void) const { return this->_path; }

/**
 * @return The root directory assigned to this location.
 */
std::string const& LocationConfig::getRoot(void) const { return this->_root; }

/**
 * @return The index file (e.g., "index.html").
 */
std::string const& LocationConfig::getIndex(void) const { return this->_indexFiles; }

/**
 * @return True if directory autoindexing is enabled.
 */
bool LocationConfig::getAutoindex(void) const { return this->_autoindex; }

/**
 * @return The list of allowed HTTP methods (GET, POST, DELETE).
 */
std::vector<RequestMethod::Method> const& LocationConfig::getMethods(void) const { return this->_methods; }

/**
 * @return HTTP redirect rule as (status_code, target_url).
 */
std::pair<int, std::string> const& LocationConfig::getReturn(void) const { return this->_return; }

/**
 * @return Directory path where uploaded files will be stored.
 */
std::string const& LocationConfig::getUploadPath(void) const { return this->_uploadPath; }

/**
 * @return True if uploads are enabled for this location.
 */
bool LocationConfig::getUploadEnabled(void) const { return this->_uploadEnabled; }

/**
 * @return Path to the CGI executable handler.
 */
std::string const& LocationConfig::getCgiPath(void) const { return this->_cgiPath; }

/**
 * @return Map of CGI file extensions and their associated interpreters.
 */
std::map<std::string, std::string> const& LocationConfig::getCgiExtension(void) const { return this->_cgiExtension; }

/**
 * @return True if a root directive is explicitly set.
 */
bool LocationConfig::getHasRoot(void) const { return this->_hasRoot; }

/**
 * @return True if an index file is explicitly set.
 */
bool LocationConfig::getHasIndexFiles(void) const { return this->_hasIndexFiles; }

/**
 * @return True if autoindex directive was explicitly set.
 */
bool LocationConfig::getHasAutoIndex(void) const { return this->_hasAutoIndex; }

/**
 * @brief Sets the root directory for this location.
 */
void LocationConfig::setRoot(std::string newRoot)
{
	this->_root = newRoot;
	this->_hasRoot = true;
}

/**
 * @brief Defines the default index file for this location.
 */
void LocationConfig::setIndex(std::string newIndex)
{
	this->_indexFiles = newIndex;
	this->_hasIndexFiles = true;
}

/**
 * @brief Enables or disables directory listing (autoindex).
 */
void LocationConfig::setAutoindex(bool newAutoindex)
{
	this->_autoindex = newAutoindex;
	this->_hasAutoIndex = true;
}

/**
 * @brief Sets the list of allowed HTTP methods.
 */
void LocationConfig::setMethods(std::vector<RequestMethod::Method> newMethods)
{
	this->_methods = newMethods;
}

/**
 * @brief Sets the HTTP redirection rule (status, URL).
 */
void LocationConfig::setReturn(std::pair<int, std::string> newReturn)
{
	this->_return = newReturn;
}

/**
 * @brief Defines the upload directory for incoming files.
 */
void LocationConfig::setUploadPath(std::string newPath)
{
	this->_uploadPath = newPath;
}

/**
 * @brief Enables or disables file upload for this location.
 */
void LocationConfig::setUploadEnabled(bool enabled)
{
	this->_uploadEnabled = enabled;
}

/**
 * @brief Sets the path to the CGI executable.
 */
void LocationConfig::setCgiPath(std::string newPath)
{
	this->_cgiPath = newPath;
}

/**
 * @brief Adds an extension-handler pair for CGI execution.
 *
 * Example: `addCgiExtension(".py", "/usr/bin/python3");`
 */
void LocationConfig::addCgiExtension(std::string const& ext, std::string const& handler)
{
	this->_cgiExtension[ext] = handler;
}
