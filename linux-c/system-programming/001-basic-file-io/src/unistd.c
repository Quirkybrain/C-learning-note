/**
 * @file unistd_test.c
 * @author Quirkybrain
 * @brief linux 系统调用 open() write() lseek() read() close()
 * @version 0.1
 * @date 2026-08-10
 * 
 */
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


int main(void) {

        // 要操作的文件名和待写入的文本
        static const char fileName[] = "file.txt";
        static const char text[] = "Hello Linux\n";

        // read() 读取原始字节时使用的缓冲区
        char buffer[100];

        /**
         * @brief 
         * <fcntl.h>
         * int open (const char *path, int oflag, ...)
         * 
         * @param path        打开文件的路径
         * 
         * @param oflag       打开文件的模式
         *                      （1）O_RDONLY 只读
         *                      （2）O_WRONLY 只写
         *                      （3）O_RDWR   读写
         *                      （4）O_CREAT  如果不存在创建文件
         *                      （5）O_APPEND 追加写
         *                      （6）O_TRUNC  截断文件长度为0
         * 
         * @param ...           文件权限
         * 
         * @return 文件描述符，如果打开失败返回 -1, 同时设置变量 errno 表示对应错误
         * 
         * @note
         *            0 6 4 4
         *              │ │ │
         *              │ │ └─ 其他用户权限
         *              │ └─── 同组用户权限
         *              └───── 文件所有者权限
         *              计算规则：
         *                      读 r = 4
         *                      写 w = 2
         *                      执行 x = 1
         *              常见权限：
         *                      0644 → rw-r--r--    普通文本文件最常见
         *                      0600 → rw-------    只有自己能读写
         *                      0666 → rw-rw-rw-    所有人都可读写
         *                      0755 → rwxr-xr-x    可执行程序/目录常见
         *                      0700 → rwx------    只有自己有全部权限
         * 
         */
        // 以读写方式打开文件：不存在则创建，存在则清空
        int fileDescriptor = open(
                fileName,
                O_RDWR | O_CREAT | O_TRUNC,
                0644
        );

        if (fileDescriptor == -1) {
                const char errorText[] = "Failed to open the file.\n";
                write(
                        STDERR_FILENO,
                        errorText,
                        sizeof(errorText) - 1
                );
                exit(EXIT_FAILURE);
        }

        /**
         * @brief 
         * <unistd.h>
         * ssize_t write (int fd, const void *buf, size_t n)
         * 
         * @param fd          文件描述符
         * 
         * @param buf         缓存区
         * 
         * @param n           写入的长度
         * 
         * @return 写入成功返回写入的字节数，-1 表示错误
         * 
         * @note        Standard file descriptors.
         *              STDIN_FILENO	        0	Standard input.
         *              STDOUT_FILENO	        1       Standard output.
         *              STDERR_FILENO	        2	Standard error output.
         */
        // 将文本写入文件，-1 用于排除字符串末尾的 '\0'
        ssize_t writeBytes = write(
                fileDescriptor,
                text,
                sizeof(text) - 1
        );

        if (writeBytes == -1) {
                const char errorText[] = "Failed to write the file.\n";
                
                write(
                        STDERR_FILENO,
                        errorText,
                        sizeof(errorText) - 1
                );
                close(fileDescriptor);
                exit(EXIT_FAILURE);
        } else {
                const char successText[] = "Successfully wrote the file.\n";
                
                write(
                        STDOUT_FILENO,
                        successText,
                        sizeof(successText) - 1
                );
        }

        /**
         * @brief
         * <unistd.h>
         * off_t lseek (int fd, off_t offset, int whence)
         * 
         * @param fd          文件描述符
         * 
         * @param offset      偏移量
         * 
         * @param whence      基准
         * 
         * @return 成功则返回文件的偏移量，-1 表示失败
         * 
         * @note        Values for the WHENCE argument to lseek.
         *              SEEK_SET        从文件开头开始
         *              SEEK_CUR        从当前位置开始
         *              SEEK_END        从文件末尾开始
         */
        // 写入后偏移量在文件末尾，先回到开头再读取
        off_t lseekResult = lseek(fileDescriptor, 0, SEEK_SET);

        if (lseekResult == -1) {
                const char errorText[] = "Failed to seek the file.\n";
                write(
                        STDERR_FILENO,
                        errorText,
                        sizeof(errorText) - 1
                );

                close(fileDescriptor);
                exit(EXIT_FAILURE);
        } else {
                const char successText[] = "Successfully repositioned the file offset.\n";
                
                write(
                        STDOUT_FILENO,
                        successText,
                        sizeof(successText) - 1
                );
        }

        /**
         * @brief 
         * <unistd.h>
         * ssize_t read (int fd, void *buf, size_t nbytes)
         * 
         * @param fd          文件描述符
         * 
         * @param buf         缓存区
         * 
         * @param n           读取的长度
         * 
         * @return 读取成功则返回读到的字节数，
         *         -1 表示错误, 0 表示读到 EOF
         * 
         * @note        Standard file descriptors.
         *              STDIN_FILENO	        0	Standard input.
         *              STDOUT_FILENO	        1       Standard output.
         *              STDERR_FILENO	        2	Standard error output.
         */
        // 从文件开头读取数据，read() 不会自动添加 '\0'
        ssize_t readBytes = read (
                fileDescriptor,
                buffer,
                sizeof(buffer)
        );


        if (readBytes == -1) {
                const char errorText[] = "Failed to read the file.\n";
                write(
                        STDERR_FILENO,
                        errorText,
                        sizeof(errorText) - 1
                );
                close(fileDescriptor);
                exit(EXIT_FAILURE);
        } else {
                const char successText[] = "Successfully read the file.\n";
                
                write(
                        STDOUT_FILENO,
                        successText,
                        sizeof(successText) - 1
                );
        }

        // 只输出 read() 实际读到的字节
        write(
                STDOUT_FILENO,
                buffer,
                (size_t)readBytes
        );

        /**
         * @brief 
         * <unistd.h>
         * int close (int fd)
         * 
         * @param fd          文件描述符
         * 
         * @return 成功返回 0, 失败返回 -1, 同时设置 errno
         */
        // 释放文件描述符
        close(fileDescriptor);

        return 0;
}
