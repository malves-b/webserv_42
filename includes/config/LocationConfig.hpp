#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

#include <vector>
#include <string>
#include <map>
#include "request/RequestMethod.hpp"

class LocationConfig
{
	private:
		// Core features
		std::string							_path; // e.g. "/images/" or "/cgi-bin/" //not default and necessary from config
		std::string							_root; //if empty, inherits
		std::string							_indexFiles; // inherits if not set
		bool								_autoindex; // inherits if not set
		std::vector<RequestMethod::Method>	_methods; //doesn't inherit, default: GET

		// Optional features
		std::pair<int, std::string>			_return; // e.g. {301, "http://www.example.com/moved/her"} //only one redirect per location
		std::string							_uploadPath; //default: leave empty
		bool								_uploadEnabled; //default: "off"
		std::string							_cgiPath; // Base directory for CGI scripts
		std::map<std::string, std::string>	_cgiExtension; // e.g. {".py": "/usr/bin/python3"}

		// Flags
		bool								_hasRoot;
		bool								_hasIndexFiles;
		bool								_hasAutoIndex;

		LocationConfig&						operator=(LocationConfig const& rhs);
	public:
		LocationConfig(std::string newPath);
		LocationConfig(LocationConfig const& src);
		~LocationConfig(void);

		//accessors
		std::string const&					getPath(void) const;
		std::string const&					getRoot(void) const;
		std::string const&					getIndex(void) const;
		bool								getAutoindex(void) const;
		std::vector<RequestMethod::Method> const&	getMethods(void) const;
		std::pair<int, std::string> const&	getReturn(void) const;
		std::string const&					getUploadPath(void) const;
		bool								getUploadEnabled(void) const;
		std::string const&					getCgiPath(void) const;
		std::map<std::string, std::string> const&	getCgiExtension(void) const; //double check
		bool								getHasRoot(void) const;
		bool								getHasIndexFiles(void) const;
		bool								getHasAutoIndex(void) const;

		//mutators
		void								setRoot(std::string);
		void								setIndex(std::string);
		void								setAutoindex(bool);
		void								setMethods(std::vector<RequestMethod::Method>);
		void								setReturn(std::pair<int, std::string>);
		void								setUploadPath(std::string);
		void								setUploadEnabled(bool);
		void								setCgiPath(std::string);
		void								addCgiExtension(std::string const&, std::string const&);
};

#endif //LOCATIONCONFIG_HPP