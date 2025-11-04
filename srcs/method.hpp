#ifndef METHOD_H
# define METHOD_H

# include <string>
# include <vector>
# include <map>
# include <iostream>
# include "request.hpp"
# include "response.hpp"
# include "server.hpp"

class Method
{
	private:

	public:

		Method();
		Method(const Method& other);
		Method& operator=(const Method& other);
		~Method();

        //static get
        //static post
        //static delete
};

#endif
