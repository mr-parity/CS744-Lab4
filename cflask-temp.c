#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOGS 5

int main() {
  printf("Welcome to socket programming!\n");

  // create the socket
  int serverFd = socket(

      PF_INET,     // domain
      SOCK_STREAM, // connection type,
      0            // protocol

  );

  if (serverFd < 0) {
    perror("socket");
    exit(1);
  }

  // bind a name to socket
  struct sockaddr_in addr;

  memset(&addr, 0, sizeof(addr)); // make struct all zeroes
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  int bindStatusCode = bind(

      serverFd,                 // the server fd
      (struct sockaddr *)&addr, // pointer to sockaddr
      sizeof(addr)              // size of sockaddr

  );

  printf("[SERVER] Started listening at 127.0.0.1:8080\n");

  if (bindStatusCode < 0) {
    perror("bind");
    close(serverFd);
    exit(1);
  }

  // listen for incoming connections
  int listenStatusCode = listen(

      serverFd, // socket
      BACKLOGS  // limit the queue to 5 connections
  );

  if (listenStatusCode < 0) {
    perror("listen");
    close(serverFd);
    exit(1);
  }

  // accept incoming connection

  struct sockaddr_in clientAddr;
  int addrSize = sizeof(clientAddr);

  int clientFd = accept(

      serverFd,                       // fd of socket which is listening
      (struct sockaddr *)&clientAddr, // pointer to sockaddr
      (socklen_t *)&addrSize          // ptr to size of sockaddr

  );

  if (clientFd < 0) {
    perror("accept");
    close(serverFd);
  }

  // communicate lets go
  // recv
  char buffer[1024];
  int bytesRecv = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

  if (bytesRecv < 0) {
    // close
    perror("read");
    close(clientFd);
    close(serverFd);
    exit(1);
  }

  buffer[1023] = '\n';

  printf("[SERVER] Recieved data \n %s", buffer);

  // Prepare the response
  char *http_response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 18\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "Hello from send()!";

  // 7. Using SEND to transmit data back to curl
  // Flag 0 makes send behave like standard write()
  int bytes_sent = send(

      clientFd, http_response, strlen(http_response), 0

  );

  if (bytes_sent < 0) {
    perror("Send failed");
  }

  // cleanup
  close(clientFd);
  close(serverFd);

  return 0;
}
