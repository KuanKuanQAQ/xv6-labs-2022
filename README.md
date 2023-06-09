# xv6-labs-2022
xv6 是 MIT 开发的一个教学用的完整的类 Unix 操作系统，并且在 MIT 的操作系统课程 6.828 中使用。

# 第 1 章 操作系统接口

操作系统有三项任务：

1. 多路复用（分时复用）：将计算机的资源在多个程序间共享。
2. 隔离：管理并抽象底层硬件，举例来说，一个文字处理软件（比如 word）不用去关心自己使用的是何种硬盘。
3. 交互：提供一种受控的交互方式，使得程序之间可以共享数据、共同工作。

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

# 第 2 章 操作系统组织

## 2.1 xv6 结构

### 2.1.1 xv6 的代码结构

xv6 内核源代码位于 kernel/ 子目录中。 按照模块化的粗略概念，源代码被分成文件； 下表列出了这些文件。 模块间接口在 defs.h (kernel/defs.h) 中定义。

![fig2.2](./assets/fig2.2.png)

### 2.1.1 xv6 的虚存结构

![fig2.3](./assets/fig2.3.png)

注意，xv6 的用户栈似乎和 linux 不太一样，是紧挨着数据段和代码段的。而 linux 则是放在虚存中用户空间的顶部。但是它们都是从上向下生长的。

另外，xv6 运行在 RISC-V 64 位指令集架构处理器上，虚存地址最多可以有 39 位（其他位用于记录各种控制信息），不过 xv6 只使用其中 38 位。也就是说 xv6 的进程虚拟地址空间的大小是 2 ^ 38 - 1，也就是 MAXVA 的值。

```c
// ==================== kernel/riscv.h 359 363 ====================
// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
```

## 2.2 xv6 的启动和第一个进程

### 2.2.1 内核启动

当 RISC-V 计算机开机时，它会自行初始化并运行存储在只读存储器中的引导加载程序。 引导加载程序将 xv6 内核加载到内存中。然后，CPU 从 _entry (kernel/entry.S:12) 开始执行 xv6。Xv6 以禁用 RISC-V 分页硬件开始：虚拟地址直接映射到物理地址。

1. 引导加载程序将 xv6 内核加载到物理地址 0x80000000 的内存中，因为地址范围 0x0:0x80000000 包含 I/O 设备。
2. _entry 处的指令设置了一个栈，以便 xv6 可以运行 C 代码。 _entry 处的代码将栈指针寄存器 sp 加载到地址 stack0+4096，堆栈的顶部，因为 RISC-V 上的栈向下增长。
3. 现在 xv6 有一个堆栈，并且能够运行 C 代码。
4. _entry 在开始时调用 C 代码（kernel/start.c 20）。
5. 执行一些仅在机器模式下允许的配置。
6. 将程序计数器设置为 main（kernel/start.c 31）。
7. 通过将 satp 设置为 0 来禁用分页（kernel/start.c 34）。
8. 启用时钟中断。
9. 切换到管理员模式并跳转到 main() （kernel/start.c 34）。

### 2.2.2 第一个进程

在 main 中，在初始化了几个设备和子系统之后，它通过调用 userinit() 创建了第一个进程。

```c
// ==================== kernel/proc.c 231 255 ====================
// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}
```

userinit() 首先用 allocproc() 找到一个空闲的进程表项。所有进程都有一个对应的进程表项，可能是进程控制块，记载了这个进程的状态、进程号、父进程、内核栈的虚存地址、占用内存大小、用户页表、打开文件、当前目录和上下文信息。在找到空闲进程表项之后 allocproc() 会设置进程页表、上下文以及下面这项我看不懂的东西。

```c
  struct trapframe *trapframe; // data page for trampoline.S
```

在 allocproc() 完成后，变量 p 为这个进程的进程表项。然后使用 uvmfirst() 把一段编译好的二进制程序直接加载到特定位置：

```c
// ==================== kernel/vm.c 207 221 ====================
// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void
uvmfirst(pagetable_t pagetable, uchar *src, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("uvmfirst: more than a page");
  mem = kalloc(); // 在 freelist 里中找到一个空闲物理页面
  memset(mem, 0, PGSIZE); // 清理这个物理页面
  mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U); // 建立映射
  memmove(mem, src, sz); // 把 src 搬入其中
}
```

而编译好的二进制程序是这样的：

```c
// ==================== kernel/vm.c 218 229 ====================
// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};
```

这个程序来自 inicode.S：

```assembly
# ==================== user/inicode.S 1 28 ====================
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

这段代码首先使用 la 将 init 对应的地址加载到 a0 寄存器，然后再用 la 将 argv 对应的地址加载到 a1 寄存器。最后将 SYS_exec 的地址加载到 a7 寄存器。ecall 指令将**权限提升到内核模式并将程序跳转到指定的地址**。

最终让第一个进程重新回到内核态，并使用 exec 系统调用运行 init 文件，无参数。init 如下：

```c
// ==================== user/init.c 14 54 ====================
int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }
		// 父进程可以运行到这里，此时变量 pid 为子进程的 pid
    for(;;){
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *) 0); // 返回的 wpid 是终止的子进程 pid。
      if(wpid == pid){
        // the shell exited; restart it.
        break;
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
```

而编译好的二进制程序是这样的：

```c
// ==================== kernel/vm.c 218 229 ====================
// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};
```

# 第 3 章 页表

页表是最流行的机制，操作系统通过它为每个进程提供自己的私有地址空间和内存。 页表决定了内存地址的含义，以及可以访问物理内存的哪些部分。 它们允许 xv6 隔离不同进程的地址空间并将它们多路复用到单个物理内存中。 页表是一种流行的设计，因为它们提供了一种间接级别，允许操作系统执行许多技巧。 Xv6 执行一些技巧：在多个地址空间中映射相同的内存（蹦床页面），并使用未映射的页面保护内核和用户堆栈。 本章的其余部分解释了 RISC-V 硬件提供的页表以及 xv6 如何使用它们。

## 3.1 Paging hardware

提醒一下，RISC-V 指令（用户和内核）操作虚拟地址。 机器的 RAM 或物理内存是用物理地址索引的。 RISC-V 页表硬件通过将每个虚拟地址映射到物理地址来连接这两种地址。

xv6 运行在 Sv39 RISC-V 上，这意味着只使用 64 位虚拟地址的低 39 位； 不使用前 25 位。 在此 Sv39 配置中，RISC-V 页表在逻辑上是一个包含 227 (134,217,728) 个页表条目 (PTE) 的数组。 每个 PTE 包含一个 44 位物理页码 (PPN) 和一些标志。 分页硬件翻译一个虚拟地址，利用39位中的高27位索引到页表中找到一个PTE，并得到一个56位的物理地址，高44位来自PTE中的PPN，低位 从原始虚拟地址复制 12 位。 图 3.1 将页表的逻辑视图显示为一个简单的 PTE 数组（有关更完整的故事，请参见图 3.2）。 页表使操作系统能够以 4096 (212) 字节的对齐块的粒度对虚拟地址到物理地址的转换进行控制。 这样的块称为页面。

在 Sv39 RISC-V 中，虚拟地址的前 25 位不用于转换。 物理地址也有增长的空间：PTE 格式中有空间让物理页号再增长 10 位。 RISC-V 的设计者根据技术预测选择了这些数字。 239 字节是 512 GB，对于在 RISC-V 计算机上运行的应用程序来说，这应该是足够的地址空间。 256 在不久的将来足以容纳许多 I/O 设备和 DRAM 芯片的物理内存空间。 如果需要更多，RISC-V 设计者已经定义了具有 48 位虚拟地址的 Sv48 [3]。

如图 3.2 所示，RISC-V CPU 通过三个步骤将虚拟地址转换为物理地址。 页表以三级树的形式存储在物理内存中。 树的根是一个 4096 字节的页表页，包含 512 个 PTE，其中包含树的下一级页表页的物理地址。 这些页面中的每一页都包含树中最后一层的 512 个 PTE。 分页硬件使用 27 位中的高 9 位选择根页表页中的 PTE，中间 9 位选择树的下一级页表页中的 PTE，低 9 位 位来选择最终的 PTE。 （在 Sv48 RISC-V 中，页表有四层，虚拟地址索引的第 39 位到第 47 位进入顶层。）

如果转换地址所需的三个 PTE 中的任何一个不存在，分页硬件将引发页面错误异常，将其留给内核来处理异常（参见第 4 章）。

与图 3.1 的单层设计相比，图 3.2 的三层结构提供了一种记录 PTE 的内存高效方式。 在大范围虚拟地址没有映射的常见情况下，三级结构可以省略整个页目录。 例如，如果一个应用程序只使用从地址 0 开始的几个页面，那么顶级页面目录的条目 1 到 511 是无效的，内核不必为 511 中间页面目录分配页面 . 此外，内核也不必为那些 511 中间页目录为底层页目录分配页面。 因此，在本例中，三级设计为中间页目录节省了 511 页，为底层页目录节省了 511 × 512 页。

尽管 CPU 在硬件中遍历三级结构作为执行加载或存储指令的一部分，但三级的潜在缺点是 CPU 必须从内存加载三个 PTE 以执行加载/存储中的虚拟地址转换 一个物理地址的指令。 为了避免从物理内存加载 PTE 的成本，RISC-V CPU 将页表条目缓存在转换后备缓冲区 (TLB) 中。

每个 PTE 都包含标志位，告诉分页硬件如何允许使用关联的虚拟地址。 PTE_V 指示 PTE 是否存在：如果未设置，则对页面的引用会导致异常（即不允许）。 PTE_R 控制是否允许指令读取页面。 PTE_W 控制是否允许指令写入页面。 PTE_X 控制 CPU 是否可以将页面内容解释为指令并执行它们。 PTE_U 控制是否允许用户态指令访问页面； 如果未设置 PTE_U，则 PTE 只能在管理员模式下使用。 图 3.2 展示了它是如何工作的。 标志和所有其他页面硬件相关结构在 (kernel/riscv.h) 中定义

要告诉 CPU 使用页表，内核必须将根页表页的物理地址写入 satp 寄存器。 CPU 将使用它自己的 satp 指向的页表来翻译后续指令生成的所有地址。 每个 CPU 都有自己的 satp，这样不同的 CPU 可以运行不同的进程，每个进程都有自己的页表描述的私有地址空间。

通常，内核将所有物理内存映射到其页表中，以便它可以使用加载/存储指令读取和写入物理内存中的任何位置。 由于页目录在物理内存中，内核可以通过使用标准存储指令写入 PTE 的虚拟地址来在页目录中编程 PTE 的内容。
关于术语的一些注释。 物理内存是指 DRAM 中的存储单元。 一个字节的物理内存有一个地址，称为物理地址。 指令只使用虚拟地址，分页硬件将其转换为物理地址，然后发送到 DRAM 硬件以读取或写入存储。 与物理内存和虚拟地址不同，虚拟内存不是物理对象，而是指内核提供的用于管理物理内存和虚拟地址的抽象和机制的集合。

## 3.2 Kernel address space

xv6 为每个进程维护一个页表，描述每个进程的用户地址空间，以及一个描述内核地址空间的页表。 内核配置其地址空间布局，使其能够访问可预测虚拟地址处的物理内存和各种硬件资源。 图 3.3 显示了此布局如何将内核虚拟地址映射到物理地址。 文件 (kernel/memlayout.h) 声明了 xv6 内核内存布局的常量。

QEMU 模拟一台包含 RAM（物理内存）的计算机，从物理地址 0x80000000 开始并持续到至少 0x88000000，xv6 称之为 PHYSTOP。 QEMU 模拟还包括 I/O 设备，例如磁盘接口。 QEMU 将设备接口暴露给软件作为内存映射控制寄存器，这些寄存器位于物理地址空间中的 0x80000000 以下。 内核可以通过读/写这些特殊的物理地址来与设备进行交互； 这样的读写与设备硬件而不是 RAM 通信。 第 4 章解释 xv6 如何与设备交互。

内核使用“直接映射”获取 RAM 和内存映射设备寄存器； 也就是说，将资源映射到与物理地址相同的虚拟地址。 例如，内核本身在虚拟地址空间和物理内存中都位于 KERNBASE=0x80000000。 直接映射简化了读取或写入物理内存的内核代码。 例如，当fork为子进程分配用户内存时，allocator返回那块内存的物理地址； fork 在将父进程的用户内存复制到子进程时，直接使用该地址作为虚拟地址。

有几个内核虚拟地址不是直接映射的：

* 蹦床页面。 它被映射到虚拟地址空间的顶部； 用户页表具有相同的映射。 第 4 章讨论了 trampoline 页面的作用，但我们在这里看到了一个有趣的页表用例； 物理页面（保存蹦床代码）在内核的虚拟地址空间中映射两次：一次在虚拟地址空间的顶部，一次直接映射。
* 内核堆栈页面。 每个进程都有自己的内核栈，它被映射到高位，这样 xv6 可以在它下面留下一个未映射的保护页。 保护页的 PTE 无效（即 PTE_V 未设置），因此如果内核溢出内核堆栈，很可能会导致异常，内核会 panic。 如果没有保护页，溢出的堆栈会覆盖其他内核内存，从而导致不正确的操作。 恐慌崩溃是可取的。

虽然内核通过高端内存映射使用其堆栈，但内核也可以通过直接映射地址访问它们。 另一种设计可能只有直接映射，并在直接映射地址处使用堆栈。 然而，在这种安排中，提供保护页将涉及取消映射虚拟地址，否则这些虚拟地址将引用物理内存，这将很难使用。

内核使用 PTE_R 和 PTE_X 权限映射 trampoline 和内核文本的页面。 内核从这些页面读取并执行指令。 内核用PTE_R 和PTE_W 权限映射其他页面，以便它可以读写这些页面中的内存。 防护页的映射无效。

# Lab: system calls

## Using gdb

### 问题 2：p->trapframe->a7 的值是多少，表示什么？

文档里说用 p/x *p 查看 p，这样只能看到 trapframe 这个指针的值。这个指针指向一个结构体：

```c
// ==================== user/init.c 31 80 ====================
// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};
```

这个结构体里含有一个 a7 成员。我认为这个结构体里保存的是各种寄存器的值。我们可以用 p p->trapframe->a7 来直接查看 a7 成员的值（gdb 甚至可以自动补全），a7 的值显示为 7。

此时的 p 刚刚经过 myproc() 的赋值：

```c
// ==================== kernel/proc.c 71 90 ====================
// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}
```

我认为这里并没有修改 c->proc->trapframe 的值，它的值应该是被预设好的。

2.2.2 节写到：使用 uvmfirst() 把 inicode.S 编译好的二进制程序直接加载到特定位置。inicode.S 如下：

```assembly
# Initial process that execs /init.
# This code runs in user space.

#include "syscall.h"

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

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

```

注意，它把 SYS_exec 放入了 a7，而 SYS_exec 在 syscall.h 中定义为 7：

```c
// System call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
```

### 问题 3：CPU 在什么态（mode）

文档给出的指令是 p/x $sstatus，这个指令可以打印指定寄存器 sstatus 的值。也可以用 info register 或简写为 i register 打印寄存器的值，但是不包括浮点寄存器和向量寄存器的值，i all-registers 是全部打印。

### 问题4：写出内核 panic 的位置，哪个寄存器存了变量 num？

xv6 内核崩溃的时候会输出 scause、sepc 和 stval 三个寄存器的值：

```shell
scause 0x000000000000000d
sepc=0x0000000080001ff8 stval=0x0000000000000000
```

Supervisor Cause Register（scause）保存的是导致 trap 的事件的编号，这里的编号是 d 也就是 13 load page fault。

Supervisor Scratch Register（sscratch）保存的是导致 trap 的指令的虚拟地址。

Supervisor Trap Value Register （stval）没看懂，暂时够用了。

所以内核 panic 的位置就是 sepc 保存的值，0x0000000080001ff8，执行的指令是

```assembly
 0x80001ff8 <syscall+20> lw      a3,0(zero) # 0x0   
```

看来就是把 0 往寄存器 a3 里装，所以是寄存器 a3 保存了 num。

我们也可以用 gbd 的 ni 来调试，表示执行下一条 instruction。也会发现执行到这条就 panic 了。


# Lab: page tables

## Speed up system calls
第一个用户态进程由 userinit() 创建，采用了 allocproc() 函数生成进程控制块 struct proc。之后所有进程都使用 fork() 创建，同样采用 allocproc()。

allocproc() 首先遍历 proc 数组，这个数组是一个进程空间块数组，找到一个状态为 UNUSED 的进程控制块。它目前没有控制任何进程，就用它来控制即将生成的进程。

找到空闲进程控制块后生成 pid，修改进程控制块状态为 USED。然后分配 trapframe 页。p->trapframe 保存在这个页中。而 p->trapframe 是一个保存了各个寄存器的值的数据结构。usyscall 也是通过相似的方法得到一个页面。注意这两个页面都是通过 kalloc 分配，这是一个定义在内核中的函数，不能由用户态进程访问，所分配的页面也属于内核虚存空间，因此这两个页面也不对用户态进程开放。
随后建立进程的页表。proc_pagetable() 负责完成这个任务。先用 uvmcreate 申请一个空页，这个页就是 2 级页表了。然后往这个页表上挂 trampoline trapframe 和 usyscall 三个页。注意它们的权限，trampoline 和 trapframe 页都没有 PTE_U 这个标志项，用户没法访问他们，那些 PTE_R PTE_W 和 PTE_X 都是针对具有内核级别权限的指令而言的。
而 usyscall 页面则有所不同，设置了 PTE_U，使得这个页面允许用户进行操作，但是也只允许进行 PTE_R 也就是读操作。写操作是在 allocproc() 中最后完成的，内核代码堪当其任。

进程终止时使用 freeproc 释放进程控制块。首先要把 trapframe usyscall 占用的页面释放掉，即 kfree，然后取消它们在页表中建立的映射即 proc_freepagetable->uvmunmap，最后把页表也释放了。

## Detect which pages have been accessed
这个题不难，我觉得没有 Speed up system calls 难。

关键点：
* PTE_A 这个标志位可以由硬件自动设置，也可以受到内核态代码控制。当我们通过 buf[PGSIZE * 1] += 1; 之类的方法访问一个页面时，它的 PTE_A 就被自动置 1 了。我们也可以找到页表项，然后用内核态代码来修改 PTE_A 这个标志位。
* PTE_A 这个标志位是第 7 位，这是硬件规定的。一个页面被访问了，就把第 7 位置 1。你定义它是第 6 位硬件也不知道，也不能按你的实现。
* pgaccess 这个函数查看页面的使用情况。但是只查看页表项 PTE 的话其实并不会影响页面的访问情况。但是这个函数正确的功能应该是查看后就把 PTE_A 置 0，因为这样才能确定下次调用 pgaccess 和这次调用 pgaccess 之间是否访问了这个页面。