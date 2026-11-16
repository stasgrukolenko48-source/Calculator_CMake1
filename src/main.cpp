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

        std::cout << "\nReady. Enter expression or 'exit'.\n";

        std::string line;
        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) break;
            
           
            for (char& c : line) {
                if (static_cast<unsigned char>(c) == 0x96) c = '-';
            }

            if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
            if (line.find("exit") != std::string::npos || line.find("quit") != std::string::npos) break;

            try {
                std::cout << "= " << calculator.calculate(line) << "\n";
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << "\n";
            }
        }
        
        calculator.clearFunctions();
    } 
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown fatal error.\n";
        return 1;
    }

    return 0;
}
