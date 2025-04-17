此工具已迁移至https://github.com/ywq1145/StaRailAutoSkipV2


# 星穹铁道自动对话工具

一个简单的崩坏：星穹铁道自动对话工具，可以自动检测剧情对话并点击继续。当检测到游戏中出现剧情对话时，会自动按下空格键过剧情和1键选择选项以过剧情。代码参考[SyrieYume/GenshinAutoV2: 原神后台自动点剧情工具 v2版本。](https://github.com/SyrieYume/GenshinAutoV2)

## 系统要求

- Windows操作系统
- 游戏需要以窗口模式运行
- Visual C++ 运行时库（大多数Windows系统已预装）

## 使用方法

### 直接使用预编译版本

1. 在[Releases](https://github.com/ywq1145/StaRailAutoSkip/releases/)下载最新的发布版本 
2. 运行游戏，确保是窗口模式
3. 运行 `GenshinAuto.exe`
4. 程序会自动检测游戏窗口并开始监控剧情对话（通过检测像素，所以别开滤镜）
5. 按 `Ctrl+P` 可以随时暂停/继续自动点击功能

### 从源代码编译

如果你想自己编译项目，需要安装MinGW-64或msvc。

编译命令：

```bash
gcc -o auto src/main.c src/console.utils.c src/win.utils.c -lgdi32 -lgdiplus
```

## 工作原理

程序通过以下步骤实现自动点击功能：

1. 检测 StarRail.exe 进程并获取窗口句柄
2. 获取窗口客户区域尺寸，计算坐标缩放比例
3. 对游戏窗口进行截图，并当检测到剧情对话界面特征时，自动向游戏窗口发送按键消息
4. 发送按键

