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

// ȫ�ֹ��Ӿ��
HHOOK hKeyboardHook;

// �ص��������ڲ���ָ���Ŀ�ݼ����ʱִ��
void OnWinSpacePressed() {
	std::cout << "Win + Space pressed!" << std::endl;
	// �ڴ˴������ϣ��ִ�еĺ����߼�

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

// �ͼ����̹��ӻص�����
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
		KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

		// ����Ƿ�����Win��
		bool bIsWinPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);

		// ����Ƿ�����Space��
		bool bIsSpacePressed = (pKeyboard->vkCode == VK_SPACE);

		if (bIsWinPressed && bIsSpacePressed) {
			OnWinSpacePressed();
			return 1; // ĳЩ��ϼ�������Ҫ����1�Է�ֹ���ݵ����������ϵͳ
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

bool InstallHook() {
	// ����ȫ�ֵͼ����̹���
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (!hKeyboardHook) {
		std::cerr << "Failed to install hook!" << std::endl;
		return false;
	}

	std::cout << "Hook installed. Press Win + Space to trigger the function. Press Ctrl+C to exit." << std::endl;

	return true;
}

void UninstallHook() {
	// ж�ع���
	if (hKeyboardHook != NULL) {
		UnhookWindowsHookEx(hKeyboardHook);
		hKeyboardHook = NULL;
	}
}
