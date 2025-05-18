#include "server.hpp"
#include "threadpool.hpp"

std::string gzip_compress(std::string str)
{
	std::string buffer;
	const size_t BUFSIZE = 4096;
	char tmp[BUFSIZE];
	/* zlib reinterpret function requires input to be unsigned char */
	unsigned char data_in[BUFSIZE];
	std::copy(str.begin(), str.end(), data_in);
	z_stream zs;
	memset(&zs, 0, sizeof(zs));
	zs.next_in	= reinterpret_cast<uint8_t *>(data_in);
	zs.avail_in = (size_t)str.length();
	deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
	int ret;
	std::string output;
	do
	{
		zs.next_out	 = reinterpret_cast<Bytef *>(tmp);
		zs.avail_out = sizeof(tmp);
		ret			 = deflate(&zs, Z_FINISH);
		if (output.size() < zs.total_out) output.append(tmp, zs.total_out - output.size());
	} while (ret == Z_OK);
	deflateEnd(&zs);
	return output;
}

std::vector<std::string> split(const char *str, char c = ' ')
{
	std::vector<std::string> result;

	do
	{
		const char *begin = str;

		while (*str != c && *str)
			str++;

		result.push_back(std::string(begin, str));
	} while (0 != *str++);

	return result;
}

HttpServer::HttpServer(int port)
{
	this->port = port;
	server_fd  = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		std::cerr << "Failed to create server socket\n";
		throw std::runtime_error("Failed to create server socket");
		return;
	}
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		std::cerr << "setsockopt failed\n";
		throw std::runtime_error("setsockopt failed");
		return;
	}
	server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port		= htons(port);
}

HttpServer::~HttpServer()
{
	close(server_fd);
	std::cout << "Server socket closed\n";
}

int HttpServer::Bind()
{
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
	{
		std::cerr << "Failed to bind to port " << std::to_string(port) << "\n";
		return 1;
	}
	return 0;
}

int HttpServer::Listen()
{
	if (listen(server_fd, connection_backlog) != 0)
	{
		std::cerr << "listen failed\n";
		return 1;
	}
	return 0;
}

HttpRequest HttpServer::Parse(std::string request)
{
	HttpRequest http_request;
	std::stringstream ss(request);
	std::string line;

	// get request method, url and http version
	std::getline(ss, line);
	std::vector<std::string> tokens = split(line.c_str());
	if (tokens[0] == "GET")
		http_request.Method = HttpMethod::GET;
	else if (tokens[0] == "POST")
		http_request.Method = HttpMethod::POST;
	else if (tokens[0] == "PUT")
		http_request.Method = HttpMethod::PUT;
	else if (tokens[0] == "DELETE")
		http_request.Method = HttpMethod::DELETE;
	else
		throw std::runtime_error("Invalid request method");
	http_request.RequestTarget = tokens[1];
	http_request.HttpVersion   = tokens[2];

	if (DEBUG)
	{
		std::cout << "Request method: " << static_cast<int>(http_request.Method) << std::endl;
		std::cout << "Request target: " << http_request.RequestTarget << std::endl;
		std::cout << "HTTP version: " << http_request.HttpVersion << std::endl;
	}

	// Parse headers
	while (std::getline(ss, line))
	{
		// check for empty line or line containing only \r\n (CRLF)
		if (line.empty() || (line.length() == 1 && line[0] == '\r')) break;
		std::vector<std::string> tokens = split(line.c_str(), ':');
		if (tokens.size() >= 2)
		{
			// remove leading space from the value
			std::string value = tokens[1];
			if (!value.empty() && value[0] == ' ')
			{
				value = value.substr(1);
			}
			// remove trailing \r if present
			if (!value.empty() && value.back() == '\r')
			{
				value.pop_back();
			}
			http_request.Headers.push_back({ tokens[0], value });
		}
	}

	// Read the body (POST or PUT methods)
	if (http_request.Method == HttpMethod::POST || http_request.Method == HttpMethod::PUT)
	{
		// Look for Content-Length header
		size_t content_length = 0;
		for (const auto &header : http_request.Headers)
		{
			if (header.first == "Content-Length")
			{
				content_length = std::stoul(header.second);
				break;
			}
		}

		if (content_length > 0)
		{
			// The body starts after the empty line following headers
			// Everything remaining in the stream is part of the body
			std::stringstream body_stream;
			while (std::getline(ss, line))
			{
				// Add back the newline that getline removes
				body_stream << line;
				if (!ss.eof()) body_stream << '\n';
			}
			http_request.Body = body_stream.str();

			// Limit body to content_length
			if (http_request.Body.size() > content_length)
			{
				http_request.Body = http_request.Body.substr(0, content_length);
			}

			if (DEBUG)
			{
				std::cout << "Request body size: " << http_request.Body.size() << std::endl;
				std::cout << "Request body: " << http_request.Body << std::endl;
			}
		}
	}

	// print headers if debug is enabled
	if (DEBUG)
	{
		for (const auto &header : http_request.Headers)
		{
			std::cout << header.first << ": " << header.second << std::endl;
		}
		std::cout << "-----------------------------------\n";
	}

	return http_request;
}

void HttpServer::HandleRequest(HttpRequest request, int client_fd, int argc, char **argv)
{
	HttpResponse http_response;

	bool is_files_request
		= (request.RequestTarget.length() >= 7) && (request.RequestTarget.compare(0, 7, "/files/") == 0);
	// Check if the string starts with "/files/" using a more reliable method
	std::cout << "Is files request: " << (is_files_request ? "true" : "false") << std::endl;
	// handle arguments (--directory)
	std::string_view directory {};
	for (int i = 0; i < argc - 1; ++i)
	{
		if (std::string(argv[i]) == "--directory" && i + 1 < argc)
		{
			directory = argv[i + 1];
			break;
		}
	}

	// check for content encoding
	for (const auto &header : request.Headers)
	{
		if (header.first == "Accept-Encoding")
		{
			std::vector<std::string> encodings = split(header.second.c_str(), ',');
			request.Compression				   = CompressionType::NONE;
			for (auto &encoding : encodings)
			{
				if (!encoding.empty() && encoding[0] == ' ')
				{
					encoding = encoding.substr(1);
				}

				if (DEBUG)
				{
					std::cout << "Encoding: '" << encoding << "'" << std::endl;
				}

				if (encoding == "gzip")
				{
					request.Compression = CompressionType::GZIP;
					http_response.AddHeader("Content-Encoding", "gzip");
					break;
				}
			}
		}
	}

	// check if the client sent a close request and add it to the response
	for (const auto &header : request.Headers)
	{
		if (header.first == "Connection" && header.second == "close") 
		{
			http_response.AddHeader("Connection", "close");
		}
	}

	// handle all requests
	if (request.RequestTarget == "/")
	{
		http_response.Response200();
	}
	else if (request.RequestTarget.substr(0, 6) == "/echo/")
	{
		std::string content = request.RequestTarget.substr(6);
		http_response.Response200();
		http_response.AddHeader("Content-Type", "text/plain");
		http_response.AddHeader("Content-Length", std::to_string(content.length()));
		http_response.SetBody(content);
	}
	else if (request.RequestTarget == "/user-agent")
	{
		int i = 0;
		while (request.Headers[i].first != "User-Agent")
			i++;
		std::string content = request.Headers[i].second;
		if (DEBUG)
		{
			std::cout << "User-Agent: " << content << std::endl;
			std::cout << "length: " << content.length() << std::endl;
		}
		http_response.Response200();
		http_response.AddHeader("Content-Type", "text/plain");
		http_response.AddHeader("Content-Length", std::to_string(content.length()));
		http_response.SetBody(content);
	}
	else if (is_files_request)
	{
		std::filesystem::path filename = request.RequestTarget.substr(7);
		std::filesystem::path path	   = directory / filename;
		if (DEBUG)
		{
			std::cout << "Filename: " << filename << std::endl;
			std::cout << "Path: " << path << std::endl;
		}
		if (request.Method == HttpMethod::GET)
		{
			if (std::filesystem::exists(path))
			{
				if (DEBUG)
				{
					std::cout << "Path exists: " << path << std::endl;
				}
				std::ifstream file(path, std::ios::binary | std::ios::in);
				if (!file)
				{
					if (DEBUG)
					{
						std::cerr << "Failed to open file: " << path << std::endl;
					}
					http_response.Response404();
				}
				else
				{
					if (DEBUG)
					{
						std::cout << "File found: " << filename << std::endl;
					}
					std::string contents {};
					file.seekg(0, std::ios::end);
					contents.resize(file.tellg());
					file.seekg(0, std::ios::beg);
					file.read(&contents[0], contents.size());
					http_response.Response200();
					http_response.AddHeader("Content-Type", "application/octet-stream");
					http_response.AddHeader("Content-Length", std::to_string(contents.length()));
					http_response.SetBody(contents);
				}
			}
			else
			{
				if (DEBUG)
				{
					std::cout << "Path does not exist: " << path << std::endl;
				}
				http_response.Response404();
			}
		}
		else if (request.Method == HttpMethod::POST)
		{
			std::filesystem::create_directories(path.parent_path());
			std::ofstream file(path, std::ios::binary);
			if (!file)
			{
				if (DEBUG)
				{
					std::cout << "Failed to create file: " << path << std::endl;
				}
				http_response.Response500();
			}
			else
			{
				if (DEBUG)
				{
					std::cout << "Writing body to file: " << path << std::endl;
				}
				file.write(request.Body.data(), request.Body.size());
				file.close();
				http_response.Response201();
			}
		}
	}
	else
		http_response.Response404();

	if (request.Compression == CompressionType::GZIP)
	{
		std::string compressed_body = gzip_compress(http_response.GetBody());
		http_response.SetBody(compressed_body);
		if (DEBUG)
		{
			std::cout << "Compressed body: " << std::hex << compressed_body << std::endl;
		}
		http_response.UpdateHeader("Content-Length", std::to_string(compressed_body.length()));
	}

	std::string response = http_response.Serialize();
	ssize_t bytes_sent	 = send(client_fd, response.c_str(), response.length(), 0);
	if (bytes_sent < 0)
	{
		std::cerr << "Failed to send response: " << strerror(errno) << std::endl;
	}
}

int HttpServer::Run(int argc, char **argv)
{
	ThreadPool pool(MAX_THREADS);
	int client_addr_len = sizeof(client_addr);
	std::cout << "Waiting for a client to connect...\n";
	while (true)
	{
		int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
		if (client_fd < 0)
		{
			std::cerr << "Failed to accept client connection\n";
			close(client_fd);
			continue;
		}
		std::cout << "Client connected\n";
		pool.enqueue(
			[this, client_fd, argc, argv]()
			{
				bool keep_alive = true;
				while (keep_alive)
				{
					char buffer[RCV_BUFFER_SIZE];
					ssize_t bytes_rcvd = recv(client_fd, buffer, RCV_BUFFER_SIZE - 1, 0);
					if (bytes_rcvd > 0)
					{
						buffer[bytes_rcvd] = '\0';
						if (DEBUG)
						{
							std::cout << "\n";
							std::cout << "Received data from client\n";
							std::cout << "-----------------------------------\n";
							std::cout << buffer << std::endl;
							std::cout << "-----------------------------------\n";
						}
						HttpRequest http_request = Parse(std::string(buffer));
						if (DEBUG)
						{
							std::cout << "Request target: " << http_request.RequestTarget << std::endl;
							std::cout << "Checking if starts with /files/: "
									  << (http_request.RequestTarget.substr(0, 6) == "/files/") << std::endl;
						}
						std::cout << "Client fd: " << client_fd << std::endl;
						std::cout << "Handling request for client fd: " << client_fd << std::endl;
						HandleRequest(http_request, client_fd, argc, argv);
						// check if the client sent a close request
						for (const auto &header : http_request.Headers)
						{
							if (header.first == "Connection" && header.second == "close") 
							{
								keep_alive = false;
							}
						}
					}
				}
				close(client_fd);
				std::cout << "Closed client fd: " << client_fd << std::endl;
			});
	}
}
