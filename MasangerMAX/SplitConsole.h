#pragma once
#include <windows.h>
#include <iostream>

class SplitConsole {
private:
    HANDLE hConsole;
    COORD chatAreaStart;
    COORD inputAreaStart;
    int consoleWidth, consoleHeight;

public:
    SplitConsole() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        setupConsole();
    }

    void setupConsole() {
        // Устанавливаем размер консоли
        SMALL_RECT rect = { 0, 0, 119, 49 }; // 120x50 символов
        SetConsoleWindowInfo(hConsole, TRUE, &rect);

        COORD size = { 120, 50 };
        SetConsoleScreenBufferSize(hConsole, size);

        // Получаем новые размеры
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        // Определяем зоны
        chatAreaStart = { 0, 0 };
        inputAreaStart = { 0, (short)(consoleHeight - 3) }; // 3 строки для ввода
    }

    void clearChatArea() {
        COORD topLeft = { 0, 0 };
        DWORD written;
        DWORD chatAreaSize = consoleWidth * (consoleHeight - 3);

        FillConsoleOutputCharacter(hConsole, ' ', chatAreaSize, topLeft, &written);
        FillConsoleOutputAttribute(hConsole, 7, chatAreaSize, topLeft, &written);
        SetConsoleCursorPosition(hConsole, topLeft);
    }

    void setCursorToInput() {
        SetConsoleCursorPosition(hConsole, inputAreaStart);
        std::cout << "> "; // Приглашение для ввода
        COORD inputPos = { 2, inputAreaStart.Y };
        SetConsoleCursorPosition(hConsole, inputPos);
    }

    void printToChat(const std::string& message) {
        // Сохраняем позицию курсора
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);

        // Печатаем в области чата
        SetConsoleCursorPosition(hConsole, chatAreaStart);

        // Прокручиваем чат вверх если нужно
        // ... логика прокрутки ...

        std::cout << message << std::endl;

        // Возвращаем курсор в область ввода
        setCursorToInput();
    }
};

