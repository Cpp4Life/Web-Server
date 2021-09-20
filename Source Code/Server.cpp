//Socket header files
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <fstream>
#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080

int main()
{

	//Initialize WinSock
	WSADATA WinSockData;
	WORD ver = MAKEWORD(2, 2); //version 2.2

	long WinSockSuccess = WSAStartup(ver, &WinSockData);
	if (WinSockSuccess != 0)
	{
		std::cout << "Can't initialize winsock! Quitting" << std::endl;
		return 0;
	}

	// Creating socket file descriptor 
	//TCP = SOCK_STREAM
	//UDP = SOCK_DGRAM
	SOCKET server_fd = socket(AF_INET, SOCK_STREAM, NULL);
	if (server_fd == INVALID_SOCKET)
	{
		perror("Can't create a socket! Quitting");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 80 
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR)
	{
		perror("setsockopt");
		std::cout << "Error #" << WSAGetLastError << std::endl;
		exit(EXIT_FAILURE);
	}

	//Bind the IP address and port to a socket
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);	//host to network, short
	address.sin_addr.s_addr = INADDR_ANY; //Could also use inet_pton...

	//Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		perror("bind failed");
		std::cout << "Error #" << WSAGetLastError << std::endl;
		exit(EXIT_FAILURE);
	}

	//Listen client to connect server
	if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR)
	{
		perror("listen");
		std::cout << "Error #" << WSAGetLastError << std::endl;
		exit(EXIT_FAILURE);
	}

	//Client connected to server
	sockaddr_in client;
	int clientSize = sizeof(client);
	int addrSize = sizeof(address);

	SOCKET clientSocket = accept(server_fd, (sockaddr*)&client, (socklen_t*)&clientSize);
	if (clientSocket == INVALID_SOCKET)
	{
		perror("accept");
		std::cout << "Error #" << WSAGetLastError << std::endl;
		exit(EXIT_FAILURE);
	}

	//Get client information
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXSERV);

	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
	{
		std::cout << host << " connected on port " << service << std::endl;
	}
	else
	{
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
	}

	//While loop: accept and send message back to client
	char buffer[4096];
	while (true)
	{
		ZeroMemory(buffer, 4096); //same as memset(buffer, 0, 4096)

		if ((clientSocket = accept(server_fd, (struct sockaddr*)&client,
			(socklen_t*)&clientSize)) == INVALID_SOCKET)
		{
			perror("accept");
			std::cout << "Error #" << WSAGetLastError << std::endl;
			exit(EXIT_FAILURE);
		}

		//Wait for client to send data
		int bytesReceived = recv(clientSocket, buffer, 4096, 0);
		if (bytesReceived == SOCKET_ERROR)
		{
			perror("receive");
			break;
		}

		if (bytesReceived == 0)
		{
			std::cout << "Client disconnected" << std::endl;
			break;
		}

		//HTTP requests to server
		std::string dupMess(buffer);
		std::cout << dupMess << std::endl;
		std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;

		//Parse string to grab specific info
		std::istringstream iss(dupMess);
		std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

		std::string content = "<h1>Hello, client! Welcome to the Virtual Machine Web..</h1>";

		//http content-type
		std::string accept_type;
		std::string getFile;

		//HTTP responses
		if (parsed.size() >= 3 && parsed[0] == "GET")
		{
			getFile = parsed[1];
			if (getFile == "/")
				getFile = "/index.html";
		}

		//Get file extension
		int dot = getFile.find_first_of('.');
		std::string extension = getFile.substr(dot + 1);

		if (extension == "html" || extension == "css")
			accept_type = "text/" + extension;
		else if (extension == "jpg" || extension == "png")
			accept_type = "image/" + extension;

		if (parsed.empty() == FALSE && parsed[parsed.size() - 1].find('&', 0) != std::string::npos)
		{
			std::string userinfo = parsed[parsed.size() - 1];
			int fstequal = userinfo.find_first_of('=');
			int lstequal = userinfo.find_last_of('=');
			int ampersand = userinfo.find('&');

			std::string _username = userinfo.substr(fstequal + 1, ampersand - (fstequal + 1));
			std::string _password = userinfo.substr(lstequal + 1);

			if (_username == "admin" && _password == "admin")
				getFile = "/info.html";
		}

		//Set http status code as 404
		int httpCode = 404;

		std::ifstream infile("./wwwroot" + getFile, std::ios::binary);
		if (infile.good()) 
		{
			std::string str((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
			content = str;
			httpCode = 200;
		}
		else
		{
			getFile = "/404.html";
			std::ifstream file_404("./wwwroot" + getFile, std::ios::binary);
			if (file_404.good())
			{
				std::string str((std::istreambuf_iterator<char>(file_404)), std::istreambuf_iterator<char>());
				content = str;
				httpCode = 404;
			}
			file_404.close();
		}
		infile.close();

		std::ostringstream oss;
		oss << "HTTP/1.1 " << httpCode << " Okay\r\n";
		oss << "Host: " << service << "\r\n";
		oss << "Cache-Control: no-cache, private\r\n";
		oss << "Content-Type: " << accept_type << "\r\n";
		oss << "charset=UTF-8\r\n";
		//oss << "Transfer-Encoding: chunked\r\n";
		oss << "Content-Length: " << content.size() << "\r\n";
		//oss << "Connection: keep-alive\r\n";
		oss << "\r\n";
		oss << content;

		std::string output = oss.str();
		send(clientSocket, output.c_str(), output.size() + 1, 0);
	}

	closesocket(clientSocket);
	WSACleanup();

	//SOCKET clientSocket;
	//int addrlen = sizeof(address);
	//char buffer[4096]{ 0 };
	//while (true)
	//{
	//	if ((clientSocket = accept(server_fd, (struct sockaddr*)&address,
	//		(socklen_t*)&addrlen)) == INVALID_SOCKET)
	//	{
	//		perror("accept");
	//		std::cout << " Error #" << WSAGetLastError << std::endl;
	//		exit(EXIT_FAILURE);
	//	}

	//	int bytesReceived = recv(clientSocket, buffer, 4096, 0);
	//
	//	std::cout << std::string(buffer, 0, bytesReceived) << std::endl;

	//	char message[] = "HTTP/1.1 200 Okay\r\nContent-Type: text/html; charset=ISO-8859-4 \r\n\r\n<h1>Hello, client! Welcome to the Virtual Machine Web..</h1>";
	//	
	//	send(clientSocket, message, strlen(message), 0);
	//	bytesReceived = recv(clientSocket, buffer, 4096, 0);
	//}
}