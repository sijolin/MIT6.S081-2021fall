#include "kernel/types.h"
#include "user/user.h"

#define READEND 0
#define WRITEEDN 1

int main() {
  int p1[2];
  int p2[2];
  char buf[1];

  pipe(p1);
  pipe(p2);

  if (fork() == 0) {
    // child progress
    close(p1[WRITEEDN]);
    close(p2[READEND]);
    read(p1[READEND], buf, 1);
    printf("%d: received ping\n", getpid());
    write(p2[WRITEEDN], " ", 1);
    close(p1[READEND]);
    close(p2[WRITEEDN]);
  } else {
    // parent progress
    close(p1[READEND]);
    close(p2[WRITEEDN]);
    write(p1[WRITEEDN], " ", 1);
    read(p2[READEND], buf, 1);
    printf("%d: received pong\n", getpid());
    close(p1[WRITEEDN]);
    close(p2[READEND]);
  }
  exit(0);
}
