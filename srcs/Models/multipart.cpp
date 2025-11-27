#include "multipart.hpp"

MultiPart::MultiPart():
	_name(""),
	_file_name(""),
	_MIME_type(""),
	_file_data() {}

MultiPart::MultiPart(const MultiPart& other) :
	_name(other._name),
	_file_name(other._file_name),
	_MIME_type(other._MIME_type),
	_file_data(other._file_data) 
	{}

MultiPart& MultiPart::operator=(const MultiPart& other)
{
	if (this != &other)
	{
		_name = other._name;
		_file_name = other._file_name;
		_MIME_type = other._MIME_type;
		_file_data = other._file_data; 
	}
	return (*this);
}

MultiPart::~MultiPart() {}

