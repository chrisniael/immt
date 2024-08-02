#include <iostream>
#include <string>
#undef UNICODE
#include <Windows.h>

#pragma comment(lib, "imm32.lib")

#define IMC_GETCONVERSIONMODE 0x01
#define IMC_SETCONVERSIONMODE 0x02

int GetCurrentInputMethod() {
	HWND hWnd = GetForegroundWindow();
	DWORD threadId = GetWindowThreadProcessId(hWnd, nullptr);
	HKL hKL = GetKeyboardLayout(threadId);
	uintptr_t langId = reinterpret_cast<uintptr_t>(hKL) & 0xFFFF;
	return static_cast<int>(langId);
}

void SwitchInputMethod(int langId) {
	HWND hWnd = GetForegroundWindow();
	PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, static_cast<LPARAM>(langId));
}

int GetCurrentInputMethodMode() {
	HWND hWnd = GetForegroundWindow();
	HWND hIMEWnd = ImmGetDefaultIMEWnd(hWnd);
	LRESULT mode = SendMessage(hIMEWnd, WM_IME_CONTROL, IMC_GETCONVERSIONMODE, 0);
	return static_cast<int>(mode);
}

void SwitchInputMethodMode(int mode) {
	HWND hWnd = GetForegroundWindow();
	HWND hIMEWnd = ImmGetDefaultIMEWnd(hWnd);
	SendMessage(hIMEWnd, WM_IME_CONTROL, IMC_SETCONVERSIONMODE, static_cast<LPARAM>(mode));
}

// 全局钩子句柄
HHOOK hKeyboardHook;

// 回调函数，在捕获到指定的快捷键组合时执行
void OnWinSpacePressed() {
	std::cout << "Win + Space pressed!" << std::endl;
	// 在此处添加你希望执行的函数逻辑

	int mode1 = 0;
	int mode2 = 1025;
	int curMode = GetCurrentInputMethodMode();
	if (curMode == mode1) {
		SwitchInputMethodMode(mode2);
	}
	else {
		SwitchInputMethodMode(mode1);
	}
}

// 低级键盘钩子回调函数
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
		KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

		// 检查是否按下了Win键
		bool bIsWinPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);

		// 检查是否按下了Space键
		bool bIsSpacePressed = (pKeyboard->vkCode == VK_SPACE);

		if (bIsWinPressed && bIsSpacePressed) {
			OnWinSpacePressed();
			//return 1; // 某些组合键可能需要返回1以防止传递到其他程序或系统
			// 这里如果下一帧 Win 先抬起，那么，当前帧的 Space 按下消息被吃掉后，Win 就能形成按下抬起的消息组合，触发开始菜单的打开。
			// 看下面这个键盘消息序列：
			//1, WM KEYDOWN, vkcode=91, Win 按下
			//2. WM KEYDOWN, vkcode=32, Space 按下（这里触发输入法状态切换，并且吃掉 Space Down 这个消息）
			//3. WM KEYUP, vkcode=91, Win 抬起（因为上一步的 Space 按下消息被吃掉了，所以从系统角度看，就是按下并抬起了 Win 键）
			//4. WM KEYUP, vkCode=32, Space 抬起
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

bool InstallHook() {
	// 设置全局低级键盘钩子
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (!hKeyboardHook) {
		std::cerr << "Failed to install hook!" << std::endl;
		return false;
	}

	std::cout << "Hook installed. Press Win + Space to trigger the function. Press Ctrl+C to exit." << std::endl;

	return true;
}

void UninstallHook() {
	// 卸载钩子
	if (hKeyboardHook != NULL) {
		UnhookWindowsHookEx(hKeyboardHook);
		hKeyboardHook = NULL;
	}
}
