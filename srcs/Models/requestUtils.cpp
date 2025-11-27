#include "request.hpp"

int Request::http_requirements_met()
{
	if (_method.empty() || _uri.empty() || _version.empty())
	{
		std::cout << "something is empty, _method = " << _method << "; _uri = " << _uri << ";_version = " << _version << std::endl;
		return 505;
	}
	if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
	{
		std::cout << "version is " << _version << std::endl;
		return 505;
	}
	return 200;
}

bool Request::http_can_have_body()
{
	if (_method != "POST")
		return false;
	return true;
}

void Request::flush_request_data()
{
	_request_data.clear();
	_header.clear();
	_method.clear();
	_uri.clear();
	_version.clear();
	_header_kv.clear();
	_content_length = 0;
	_fullPathURI.clear();
	_root.clear();
	_isDirectory = false;
	memset(&_stat, 0, sizeof(_stat));
	_isCGI = false;
	_body.clear();
	_body_kv.clear();
	_multiparts.clear();
	_body_data.clear();
}

std::vector<std::string> Request::tokenise_url_encoded(std::string &str)
{
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string word;

	std::cout << "Tokenizing....  " << std::endl;

	while (iss >> word)
	{
		std::string current;
		for (size_t i = 0; i < word.size(); ++i)
		{
			std::cout << "current: " << current << std::endl;
			if (word[i] == '&' || word[i] == '=')
			{
				if (!current.empty())
				{
					tokens.push_back(current);
					current.clear();
				}
				tokens.push_back(std::string(1, word[i]));
			}
			else
				current += word[i];
		}
		if (!current.empty())
			tokens.push_back(current);
	}
	return (tokens);
}

int Request::parse_url_encoded()
{
	std::vector<std::string> tokens = tokenise_url_encoded(_body);
	std::cout << "after tokenizer, tokens.size(): " << tokens.size() << std::endl;
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		std::cout << "checking this token: " << tokens[i] << std::endl;
		if (tokens[i] == "=" || tokens[i] == "&")
			return (1);
		std::string key = tokens[i];
		i++;
		if (tokens[i] != "=")
			return (1);
		i++;
		if (tokens[i] == "=" || tokens[i] == "&")
			return (1);
		std::string value = tokens[i];
		_body_kv[key] = value;
		i++;
		if (i == tokens.size())
			return (0);
		if (tokens[i] != "&")
			return (1);
	}
	return (0);
}

std::string Request::trimCRLF(const std::string &s)
{
	size_t start = 0;
	size_t end = s.size();

	while (start < end && (s[start] == '\r' || s[start] == '\n'))
		start++;
	while (end > start && (s[end - 1] == '\r' || s[end - 1] == '\n'))
		end--;

	return s.substr(start, end - start);
}

std::vector<MultiPart> Request::generate_multipart(const std::string &boundary)
{
	std::vector<MultiPart> multipart;

	std::string realBoundary = "--" + boundary;
	std::string endBoundary = realBoundary + "--";

	size_t pos = 0;

	while (true)
	{
		size_t start = _body.find(realBoundary, pos);
		if (start == std::string::npos)
			break;
		start += realBoundary.size();
		if (_body.compare(start, 2, "--") == 0)
			break;
		if (_body.compare(start, 2, "--") == 0)
			break;
		if (_body.compare(start, 2, "\r\n") == 0)
			start += 2;
		size_t headerEnd = _body.find("\r\n\r\n", start);
		if (headerEnd == std::string::npos)
			break;
		std::string headerBlock = _body.substr(start, headerEnd - start);
		size_t contentStart = headerEnd + 4;
		size_t nextBoundary = _body.find(realBoundary, contentStart);
		if (nextBoundary == std::string::npos)
			break;
		size_t contentEnd = nextBoundary;
		std::vector<char> rawContent(_body.begin() + contentStart, _body.begin() + contentEnd);
		if (rawContent.size() >= 2 && rawContent[rawContent.size() - 1] == '\n' && rawContent[rawContent.size() - 2] == '\r')
			rawContent.erase(rawContent.end() - 2, rawContent.end());

		MultiPart part;
		std::stringstream hs(headerBlock);
		std::string line;

		while (std::getline(hs, line))
		{
			line = trimCRLF(line);

			if (line.find("Content-Disposition:") == 0)
			{
				size_t namePos = line.find("name=\"");
				if (namePos != std::string::npos)
				{
					namePos += 6;
					size_t endPos = line.find("\"", namePos);
					part.set_name(line.substr(namePos, endPos - namePos));
					std::cout << "part name = " << part.get_name() << std::endl;
				}

				size_t filePos = line.find("filename=\"");
				if (filePos != std::string::npos)
				{
					filePos += 10;
					size_t endPos = line.find("\"", filePos);
					part.set_file_name(line.substr(filePos, endPos - filePos));
					std::cout << "file name = " << part.get_file_name() << std::endl;
				}
			}
			else if (line.find("Content-Type:") == 0)
			{
				size_t sep = line.find(":");
				std::string type = line.substr(sep + 1);
				while (!type.empty() && type[0] == ' ')
					type.erase(0, 1);
				part.set_MIME_type(type);
				std::cout << "file type = " << part.get_MIME_type() << std::endl;
			}
		}
		part.set_file_data(rawContent);
		multipart.push_back(part);
		pos = nextBoundary;
	}
	return multipart;
}

std::string Request::find_boundary(std::string content_type)
{
	size_t pos = content_type.find("boundary");
	if (pos == std::string::npos || content_type.size() < pos + 9)
		return ("");
	std::string boundary = content_type.substr(pos + 9);
	size_t sc = boundary.find(';');
	if (sc != std::string::npos)
		boundary = boundary.substr(0, sc);
	while (!boundary.empty() && (boundary[0] == ' ' || boundary[0] == '='))
		boundary.erase(0, 1);
	return (boundary);
}

int Request::parse_multipart(std::string content_type)
{
	std::string boundary = find_boundary(content_type);
	if (boundary.empty())
		return (1);
	_multiparts = generate_multipart(boundary);
	if (_multiparts.empty())
		return (1);
	return (0);
}

std::vector<std::string> Request::tokenise_json(std::string &str)
{
	std::vector<std::string> tokens;
	size_t i = 0;

	while (i < str.size())
	{
		char c = str[i];
		if (isspace(c))
		{
			i++;
			continue;
		}

		if (c == '"')
		{
			std::string quote_token;
			quote_token += c;
			i++;

			while (i < str.size())
			{
				quote_token += str[i];
				if (str[i] == '"' && str[i - 1] != '\\')
				{
					i++;
					break;
				}
				i++;
			}
			tokens.push_back(quote_token);
			continue;
		}

		if (c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',')
		{
			tokens.push_back(std::string(1, c));
			i++;
			continue;
		}

		std::string token;
		while (i < str.size())
		{
			char d = str[i];

			if (isspace(d) || d == '{' || d == '}' || d == '[' || d == ']' || d == ':' || d == ',')
				break;

			token += d;
			i++;
		}

		if (!token.empty())
			tokens.push_back(token);
	}
	return tokens;
}

int Request::parse_json()
{
	std::vector<std::string> tokens = tokenise_json(_body);
	// may need quite a bit more implementation, optional
	return (0);
}

int Request::treat_as_raw_body()
{
	std::vector<char> rawContent(_body.begin(), _body.end());
	if (rawContent.size() >= 2 && rawContent[rawContent.size() - 1] == '\n' && rawContent[rawContent.size() - 2] == '\r')
		rawContent.erase(rawContent.end() - 2, rawContent.end());
	_body_data = rawContent;
	return (0);
}
