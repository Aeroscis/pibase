# pibase 文档

| 文件 | 内容 |
|---|---|
| [`naming.md`](naming.md) | **家族命名规则（权威）**：两个层级、判据、各标识符类别的写法、不适用本规则的东西、结果码分区 |
| [`todo/README.md`](todo/README.md) | 还没做的事 |
| [`../include/pibase/pi_base.h`](../include/pibase/pi_base.h) | 根层本体：命名规则摘要 + 准入判据也写在它的头部注释里 |
| [`../README.md`](../README.md) | 根层是什么、放进来的准入判据、为什么是 header-only |

读代码的顺序：`docs/naming.md`（为什么这样命名）→ `include/pibase/pi_base.h`
（有哪些名字、怎么用）→ `README.md`（什么该进这一层、什么不该）。
