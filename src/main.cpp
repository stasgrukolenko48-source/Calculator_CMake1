#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>
#include <windows.h>

#include "plugin_api.h"

namespace fs = std::filesystem;


class DynamicLibrary {
private:
    HMODULE handle_ = nullptr;
    std::string filename_;

public:
    DynamicLibrary() = default;

    explicit DynamicLibrary(const fs::path& path)
        : filename_(path.filename().string()) {
        // режим SEM_FAILCRITICALERRORS подавляет системные всплывающие окна Windows
        UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        handle_ = LoadLibraryW(path.wstring().c_str());
        SetErrorMode(oldMode);

        if (!handle_) {
            throw std::runtime_error("Не удалось загрузить библиотеку '" + filename_ + "' (некорректный образ DLL)");
        }
    }

    // запрещаем копирование
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    // разрешаем перемещение
    DynamicLibrary(DynamicLibrary&& other) noexcept
        : handle_(other.handle_), filename_(std::move(other.filename_)) {
        other.handle_ = nullptr;
    }

    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept {
        if (this != &other) {
            unload();
            handle_ = other.handle_;
            filename_ = std::move(other.filename_);
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~DynamicLibrary() {
        unload();
    }

    void unload() noexcept {
        if (handle_) {
            FreeLibrary(handle_);
            handle_ = nullptr;
        }
    }

    template <typename FuncPtr>
    FuncPtr getSymbol(const char* symbolName) const {
        if (!handle_) return nullptr;
        return reinterpret_cast<FuncPtr>(GetProcAddress(handle_, symbolName));
    }

    const std::string& filename() const noexcept { return filename_; }
};

class Calculator {
public:
    using MathFunction = std::function<double(double)>;

private:
    std::unordered_map<std::string, MathFunction> functions_;

    // внутренний класс парсера для конкретного вызова calculate()
    class Parser {
    private:
        std::string_view input_;
        std::size_t pos_ = 0;
        const std::unordered_map<std::string, MathFunction>& funcs_;

        void skipSpaces() noexcept {
            while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }

        bool match(char expected) noexcept {
            skipSpaces();
            if (pos_ < input_.size() && input_[pos_] == expected) {
                ++pos_;
                return true;
            }
            return false;
        }

        std::string parseIdentifier() {
            skipSpaces();
            std::size_t start = pos_;
            while (pos_ < input_.size()) {
                unsigned char ch = static_cast<unsigned char>(input_[pos_]);
                if (!std::isalnum(ch) && ch != '_') {
                    break;
                }
                ++pos_;
            }
            if (start == pos_) {
                throw std::runtime_error("Ожидалось имя функции");
            }
            return std::string(input_.substr(start, pos_ - start));
        }

        double parseExpression() {
            double result = parseTerm();
            while (true) {
                if (match('+')) {
                    result += parseTerm();
                }
                else if (match('-')) {
                    result -= parseTerm();
                }
                else {
                    break;
                }
            }
            return result;
        }

        double parseTerm() {
            double result = parseUnary();
            while (true) {
                if (match('*')) {
                    result *= parseUnary();
                }
                else if (match('/')) {
                    double divisor = parseUnary();
                    if (divisor == 0.0) {
                        throw std::domain_error("Деление на ноль");
                    }
                    result /= divisor;
                }
                else {
                    break;
                }
            }
            return result;
        }

        double parseUnary() {
            if (match('+')) {
                return parseUnary();
            }
            if (match('-')) {
                return -parseUnary();
            }
            return parsePower();
        }

        double parsePower() {
            double base = parsePrimary();
            if (match('^')) {
                // показатель степени может начинаться с унарного оператора
                double exponent = parseUnary();
                if (base < 0.0 && std::floor(exponent) != exponent) {
                    throw std::domain_error("Возведение отрицательного числа в дробную степень");
                }
                if (base == 0.0 && exponent <= 0.0) {
                    throw std::domain_error("Ноль в неположительной степени");
                }
                double res = std::pow(base, exponent);
                if (!std::isfinite(res)) {
                    throw std::overflow_error("Переполнение или некорректный результат при возведении в степень");
                }
                return res;
            }
            return base;
        }

        double parsePrimary() {
            if (match('(')) {
                if (match(')')) {
                    throw std::invalid_argument("Пустые скобки '()'");
                }
                double result = parseExpression();
                if (!match(')')) {
                    throw std::invalid_argument("Пропущена закрывающая скобка ')'");
                }
                return result;
            }

            skipSpaces();
            if (pos_ < input_.size()) {
                unsigned char ch = static_cast<unsigned char>(input_[pos_]);
                if (std::isalpha(ch) || ch == '_') {
                    std::string funcName = parseIdentifier();
                    if (!match('(')) {
                        throw std::invalid_argument("Ожидалась '(' после имени функции '" + funcName + "'");
                    }
                    if (match(')')) {
                        throw std::invalid_argument("Функция '" + funcName + "' вызвана без аргумента");
                    }
                    double arg = parseExpression();
                    if (!match(')')) {
                        throw std::invalid_argument("Пропущена ')' после аргумента функции '" + funcName + "'");
                    }

                    auto it = funcs_.find(funcName);
                    if (it == funcs_.end()) {
                        throw std::invalid_argument("Неизвестная функция '" + funcName + "' (плагин не загружен)");
                    }
                    return it->second(arg);
                }
            }

            return parseNumber();
        }

        double parseNumber() {
            skipSpaces();
            if (pos_ >= input_.size()) {
                throw std::invalid_argument("Неожиданный конец выражения (ожидалось число или выражение)");
            }

            const char* start = input_.data() + pos_;
            char* end = nullptr;
            double value = std::strtod(start, &end);

            if (start == end) {
                throw std::invalid_argument(
                    std::string("Неожиданный символ '") + input_[pos_] + "' на позиции " + std::to_string(pos_ + 1)
                );
            }

            pos_ += static_cast<std::size_t>(end - start);
            if (!std::isfinite(value)) {
                throw std::out_of_range("Число выходит за допустимые пределы типа double");
            }
            return value;
        }

    public:
        Parser(std::string_view input, const std::unordered_map<std::string, MathFunction>& funcs)
            : input_(input), funcs_(funcs) {}

        double parse() {
            double result = parseExpression();
            skipSpaces();
            if (pos_ != input_.size()) {
                throw std::invalid_argument(
                    std::string("Лишний или неожиданный символ '") + input_[pos_] + "' на позиции " + std::to_string(pos_ + 1)
                );
            }
            if (!std::isfinite(result)) {
                throw std::overflow_error("Результат вычисления выходит за допустимые пределы (Infinity / NaN)");
            }
            return result;
        }
    };

public:
    void registerFunction(const std::string& name, MathFunction func) {
        functions_[name] = std::move(func);
    }

    void clearFunctions() noexcept {
        functions_.clear();
    }

    double calculate(std::string_view expression) const {
        Parser parser(expression, functions_);
        return parser.parse();
    }
};

class PluginManager {
private:
    std::vector<DynamicLibrary> loadedLibs_;

    // находит папку plugins
    static fs::path resolvePluginDirectory() {
        fs::path localPlugins = fs::current_path() / "plugins";
        if (fs::exists(localPlugins) && fs::is_directory(localPlugins)) {
            return localPlugins;
        }

        wchar_t exePathBuf[MAX_PATH];
        if (GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH) > 0) {
            fs::path exePlugins = fs::path(exePathBuf).parent_path() / "plugins";
            if (fs::exists(exePlugins) && fs::is_directory(exePlugins)) {
                return exePlugins;
            }
        }
        return localPlugins;
    }

public:
    PluginManager() = default;
    ~PluginManager() = default;

    void loadAll(Calculator& calculator) {
        fs::path pluginDir = resolvePluginDirectory();

        std::error_code ec;
        if (!fs::exists(pluginDir, ec) || !fs::is_directory(pluginDir, ec)) {
            std::cout << "[Предупреждение] Папка 'plugins' не найдена (" << pluginDir.string() << ").\n"
                << "Создаю пустую директорию './plugins'. Доступны только базовые операции.\n";
            fs::create_directory("plugins", ec);
            return;
        }

        std::size_t dllCount = 0;
        std::size_t successCount = 0;

        for (const auto& entry : fs::directory_iterator(pluginDir, ec)) {
            if (ec || !entry.is_regular_file()) continue;

            // сравниваем расширение без учета регистра
            std::string ext = entry.path().extension().string();
            for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (ext != ".dll") continue;

            ++dllCount;

            try {
                DynamicLibrary lib(entry.path());

                auto getPlugin = lib.getSymbol<GetPluginFunc>("GetPlugin");
                if (!getPlugin) {
                    std::cerr << "[Ошибка плагина] В файле '" << lib.filename()
                        << "' не найдена точка входа GetPlugin(). Пропуск.\n";
                    continue;
                }

                const Plugin* plugin = nullptr;
                try {
                    plugin = getPlugin();
                }
                catch (...) {
                    std::cerr << "[Ошибка плагина] Исключение при вызове GetPlugin() в '" << lib.filename() << "'.\n";
                    continue;
                }

                if (!plugin) {
                    std::cerr << "[Ошибка плагина] GetPlugin() вернул nullptr в '" << lib.filename() << "'.\n";
                    continue;
                }

                if (plugin->version != 1) {
                    std::cerr << "[Ошибка плагина] Неподдерживаемая версия API (" << plugin->version
                        << ") в '" << lib.filename() << "'. Ожидалась версия 1.\n";
                    continue;
                }

                if (!plugin->name || plugin->name[0] == '\0' || !plugin->func) {
                    std::cerr << "[Ошибка плагина] Некорректная структура Plugin (пустое имя или указатель на функцию) в '"
                        << lib.filename() << "'.\n";
                    continue;
                }

                // имя функции берется строго из структуры Plugin, а не из имени файла DLL
                std::string funcName(plugin->name);
                MathFunc rawFunc = plugin->func;

                calculator.registerFunction(funcName, [rawFunc, funcName](double x) -> double {
                    double result = 0.0;
                    int status = -1;
                    try {
                        status = rawFunc(x, &result);
                    }
                    catch (const std::exception& e) {
                        throw std::runtime_error("Исключение в функции '" + funcName + "': " + e.what());
                    }
                    catch (...) {
                        throw std::runtime_error("Неизвестное исключение в функции '" + funcName + "'");
                    }

                    if (status != 0) {
                        throw std::domain_error(
                            "Ошибка вычисления функции '" + funcName + "' для аргумента " + std::to_string(x) +
                            " (недопустимый аргумент / выход из области определения)"
                        );
                    }
                    if (!std::isfinite(result)) {
                        throw std::overflow_error("Функция '" + funcName + "' вернула некорректное число (NaN/Inf)");
                    }
                    return result;
                    });

                std::cout << "[Плагин загружен] Функция '" << funcName << "' (файл: " << lib.filename() << ")\n";
                loadedLibs_.push_back(std::move(lib));
                ++successCount;
            }
            catch (const std::exception& e) {
                std::cerr << "[Ошибка загрузки] " << e.what() << "\n";
            }
        }

        if (dllCount == 0) {
            std::cout << "[Инфо] В папке 'plugins' нет ни одного .dll файла. Доступны только базовые операции (+, -, *, /, ^, ()).\n";
        }
        else {
            std::cout << "[Инфо] Успешно подключено плагинов: " << successCount << " из " << dllCount << ".\n";
        }
    }
};


int main() {
    
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    PluginManager pluginManager;
    Calculator calculator;

    pluginManager.loadAll(calculator);

    std::cout << "\nКалькулятор готов к работе. Введите выражение (или 'exit' для выхода).\n";
    std::cout << "Пример: 16 + 4 * (3 - 1)  или  2^4 + sin(90)\n\n";

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        // заменяем длинное тире '-' на обычный минус '-'
        for (std::size_t i = 0; i < line.size(); ++i) {
            if (static_cast<unsigned char>(line[i]) == 0x96) { 
                line[i] = '-';
            }
        }

        std::size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }
        std::size_t last = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(first, last - first + 1);

        if (trimmed == "exit" || trimmed == "quit") {
            break;
        }

        try {
            double result = calculator.calculate(trimmed);
            std::cout << "Вывод: " << result << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "Ошибка: " << e.what() << "\n";
        }
    }

    // очищаем ссылки на функции до выгрузки DLL
    calculator.clearFunctions();
    std::cout << "Работа калькулятора завершена.\n";
    return 0;
}
