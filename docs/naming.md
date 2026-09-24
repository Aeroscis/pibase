# 命名规则（家族级，权威）

> 这份文档是 PI 家族**命名规则的唯一权威**。其他成员库的文档若与本文冲突，以本文为准。
> 规则本体也写在 `include/pibase/pi_base.h` 的头部注释里（代码旁边一份，便于就地查阅）。

## 1. 两个层级

PI 是一个**库家族**。家族里每个库有自己的名字、自己的头文件、自己的发布节奏。于是名字分两层：

```
家族根词汇    裸 pi_ / Pi / PI_              ← 本库（pibase）提供
成员自有名字  pi_<成员>_ / Pi<成员> / PI_<成员>_   ← 各成员库自己
```

判据只有一个问题：

> **这个名字出现在「不止一个」成员的 API 签名里吗？**

- **是** → 家族根词汇，放 pibase，用裸前缀。
- **否** → 某个成员自己的东西，带该成员的段。

`PiResult`、`PiGuid`、`IPiUnknown` 之所以是裸前缀，不是"暂时没改"，而是它们本来就是家族级的：任何成员的错误码、标识符、根接口都是同一个东西。反过来 `pi_plugin_module_load` 里的 `plugin` 段说明它只属于那一个成员。

> 起名的直觉检验：把某个成员从家族里删掉，这个标识符对剩下的成员还有意义吗？
> 有意义 → 家族级；没意义 → 它是那个成员的。

## 2. 各标识符类别的写法

| 类别 | 写法 | 例 |
|---|---|---|
| C 函数 | snake_case | `pi_plugin_module_load` / `pi_guid_equal` |
| C 类型 | PascalCase | `PiPluginDescriptor` / `PiResult` |
| 宏、常量 | SCREAMING_SNAKE_CASE | `PI_PLUGIN_BUILD_TESTS` / `PI_OK` |
| 接口 | `I` + PascalCase | `IPiPluginHostServices` / `IPiUnknown` |
| CMake 缓存变量、Conan 选项 | 同宏 | `PI_PLUGIN_BUILD_ADAPTER_QT` |
| CMake 目标名（构建树内） | snake_case | `pi_plugin_test_host_imgui` |
| 导出给消费者的 CMake 目标 | `pi::` + 成员名 | `pi::plugin` / `pi::base` |

C 函数用 snake_case 是跨平台 C 库的公约数（GTK `gtk_widget_show`、SQLite
`sqlite3_open_v2`、libuv `uv_timer_start`）。裸 PascalCase 的 C **函数**是平台/厂商
SDK 的习惯（Win32 `CreateFileW`、Vulkan `vkCreateInstance`），库不该照抄。

## 3. 不适用本规则的东西

命名规则管的是**标识符**。以下不是标识符，不走这套前缀：

| 东西 | 为什么 | 例 |
|---|---|---|
| 产物名、包名 | 分发身份，消费者按它 `find_package` / 链接 | `piplugin.dll`、`find_package(piplugin)`、`pibase` |
| 源文件名 | 是文件不是符号 | `pi_plugin_types.h`、`pi_host_session.c` |
| 家族共享构建脚本 API | 另一套范式，多个项目共用，不属于任何成员 | `cmake/pi/` 的 `pi_tar_msg`、`pi_init_glob_proj`、`PI_HEADER_EXTS` |
| CMake 导出命名空间 | 是家族层，不是成员层 | `pi::` |

最后一条容易反直觉：**`pi::` 本身不带给成员**——命名空间是家族的，成员名在它后面
（`pi::plugin`、`pi::base`）。这正是"家族根用裸前缀"在 CMake 里的对应形态。

## 4. 结果码的分区

结果码是家族级词汇里唯一需要划区的一条：

- 框架占用 `-1..-9`；
- **app / 成员库的自定义错误码取 `<= -100`**（`PI_APP_RESULT_BASE` 是起点），
  给框架将来加码留出量级余量；
- **符号即语义**：`PI_SUCCEEDED` / `PI_FAILED` 按符号判定，所以自定义**错误码**必须是负数；
  而"已受理、结果稍后送达"这种**成功**语义必须另开**正值**结果码——用负数会让
  `PI_FAILED()` 对它声称回答的问题给出错误答案，而且错得很安静。

## 5. 当前家族

| 成员 | 段 | CMake 目标 | 说明 |
|---|---|---|---|
| pibase | （根，裸前缀） | `pi::base` | 本库：家族根词汇 |
| piplugin | `plugin` | `pi::plugin`（及其 `pi::plugin_*` 套件） | 插件框架 |

新增成员时：给自己取一个段、把自有标识符按上面的写法加前缀、在 `pi::` 下暴露一个
`pi::<成员>` 目标，然后回来更新这张表。
