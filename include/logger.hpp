#pragma once

#include "config.hpp"
#include <fstream>  // Для работы с файловым потоком
#include <mutex>    // Для синхронизации потоков
#include <string>

// Класс для логирования сообщений (информация, ошибки, предупреждения)
class Logger {
public:
    // Конструктор, который инициализирует логер с конфигурацией
    Logger(const Config& config);

    // Деструктор, который закрывает файл (если логирование в файл включено)
    ~Logger();

    // Метод для записи информационного сообщения
    void info(const std::string& message);

    // Метод для записи сообщения об ошибке
    void error(const std::string& message);

    // Метод для записи предупреждающего сообщения
    void warn(const std::string& message);

private:
    // Метод для записи сообщения с определённым уровнем логирования
    void log(const std::string& level, const std::string& message);

    // Флаг, указывающий, нужно ли выводить логи в консоль
    bool logToConsole;

    // Флаг, указывающий, нужно ли выводить логи в файл
    bool logToFile;

    // Поток для записи логов в файл (если logToFile = true)
    std::ofstream fileStream;

    // Мьютекс для синхронизации логирования в многопоточной среде
    std::mutex logMutex;
};
