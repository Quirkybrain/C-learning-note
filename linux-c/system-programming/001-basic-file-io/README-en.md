# From File Descriptors to File Offsets: A Basic Linux File I/O Example

- Author: Quirkybrain
- GitHub Repository: [Quirkybrain/C-learning-note](https://github.com/Quirkybrain/C-learning-note)

When learning file operations in C, we usually begin with `fopen()`, `fprintf()`, `fread()`, and `fclose()`. These functions represent a file with a `FILE*` and let the C standard library handle details such as buffering.

This example takes a perspective closer to low-level Linux file I/O and directly uses:

- `open()` to open or create a file.
- `write()` to write raw bytes.
- `lseek()` to change the file offset.
- `read()` to read raw bytes.
- `close()` to release the file descriptor.

The complete process can be summarized as follows:

```text
open file.txt
    ↓
write "Hello Linux\n"
    ↓
use lseek to move the file offset back to the beginning
    ↓
read the file contents again
    ↓
write the bytes that were read to the terminal
    ↓
close the file
```

The code is small, but it connects several important Linux file I/O concepts: file descriptors, byte counts, file offsets, and the C string terminator.

## Building and Running

Run the following commands inside the current directory:

```bash
make
make run
```

Example output:

```text
Successfully wrote the file.
Successfully repositioned the file offset.
Successfully read the file.
Hello Linux
```

The program also creates or overwrites `file.txt` in the current directory. Its contents are:

```text
Hello Linux
```

## Project Structure

```text
linux-c/system-programming/001-basic-file-io
├── src
│   └── unistd.c   # System call example
├── README.md       # Chinese learning note
├── README-en.md    # English learning note
├── Makefile        # Build script
└── file.txt        # Test file generated after running the program
```

The `Makefile` compiles `src/unistd.c` into `src/unistd.o`, then links it into the `file_io` executable in the current directory.

## File Descriptors: Representing an Open File with an Integer

The standard I/O library uses `FILE*`, while `open()` returns an `int`:

```c
int fileDescriptor = open(
        fileName,
        O_RDWR | O_CREAT | O_TRUNC,
        0644
);
```

This integer is a file descriptor, commonly abbreviated as `fd`.

For now, it can be understood as a number that the current process uses to locate an open file. Later calls to `read()`, `write()`, `lseek()`, and `close()` use this file descriptor instead of the file name.

A Linux process usually starts with three standard file descriptors already open:

```text
STDIN_FILENO   = 0   standard input
STDOUT_FILENO  = 1   standard output
STDERR_FILENO  = 2   standard error
```

Therefore, we can write not only to `fileDescriptor`, but also directly to the terminal through `STDOUT_FILENO` or `STDERR_FILENO`.

## `open()`: Opening a File and Setting Its Initial State

A simplified form of the public `open()` interface is:

```c
int open(const char *path, int flags, ...);
```

When `flags` contains `O_CREAT`, an additional argument must be provided to specify the requested permissions for a newly created file.

This example uses three flags:

```c
O_RDWR | O_CREAT | O_TRUNC
```

They have the following meanings:

- `O_RDWR`: open the file for both reading and writing, because the example later calls both `write()` and `read()`.
- `O_CREAT`: create the file if it does not already exist.
- `O_TRUNC`: if the file already exists, truncate its length to zero and discard its old contents.

The third argument, `0644`, specifies the requested permissions when creating the file:

```text
0 6 4 4
  │ │ │
  │ │ └─ other users: read only
  │ └── group users: read only
  └─── file owner: read and write
```

The final permissions are also affected by the process's current `umask`. For this introductory example, it is enough to remember that `0644` requests read and write permission for the owner and read-only permission for everyone else.

On success, `open()` returns a nonnegative integer. On failure, it returns `-1`:

```c
if (fileDescriptor == -1) {
        /* Print an error message and terminate the program. */
}
```

## `write()`: Writing Raw Bytes to a File

The `write()` interface is:

```c
ssize_t write(int fd, const void *buf, size_t count);
```

Its three arguments mean:

- `fd`: the file descriptor to write to.
- `buf`: the memory address containing the data to write.
- `count`: the maximum number of bytes to write.

The text written by this example is:

```c
static const char text[] = "Hello Linux\n";
```

In memory, it also contains a string terminator:

```text
H e l l o   L i n u x \n \0
└─────── 12 bytes ──────┘  └─ C string terminator
```

`write()` does not understand C strings and does not search for `\0`. It writes exactly the number of bytes requested by `count`. Therefore, this example uses:

```c
ssize_t writeBytes = write(
        fileDescriptor,
        text,
        sizeof(text) - 1
);
```

`sizeof(text)` includes the trailing `\0`. Subtracting one gives the number of text bytes that should be written to the file.

On success, `write()` returns the number of bytes actually written. On failure, it returns `-1`. The unit here is bytes, not characters.

## Where Does the File Offset Go After Writing?

A file descriptor does more than identify which file the kernel should operate on. The corresponding open-file state also records the current file offset.

Immediately after the file is opened and truncated, the offset starts at zero:

```text
Hello Linux\n
^
offset 0
```

After 12 bytes are written, the offset advances by 12 bytes and reaches the end of the file:

```text
Hello Linux\n
             ^
             offset 12, also EOF
```

If `read()` were called immediately at this point, it would start reading at offset 12. Because that position is already the end of the file, `read()` would return zero to indicate EOF.

This is why the example needs an `lseek()` call between `write()` and `read()`.

## `lseek()`: Moving the File Offset Back to the Beginning

The `lseek()` interface is:

```c
off_t lseek(int fd, off_t offset, int whence);
```

This example calls it as follows:

```c
off_t lseekResult = lseek(fileDescriptor, 0, SEEK_SET);
```

This can be read as:

```text
use the beginning of the file, SEEK_SET, as the reference point,
then move forward by 0 bytes.
```

The result is that the current file offset is set to zero.

The three commonly used values for `whence` are:

| Value | Offset reference point |
| --- | --- |
| `SEEK_SET` | Beginning of the file |
| `SEEK_CUR` | Current file offset |
| `SEEK_END` | End of the file |

On success, `lseek()` returns the resulting file offset. On failure, it returns `-1`.

Not every file descriptor supports `lseek()`. Regular files usually do, while byte streams such as pipes do not have a position that can be moved arbitrarily.

## `read()`: Reading from the Current File Offset

The `read()` interface is:

```c
ssize_t read(int fd, void *buf, size_t count);
```

This example prepares a 100-byte buffer:

```c
char buffer[100];
```

It then reads data from the file:

```c
ssize_t readBytes = read(
        fileDescriptor,
        buffer,
        sizeof(buffer)
);
```

The full `sizeof(buffer)` can be used here because the program does not later pass `buffer` to C string functions such as `printf("%s")` or `strlen()`.

`read()` does not automatically append `\0`. It only places bytes into the buffer and reports how many bytes were actually read through its return value:

- Greater than zero: the number of bytes actually read.
- Equal to zero: EOF has been reached.
- Equal to `-1`: the read failed.

The output operation therefore uses `readBytes` as its length:

```c
write(
        STDOUT_FILENO,
        buffer,
        (size_t)readBytes
);
```

This `write()` call does not require `buffer` to end with `\0`, because its third argument already provides the exact byte count.

If the data were later going to be used as a C string, the code would instead need to reserve one byte for the terminator:

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

The `-1` here is not required by `read()`. It reserves one position for the C string terminator.

## Standard Output and Standard Error Are File Descriptors Too

The example uses `write()` rather than `printf()` to print status messages:

```c
const char successText[] = "Successfully read the file.\n";

write(
        STDOUT_FILENO,
        successText,
        sizeof(successText) - 1
);
```

Errors are written to `STDERR_FILENO`:

```c
const char errorText[] = "Failed to read the file.\n";

write(
        STDERR_FILENO,
        errorText,
        sizeof(errorText) - 1
);
```

This demonstrates the Linux interface style often summarized as "everything is a file": regular files, terminal output, and error output can all be passed to `write()` through file descriptors.

## `close()`: Releasing the File Descriptor

After the read and write operations are complete, the program calls `close()`:

```c
close(fileDescriptor);
```

The `close()` interface is:

```c
int close(int fd);
```

After it has been closed, the file descriptor can no longer be used with `read()`, `write()`, or `lseek()`.

`close()` returns zero on success. On failure, it returns `-1` and sets `errno`. This example focuses on the main file I/O sequence, so it does not expand on error handling for `close()`.

## Putting the Complete Call Chain Together

If the error messages and interface explanations are temporarily folded away, the main path of the program is:

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

The important part is not only the names of these five functions, but also the state changes between them:

```text
the file does not exist or still contains old data
    ↓ open(O_CREAT | O_TRUNC)
the file is open, its length is 0, and its offset is 0
    ↓ write(12 bytes)
the file length is 12 and the offset is 12
    ↓ lseek(fd, 0, SEEK_SET)
the file length is 12 and the offset is back at 0
    ↓ read(...)
12 bytes have been read and the offset reaches 12 again
    ↓ close(fd)
the file descriptor is released
```

## What This Example Covers and What It Leaves for Later

This introductory example demonstrates:

- How to obtain a file descriptor with `open()`.
- How to write bytes from memory to a file with `write()`.
- Why `lseek()` is needed to reset the offset after writing.
- How to obtain raw bytes and their actual length with `read()`.
- The difference between `\0` and the number of meaningful text bytes.
- How to write messages through `STDOUT_FILENO` and `STDERR_FILENO`.
- How to finish the file operation with `close()`.

To keep the main learning path clear, the example does not yet cover:

- Looping to handle short reads and short writes.
- Handling `EINTR` when a system call is interrupted by a signal.
- Reporting more detailed errors through `errno`.
- Requesting data synchronization with `fsync()`.
- Complete error handling for `close()`.

These topics are important, but they do not change the core process demonstrated here. The first goal is to establish the relationship between file descriptors, file offsets, and raw byte I/O. More complete error handling can be added later.

## Observing the Calls the Program Actually Makes

In addition to reading the source code, `strace` can be used to observe the calls between the program and the Linux kernel:

```bash
strace -e trace=openat,read,write,lseek,close ./file_io
```

On some platforms and C library implementations, the source-level `open()` call may appear as `openat()` in `strace`. User code normally enters the kernel through C library wrappers, and the library may select a different underlying system call.

This is especially useful when learning system calls: the source code tells us what the program intends to do, while `strace` shows what it actually asks the kernel to do at runtime.

## Summary

This example demonstrates one of the most fundamental Linux file I/O sequences:

```text
open -> write -> lseek -> read -> close
```

- `open()` establishes the connection to an open file.
- `write()` writes bytes and advances the file offset.
- `lseek()` changes the file offset explicitly.
- `read()` reads bytes from the current offset and advances it again.
- `close()` releases the file descriptor.

In one sentence:

> Low-level Linux file I/O does not depend on `\0` to determine where data ends. It uses a file descriptor, an explicit byte count, and the current file offset to read and write data.
