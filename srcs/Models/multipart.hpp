#ifndef MULTIPART_H
#define MULTIPART_H

#include <string>
#include <vector>

class MultiPart
{
	private:
		std::string _name;
		std::string _file_name;
		std::string _MIME_type;
		std::vector<char> _file_data;

	public:
		MultiPart();
		MultiPart(const MultiPart &other);
		MultiPart &operator=(const MultiPart &other);
		~MultiPart();

		std::string get_name() { return _name; };
		std::string get_file_name() { return _file_name; };
		std::string get_MIME_type() { return _MIME_type; };
		std::vector<char> get_file_data() { return _file_data; };

		void	set_name (std::string name) { _name = name; };
		void	set_file_name (std::string file_name) { _file_name = file_name; };
		void	set_MIME_type (std::string type) { _MIME_type = type; };
		void	set_file_data (std::vector<char>& file_data) { _file_data.swap(file_data); };
};

#endif
