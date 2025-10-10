#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

class LocationConfig
{
	private:
		std::string							_path;
		std::string							_root; //A location block root can override ServerRoot for a specific path
		bool								_autoIndex;
		std::vector<std::string>			_methods; //can override server-level defaults
		std::string							_redirection;
		std::string							_index;
		bool								_uploadEnabled;
		std::string							_uploadPath;
		std::map<std::string, std::string>	_cgi;
	public:
		//accessors
};

#endif //LOCATIONCONFIG_HPP