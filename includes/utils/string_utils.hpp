#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>

/**
 * @file string_utils.hpp
 * @brief Collection of inline string utility functions used across Webservinho.
 *
 * This header provides lightweight helpers for string manipulation,
 * trimming, case conversion, splitting, joining, and validation.
 *
 * All functions are inline for performance and are compatible with C++98.
 */

/// @brief Removes leading whitespace characters from a string.
inline std::string lTrim(const std::string& str)
{
	std::string::size_type start = 0;
	while (start < str.size()
		&& std::isspace(static_cast<unsigned char>(str[start])))
	{
		++start;
	}
	return (str.substr(start));
}

/// @brief Removes trailing whitespace characters from a string.
inline std::string rTrim(const std::string& str)
{
	if (str.empty())
		return (str);

	std::string::size_type end = str.size() - 1;
	while (end != std::string::npos
		&& std::isspace(static_cast<unsigned char>(str[end])))
	{
		if (end == 0)
			return ("");
		--end;
	}
	return (str.substr(0, end + 1));
}

/// @brief Converts a string to lowercase.
inline std::string toLower(const std::string& str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(),
					(int(*)(int)) std::tolower);
	return (result);
}

/// @brief Converts a string to uppercase.
inline std::string toUpper(const std::string& str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(),
					(int(*)(int)) std::toupper);
	return (result);
}

/// @brief Removes leading and trailing whitespace characters from a string.
inline std::string trim(const std::string& str)
{
	return (rTrim(lTrim(str)));
}

/// @brief Splits a string by a given separator substring.
///
/// @param str The input string.
/// @param sep The separator substring.
/// @return A vector containing all tokens.
inline std::vector<std::string> split(const std::string& str, const std::string& sep)
{
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = str.find(sep);

	while (end != std::string::npos)
	{
		tokens.push_back(str.substr(start, end - start));
		start = end + sep.length();
		end = str.find(sep, start);
	}
	tokens.push_back(str.substr(start));
	return (tokens);
}

/// @brief Converts a hexadecimal string into an integer.
///
/// @return The converted value, or -1 if conversion fails.
inline int stringToHex(const std::string& str)
{
	std::istringstream iss(str);
	int value;

	iss >> std::hex >> value;
	if (iss.fail() || !iss.eof())
		return (-1);
	return (value);
}

/// @brief Converts any printable type to a string representation.
template <typename T>
inline std::string toString(T value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

/// @brief Checks whether a string starts with a given prefix.
inline bool startsWith(const std::string& s, const std::string& prefix)
{
	return s.size() >= prefix.size() &&
			std::equal(prefix.begin(), prefix.end(), s.begin());
}

/// @brief Joins two path segments, ensuring correct slash placement.
///
/// @param base Base path.
/// @param sub Subdirectory or file name.
/// @return Combined path with normalized slashes.
inline static std::string joinPaths(const std::string& base, const std::string& sub)
{
	if (base.empty())
		return (sub);
	if (sub.empty())
		return (base);

	if (base[base.size() - 1] == '/' && sub[0] == '/')
		return (base + sub.substr(1));
	else if (base[base.size() - 1] != '/' && sub[0] != '/')
		return (base + '/' + sub);
	else
		return (base + sub);
}

/// @brief Checks if a path string contains parent directory traversal (`..`) sequences.
inline static bool hasParentTraversal(const std::string& s)
{
	if (s.find("/../") != std::string::npos)
		return (true);

	if (s.size() >= 3 && s.substr(0, 3) == "../")
		return (true);

	if (s.size() >= 3 && s.substr(s.size() - 3) == "/..")
		return (true);

	return (false);
}

/// @brief Returns a copy of a string with surrounding spaces and tabs removed.
inline static std::string trim_copy(const std::string& s)
{
	size_t a = 0;
	size_t b = s.size();

	while (a < b && (s[a] == ' ' || s[a] == '\t'))
		a++;
	while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t'))
		b--;

	return (s.substr(a, b - a));
}

/// @brief Extracts the file extension from a path, including the dot.
///
/// @return The extension (e.g., ".html") or an empty string if none.
inline static std::string getFileExtension(const std::string& path)
{
	std::string::size_type dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos)
		return ("");

	std::string::size_type slashPos = path.find_last_of('/');
	if (slashPos != std::string::npos && dotPos < slashPos)
		return ("");

	return (path.substr(dotPos));
}

/// @brief Encodes a string for safe inclusion in a URI (percent-encoding).
///
/// @param s The input string.
/// @return The URI-encoded version of the string.
inline static std::string uriEncode(const std::string& s)
{
	std::ostringstream oss;
	for (size_t i = 0; i < s.size(); ++i)
	{
		unsigned char c = s[i];
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			oss << c;
		else
			oss << '%' << std::uppercase << std::hex << std::setw(2)
				<< std::setfill('0') << int(c);
	}
	return oss.str();
}

#endif // STRING_UTILS_HPP
