#include <LocationConfig.hpp>
#include <request/RequestMethod.hpp>

LocationConfig::LocationConfig(void)
{
	// this->_path = "/";
	// this->_root; //inherits if empty, overrides if not empty
	// this->_indexFiles; //inherits if empty, overrides if not empty
	// this->_autoIndex; //inherits if empty, overrides if not empty
	this->_methods.push_back(RequestMethod::GET);
	this->_uploadEnabled = false;
	this->_hasRoot = false;
	this->_hasAutoIndex = false;
	this->_hasIndexFiles = false;
}

LocationConfig::~LocationConfig(void) {}