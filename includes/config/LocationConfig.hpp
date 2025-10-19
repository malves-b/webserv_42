#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <request/RequestMethod.hpp>

class LocationConfig
{
	private:
		// Core features
		std::string							_path; // e.g. "/images/" or "/cgi-bin/" //not default and necessary from config
		std::string							_root; //if empty, inherits
		std::string							_indexFiles; // inherits if not set
		bool								_autoIndex; // inherits if not set
		std::vector<RequestMethod::Method>	_methods; //doesn't inherit, default: GET

		// Optional features
		std::pair<int, std::string>			_return; // e.g. {301, "http://www.example.com/moved/her"} //only one redirect per location
		std::string							_uploadPath; //default: leave empty
		bool								_uploadEnabled; //default: false
		std::string							_cgiPath; // Base directory for CGI scripts
		std::map<std::string, std::string>	_cgiExtension; // e.g. {".py": "/usr/bin/python3"}

		// Flags
		bool								_hasRoot;
		bool								_hasAutoIndex;
		bool								_hasIndexFiles;

		LocationConfig(LocationConfig const& src);
		LocationConfig&						operator=(LocationConfig const& rhs);
	public:
		LocationConfig(void);
		~LocationConfig(void);

		//accessors

		//mutators
};

#endif //LOCATIONCONFIG_HPP