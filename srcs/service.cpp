#include "service.hpp"

Service::Service():servers() {}

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
