#include "http.hpp"

void HttpResponse::SetStatus(HttpStatus status) { Status = status; }

void HttpResponse::AddHeader(std::string key, std::string value) { Headers.push_back(std::make_pair(key, value)); }

void HttpResponse::SetBody(std::string body) { Body = body; }

int HttpResponse::StatusToCode(HttpStatus status)
{
	switch (status)
	{
	case HttpStatus::OK:
		return 200;
	case HttpStatus::CREATED:
		return 201;
	case HttpStatus::BAD_REQUEST:
		return 400;
	case HttpStatus::NOT_FOUND:
		return 404;
	case HttpStatus::INTERNAL_SERVER_ERROR:
		return 500;
	default:
		return 200;
	}
}

void HttpResponse::Response200() { Status = HttpStatus::OK; }

void HttpResponse::Response201() { Status = HttpStatus::CREATED; }

void HttpResponse::Response404() { Status = HttpStatus::NOT_FOUND; }

void HttpResponse::Response500() { Status = HttpStatus::INTERNAL_SERVER_ERROR; }

std::string HttpResponse::StatusToString(HttpStatus status)
{
	switch (status)
	{
	case HttpStatus::OK:
		return "OK";
	case HttpStatus::NOT_FOUND:
		return "Not Found";
	case HttpStatus::BAD_REQUEST:
		return "Bad Request";
	case HttpStatus::INTERNAL_SERVER_ERROR:
		return "Internal Server Error";
	case HttpStatus::CREATED:
		return "Created";
	default:
		return "OK";
	}
}

std::string HttpResponse::Serialize()
{
	std::string response;
	response += "HTTP/1.1 " + std::to_string(StatusToCode(Status)) + " " + StatusToString(Status) + "\r\n";
	if (Headers.size() > 0)
	{
		for (const auto &header : Headers)
		{
			response += header.first + ": " + header.second + "\r\n";
		}
		response += "\r\n";
	}
	else
	{
		response += "\r\n";
	}
	response += Body;
	return response;
}

std::string HttpResponse::GetBody() { return Body; }

void HttpResponse::RemoveHeader(std::string key)
{
	for (auto it = Headers.begin(); it != Headers.end(); ++it)
	{
		if (it->first == key)
		{
			Headers.erase(it);
		}
	}
}

void HttpResponse::UpdateHeader(std::string key, std::string value)
{
	for (auto it = Headers.begin(); it != Headers.end(); ++it)
	{
		if (it->first == key)
		{
			it->second = value;
			return;
		}
	}
	Headers.push_back(std::make_pair(key, value));
}
