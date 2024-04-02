#include "../includes/StringVector.hpp"
#include <cstring>
#include <netinet/in.h>

StringVector::StringVector() {}

StringVector::StringVector(const std::vector<std::string>& vec) {
	_convert = new char*[vec.size() + 1];
	for (size_t i = 0; i < vec.size(); ++i) {
		_convert[i] = new char[vec[i].length() + 1];
		strcpy(_convert[i], vec[i].c_str());
	}
	_convert[vec.size()] = 0;
}

StringVector::~StringVector() {
	if (_convert) {
		for (size_t i = 0; _convert[i] != 0; ++i) {
			if (_convert[i]) {
				delete[] _convert[i];
			}
		}
		delete[] _convert;
	}
}

StringVector::StringVector(const StringVector& source) {
	if (this == &source)
		return;

	*this = source;
}

StringVector& StringVector::operator=(const StringVector& source) {
	if (this == &source)
		return *this;

	_convert = source._convert;
	return *this;
}

StringVector::operator char**() { return _convert; }