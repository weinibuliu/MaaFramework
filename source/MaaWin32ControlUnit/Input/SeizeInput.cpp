#include "SeizeInput.h"

#include "Utils/Logger.h"
#include "Utils/MicroControl.hpp"
#include "Utils/Platform.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

void SeizeInput::ensure_foreground()
{
    if (hwnd_ == GetForegroundWindow()) {
        return;
    }
    ShowWindow(hwnd_, SW_MINIMIZE);
    ShowWindow(hwnd_, SW_RESTORE);
    SetForegroundWindow(hwnd_);
}

bool SeizeInput::click(int x, int y)
{
    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    LogInfo << VAR(x) << VAR(y) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    SetCursorPos(point.x, point.y);

    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

bool SeizeInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    POINT point1 = { x1, y1 };
    POINT point2 = { x2, y2 };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point1);
        ClientToScreen(hwnd_, &point2);
    }
    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 500;
    }

    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration) << VAR(point1.x) << VAR(point1.y) << VAR(point2.x) << VAR(point2.y)
            << VAR_VOIDP(hwnd_);

    micro_swipe(
        point1.x,
        point1.y,
        point2.x,
        point2.y,
        duration,
        [&](int x, int y) {
            SetCursorPos(x, y);

            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &input, sizeof(INPUT));
        },
        [&](int x, int y) { SetCursorPos(x, y); },
        [&]([[maybe_unused]] int x, [[maybe_unused]] int y) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(INPUT));
        });

    return true;
}

bool SeizeInput::touch_down(int contact, int x, int y, int pressure)
{
    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    SetCursorPos(point.x, point.y);

    INPUT input = {};

    input.type = INPUT_MOUSE;

    switch (contact) {
    case 0:
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        break;
    case 1:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        break;
    case 2:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        break;
    case 3:
        input.mi.dwFlags = MOUSEEVENTF_XDOWN;
        input.mi.mouseData = XBUTTON1;
        break;
    case 4:
        input.mi.dwFlags = MOUSEEVENTF_XDOWN;
        input.mi.mouseData = XBUTTON2;
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::touch_move(int contact, int x, int y, int pressure)
{
    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    SetCursorPos(point.x, point.y);

    return true;
}

bool SeizeInput::touch_up(int contact)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(contact) << VAR(hwnd_);

    INPUT input = {};

    input.type = INPUT_MOUSE;
    switch (contact) {
    case 0:
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        break;
    case 1:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        break;
    case 2:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        break;
    case 3:
        input.mi.dwFlags = MOUSEEVENTF_XUP;
        input.mi.mouseData = XBUTTON1;
        break;
    case 4:
        input.mi.dwFlags = MOUSEEVENTF_XUP;
        input.mi.mouseData = XBUTTON2;
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::press_key(int key)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(key) << VAR(hwnd_);

    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(key);

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = static_cast<WORD>(key);
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

bool SeizeInput::input_text(const std::string& text)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(text) << VAR(hwnd_);

    if (std::ranges::any_of(text, [](const char& c) { //
            return static_cast<unsigned>(c) > 127;
        })) {
        LogError << "text contains non-ascii characters" << VAR(text);
        return false;
    }

    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    for (auto& c : text) {
        inputs[0].ki.wVk = static_cast<WORD>(c);
        inputs[1].ki.wVk = static_cast<WORD>(c);

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
