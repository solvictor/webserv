#pragma once

#include <map>
#include <string>
#include <vector>

class Location {
private:
	std::string _path;
	std::string _root;
	bool _autoindex;
	std::string _index;
	short _methods;
	std::string _return;
	std::string _alias;
	std::vector<std::string> _cgi_path;
	std::vector<std::string> _cgi_ext;
	unsigned long _client_max_body_size;

public:
	std::map<std::string, std::string> _ext_path;

	Location();
	Location(const Location& source);
	Location& operator=(const Location& source);
	~Location();

	void setPath(std::string& parameter);
	void setRootLocation(std::string parameter);
	void setAutoindex(std::string& parameter);
	void setIndexLocation(std::string& parameter);
	void setReturn(std::string& parameter);
	void setAlias(std::string& parameter);
	void setCgiPath(std::vector<std::string>& path);
	void setCgiExtension(std::vector<std::string>& extension);
	void setMaxBodySize(std::string& parameter);
	void setMaxBodySize(unsigned long& parameter);
	void enableMethod(std::string& method);

	const std::string& getPath() const;
	const std::string& getRootLocation() const;
	const short& getMethods() const;
	const bool& getAutoindex() const;
	const std::string& getIndexLocation() const;
	const std::string& getReturn() const;
	const std::string& getAlias() const;
	const std::vector<std::string>& getCgiPath() const;
	const std::vector<std::string>& getCgiExtension() const;
	const std::map<std::string, std::string>& getExtensionPath() const;
	const unsigned long& getMaxBodySize() const;

	std::string getPrintMethods() const;
};