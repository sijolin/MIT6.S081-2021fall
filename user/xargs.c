#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXLEN 100

int main(int argc, char *argv[]) {
  char *command = argv[1];
  char bf;
  char paramv[MAXARG][MAXLEN];
  char *para[MAXARG];

  while (1) {
    int cnt = argc - 1;
    memset(paramv, 0, MAXARG * MAXLEN);
    for (int i = 1; i < argc - 1; ++i) {
      strcpy(paramv[i - 1], argv[i + 1]);
    }
    
    int ret;
    int cursor = 0;
    int flag = 0; // 1 is reading

    while ((ret = read(0, &bf, 1)) > 0 && bf != '\n') {
      if (bf != ' ') {
        paramv[cnt][cursor++] = bf;
        flag = 1;
      } else if (bf == ' ' && flag == 1) {
        cnt++;
        cursor = 0;
        flag = 0;
      }
    }

    if (ret <= 0) {
      break;
    }

    for (int i = 0; i < MAXARG - 1; ++i) {
      para[i] = paramv[i];
    }
    para[MAXARG - 1] = 0;

    if (fork() == 0) {
      exec(command, para);
      exit(0);
    } else {
      wait((int *) 0);
    }
  }
  exit(0);
}

