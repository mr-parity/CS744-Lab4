#define _GNU_SOURCE
#include "server-utils.h"
#include "functions.h"
#include "functionslist.h"
#include "http-parser.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int validArgs(int argc, char *argv[]) {

  // invalid args passed
  if (argc < 2) {
    printf("[SERVER] No mode specified exiting...\n");
    return -1;
  }

  char mode = *argv[1];

  // less args than expected for the passed mode
  if ((mode == 's' || mode == 'm') && argc < 3) {
    printf("[SERVER] No PORT passed. Exiting...\n");
    return -1;
  }

  // valid mode passed (s/m/t)
  if (!(mode == 's' || mode == 'm')) {
    printf("Invalid mode passed !\n");
    return -1;
  }

  return 0;
}

int createBindListenSocket(char *ip, int port, int backlogs) {

  // create a socket
  int socketFd = socket(PF_INET, SOCK_STREAM, 0);
  if (socket < 0) {
    perror("socket");
    return -1;
  }

  // bind socket to an address
  struct sockaddr_in socketAddress;

  memset(&socketAddress, 0, sizeof socketAddress);

  socketAddress.sin_family = AF_INET;
  socketAddress.sin_port = htons(port);
  socketAddress.sin_addr.s_addr = inet_addr(ip);

  int reuseAddr = 1;
  setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof reuseAddr);

  int bindStatus =
      bind(socketFd, (struct sockaddr *)&socketAddress, sizeof socketAddress);
  if (bindStatus < 0) {
    perror("bind");
    close(socketFd);
    return -1;
  }

  // listen
  int listenStatus = listen(socketFd, backlogs);
  if (listenStatus < 0) {
    perror("listen");
    close(socketFd);
    return -1;
  }

  printf("[SERVER] Started listening at %s:%d\n", ip, port);

  return socketFd;
}

int createConnectionSocket(int serverFd) {

  struct sockaddr_in connAddr;
  int addrSize = sizeof(connAddr);

  int connFd = accept(

      serverFd,                     // fd of socket which is listening
      (struct sockaddr *)&connAddr, // pointer to sockaddr
      (socklen_t *)&addrSize        // ptr to size of sockaddr

  );

  if (connFd < 0) {
    perror("accept");
    return -1;
  }

  return connFd;
}

char *requestHandler(char buffer[], int size) {

  // parse the http request
  ParsedRequest req;
  parse_http_request(buffer, &req);

  long threadid = (long)gettid();

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
    printf("[thread %ld] GET %s 404 Not found\n", threadid, uri);
    return "404";
  }

  char *out = functions[functionIndex]();
  printf("[thread %ld] GET %s 200 OK \n", threadid, uri);

  return out;
}

int responseHandler(int connectionFd, char *message) {

  // make message null terminated
  int size = strlen(message);
  char value[size + 2];
  strcpy(value, message);
  value[size] = '\n';
  value[size + 1] = '\0';

  // create response body
  const char *body = value;
  size_t len = strlen(body);
  size_t response_size = 0;

  char *statusCode;
  if (strcmp(message, "404") == 0) {
    statusCode = "400 Not found";
  } else {
    statusCode = "200 OK";
  }

  char *http_response = create_http_response(

      statusCode, "text/html", body, len, &response_size

  );

  if (response_size <= 0) {
    return -1;
  }

  int bytes_sent = send(

      connectionFd, http_response, strlen(http_response), 0

  );

  if (bytes_sent < 0) {
    perror("Send failed");
    return -1;
  }

  // cleanup
  free(http_response);

  return 0;
}

int singleThreadedWorker(int connectionFd) {
  // buffer to read
  char buffer[1024];

  // read
  int bytesRecv = recv(connectionFd, buffer, sizeof(buffer) - 1, 0);

  if (bytesRecv < 0) {
    perror("recv");
    close(connectionFd);
    return -1;
  }

  if (bytesRecv == 0) {
    return -1;
  }

  buffer[bytesRecv] = '\n'; // null terminate raw buffer

  // get and process request
  char *value = requestHandler(buffer, bytesRecv);

  // send response
  int status = responseHandler(connectionFd, value);

  close(connectionFd);

  return status;
}

// worker thread function
void *httpWorkerFunction(void *arg) {

  // get conection Fd
  int connectionFd = *(int *)(arg);
  free(arg);

  // get http request and process
  char buffer[1024];
  int bytesRecv = recv(connectionFd, buffer, sizeof(buffer) - 1, 0);

  if (bytesRecv < 0) {
    // close
    perror("recv");
    close(connectionFd);
    pthread_exit(NULL);
  }

  buffer[bytesRecv] = '\n';

  char *msg = requestHandler(buffer, bytesRecv);
  int status = responseHandler(connectionFd, msg);

  close(connectionFd);
  pthread_exit(NULL);
}

int multiThreadedWorker(int connectionFd) {

  int *connFdPtr = malloc(sizeof connectionFd);
  *connFdPtr = connectionFd;

  pthread_t worker_thread;
  pthread_create(&worker_thread, NULL, httpWorkerFunction, (void *)connFdPtr);

  pthread_detach(worker_thread);

  return 0;
}
