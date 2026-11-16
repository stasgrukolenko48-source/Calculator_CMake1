#include "calculator.h"
#include <stdexcept>
#include <cctype>
#include <cstdlib>
#include <cmath>

class Parser {
private:
    std::string_view input_;
    std::size_t pos_ = 0;
    const std::unordered_map<std::string, Calculator::MathFunction>& funcs_;

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
            if (!std::isalnum(ch) && ch != '_') break;
            ++pos_;
        }
        if (start == pos_) throw std::runtime_error("Ожидалось имя функции");
        return std::string(input_.substr(start, pos_ - start));
    }

    double parseExpression() {
        double result = parseTerm();
        while (true) {
            if (match('+')) result += parseTerm();
            else if (match('-')) result -= parseTerm();
            else break;
        }
        return result;
    }

    double parseTerm() {
        double result = parseUnary();
        while (true) {
            if (match('*')) result *= parseUnary();
            else if (match('/')) {
                double divisor = parseUnary();
                if (divisor == 0.0) throw std::domain_error("Деление на ноль");
                result /= divisor;
            }
            else break;
        }
        return result;
    }

    double parseUnary() {
        if (match('+')) return parseUnary();
        if (match('-')) return -parseUnary();
        return parsePower();
    }

    double parsePower() {
        double base = parsePrimary();
        if (match('^')) {
            double exponent = parsePower(); 
            if (base < 0.0 && std::floor(exponent) != exponent) throw std::domain_error("Возведение отрицательного числа в дробную степень");
            if (base == 0.0 && exponent <= 0.0) throw std::domain_error("Ноль в неположительной степени");
            double res = std::pow(base, exponent);
            if (!std::isfinite(res)) throw std::overflow_error("Переполнение");
            return res;
        }
        return base;
    }

    double parsePrimary() {
        if (match('(')) {
            if (match(')')) throw std::invalid_argument("Пустые скобки");
            double result = parseExpression();
            if (!match(')')) throw std::invalid_argument("Пропущена скобка");
            return result;
        }

        skipSpaces();
        if (pos_ < input_.size()) {
            unsigned char ch = static_cast<unsigned char>(input_[pos_]);
            if (std::isalpha(ch) || ch == '_') {
                std::string funcName = parseIdentifier();
                if (!match('(')) throw std::invalid_argument("Ожидалась '('");
                if (match(')')) throw std::invalid_argument("Функция вызвана без аргумента");
                double arg = parseExpression();
                if (!match(')')) throw std::invalid_argument("Пропущена ')'");

                auto it = funcs_.find(funcName);
                if (it == funcs_.end()) throw std::invalid_argument("Неизвестная функция");
                return it->second(arg);
            }
        }
        return parseNumber();
    }

    double parseNumber() {
        skipSpaces();
        if (pos_ >= input_.size()) throw std::invalid_argument("Ожидалось число");
        const char* start = input_.data() + pos_;
        char* end = nullptr;
        double value = std::strtod(start, &end);
        if (start == end) throw std::invalid_argument("Неожиданный символ");
        pos_ += static_cast<std::size_t>(end - start);
        if (!std::isfinite(value)) throw std::out_of_range("Число вне пределов");
        return value;
    }

public:
    Parser(std::string_view input, const std::unordered_map<std::string, Calculator::MathFunction>& funcs)
        : input_(input), funcs_(funcs) {}

    double parse() {
        double result = parseExpression();
        skipSpaces();
        if (pos_ != input_.size()) throw std::invalid_argument("Лишний символ");
        if (!std::isfinite(result)) throw std::overflow_error("Результат вне пределов");
        return result;
    }
};

void Calculator::registerFunction(const std::string& name, MathFunction func) {
    functions_[name] = std::move(func);
}

void Calculator::clearFunctions() noexcept {
    functions_.clear();
}

double Calculator::calculate(std::string_view expression) const {
    Parser parser(expression, functions_);
    return parser.parse();
}
