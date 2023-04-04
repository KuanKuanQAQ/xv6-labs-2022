#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* get_last_slash(char *c) {
  // printf("getting last slash for string %s\n", c);
  char *p = c + strlen(c);
  while (p >= c && *p != '/') --p;
  ++p;
  return p;
}

void find(char *path, char *file_name) {
  // printf("looking for %s in %s\n", file_name, path);
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_DEVICE:
    case T_FILE:
      if (strcmp(file_name, get_last_slash(path)) == 0) {
        printf("%s\n", path);
      }
      break;
    case T_DIR:
      if (strlen(path) + 1 > sizeof buf) {
        printf("find: path too long\n");
        break;
      }
      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if (de.inum * strcmp(de.name, ".") * strcmp(de.name, "..") == 0)
          continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0) {
          printf("find: cannot stat %s\n", buf);
          continue;
        }
        find(buf, file_name);
      }
      break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "Usage: look for a filename and a path...\n");
    exit(0);
  }
  find(argv[1], argv[2]);
  exit(0);
}
