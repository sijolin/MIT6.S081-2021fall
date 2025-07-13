#include "kernel/types.h"
#include "user/user.h"

#define READEND 0
#define WRITEEND 1
#define MAXFEEDS 35

void child(int *pl); 

int main() {
  int p[2];
  pipe(p);

  if (fork() == 0) {
    child(p);
  } else {
    close(p[READEND]);
    for (int i = 2; i <= MAXFEEDS; ++i) {
      write(p[WRITEEND], &i, sizeof(int));
    }
    close(p[WRITEEND]);
    wait((int *) 0);
  }
  exit(0);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"

void child(int *pl) {
  int pr[2];
  int n;
  close(pl[WRITEEND]);

  int ret = read(pl[READEND], &n, sizeof(int));
  if (ret == 0) {
    exit(0);
  }

  pipe(pr);

  if (fork() == 0) {
    child(pr);
  } else {
    close(pr[READEND]);
    printf("prime %d\n", n);
    int prime = n;
    while (read(pl[READEND], &n, sizeof(int)) != 0) {
      if (n % prime != 0) {
        write(pr[WRITEEND], &n, sizeof(int));
      }
    }
    close(pr[WRITEEND]);
    wait((int *) 0);
  }
  exit(0);
}

#pragma GCC diagnostic pop
