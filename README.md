# xv6-labs-2022
xv6 是 MIT 开发的一个教学用的完整的类 Unix 操作系统，并且在 MIT 的操作系统课程 6.828 中使用。

# 第 1 章 操作系统接口

操作系统有三项任务：

1. 将计算机的资源在多个程序间共享。
2. 管理并抽象底层硬件，举例来说，一个文字处理软件（比如 word）不用去关心自己使用的是何种硬盘。
3. 提供一种受控的交互方式，使得程序之间可以共享数据、共同工作。

操作系统完成这三项任务的方法是为用户程序提供服务，而用户程序可以通过操作系统接口来调用这些服务。

这些操作系统接口又被称为系统调用，xv6 中包含以下系统调用：

| 系统调用                  | 描述                               |
| ------------------------- | ---------------------------------- |
| fork()                    | 创建进程                           |
| exit()                    | 结束当前进程                       |
| wait()                    | 等待子进程结束                     |
| kill(pid)                 | 结束 pid 所指进程                  |
| getpid()                  | 获得当前进程 pid                   |
| sleep(n)                  | 睡眠 n 秒                          |
| exec(filename, *argv)     | 加载并执行一个文件                 |
| sbrk(n)                   | 为进程内存空间增加 n 字节          |
| open(filename, flags)     | 打开文件，flags 指定读/写模式      |
| read(fd, buf, n)          | 从文件中读 n 个字节到 buf          |
| write(fd, buf, n)         | 从 buf 中写 n 个字节到文件         |
| close(fd)                 | 关闭打开的 fd                      |
| dup(fd)                   | 复制 fd                            |
| pipe(p)                   | 创建管道， 并把读和写的 fd 返回到p |
| chdir(dirname)            | 改变当前目录                       |
| mkdir(dirname)            | 创建新的目录                       |
| mknod(name, major, minor) | 创建设备文件                       |
| fstat(fd)                 | 返回文件信息                       |
| link(f1, f2)              | 给 f1 创建一个新名字(f2)           |
| unlink(filename)          | 删除文件                           |

## 1.1 进程和内存

一个进程可以通过系统调用 fork() 来创建一个新的进程。fork() 创建的新进程被称为**子进程**，子进程的内存内容同创建它的进程（父进程）一样，但是父子进程拥有不同的内存空间和寄存器，改变一个进程中的变量不会影响另一个进程。fork 函数在父进程、子进程中都返回（一次调用两次返回）。对于父进程它返回子进程的 pid，对于子进程它返回 0。

exit() 使当前进程退出，并且会返回退出状态。例如 exit(0) 会返回 0，表示进程正常退出。

wait() 会返回一个已退出的子进程的 pid，并向传入的地址中记录子进程的退出状态，例如 wait(&s)，并且子进程正常退出，那么 s 就会等于 0。如果当前没有已退出的子进程，那么 wait() 会一直等待。如果当前进程根本没有子进程，那么 wait() 会立即返回 -1。如果我们不关心子进程的退出状态，我们可以直接向 wait(0) 中传入 0。

> 实际上，wait() 和 exit() 的参数在 xv6 中是必需的。

系统调用 exec() 将从某个文件（通常是可执行文件）里读取内存镜像（包含代码段、数据段等内容），并将其替换到调用它的进程的内存空间。这份文件必须符合特定的格式，规定文件的哪一部分是指令，哪一部分是数据，哪里是指令的开始等等。xv6 使用 ELF 文件格式，第2章将详细介绍它。当 exec() 执行成功后，它并不返回到原来的调用进程，而是从 ELF 头中声明的入口开始，执行从文件中加载的指令。exec() 接受两个参数：可执行文件名和一个字符串参数数组。举例来说：

```c
char *argv[3];
argv[0] = "echo";
argv[1] = "hello";
argv[2] = 0;
exec("/bin/echo", argv);
printf("exec error\n");
```

这段代码将进程替换为 /bin/echo 这个程序，参数列表是 echo hello。

xv6 shell 用 fork() 和 exec() 为用户执行程序。首先通过 getcmd() 读取命令行的输入，然后 fork() 出一个子进程，父进程 wait()。子进程使用 exec() 执行用户命令。

这种 shell 和具体程序分离的做法有利于输入输出重定向。子进程可以做任何事情 open()、read()，shell 不会受到子进程的影响。

## 1.2 I/O 和文件描述符

每个进程都保存了一张文件描述符数组，数组中的每一项里记载一个进程打开的文件的信息，包括文件保存的地址。所以通过这个数组，我们就可以找到文件。在操作系统中我们更习惯称用于记录信息的数组为表，所以一般称为文件描述符表。

文件描述符 fd 是一个整数，实际上是文件描述符表的下标，代表了一个受内核管理的、可由进程读写的对象。这个对象可能是文件、目录、设备、管道或者 socket。这些对象在 Unix 中被统称为文件。

每个进程会把它所打开的文件从 0 开始进行标号，并放到文件描述符表的对应下标处，这个下标就被称为 fd。在默认情况下 fd0 表示标准输入，fd1 表示标准输出，fd2 表示标准错误输出。Shell 作为一个进程，启动时会确保这三个 fd 都被打开了。

> 所谓默认情况，是用户程序默认从 fd0 读入输入，fd1 输出标准输出，fd2 输出标准错误输出。

read(fd, buf, n) 和 write(fd, buf, n) 两个系统调用会在指定的 fd 中读写，其位置由偏移量 offset 决定。在每次读写之后都会更新 offset，来指示下次读写的位置。也可以随时设置 offset。

> 每个文件描述符不仅规定了文件的位置，还规定了文件的操作，只能是读或写的一种。所以如果想既读又写一个文件，就要为其打开两个文件描述符，但是它们有各自的文件操作以及各自的 offset。

close(fd) 会根据 fd 来释放文件描述符表中的一项。这样文件描述符表中就空出一项，未来可以装填一个其他文件。**值得注意的是，在新打开一个文件时，总是会被装载到文件描述符表中下标最小的那个空闲项中。**

### 1.2.1 I/O 重定向

Unix shell 利用这个特性以及系统调用 fork() 和 exec() 来实现 I/O 重定向。

fork() 会复制父进程包括文件描述符在内的所有使用的内存，所以子进程和父进程的文件描述符一模一样。

exec() 会替换调用它的进程的内存**但是会保留它的文件描述符表**。

所以 shell 可以这样实现重定向：fork 一个进程，重新打开指定文件的文件描述符，然后执行新的程序。

下面是一个简化版的 shell 执行 `cat <input.txt` 的代码。这段代码使得 cat 在标准输入指向 input.txt 的情况下运作。cat 这个函数是由 shell 提供的，效果是把若干文件连接（concatenate）起来，然后输出到标准输出中即 fd2 中。

cat 的输入本来就是参数中的文件，例如 cat input.txt 它的输入就是 input.txt 中的内容。cat 1<input.txt 只是把标准输入重定向为了 input.txt，并且没有给 cat 传递任何参数。如果不给 cat 传递参数，cat 本来会等待标准输入的输入，直接把它输出到标准输出中。现在重定向了标准输入到 input.txt，就直接输出到标准输出中来了。

```c
char *argv[2];
argv[0] = “cat”;
argv[1] = 0;
if (fork() == 0) { // fork() 得到新的子进程，保持 fd0 fd1 fd2 不变。
  close(0); // 关闭 fd0
  open(“input.txt”, O_RDONLY); // open() 会默认把文件描述符装载到文件描述符表的首个空闲项中，即 fd0
  exec(“cat”, argv); // exec() 替换内存执行 cat，并保留文件描述符表。
}
```

> <input.txt 是对 0<input.txt 的简写，意思是重定向标准输入。之所以能够简写是因为标准输入的文件描述符下标默认是 0。
>
> 同理，>output.txt 是对 1>output.txt 的简写，意思是重定向标准输出。
>
> 另外，也存在 << 和 >> 表示新写入的内容添加到文件末尾。

### 1.2.2 文件描述符共享 offset

fork() 得到的子进程会从父进程那里复制文件描述符，但与父进程共享 offset。

```c
if (fork() == 0) { // fork() 在父进程中返回子进程进程号 pid，在子进程中返回 0
  write(1, “hello “, 6); // 子进程在 fd1 中写入了 “hello ”
  exit(0);
} else {
  wait(0); // wait() 会等待子进程结束
  write(1, “world\n”, 6); // 在子进程结束后向 fd1 中写入 “world\n”
}
```

dup() 是另一种共享 offset 的情况。

dup() 传入一个文件描述符表的下标 fd1，返回一个文件描述符表的下标 fd2，使 fd2 和 fd1 表示同一个文件，并且二者也会共享 offset。

这在重定向标准错误输出时十分有用。我们常常想让标准输出和标准错误输出在同一个文件中。0.1.1 节我们已经知道了可以用 1>output.txt 来重定向标准输入，如果我们在后面再加上 2>output.txt 行不行呢？是这不行的。

这会使得 output.txt 被打开两次，有着各自的 offset，导致写覆盖。如果改为 1>>output.txt 2>>output.txt 行不行呢？这也是不好的，虽然它们都将写入 offset 置为文件结尾，但是这会产生潜在的资源竞争，效率较低。

正确的做法是利用 dup()，写为 1>output.txt 2>&1。前半句将标准输出重定向为 output.txt，后半句在标准错误输出对应的文件描述符 fd2 中保存了一个 fd1 的副本，它们共享 offset，并且 output.txt 只打开一次。

> 我还是搞不懂。
>
> 例如 cat 0<1.txt 1>1.txt 会导致 1.txt 里的内容被清空。

## 1.3 管道

管道是一个小的内核缓冲区，它以文件描述符**对**的形式提供给进程，一个用于写操作，一个用于读操作。从管道的一端写的数据可以从管道的另一端读取。管道提供了一种进程间交互的方式。

接下来的示例代码运行了程序 wc，它的标准输出绑定到了一个管道的读端口。

```c
int p[2];
char *argv[2];
argv[0] = "wc";
argv[1] = 0;
pipe(p);
if(fork() == 0) {
    close(0);
    dup(p[0]);
    close(p[0]); // close() 并没有关闭管道，只是在进程中清空了对应的文件描述符
    close(p[1]);
    exec("/bin/wc", argv);
} else {
    write(p[1], "hello world\n", 12);
    close(p[0]);
    close(p[1]);
}
```

close() 并没有关闭管道，只是在进程中清空了对应的文件描述符。只有在一个文件所有的文件描述符都被清空的时候，文件才真正被 close()。在上面的代码段中，close(p[0]) 之前 dup(p[0])，导致这个管道会持续存在。

这个 dup(p[0]) 在 close(0) 之后，使得获得的文件描述符是放在 fd0 上的。

对管道进行的 read() 操作会一直等待管道关闭（这个管道所有写那一端的文件描述符都关闭），再将所有所读输出。所以在执行 wc 之前要关闭所有的 p[1]。

### 1.3.1 xv6 shell 对管道的实现

xv6 shell 对管道的实现（比如 fork sh.c | wc -l）和上面的描述是类似的（7950行）。子进程创建一个管道连接管道的左右两端。然后它为管道左右两端都调用 runcmd，然后通过两次 wait 等待左右两端结束。管道右端可能也是一个带有管道的指令，如 a | b | c，它 fork 两个新的子进程（一个 b 一个 c），因此，shell 可能创建出一颗进程树。树的叶子节点是命令，中间节点是进程，它们会等待左子和右子执行结束。理论上，你可以让中间节点都运行在管道的左端，但做的如此精确会使得实现变得复杂。

pipe 可能看上去和临时文件没有什么两样：命令

```shell
echo hello world | wc
```

可以用无管道的方式实现：

```shell
echo hello world > /tmp/xyz; wc < /tmp/xyz
```

但管道和临时文件起码有三个关键的不同点。首先，管道会进行自我清扫，如果是 shell 重定向的话，我们必须要在任务完成后删除 /tmp/xyz。第二，管道可以传输任意长度的数据。第三，管道允许同步：两个进程可以使用一对管道来进行二者之间的信息传递，每一个读操作都阻塞调用进程，直到另一个进程用 write 完成数据的发送。

以下是一个简单的管道实现：

```c
  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic(“pipe”);
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
    break;
```

## 1.4 文件系统

进程的当前目录可以通过 chdir 这个系统调用进行改变，例如下面两部分代码打开的是同一个文件：

```c
/*----代码段 1----*/
chdir("/a");
chdir("b");
open("c", O_RDONLY);
/*----代码段 2----*/
open("/a/b/c", O_RDONLY);
```

chdir 这个系统调用用于实现 shell 中的 cd 程序。但是和其他 shell 提供的程序不同的是，cd 并不采用 fork() + exec() 的方式执行，因为这样只会改变子进程的工作目录。cd 会直接改变 shell 自身的当前工作目录。

fstat(fd, &st) 可以获得一个文件描述符 fd 指向的文件的信息，它填充一个名为 stat 的结构体：

```c
#define T_DIR  1
#define T_FILE 2
#define T_DEV  3
// Directory
// File
// Device
     struct stat {
       short type;  // Type of file
       int dev;     // File system’s disk device
       uint ino;    // Inode number
       short nlink; // Number of links to file
       uint size;   // Size of file in bytes
};
```

并将这个结构体赋值给 st。

# 第 2 章 OS 组织

操作系统必须安排进程之间的隔离。 如果一个进程有错误并失败了，它不应该影响其他不依赖于失败进程的进程。

操作系统必须满足三个要求：多路复用、隔离和交互。

## 2.1 抽象物理资源

### 2.1.1 CPU 支持的三种模式
CPU 为强隔离提供硬件支持。 例如RISC-V有三种CPU可以执行指令的模式：机器模式、管理模式和用户模式。
在机器模式下执行的指令具有完全特权； CPU 以机器模式启动。 机器模式主要用于配置计算机。

在管理员模式下，CPU 可以执行特权指令：例如，启用和禁用中断、读取和写入保存页表地址的寄存器等。

应用程序只能执行用户模式指令。

### 2.1.2 应用程序想要调用内核函数（系统调用）

想要调用内核函数（例如 xv6 中的读取系统调用）的应用程序必须转换到内核。 CPU 提供了一条特殊的指令，可以将 CPU 从用户模式切换到管理员模式，并在内核指定的入口点进入内核。 （RISC-V 为此提供了 ecall 指令。）
出于安全原因，内核控制转换到主管模式的入口点。

## 2.2 xv6 组织

### 2.2.1 xv6 代码结构

![image-20230405220630310](/Users/Kuan/Library/Application Support/typora-user-images/image-20230405220630310.png)

### 2.2.2 用户虚存空间组织

![image-20230405221027076](/Users/Kuan/Library/Application Support/typora-user-images/image-20230405221027076.png)

xv6 在 RISC-V 上运行，虚拟地址有 39 位，但只使用 38 位。 因此，最大地址为 2 ^ 38 − 1 = 0x3fffffffff，即 MAXVA (kernel/riscv.h:349)。

每个进程都有一个执行线程（或简称线程）来执行进程的指令。 一个线程可以被挂起并稍后恢复。 为了在进程之间透明地切换，内核挂起当前运行的线程并恢复另一个进程的线程（上下文切换）。

## 2.3 xv6 的启动和第一个进程

### 2.3.1 内核启动

当 RISC-V 计算机开机时，它会自行初始化并运行存储在只读存储器中的引导加载程序。 引导加载程序将 xv6 内核加载到内存中。 然后，在机器模式下，CPU 从 _entry (kernel/entry.S:12) 开始执行 xv6。 Xv6 以禁用 RISC-V 分页硬件开始：虚拟地址直接映射到物理地址。

1. 引导加载程序将 xv6 内核加载到物理地址 0x80000000 的内存中，因为地址范围 0x0:0x80000000 包含 I/O 设备。
2. _entry 处的指令设置了一个堆栈，以便 xv6 可以运行 C 代码。 _entry 处的代码将堆栈指针寄存器 sp 加载到地址 stack0+4096，堆栈的顶部，因为 RISC-V 上的堆栈向下增长。
3. 现在 xv6 有一个堆栈，并且能够运行 C 代码。
4. _entry 在开始时调用 C 代码（kernel/start.c:21）。
5. 执行一些仅在机器模式下允许的配置。
6. 将程序计数器设置为 main。
7. 通过将 satp 设置为 0 来禁用分页。
8. 启用时钟中断。
9. 切换到管理员模式并跳转到 main()。

### 2.3.2 第一个进程

在 main 中，在初始化了几个设备和子系统之后，它通过调用 userinit() 创建了第一个进程。

```c
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy init’s instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first “return” from kernel to user.
  p->tf->epc = 0;      // user program counter
  p->tf->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, “initcode”, sizeof(p->name));
  p->cwd = namei(“/“);

  p->state = RUNNABLE;

  release(&p->lock);
}
```

第一个进程执行一个小程序：initcode。 它是以一段二进制的形式被 uvminit() 直接放到内存中的：

```c
// a user program that calls exec(“/init”)
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x05, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x05, 0x02,
  0x9d, 0x48, 0x73, 0x00, 0x00, 0x00, 0x89, 0x48,
  0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0xbf, 0xff,
  0x2f, 0x69, 0x6e, 0x69, 0x74, 0x00, 0x00, 0x01,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00
};
```

以上是initcode.S构建的二进制的八进制数据格式，而 initcode.S 本来是这样的：

```assembly
# Initial process execs /init.
# This code runs in user space.

#include “syscall.h”

# exec(init, argv)
.globl start
start:
        la a0, init
        la a1, argv
        li a7, SYS_exec
        ecall

# for(;;) exit();
exit:
        li a7, SYS_exit
        ecall
        jal exit

# char init[] = “/init\0”;
init:
  .string “/init\0”

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0
```

上面的initcode程序通过调用exec系统调用运行一个新的程序 init 重新进入内核。

```c
int
main(void)
{
  int pid, wpid;

  if(open(“console”, O_RDWR) < 0){
    mknod(“console”, 1, 1);
    open(“console”, O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf(“init: starting sh\n”);
    pid = fork();
    if(pid < 0){
      printf(“init: fork failed\n”);
      exit(1);
    }
    if(pid == 0){
      exec(“sh”, argv);
      printf(“init: exec sh failed\n”);
      exit(1);
    }
    while((wpid=wait(0)) >= 0 && wpid != pid){
      //printf(“zombie!\n”);
    }
  }
}
```

该程序创建文件描述符 0、1、2。启动控制台 shell。 等待 shell 存在，然后重复。













