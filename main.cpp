#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

int main() {
    WSADATA wsaData;
    if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
    {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET serverSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if ( serverSocket == INVALID_SOCKET )
    {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    u_long mode = 1;
    if ( ioctlsocket( serverSocket, FIONBIO, &mode ) != NO_ERROR )
    {
        std::cerr << "ioctlsocket failed.\n";
        closesocket( serverSocket );
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons( PORT );

    if ( bind( serverSocket, ( struct sockaddr* )&serverAddr,
        sizeof( serverAddr ) ) == SOCKET_ERROR )
    {
        std::cerr << "Bind failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if ( listen( serverSocket, SOMAXCONN ) == SOCKET_ERROR )
    {
        std::cerr << "Listen failed.\n";
        closesocket( serverSocket );
        WSACleanup();
        return 1;
    }

    std::cout << "HTTP Server listening on port " << PORT << "...\n";

    fd_set masterSet;
    FD_ZERO( &masterSet );
    FD_SET( serverSocket, &masterSet );

    while ( true )
    {
        fd_set readSet = masterSet;

        int activity = select( 0, &readSet, nullptr,
        nullptr, nullptr );
        if ( activity == SOCKET_ERROR )
        {
            std::cerr << "select() failed.\n";
            break;
        }

        for ( u_int i = 0; i < readSet.fd_count; i++ )
        {
            SOCKET s = readSet.fd_array[i];

            if ( s == serverSocket )
            {
                sockaddr_in clientAddr;
                int clientAddrLen = sizeof( clientAddr );
                SOCKET clientSocket = accept( serverSocket,
                    ( struct sockaddr* )&clientAddr, &clientAddrLen );

                if ( clientSocket != INVALID_SOCKET )
                {
                    u_long clientMode = 1;
                    ioctlsocket( clientSocket, FIONBIO, &clientMode );

                    FD_SET( clientSocket, &masterSet );

                    char clientIp[ INET_ADDRSTRLEN ];
                    inet_ntop( AF_INET, &( clientAddr.sin_addr ),
                        clientIp, INET_ADDRSTRLEN );

                    std::cout << "New connection accepted from " << clientIp << ":" << ntohs( clientAddr.sin_port ) << "\n";
                }
            }
            else
            {

                char buffer[ 4096 ];
                int bytesReceived = recv( s, buffer, sizeof( buffer ) - 1, 0 );

                if ( bytesReceived > 0 )
                {
                    buffer[ bytesReceived ] = '\0';
                    std::string request( buffer );

                    std::istringstream requestStream( request );
                    std::string method, path, version;

                    requestStream >> method >> path >> version;

                    if ( method == "GET" )
                    {
                        if ( path == "/" )
                        {
                            path = "/index.html";
                        }

                        std::cout << "Parsed Request -> Method: " << method
                                  << " | Path: " << path
                                  << " | Version: " << version << "\n";

                        std::string filepath = "../public" + path;
                        std::ifstream file( filepath, std::ios::binary );

                        std::string response;

                        if ( file )
                        {

                            std::ostringstream fileContent;
                            fileContent << file.rdbuf();
                            std::string body = fileContent.str();

                            response = "HTTP/1.1 200 OK\r\n";
                            response += "Content-Type: text/html\r\n";
                            response += "Content-Length: " + std::to_string( body.size() ) + "\r\n";
                            response += "Connection: close\r\n\r\n";
                            response += body;
                        }
                        else
                        {
                            std::string body = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>";
                            response = "HTTP/1.1 404 Not Found\r\n";
                            response += "Content-Type: text/html\r\n";
                            response += "Content-Length: " + std::to_string( body.size() ) + "\r\n";
                            response += "Connection: close\r\n\r\n";
                            response += body;
                            std::cout << "File not found. Sending 404 for: " << filepath << "\n";
                        }

                        send( s, response.c_str(), response.size(), 0 );
                    }
                    else
                    {
                        std::cout << "Unsupported HTTP method: " << method << "\n";
                    }

                }
                else if ( bytesReceived == 0 )
                {
                    std::cout << "Client disconnected gracefully.\n";
                }
                else
                {
                    std::cerr << "recv() failed or connection reset by peer.\n";
                }
                closesocket( s );
                FD_CLR( s, &masterSet );
            }
        }
    }
    closesocket( serverSocket );
    WSACleanup();
    return 0;
}