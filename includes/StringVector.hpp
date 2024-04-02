#pragma once

#include <string>
#include <vector>

class StringVector {
private:
	char** _convert;

public:
	StringVector();
	StringVector(const std::vector<std::string>& vec);
	~StringVector();
	StringVector(const StringVector& source);
	StringVector& operator=(const StringVector& source);

	operator char**();
};