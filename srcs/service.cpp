#include "service.hpp"

Service::Service() {}

Service::Service(const Service& other) 
{
	(void)other;
}

Service& Service::operator=(const Service& other)
{
	(void)other;
	return (*this);
}

Service::~Service() {}
