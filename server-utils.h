#ifndef SERVERUTILS
#define SERVERUTILS

int validArgs(int argc, char *argv[]);

int createBindListenSocket(char *ip, int port, int backlogs);

int createConnectionSocket(int serverFd);

int singleThreadedWorker(int connectionFd);

int multiThreadedWorker(int connectionFd);

char *requestHandler(char *buffer, int size);

int responseHandler(int connectionFd, char *message);

void *httpWorkerFunction(void *arg);

#endif // !SERVERUTILS
