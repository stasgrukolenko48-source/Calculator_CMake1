#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <functional>

class Calculator {
public:
    using MathFunction = std::function<double(double)>;

    void registerFunction(const std::string& name, MathFunction func);
    void clearFunctions() noexcept;
    double calculate(std::string_view expression) const;

private:
    std::unordered_map<std::string, MathFunction> functions_;
};
