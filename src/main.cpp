#include "calculator.h"
#include "plugin_manager.h"
#include <iostream>
#include <windows.h>

int main() {
    try {
        SetConsoleCP(1251);
        SetConsoleOutputCP(1251);
        setlocale(LC_ALL, "Russian");

        PluginManager pluginManager;
        Calculator calculator;

        pluginManager.loadAll(calculator);

        std::cout << "\nКалькулятор готов к работе. Введите выражение (или 'exit' для выхода).\n> ";

        std::string line;
        while (std::getline(std::cin, line)) {
            for (char& c : line) {
                if (static_cast<unsigned char>(c) == 0x96) c = '-';
            }

            std::size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                std::cout << "> ";
                continue;
            }
            
            std::size_t last = line.find_last_not_of(" \t\r\n");
            std::string trimmed = line.substr(first, last - first + 1);

            if (trimmed == "exit" || trimmed == "quit") break;

            try {
                std::cout << "Вывод: " << calculator.calculate(trimmed) << "\n";
            } catch (const std::exception& e) {
                std::cout << "Ошибка: " << e.what() << "\n";
            }
            std::cout << "> ";
        }
        
        calculator.clearFunctions();
        std::cout << "Работа калькулятора завершена.\n";
    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Неизвестная критическая ошибка.\n";
        return 1;
    }
    return 0;
}
