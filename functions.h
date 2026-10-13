// Guard
#ifndef FUNCTIONH
#define FUNCTIONH

#define NUM_ROUTES 5
struct Route {
  const char *route;
  int index;
};

static struct Route routes[5] = {

    (struct Route){"/", 0}, (struct Route){"/square", 1},
    (struct Route){"/cube", 2}, (struct Route){"/helloworld", 3},
    (struct Route){"/pingpong", 4}

};

// function signatures
char *root();
char *square();
char *cube();
char *pingpong();
char *helloworld();

#endif // !FUNCTIONH
