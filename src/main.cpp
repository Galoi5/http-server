#include "server.hpp"
#include <iostream>

int main(int argc, char **argv)
{
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	HttpServer server(PORT);

	server.Bind();
	server.Listen();
	server.Run(argc, argv);

	return (EXIT_SUCCESS);
}
