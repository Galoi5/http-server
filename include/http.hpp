#ifndef HTTP_HPP
#define HTTP_HPP

#include <string>
#include <vector>

enum class CompressionType
{
	NONE,
	GZIP,
};

enum class HttpStatus
{
	OK					  = 200,
	CREATED				  = 201,
	NOT_FOUND			  = 404,
	BAD_REQUEST			  = 400,
	INTERNAL_SERVER_ERROR = 500,
};

enum class HttpMethod
{
	GET	   = 0,
	POST   = 1,
	PUT	   = 2,
	DELETE = 3,
};

class HttpResponse
{
public:
	void SetStatus(HttpStatus status);
	void AddHeader(std::string key, std::string value);
	void RemoveHeader(std::string key);
	void UpdateHeader(std::string key, std::string value);
	void SetBody(std::string body);
	std::string GetBody();
	std::string Serialize();
	std::string StatusToString(HttpStatus status);
	int StatusToCode(HttpStatus status);
	void Response200();
	void Response201();
	void Response404();
	void Response500();

private:
	HttpStatus Status;
	std::vector<std::pair<std::string, std::string>> Headers;
	std::string Body;
};

class HttpRequest
{
public:
	HttpMethod Method;
	std::string RequestTarget;
	std::string HttpVersion;
	std::vector<std::pair<std::string, std::string>> Headers;
	CompressionType Compression;
	std::string Body;
};

#endif