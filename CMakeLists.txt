cmake_minimum_required(VERSION 3.10)
project(MyCalculator)

# Устанавливаем современный стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Указываем глобальную директорию с заголовочными файлами
# Это ОБЯЗАТЕЛЬНО должно быть до add_executable и add_library
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 2. Собираем основное приложение
add_executable(calc src/main.cpp) # Добавьте другие .cpp файлы, если они есть

# 3. Настройка вывода для плагинов
set(PLUGIN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/plugins")

# 4. Макрос для удобного добавления плагинов
macro(add_calc_plugin PLUGIN_NAME PLUGIN_SRC)
    add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SRC})
    
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
    )
    
    # Говорим CMake, что основной проект зависит от плагинов
    add_dependencies(calc ${PLUGIN_NAME})
endmacro()

# 5. Собираем плагины
add_calc_plugin(funccos plugins/funccos.cpp)
add_calc_plugin(funcexp plugins/funcexp.cpp)
add_calc_plugin(funcln plugins/funcln.cpp)
add_calc_plugin(funcsin plugins/funcsin.cpp)
add_calc_plugin(funcsqrt plugins/funcsqrt.cpp)