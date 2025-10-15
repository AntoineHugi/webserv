#ifndef PARSER_H
# define PARSER_H

class Parser
{
	private: 
		Parser();
		Parser(const Parser& other);
		Parser& operator=(const Parser& other);
		~Parser();
	
	public:
		static int	open_config(char *arg, Service* service);
		static int	parse_file(std::ifstream& file, Service* service);
};

#endif