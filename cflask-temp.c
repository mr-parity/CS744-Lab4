#include "http-parser.h"
#include "server-utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOGS 5

int main(int argc, char *argv[]) {

  // check for valid arguements
  if (validArgs(argc, argv) < 0) {
    exit(1);
  }

  // get coressponding mode
  char mode = *argv[1];
  char *modeValue;

  if (mode == 's') {
    modeValue = "single-threaded";
  } else if (mode == 'm') {
    modeValue = "multi-threaded";
  } else {
    modeValue = "thread-pool";
  }

  // get PORT
  int PORT = atoi(argv[2]);

  // create the socket
  int serverFd = createBindListenSocket("127.0.0.1", PORT, BACKLOGS);

  if (serverFd < 0) {
    printf("[SERVER] Failed to create and bind to a socket. Exiting ...");
    exit(1);
  }

  printf("[SERVER] MODE: %s\n", modeValue);

  while (1) {

    // accept incoming connection

    int connFd = createConnectionSocket(serverFd);

    if (connFd < 0) {
      continue;
    }

    // single threaded mode
    if (mode == 's') {

      int status = singleThreadedWorker(connFd);
      if (status < 0) {
        printf("[SERVER] Failed to send response\n");
      }
      continue;

    } else if (mode == 'm') {

      int status = multiThreadedWorker(connFd);
      if (status < 0) {
        printf("[SERVER] ERROR");
      }
      continue;
    }
  }

  // cleanup
  close(serverFd);

  return 0;
}
