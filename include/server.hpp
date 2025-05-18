#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <map>
#include <filesystem>
#include <fstream>
#include <zlib.h>

#include "utils.hpp"
#include "http.hpp"
#define RCV_BUFFER_SIZE 4096

std::string gzip_compress(std::string str);

class HttpServer {
public:
	HttpServer(int port);
	~HttpServer();

	int Bind();
	int Listen();
	int Run(int argc, char **argv);
	//HttpRequest Parse(const std::string &request);
	HttpRequest Parse(std::string request);
	//void RegisterEndpoint(std::string endpoint);
	void HandleRequest(HttpRequest request, int client_fd, int argc, char **argv);

private:
	struct sockaddr_in server_addr;
	int server_fd;
	int port;
	int connection_backlog;
	// std::vector<struct sockaddr_in> client_addr;
	struct sockaddr_in client_addr;
	// std::vector<int> client_fd;
	int client_fd;
	std::string url;
	//std::vector<std::string> endpoints;
};

#endif