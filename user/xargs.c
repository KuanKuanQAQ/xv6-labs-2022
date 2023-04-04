#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define ARGNUM 32
#define MAXLEN 512

int split(char *vtgt[], char *vsrc, char c) {
  char **tgt = vtgt;
  int len = 1;
  *tgt++ = vsrc;
  for (int i = 0; i < strlen(vsrc); ++i) {
    if (vsrc[i] == c) {
      vsrc[i] = 0;
      *tgt++ = vsrc + i + 1;
      ++len;
    }
  }
  *tgt = 0;
  return len;
}

void print_char_pointer_array(char* name, char *c[]) {
  printf("%s[]:", name);
  for (int i = 0; c[i]; ++i) {
    printf(" %s", c[i]);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  char buf[MAXLEN];
  while (1) {
    memset(buf, 0, MAXLEN);
    gets(buf, MAXLEN);
    if (buf[0] == 0) break;
    buf[strlen(buf) - 1] = 0;
    char *xargs[ARGNUM], *tmp[ARGNUM];
    memset(xargs, 0, ARGNUM);
    memset(tmp, 0, ARGNUM);
    memcpy(xargs, argv + 1, (argc - 1) * sizeof(char *) / sizeof(char));
    /*
    a 数组每个元素都是一个 char * 指针，指向一个字符串。
    a 则是一个 char ** 指针，指向一个 char * 指针。
    在 memcpy() 中，以 char 为单位一个一个地把 src 中的东西复制到 dst 中。
    而这里是想把指针数组 argv 里的指针复制到指针数组 a 里。
    这里的每个指针的大小都是 char *，指针大小与类型无关，只与数据总线宽度有关，即处理器位数
    sizeof(char *) == sizeof(int *)
    所以 memcpy() 每次复制 sizeof(char) 个字节，
    复制一个 char * 一共要复制 sizeof(char *) / sizeof(char) 次
    */
    int tmp_len = split(tmp, buf, ' ');
    // print_char_pointer_array("tmp", tmp);
    memcpy(xargs + argc - 1, tmp, tmp_len * sizeof(char *) / sizeof(char));
    // print_char_pointer_array("xargs", xargs);
    if (fork() == 0) {
      exec(xargs[0], xargs);
    } else {
      wait(0);
    }
  }
  exit(0);
}
