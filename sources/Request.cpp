#include "../includes/Request.hpp"
#include "../includes/Logger.hpp"
#include "../includes/Webserv.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdint.h>

Request::Request() {
	_path = "";
	_query = "";
	_fragment = "";
	_body_str = "";
	_error_code = 0;
	_chunk_length = 0;
	_method = NONE;
	_method_index = 1;
	_state = Request_Line;
	_fields_done_flag = false;
	_body_flag = false;
	_body_done_flag = false;
	_chunked_flag = false;
	_body_length = 0;
	_storage = "";
	_key_storage = "";
	_multiform_flag = false;
	_boundary = "";
}

Request::~Request() {
	_path.clear();
	_query.clear();
	_fragment.clear();
	_request_headers.clear();
	_body.clear();
	_boundary.clear();
	_storage.clear();
	_key_storage.clear();
	_server_name.clear();
	_body_str.clear();
}

Request::Request(const Request& source) {
	if (this == &source)
		return;

	*this = source;
}

Request& Request::operator=(const Request& source) {
	if (this == &source)
		return *this;

	_path = source._path;
	_query = source._query;
	_fragment = source._fragment;
	_request_headers = source._request_headers;
	_body = source._body;
	_boundary = source._boundary;
	_method = source._method;
	_state = source._state;
	_max_body_size = source._max_body_size;
	_body_length = source._body_length;
	_error_code = source._error_code;
	_chunk_length = source._chunk_length;
	_storage = source._storage;
	_key_storage = source._key_storage;
	_method_index = source._method_index;
	_ver_major = source._ver_major;
	_ver_minor = source._ver_minor;
	_server_name = source._server_name;
	_body_str = source._body_str;
	_fields_done_flag = source._fields_done_flag;
	_body_flag = source._body_flag;
	_body_done_flag = source._body_done_flag;
	_complete_flag = source._complete_flag;
	_chunked_flag = source._chunked_flag;
	_multiform_flag = source._multiform_flag;
	return *this;
}

bool checkUriPos(std::string path) {
	std::stringstream stream(path);
	std::string token;
	int pos = 0;

	while (getline(stream, token, '/')) {
		if (token == "..")
			pos--;
		else
			pos++;
		if (pos < 0)
			return true;
	}
	return false;
}

bool allowedCharURI(uint8_t ch) {
	return isalnum(ch) || (ch >= '#' && ch <= ';') ||
		   (ch >= '?' && ch <= '[') || ch == '!' || ch == '=' || ch == ']' ||
		   ch == '_' || ch == '~';
}

bool isToken(uint8_t ch) {
	return ch == '!' || (ch >= '#' && ch <= '\'') || ch == '*' || ch == '+' ||
		   ch == '-' || ch == '.' || (ch >= '^' && ch <= '`') || ch == '|' ||
		   isalnum(ch);
}

void trimStr(std::string& str) {
	static const char* spaces = " \t";
	str.erase(0, str.find_first_not_of(spaces));
	str.erase(str.find_last_not_of(spaces) + 1);
}

void Request::feed(char* data, size_t size) {
	u_int8_t character;
	static std::stringstream s;

	for (size_t i = 0; i < size; ++i) {
		character = data[i];
		switch (_state) {
		case Request_Line: {
			if (character == 'G')
				_method = GET;
			else if (character == 'P') {
				_state = Request_Line_Post_Put;
				break;
			} else if (character == 'D')
				_method = DELETE;
			else if (character == 'H')
				_method = HEAD;
			else {
				_error_code = 501;
				Logger::log(RESET, false,
							"Method Error Request_Line and Character is = %c",
							character);
				return;
			}
			_state = Request_Line_Method;
			break;
		}
		case Request_Line_Post_Put: {
			if (character == 'O')
				_method = POST;
			else if (character == 'U')
				_method = PUT;
			else {
				_error_code = 501;
				Logger::log(RESET, false,
							"Method Error Request_Line and Character is = %c",
							character);
				return;
			}
			_method_index++;
			_state = Request_Line_Method;
			break;
		}
		case Request_Line_Method: {
			if (character == methodToString(_method)[_method_index])
				_method_index++;
			else {
				_error_code = 501;
				Logger::log(RESET, false,
							"Method Error Request_Line and Character is = %c",
							character);
				return;
			}

			if ((size_t)_method_index == methodToString(_method).length())
				_state = Request_Line_First_Space;
			break;
		}
		case Request_Line_First_Space: {
			if (character != ' ') {
				_error_code = 400;
				Logger::log(RESET, false,
							"Bad Character (Request_Line_First_Space)");
				return;
			}
			_state = Request_Line_URI_Path_Slash;
			continue;
		}
		case Request_Line_URI_Path_Slash: {
			if (character == '/') {
				_state = Request_Line_URI_Path;
				_storage.clear();
			} else {
				_error_code = 400;
				Logger::log(RESET, false,
							"Bad Character (Request_Line_URI_Path_Slash)");
				return;
			}
			break;
		}
		case Request_Line_URI_Path: {
			if (character == ' ') {
				_state = Request_Line_Ver;
				_path.append(_storage);
				_storage.clear();
				continue;
			}
			if (character == '?') {
				_state = Request_Line_URI_Query;
				_path.append(_storage);
				_storage.clear();
				continue;
			}
			if (character == '#') {
				_state = Request_Line_URI_Fragment;
				_path.append(_storage);
				_storage.clear();
				continue;
			}
			if (!allowedCharURI(character)) {
				_error_code = 400;
				Logger::log(RESET, false,
							"Bad Character (Request_Line_URI_Path)");
				return;
			}
			if (i > MAX_URI_LENGTH) {
				_error_code = 414;
				Logger::log(RESET, false,
							"URI Too Long (Request_Line_URI_Path)");
				return;
			}
			break;
		}
		case Request_Line_URI_Query: {
			if (character == ' ') {
				_state = Request_Line_Ver;
				_query.append(_storage);
				_storage.clear();
				continue;
			}
			if (character == '#') {
				_state = Request_Line_URI_Fragment;
				_query.append(_storage);
				_storage.clear();
				continue;
			}
			if (!allowedCharURI(character)) {
				_error_code = 400;
				Logger::log(RESET, false,
							"Bad Character (Request_Line_URI_Query)");
				return;
			}
			if (i > MAX_URI_LENGTH) {
				_error_code = 414;
				Logger::log(RESET, false,
							"URI Too Long (Request_Line_URI_Path)");
				return;
			}
			break;
		}
		case Request_Line_URI_Fragment: {
			if (character == ' ') {
				_state = Request_Line_Ver;
				_fragment.append(_storage);
				_storage.clear();
				continue;
			}
			if (!allowedCharURI(character)) {
				_error_code = 400;
				Logger::log(RESET, false,
							"Bad Character (Request_Line_URI_Fragment)");
				return;
			}
			if (i > MAX_URI_LENGTH) {
				_error_code = 414;
				Logger::log(RESET, false,
							"URI Too Long (Request_Line_URI_Path)");
				return;
			}
			break;
		}
		case Request_Line_Ver: {
			if (checkUriPos(_path)) {
				_error_code = 400;
				Logger::log(RESET, false,
							"Request URI ERROR: goes before root !!");
				return;
			}
			if (character != 'H') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_Ver)");
				return;
			}
			_state = Request_Line_HT;
			break;
		}
		case Request_Line_HT: {
			if (character != 'T') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_HT)");
				return;
			}
			_state = Request_Line_HTT;
			break;
		}
		case Request_Line_HTT: {
			if (character != 'T') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_HTT)");
				return;
			}
			_state = Request_Line_HTTP;
			break;
		}
		case Request_Line_HTTP: {
			if (character != 'P') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_HTTP)");
				return;
			}
			_state = Request_Line_HTTP_Slash;
			break;
		}
		case Request_Line_HTTP_Slash: {
			if (character != '/') {
				_error_code = 400;
				Logger::log(RESET, false,
							"Bad Character (Request_Line_HTTP_Slash)");
				return;
			}
			_state = Request_Line_Major;
			break;
		}
		case Request_Line_Major: {
			if (!isdigit(character)) {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_Major)");
				return;
			}
			_ver_major = character;
			_state = Request_Line_Dot;
			break;
		}
		case Request_Line_Dot: {
			if (character != '.') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_Dot)");
				return;
			}
			_state = Request_Line_Minor;
			break;
		}
		case Request_Line_Minor: {
			if (!isdigit(character)) {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_Minor)");
				return;
			}
			_ver_minor = character;
			_state = Request_Line_CR;
			break;
		}
		case Request_Line_CR: {
			if (character != '\r') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_CR)");
				return;
			}
			_state = Request_Line_LF;
			break;
		}
		case Request_Line_LF: {
			if (character != '\n') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Request_Line_LF)");
				return;
			}
			_state = Field_Name_Start;
			_storage.clear();
			continue;
		}
		case Field_Name_Start: {
			if (character == '\r')
				_state = Fields_End;
			else if (isToken(character))
				_state = Field_Name;
			else {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Field_Name_Start)");
				return;
			}
			break;
		}
		case Fields_End: {
			if (character == '\n') {
				_storage.clear();
				_fields_done_flag = true;
				_handle_headers();
				// if no body then parsing is completed.
				if (_body_flag == 1)
					_state =
						_chunked_flag ? Chunked_Length_Begin : Message_Body;
				else
					_state = Parsing_Done;
				continue;
			} else {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Fields_End)");
				return;
			}
			break;
		}
		case Field_Name: {
			if (character == ':') {
				_key_storage = _storage;
				_storage.clear();
				_state = Field_Value;
				continue;
			}
			if (!isToken(character)) {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Field_Name)");
				return;
			}
			break;
			// if (!character allowed)
			//  error;
		}
		case Field_Value: {
			if (character == '\r') {
				setHeader(_key_storage, _storage);
				_key_storage.clear();
				_storage.clear();
				_state = Field_Value_End;
				continue;
			}
			break;
		}
		case Field_Value_End: {
			if (character == '\n') {
				_state = Field_Name_Start;
				continue;
			} else {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Field_Value_End)");
				return;
			}
			break;
		}
		case Chunked_Length_Begin: {
			if (isxdigit(character) == 0) {
				_error_code = 400;
				Logger::log(RESET, false,
							"Bad Character (Chunked_Length_Begin)");
				return;
			}
			s.str("");
			s.clear();
			s << character;
			s >> std::hex >> _chunk_length;
			if (_chunk_length == 0)
				_state = Chunked_Length_CR;
			else
				_state = Chunked_Length;
			continue;
		}
		case Chunked_Length: {
			if (isxdigit(character) != 0) {
				int temp_len = 0;
				s.str("");
				s.clear();
				s << character;
				s >> std::hex >> temp_len;
				_chunk_length *= 16;
				_chunk_length += temp_len;
			} else if (character == '\r')
				_state = Chunked_Length_LF;
			else
				_state = Chunked_Ignore;
			continue;
		}
		case Chunked_Length_CR: {
			if (character == '\r')
				_state = Chunked_Length_LF;
			else {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Chunked_Length_CR)");
				return;
			}
			continue;
		}
		case Chunked_Length_LF: {
			if (character == '\n')
				_state = _chunk_length == 0 ? Chunked_End_CR : Chunked_Data;
			else {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Chunked_Length_LF)");
				return;
			}
			continue;
		}
		case Chunked_Ignore: {
			if (character == '\r')
				_state = Chunked_Length_LF;
			continue;
		}
		case Chunked_Data: {
			_body.push_back(character);
			--_chunk_length;
			if (_chunk_length == 0)
				_state = Chunked_Data_CR;
			continue;
		}
		case Chunked_Data_CR: {
			if (character == '\r')
				_state = Chunked_Data_LF;
			else {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Chunked_Data_CR)");
				return;
			}
			continue;
		}
		case Chunked_Data_LF: {
			if (character == '\n')
				_state = Chunked_Length_Begin;
			else {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Chunked_Data_LF)");
				return;
			}
			continue;
		}
		case Chunked_End_CR: {
			if (character != '\r') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Chunked_End_CR)");
				return;
			}
			_state = Chunked_End_LF;
			continue;
		}
		case Chunked_End_LF: {
			if (character != '\n') {
				_error_code = 400;
				Logger::log(RESET, false, "Bad Character (Chunked_End_LF)");
				return;
			}
			_body_done_flag = true;
			_state = Parsing_Done;
			continue;
		}
		case Message_Body: {
			if (_body.size() < _body_length)
				_body.push_back(character);
			if (_body.size() == _body_length) {
				_body_done_flag = true;
				_state = Parsing_Done;
			}
			break;
		}
		case Parsing_Done:
			return;
		}
		_storage += character;
	}
	if (_state == Parsing_Done)
		_body_str.append((char*)_body.data(), _body.size());
}

bool Request::parsingCompleted() { return _state == Parsing_Done; }

HttpMethod& Request::getMethod() { return _method; }

std::string& Request::getPath() { return _path; }

std::string& Request::getQuery() { return _query; }

std::string Request::getHeader(std::string const& name) {
	return _request_headers[name];
}

const std::map<std::string, std::string>& Request::getHeaders() const {
	return _request_headers;
}

std::string& Request::getBody() { return _body_str; }

std::string Request::getServerName() { return _server_name; }

bool Request::getMultiformFlag() { return _multiform_flag; }

std::string& Request::getBoundary() { return _boundary; }

void Request::setBody(std::string body) {
	_body.clear();
	_body.insert(_body.begin(), body.begin(), body.end());
	_body_str = body;
}

void Request::setHeader(std::string& name, std::string& value) {
	trimStr(value);
	transform(name.begin(), name.end(), name.begin(), tolower);
	_request_headers[name] = value;
}

void Request::setMaxBodySize(size_t size) { _max_body_size = size; }

void Request::_handle_headers() {
	std::stringstream ss;

	if (_request_headers.count("content-length")) {
		_body_flag = true;
		ss << _request_headers["content-length"];
		ss >> _body_length;
	}
	if (_request_headers.count("transfer-encoding")) {
		if (_request_headers["transfer-encoding"].find_first_of("chunked") !=
			std::string::npos)
			_chunked_flag = true;
		_body_flag = true;
	}
	if (_request_headers.count("host")) {
		size_t pos = _request_headers["host"].find_first_of(':');
		_server_name = _request_headers["host"].substr(0, pos);
	}
	if (_request_headers.count("content-type") &&
		_request_headers["content-type"].find("multipart/form-data") !=
			std::string::npos) {
		size_t pos = _request_headers["content-type"].find("boundary=", 0);
		if (pos != std::string::npos)
			_boundary = _request_headers["content-type"].substr(
				pos + 9, _request_headers["content-type"].size());
		_multiform_flag = true;
	}
}

short Request::errorCode() { return _error_code; }

void Request::clear() {
	_path.clear();
	_error_code = 0;
	_query.clear();
	_fragment.clear();
	_method = NONE;
	_method_index = 1;
	_state = Request_Line;
	_body_length = 0;
	_chunk_length = 0x0;
	_storage.clear();
	_body_str = "";
	_key_storage.clear();
	_request_headers.clear();
	_server_name.clear();
	_body.clear();
	_boundary.clear();
	_fields_done_flag = false;
	_body_flag = false;
	_body_done_flag = false;
	_complete_flag = false;
	_chunked_flag = false;
	_multiform_flag = false;
}

bool Request::keepAlive() {
	return !_request_headers.count("connection") ||
		   _request_headers["connection"].find("close", 0) == std::string::npos;
}
