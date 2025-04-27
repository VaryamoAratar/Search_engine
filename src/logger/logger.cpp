#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>      
#include <ctime>
#include <filesystem>
#include <sstream> 

// Конструктор класса Logger, который настраивает логирование в консоль и/или в файл
Logger::Logger(const Config& config)
    : logToConsole(config.isConsoleLoggingEnabled()), // Определяем, нужно ли логировать в консоль
    logToFile(config.isFileLoggingEnabled()) {      // Определяем, нужно ли логировать в файл

    // Если логирование в файл включено, создаём директории для логов и открываем файл
    if (logToFile) {
        std::filesystem::create_directories(config.getLogDir());  // Создаём директорию, если её нет
        std::string filename = config.getLogDir() + "/log.txt";    // Имя файла лога
        fileStream.open(filename, std::ios::app);  // Открываем файл в режиме добавления
        if (!fileStream) {  // Проверяем, удалось ли открыть файл
            throw std::runtime_error("Не удалось открыть файл логов: " + filename);  // Генерируем исключение, если файл не открылся
        }
    }
}

// Деструктор, закрывающий файл лога при уничтожении объекта
Logger::~Logger() {
    if (fileStream.is_open()) {
        fileStream.close();  // Закрываем файл, если он открыт
    }
}

// Метод для логирования информационных сообщений
void Logger::info(const std::string& message) {
    log("INFO", message);  // Вызываем общий метод log с уровнем INFO
}

// Метод для логирования ошибок
void Logger::error(const std::string& message) {
    log("ERROR", message);  // Вызываем общий метод log с уровнем ERROR
}

// Метод для логирования предупреждений
void Logger::warn(const std::string& message) {
    log("WARN", message);  // Вызываем общий метод log с уровнем WARN
}

// Общий метод для логирования сообщений с указанным уровнем
void Logger::log(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);  // Защищаем доступ к логированию с помощью мьютекса

    auto now = std::chrono::system_clock::now();  // Получаем текущее время
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);  // Преобразуем в time_t для использования с std::localtime
    std::tm* localTime = std::localtime(&timeNow);  // Преобразуем в локальное время

    std::ostringstream oss;  // Строковый поток для форматированного вывода
    oss << "[" << std::put_time(localTime, "%Y-%m-%d %H:%M:%S") << "] ";  // Форматируем время в строку

    // Определяем цвет для вывода в консоль в зависимости от уровня логирования
    std::string colorCode;
    if (level == "INFO") colorCode = "\033[32m";     // Зеленый для INFO
    else if (level == "WARN") colorCode = "\033[33m"; // Желтый для WARN
    else if (level == "ERROR") colorCode = "\033[31m"; // Красный для ERROR
    else colorCode = "\033[0m";                      // Без цвета для остальных уровней

    oss << "[" << level << "] " << message << "\n";  // Формируем финальное сообщение

    // Логируем в консоль, если это указано в конфигурации
    if (logToConsole) {
        std::cout << colorCode << oss.str() << "\033[0m"; // Выводим с цветом, сбрасывая цвет после сообщения
    }

    // Логируем в файл, если это указано и файл открыт
    if (logToFile && fileStream.is_open()) {
        fileStream << oss.str();  // Записываем сообщение в файл
        fileStream.flush();        // Немедленно записываем в файл
    }
}
