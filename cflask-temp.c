#include "functions.h"
#include "functionslist.h"
#include "http-parser.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOGS 5

char *requestHandler(char buffer[]) {
  ParsedRequest req;
  parse_http_request(buffer, &req);

  // Print parsed output to verify
  // printf("--- Request Line ---\n");
  // printf("Method:  %s\n", req.request_line.method);
  // printf("URI:     %s\n", req.request_line.uri);
  // printf("Version: %s\n\n", req.request_line.version);

  // printf("--- Headers parsed: %d ---\n", req.header_count);
  // for (int i = 0; i < req.header_count; i++) {
  //   printf("%s -> %s\n", req.headers[i].key, req.headers[i].value);
  // }

  // if (req.body) {
  //   printf("\n--- Body ---\n%s\n\n", req.body);
  // }

  long threadid = (long)pthread_self();

  // handle request
  char *uri = req.request_line.uri;
  int functionIndex = -1;

  for (int i = 0; i < NUM_ROUTES; i++) {
    if (strcmp(routes[i].route, uri) == 0) {
      functionIndex = routes[i].index;
      break;
    }
  }

  if (functionIndex == -1) {
    // printf("Invalid route\n");
    printf("[%ld] GET %s 404 Not found\n", threadid, uri);
    return "404";
  }

  char *out = functions[functionIndex]();
  printf("[%ld] GET %s 200 OK \n", threadid, uri);

  return out;
}

char *responseHandler(char *value) {
  const char *body = value;
  size_t len = strlen(body);
  size_t response_size = 0;

  char *http_response = create_http_response(

      "200 OK", "text/html", body, len, &response_size

  );

  if (http_response) {
    return http_response;
  }

  return NULL;
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    printf("[SERVER] Invalid args passed <usage>: <executable> <s/m> <PORT>");
    exit(1);
  }

  char mode = *argv[1];
  char *modeValue = mode == 's' ? "single-threaded" : "multi-threaded";

  int PORT = atoi(argv[2]);

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
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  int bindStatusCode = bind(

      serverFd,                 // the server fd
      (struct sockaddr *)&addr, // pointer to sockaddr
      sizeof(addr)              // size of sockaddr

  );

  printf("[SERVER] Started listening at 127.0.0.1:%d , [%s]\n", PORT,
         modeValue);

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

  while (1) {

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

    buffer[bytesRecv] = '\n';

    char *msg = requestHandler(buffer);

    //@unused: replaced by requestHandler()
    // printf("[SERVER] Recieved data \n %s", buffer);

    // Prepare the response
    char *http_response = responseHandler(msg);

    // Using SEND to transmit data back to curl
    // Flag 0 makes send behave like standard write()
    int bytes_sent = send(

        clientFd, http_response, strlen(http_response), 0

    );

    if (bytes_sent < 0) {
      perror("Send failed");
    }

    // cleanup
    free(http_response);

    close(clientFd);
  }

  // cleanup
  close(serverFd);

  return 0;
}
