# http-server
A simple multithreaded HTTP server made using C++. The server is multithreaded using a simple implementation of thread pools. It handles requests and executes itself using OOP principles. As well as handling multiple concurrent connections, it also supports persistent connections (not closing the client file descriptor for a valid connection until a `Connection: Close` header is given, where it will execute the last request, close the client and free a spot in the thread pool.)

### Building
Building uses Cmake. Simply run the `build.sh` file:
```
$ chmod +x build.sh
$ build.sh
```
### Running
To run the server, simply execute the executable and specify a serving directory to serve the `/files/` endpoint:
```
$ cd build
$ ./server --directory /some/given/directory
```

### Endpoints
The server features multiple endpoints:

- The `/` URL endpoint returns formatted response code 200. An example request using curl would be `curl -v http://localhost:4221`. You would expect to recieve `HTTP/1.1 200 OK\r\n\r\n`.
- The `/echo/*` URL endpoint returns formatted reponse code 200 with a body containg all characters `*`. An example request using curl would be `curl -v http://localhost:4221/echo/apple`. 
- The `/user-agent` URL endpoint returns formatted response code 200 with a body containing the contents of the `User-Agent` header in the request.
- The `/files/*` URL endpoint with the `GET` method returns formatted response code 200 if the specified file exists with a body of the files content, and 404 otherwise. You can set the base directory using the `--directory` argument and giving the path you want as third argument. For example you would run `./server --directory /given/directory`..
- The `/files/*` URL endpoint with the `POST` method creates a file using the content from the request body in the specified place and returns formatted response code 201, or 500 if the file could not be created. Likewise, you can set the directory.

### Compression
The server supports compression. For now the only compression implemented is GZIP. So compress your request, specify the `Accept-Encoding: gzip` header in your request. The response will contain the `Content-Encoding: gzip` header to confirm that it was compressed and have its body content compressed.