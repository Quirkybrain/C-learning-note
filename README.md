# C 语言学习笔记

这个仓库用来记录我在学习 C 语言过程中的笔记、示例代码和一些面向底层实现细节的整理。

内容会尽量围绕“概念 + 可运行示例”展开，不只记录结论，也会保留推导过程、设计思路和代码演化，方便后续回看和持续补充。

## 组织方式

每篇笔记通常会包含：

- `README.md`：该主题的文字说明与推导过程
- `include/`：头文件，放抽象接口或公开声明
- `src/`：源文件，放具体实现
- `main.c`：最小可运行示例
- `Makefile`：构建脚本

## 说明

这个仓库以学习记录为主，因此部分示例会刻意保留“教学型写法”，用于展示某种机制为什么能工作、以及为什么某些写法会出问题。

会持续补充更多和 C 语言相关的主题。

## License

Unless otherwise noted, the original learning notes, example code, and repository-specific modifications in this project are licensed under the MIT License.

Third-party source code preserved for study purposes retains its original upstream copyright and license terms.
