#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
// 修改GDI+的包含方式，解决MSVC编译问题
#ifdef __cplusplus
#include <gdiplus.h>
using namespace Gdiplus;
#else
#define WINGDIPAPI __stdcall
#define GDIPCONST const
#include <objidl.h>
typedef enum {
    GdiplusAddMemory = 0,
    GdiplusRemoveMemory = 1
} GdiplusManageMemory;
// 这里只声明使用到的GDI+数据类型，其他的可以根据需要添加
typedef void* GpImage;
typedef void* GpBitmap;
#endif

#include "win.utils.h"
#include "console.utils.h"
#include <tlhelp32.h>
#include <ShlObj.h>

// 点坐标结构
typedef struct {
    int x;
    int y;
} POINT_DATA;

// 颜色范围结构
typedef struct {
    int r_min, r_max;
    int g_min, g_max;
    int b_min, b_max;
} COLOR_RANGE;

// WINDOW_INFO结构体
typedef struct {
    HWND hwnd;
    RECT window_rect;      // 窗口在屏幕上的坐标（包含边框）
    RECT client_rect;      // 客户区域相对于窗口左上角的坐标
    SIZE window_size;      // 窗口大小（包含边框）
    SIZE client_size;      // 客户区域大小
    POINT client_offset;   // 客户区域相对于窗口左上角的偏移
    char title[256];       // 窗口标题
} WINDOW_INFO;

POINT_DATA points[] = {
    {150, 66},  
    {267, 57},  
    {155, 57},  
    {281, 60}   
};

COLOR_RANGE color_ranges[] = {
    {239, 243, 211, 215, 151, 155},  // 第一个点的颜色范围
    {237, 241, 160, 164, 114, 118},  // 第二个点的颜色范围
    {9, 13, 7, 11, 5, 9},
    {9, 13, 7, 11, 4, 8}
};

// 确保DPI感知
void SetDpiAwareness() {
    typedef HRESULT (WINAPI *SetProcessDpiAwarenessFunc)(int);
    HMODULE shcore = LoadLibraryA("shcore.dll");
    if (shcore) {
        SetProcessDpiAwarenessFunc setDpiFunc = (SetProcessDpiAwarenessFunc)GetProcAddress(shcore, "SetProcessDpiAwareness");
        if (setDpiFunc) {
            setDpiFunc(2); // PROCESS_PER_MONITOR_DPI_AWARE
        }
        FreeLibrary(shcore);
    }
}

// 检查是否为管理员权限
BOOL IsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(
        &ntAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup)) {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    
    return isAdmin;
}

// 以管理员权限重启程序
void RestartAsAdmin(const char* exePath) {
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFOA);
    sei.lpVerb = "runas";
    sei.lpFile = exePath;
    sei.nShow = SW_NORMAL;
    
    Con_Info("正在请求管理员权限...");
    if (!ShellExecuteExA(&sei)) {
        Con_Error("请求管理员权限失败: %d", GetLastError());
        printf("按Enter键退出...");
        getchar();
        exit(1);
    }
    exit(0);
}

// 修改GetWindowInfoEx函数，确保获取客户区域坐标
WINDOW_INFO GetWindowInfoEx(HWND hwnd) {
    WINDOW_INFO info = {0};
    info.hwnd = hwnd;
    
    // 获取整个窗口大小（包括边框）
    GetWindowRect(hwnd, &info.window_rect);
    
    // 获取客户区域大小（不包括边框）
    GetClientRect(hwnd, &info.client_rect);
    
    // 计算窗口大小
    info.window_size.cx = info.window_rect.right - info.window_rect.left;
    info.window_size.cy = info.window_rect.bottom - info.window_rect.top;
    
    // 计算客户区域大小
    info.client_size.cx = info.client_rect.right - info.client_rect.left;
    info.client_size.cy = info.client_rect.bottom - info.client_rect.top;
    
    // 计算客户区域相对于窗口左上角的偏移
    POINT pt = {0, 0};
    ClientToScreen(hwnd, &pt);
    info.client_offset.x = pt.x - info.window_rect.left;
    info.client_offset.y = pt.y - info.window_rect.top;
    
    // 获取窗口标题
    GetWindowTextA(hwnd, info.title, sizeof(info.title));
    
    return info;
}

// 检查颜色是否在指定范围内
BOOL IsColorInRange(COLORREF color, COLOR_RANGE range) {
    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);
    
    return (range.r_min <= r && r <= range.r_max &&
            range.g_min <= g && g <= range.g_max &&
            range.b_min <= b && b <= range.b_max);
}

// 获取窗口截图
HBITMAP CaptureWindow(HWND hwnd, SIZE* size) {
    // 获取窗口大小
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    int width = rcWindow.right - rcWindow.left;
    int height = rcWindow.bottom - rcWindow.top;
    
    *size = (SIZE){width, height};
    
    // 创建设备上下文
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, width, height);
    HGDIOBJ hOld = SelectObject(hdcMemDC, hbmScreen);
    
    // 使用PW_RENDERFULLCONTENT (Windows 8.1+)
    // PW_RENDERFULLCONTENT = 0x00000002
    PrintWindow(hwnd, hdcMemDC, 0x00000002);
    
    // 清理设备上下文
    SelectObject(hdcMemDC, hOld);
    DeleteDC(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    
    return hbmScreen;
}

// 获取位图中某个像素的颜色
COLORREF GetPixelFromBitmap(HBITMAP hBitmap, int x, int y) {
    BITMAP bm;
    GetObject(hBitmap, sizeof(bm), &bm);
    
    // 创建内存DC
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    // 选择位图到内存DC
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hBitmap);
    
    // 获取像素颜色
    COLORREF color = GetPixel(hdcMem, x, y);
    
    // 清理
    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    return color;
}

// 检查多个像素点
BOOL CheckMultiplePixels(HWND hwnd, POINT_DATA* points, COLOR_RANGE* color_ranges, int point_count) {
    int match_count = 0;
    SIZE size;
    
    // 获取窗口信息
    WINDOW_INFO info = GetWindowInfoEx(hwnd);
    
    // 获取窗口截图
    HBITMAP hBitmap = CaptureWindow(hwnd, &size);
    
    // 检查每个点的颜色
    for (int i = 0; i < point_count; i++) {
        // 转换坐标
        int transformed_x = points[i].x + info.client_offset.x;
        int transformed_y = points[i].y + info.client_offset.y;
        
        // 确保点在截图范围内
        if (transformed_x >= 0 && transformed_x < size.cx && 
            transformed_y >= 0 && transformed_y < size.cy) {
            
            // 获取像素颜色
            COLORREF color = GetPixelFromBitmap(hBitmap, transformed_x, transformed_y);
            
            // 检查颜色匹配
            if (IsColorInRange(color, color_ranges[i])) {
                match_count++;
            }
        }
    }
    
    // 释放位图
    DeleteObject(hBitmap);
    
    return match_count >= 3;
}

// 修改坐标初始化函数，调整为考虑客户区域的缩放
void InitPoints(WINDOW_INFO window_info, POINT_DATA* scaled_points, int point_count) {
    double scale;
    int width = window_info.client_size.cx;
    int height = window_info.client_size.cy;
    
    // 根据宽高比计算缩放因子
    if ((width * 9) > (height * 16)) {
        scale = (double)height / 1080.0;
        Con_Debug("使用高度比例缩放: %.4f (客户区高度: %d)", scale, height);
    } else {
        scale = (double)width / 1920.0;
        Con_Debug("使用宽度比例缩放: %.4f (客户区宽度: %d)", scale, width);
    }
    
    // 缩放所有点
    for (int i = 0; i < point_count; i++) {
        scaled_points[i].x = (int)(points[i].x * scale);
        scaled_points[i].y = (int)(points[i].y * scale);
        Con_Debug("点 %d: 原始(%d,%d) -> 缩放后(%d,%d)", 
            i+1, points[i].x, points[i].y, scaled_points[i].x, scaled_points[i].y);
    }
}

// 向窗口发送按键
void SendKeyToWindow(HWND hwnd, int key) {
    // 即使窗口不在前台，也可以通过PostMessage发送按键消息
    PostMessage(hwnd, WM_KEYDOWN, key, 0);
    Sleep(75);
    PostMessage(hwnd, WM_KEYUP, key, 0);
}

// 主函数
int main(int argc, char* argv[]) {
    // 初始化控制台
    Con_Init("崩铁自动点击", 80, 25);
    
    // 设置DPI感知
    SetDpiAwareness();
    
    // 检查管理员权限
    if (!IsAdmin()) {
        RestartAsAdmin(argv[0]);
        return 0;
    }

    Con_Info("程序启动（管理员模式）");
    
    // 等待进程
    Con_Waiting("正在等待崩铁进程 StarRail.exe");
    DWORD pid = 0;
    while (!pid) {
        pid = Win_GetPid("StarRail.exe");
        if (!pid) {
            Sleep(200);
        }
    }
    
    printf(ANSI_CLEAN_LINE);
    Con_Info("Pid = %d", pid);
    
    // 等待窗口
    Con_Waiting("正在等待崩铁窗口");
    HWND hwnd = NULL;
    WINDOW_INFO window_info;
    
    while (TRUE) {
        hwnd = Win_GetHwnd(pid);
        if (hwnd) {
            window_info = GetWindowInfoEx(hwnd);
            if (window_info.window_size.cx > 400) {
                break;
            }
        }
        Sleep(200);
    }
    
    printf(ANSI_CLEAN_LINE);
    Con_Info("Hwnd = 0x%p", hwnd);
    Con_Info("窗口标题: %s", window_info.title);
    Con_Info("窗口大小: %d X %d", window_info.window_size.cx, window_info.window_size.cy);
    Con_Info("客户区大小: %d X %d", window_info.client_size.cx, window_info.client_size.cy);
    Con_Info("客户区偏移: x=%d, y=%d", window_info.client_offset.x, window_info.client_offset.y);
    
    InitPoints(window_info, points, 4);

    Con_Info("开始自动点击剧情中(当检测到进入剧情时会自动点击)...");
    Con_Info("按" ANSI_BLUE("Ctrl+p") "键暂停.");
    BOOL isActivate = TRUE;
    // afterDialog：值为0表示正在剧情对话中，值为1表示不在剧情对话中，大于1表示剧情对话刚刚结束（会在几秒内递减到1）
    int afterDialog = 1;
    // 主消息循环
    bool running = true;
    while (running) {
        if (Win_IsKeysDown(VK_CONTROL, 'P')) {
            isActivate = !isActivate;
            if (isActivate) 
                Con_Info("程序继续" ANSI_GREEN("执行") "中");
            else 
                Con_Info("程序" ANSI_ORANGE("暂停") "中");
            Sleep(400);
        }

        if (isActivate) {
            // 判断左上角的隐藏对话按钮
            if (CheckMultiplePixels(hwnd, points, color_ranges, 4)) {
                if(afterDialog > 0) {
                    afterDialog = 0;
                    Con_Info("检测到进入剧情对话");
                }
                // 发送点击空格键的消息
                SetForegroundWindow(hwnd);
                PostMessageW(hwnd, WM_KEYDOWN, 0x20, 0x210001);
                Sleep(75);
                PostMessageW(hwnd, WM_KEYUP, 0x20, 0xC0210001);
                // 发送点击1键的消息
                PostMessageW(hwnd, WM_KEYDOWN, 0x31, 0x210001);
                Sleep(75);
                PostMessageW(hwnd, WM_KEYUP, 0x31, 0xC0210001);
            }
            else if(afterDialog == 0) {
                afterDialog = 16;
                Con_Info("剧情对话结束");
            }
        }

        if(afterDialog > 1) {
            afterDialog--;
            ClipCursor(NULL);
        }
        Sleep(125);  
    }
    
    Con_Info("程序已退出");
    
    return 0;
}
