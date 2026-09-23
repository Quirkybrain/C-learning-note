# 从文件描述符到文件偏移量：用 Linux 系统调用完成一次文件读写

- 作者：Quirkybrain
- GitHub 仓库：[Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

> [!NOTE]
> ### 📝 原创与 AI 辅助说明
> 
> 本项目坚持开放、透明的技术记录原则，特此说明：
> 1. **核心原创**：仓库中所有示例代码、课程步骤演进体系以及各章节的剖析思路，均由作者Quirkybrain设计完成。
> 2. **AI 辅助范围**：各章节的配套说明文档是基于作者提供的核心思路、提纲与推导脉络，由 AI 进行语言润色与细节扩写；源码中的部分注释亦在 AI 辅助下进行了补充与可读性改良。
> 3. **人工核验**：所有经由 AI 润色的说明文章与改良注释，均由作者Quirkybrain进行了检查与技术校对，确认完全符合本系列课程的核心内容与教学设计思想。

学习 C 语言文件操作时，我们通常会先接触 `fopen()`、`fprintf()`、`fread()` 和 `fclose()`。这些函数使用 `FILE*` 表示文件，并在标准库中帮我们处理了缓冲等细节。

这份示例换一个更靠近 Linux 底层文件 I/O 的视角，直接使用：

- `open()` 打开或创建文件。
- `write()` 写入原始字节。
- `lseek()` 修改文件偏移量。
- `read()` 读取原始字节。
- `close()` 释放文件描述符。

整个过程可以概括为：

```text
open 打开 file.txt
    ↓
write 写入 "Hello Linux\n"
    ↓
lseek 把文件偏移量移回开头
    ↓
read 重新读出文件内容
    ↓
write 把读到的字节输出到终端
    ↓
close 关闭文件
```

这个例子的代码不复杂，但它刚好串起了 Linux 文件 I/O 中几个很重要的概念：文件描述符、字节数、文件偏移量和 C 字符串结束符。

## 构建与运行

进入当前目录后执行：

```bash
make
make run
```

输出示例：

```text
Successfully wrote the file.
Successfully repositioned the file offset.
Successfully read the file.
Hello Linux
```

程序还会在当前目录生成或重写 `file.txt`，其内容为：

```text
Hello Linux
```

## 文件结构

```text
linux-c/system-programming/001-basic-file-io
├── src
│   └── unistd.c   # 系统调用示例
├── README.md       # 本篇学习笔记
├── README-en.md    # 英文版学习笔记
├── Makefile        # 构建脚本
└── file.txt        # 程序运行后生成的测试文件
```

`Makefile` 会将 `src/unistd.c` 编译为 `src/unistd.o`，再在当前目录链接生成可执行文件 `file_io`。

## 文件描述符：用一个整数表示打开的文件

标准 I/O 使用 `FILE*`，而 `open()` 返回的是一个 `int`：

```c
int fileDescriptor = open(
        fileName,
        O_RDWR | O_CREAT | O_TRUNC,
        0644
);
```

这个整数就是文件描述符，简称 `fd`。

可以先把它理解成：当前进程用来找到某个已打开文件的编号。后面的 `read()`、`write()`、`lseek()` 和 `close()` 都不再使用文件名，而是使用这个文件描述符。

Linux 进程启动时通常已经拥有三个标准文件描述符：

```text
STDIN_FILENO   = 0   标准输入
STDOUT_FILENO  = 1   标准输出
STDERR_FILENO  = 2   标准错误
```

所以我们不仅可以向 `fileDescriptor` 写入文件，也可以直接向 `STDOUT_FILENO` 或 `STDERR_FILENO` 写入终端。

## open：打开文件并设置初始状态

`open()` 的公开接口可以简化写成：

```c
int open(const char *path, int flags, ...);
```

当 `flags` 中包含 `O_CREAT` 时，后面还需要提供创建权限参数。

本例使用了三个标志：

```c
O_RDWR | O_CREAT | O_TRUNC
```

它们的作用分别是：

- `O_RDWR`：以可读、可写方式打开，因为后面同时会使用 `write()` 和 `read()`。
- `O_CREAT`：如果文件不存在，就创建它。
- `O_TRUNC`：如果文件已存在，将它的长度截断为 0，也就是清空原内容。

第三个参数 `0644` 是创建文件时请求的权限：

```text
0 6 4 4
  │ │ │
  │ │ └─ 其他用户：只读
  │ └── 同组用户：只读
  └─── 文件所有者：可读可写
```

实际生成的权限还会受当前进程 `umask` 的影响。在这个示例里，先记住 `0644` 表示“所有者可读写，其他人只读”即可。

`open()` 成功时返回一个非负整数，失败时返回 `-1`：

```c
if (fileDescriptor == -1) {
        /* 输出错误信息并结束程序 */
}
```

## write：向文件写入原始字节

`write()` 的接口是：

```c
ssize_t write(int fd, const void *buf, size_t count);
```

三个参数分别表示：

- `fd`：要写入的文件描述符。
- `buf`：待写入数据所在的内存地址。
- `count`：最多写入多少个字节。

本例要写入的文本是：

```c
static const char text[] = "Hello Linux\n";
```

它在内存中实际还有一个字符串结束符：

```text
H e l l o   L i n u x \n \0
└─────── 12 字节 ──────┘  └─ C 字符串结束符
```

`write()` 不理解 C 字符串，也不会自动寻找 `\0`。它只会严格写出 `count` 指定的字节数。因此这里使用：

```c
ssize_t writeBytes = write(
        fileDescriptor,
        text,
        sizeof(text) - 1
);
```

`sizeof(text)` 包含末尾的 `\0`，减去 1 后才是需要写入文本文件的字节数。

`write()` 成功时返回实际写入的字节数，失败时返回 `-1`。这里的单位是“字节”，而不是“字符”。

## 写入之后，文件偏移量去了哪里

文件描述符不只是让内核知道“操作哪个文件”，对应的打开文件状态还会记录当前偏移量。

刚打开并清空文件时，偏移量从 0 开始：

```text
Hello Linux\n
^
偏移量 0
```

写入 12 个字节后，偏移量会向后移动 12 个字节，此时它已经在文件末尾：

```text
Hello Linux\n
             ^
             偏移量 12，也是 EOF
```

如果这时直接调用 `read()`，就会从偏移量 12 开始读。因为那里已经是文件末尾，所以 `read()` 会返回 0，表示读到 EOF。

这就是本例必须在 `write()` 和 `read()` 之间加入 `lseek()` 的原因。

## lseek：把文件偏移量移回开头

`lseek()` 的接口是：

```c
off_t lseek(int fd, off_t offset, int whence);
```

本例的调用是：

```c
off_t lseekResult = lseek(fileDescriptor, 0, SEEK_SET);
```

可以读作：

```text
以文件开头 SEEK_SET 为基准，
再向后偏移 0 个字节。
```

所以结果就是把当前文件偏移量设置为 0。

`whence` 常用的三个取值是：

| 值 | 偏移基准 |
| --- | --- |
| `SEEK_SET` | 文件开头 |
| `SEEK_CUR` | 当前偏移量 |
| `SEEK_END` | 文件末尾 |

`lseek()` 成功时返回调整后的偏移量，失败时返回 `-1`。

需要注意的是，不是所有文件描述符都能使用 `lseek()`。普通文件通常可以，而管道这种字节流没有可以随意跳转的位置。

## read：从当前偏移量开始读取

`read()` 的接口是：

```c
ssize_t read(int fd, void *buf, size_t count);
```

本例准备了一个 100 字节的缓冲区：

```c
char buffer[100];
```

然后从文件中读取数据：

```c
ssize_t readBytes = read(
        fileDescriptor,
        buffer,
        sizeof(buffer)
);
```

这里可以使用完整的 `sizeof(buffer)`，因为后面不会把 `buffer` 交给 `printf("%s")`、`strlen()` 等 C 字符串函数。

`read()` 不会自动在末尾添加 `\0`。它只负责把字节放进缓冲区，并通过返回值告诉我们实际放了多少字节：

- 大于 0：实际读到的字节数。
- 等于 0：已经到达 EOF。
- 等于 `-1`：读取失败。

因此输出时要使用 `readBytes` 作为长度：

```c
write(
        STDOUT_FILENO,
        buffer,
        (size_t)readBytes
);
```

这里的 `write()` 不需要 `buffer` 以 `\0` 结尾，因为它已经从第三个参数得到了准确的字节数。

如果后面要将数据当成 C 字符串使用，写法才需要变成：

```c
ssize_t readBytes = read(
        fileDescriptor,
        buffer,
        sizeof(buffer) - 1
);

if (readBytes >= 0) {
        buffer[readBytes] = '\0';
}
```

这里的 `-1` 不是 `read()` 的要求，而是为了给 C 字符串结束符留一个位置。

## 标准输出和标准错误也是文件描述符

示例中输出状态信息时没有使用 `printf()`，而是继续使用 `write()`：

```c
const char successText[] = "Successfully read the file.\n";

write(
        STDOUT_FILENO,
        successText,
        sizeof(successText) - 1
);
```

出错时则写入 `STDERR_FILENO`：

```c
const char errorText[] = "Failed to read the file.\n";

write(
        STDERR_FILENO,
        errorText,
        sizeof(errorText) - 1
);
```

这里再次体现了 Linux “一切皆文件”这种接口风格：普通文件、终端输出和错误输出都可以通过文件描述符交给 `write()`。

## close：释放文件描述符

完成读写后，调用 `close()`：

```c
close(fileDescriptor);
```

`close()` 的接口是：

```c
int close(int fd);
```

关闭后，这个文件描述符就不能再继续用于 `read()`、`write()` 或 `lseek()`。

`close()` 成功时返回 0，失败时返回 `-1` 并设置 `errno`。这份代码的目标是展示主要读写流程，所以没有继续展开 `close()` 错误处理。

## 把整个调用链串起来

把错误输出和接口说明暂时折叠起来，这份代码的主干就是：

```c
int fileDescriptor = open(
        "file.txt",
        O_RDWR | O_CREAT | O_TRUNC,
        0644
);

write(fileDescriptor, text, sizeof(text) - 1);

lseek(fileDescriptor, 0, SEEK_SET);

ssize_t readBytes = read(
        fileDescriptor,
        buffer,
        sizeof(buffer)
);

write(STDOUT_FILENO, buffer, (size_t)readBytes);

close(fileDescriptor);
```

真正要理解的不只是这五个函数名，而是它们之间的状态变化：

```text
文件不存在或保留旧内容
    ↓ open(O_CREAT | O_TRUNC)
文件已打开，长度为 0，偏移量为 0
    ↓ write(12 bytes)
文件长度为 12，偏移量为 12
    ↓ lseek(fd, 0, SEEK_SET)
文件长度为 12，偏移量回到 0
    ↓ read(...)
读到 12 字节，偏移量再次到达 12
    ↓ close(fd)
文件描述符被释放
```

## 这个示例解决了什么，还没展开什么

这份入门示例已经展示了：

- 如何用 `open()` 得到文件描述符。
- 如何用 `write()` 将内存中的字节写入文件。
- 为什么写入后需要用 `lseek()` 重置偏移量。
- 如何用 `read()` 获得原始字节和实际长度。
- `\0` 与文本有效字节数之间的区别。
- 如何用 `STDOUT_FILENO` 和 `STDERR_FILENO` 输出信息。
- 如何用 `close()` 结束本次文件操作。

为了保持学习主线清晰，这份代码暂时没有展开：

- 循环处理短读和短写。
- 处理系统调用被信号中断的 `EINTR`。
- 通过 `errno` 输出更具体的错误原因。
- 使用 `fsync()` 请求数据同步。
- 对 `close()` 的错误进行完整处理。

这些内容很重要，但不影响当前例子要表达的核心过程。先把“文件描述符 + 文件偏移量 + 原始字节读写”建立起来，后面再逐步加入更完整的错误处理。

## 如何观察程序真正发生了哪些调用

除了直接阅读代码，还可以用 `strace` 观察程序与 Linux 内核之间的调用：

```bash
strace -e trace=openat,read,write,lseek,close ./file_io
```

在一些平台和 C 库实现中，源代码里的 `open()` 可能会在 `strace` 中显示为 `openat()`。这是因为用户代码通常是通过 C 库包装进入内核，而具体底层调用可以由 C 库选择。

这一步很适合学习系统调用：代码告诉我们“想做什么”，`strace` 则帮我们看到程序运行时“实际请求了内核做什么”。

## 小结

这份代码展示了一条最基础、也最值得先建立起来的 Linux 文件 I/O 链路：

```text
open -> write -> lseek -> read -> close
```

- `open()` 创建这个与已打开文件之间的联系。
- `write()` 写入字节，同时推进文件偏移量。
- `lseek()` 显式修改文件偏移量。
- `read()` 从当前偏移量读取字节，并再次推进偏移量。
- `close()` 释放文件描述符。

用一句话概括就是：

> Linux 底层文件 I/O 不依赖 `\0` 判断数据结尾，而是通过“文件描述符 + 明确的字节数 + 当前文件偏移量”来完成数据读写。
