#include "config/LocationConfig.hpp"
#include "request/RequestMethod.hpp"

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

LocationConfig::~LocationConfig(void) {}

// Accessors
std::string const& LocationConfig::getPath(void) const
{
	return this->_path;
}

std::string const& LocationConfig::getRoot(void) const
{
	return this->_root;
}

std::string const& LocationConfig::getIndex(void) const
{
	return this->_indexFiles;
}

bool LocationConfig::getAutoindex(void) const
{
	return this->_autoindex;
}

std::vector<RequestMethod::Method> const& LocationConfig::getMethods(void) const
{
	return this->_methods;
}

std::pair<int, std::string> const& LocationConfig::getReturn(void) const
{
	return this->_return;
}

std::string const& LocationConfig::getUploadPath(void) const
{
	return this->_uploadPath;
}

bool LocationConfig::getUploadEnabled(void) const
{
	return this->_uploadEnabled;
}

std::string const& LocationConfig::getCgiPath(void) const
{
	return this->_cgiPath;
}

std::map<std::string, std::string> const& LocationConfig::getCgiExtension(void) const
{
	return this->_cgiExtension;
}

bool LocationConfig::getHasRoot(void) const
{
	return this->_hasRoot;
}

bool LocationConfig::getHasIndexFiles(void) const
{
	return this->_hasIndexFiles;
}

bool LocationConfig::getHasAutoIndex(void) const
{
	return this->_hasAutoIndex;
}

// Mutators
void LocationConfig::setRoot(std::string newRoot)
{
	this->_root = newRoot;
	this->_hasRoot = true;
}

void LocationConfig::setIndex(std::string newIndex)
{
	this->_indexFiles = newIndex;
	this->_hasIndexFiles = true;
}

void LocationConfig::setAutoindex(bool newAutoindex)
{
	this->_autoindex = newAutoindex;
	this->_hasAutoIndex = true;
}

void LocationConfig::setMethods(std::vector<RequestMethod::Method> newMethods)
{
	this->_methods = newMethods;
}

void LocationConfig::setReturn(std::pair<int, std::string> newReturn)
{
	this->_return = newReturn;
}

void LocationConfig::setUploadPath(std::string newPath)
{
	this->_uploadPath = newPath;
}

void LocationConfig::setUploadEnabled(bool enabled)
{
	this->_uploadEnabled = enabled;
}

void LocationConfig::setCgiPath(std::string newPath)
{
	this->_cgiPath = newPath;
}

void LocationConfig::addCgiExtension(std::string const& ext, std::string const& handler)
{
	this->_cgiExtension[ext] = handler;
}
