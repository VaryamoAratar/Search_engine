#pragma once
#include <string>

// Класс для работы с конфигурацией программы
class Config {
public:
    // Конструктор, который принимает имя файла конфигурации
    Config(const std::string& filename);

    // Методы для получения значений параметров конфигурации
    std::string getDbHost() const { return dbHost; }           // Получить хост базы данных
    int getDbPort() const { return dbPort; }                   // Получить порт базы данных
    std::string getDbName() const { return dbName; }           // Получить имя базы данных
    std::string getDbUser() const { return dbUser; }           // Получить имя пользователя для базы данных
    std::string getDbPassword() const { return dbPassword; }   // Получить пароль для базы данных
    std::string getDbConnectionString() const;                 // Получить строку подключения для базы данных

    std::string getStartUrl() const { return startUrl; }       // Получить начальный URL для сканирования
    int getMaxDepth() const { return maxDepth; }               // Получить максимальную глубину сканирования
    int getTimeout() const { return timeout; }                 // Получить таймаут запроса
    bool shouldFilterStopwords() const { return filterStopwords; }  // Проверить, нужно ли фильтровать стоп-слова

    int getServerPort() const { return serverPort; }           // Получить порт сервера

    bool isConsoleLoggingEnabled() const { return logToConsole; } // Проверить, включен ли вывод в консоль
    bool isFileLoggingEnabled() const { return logToFile; }     // Проверить, включен ли вывод в файл
    std::string getLogDir() const { return logDir; }           // Получить директорию для логов

private:
    // Переменные, хранящие параметры конфигурации
    std::string dbHost;        // Хост базы данных
    int dbPort;                // Порт базы данных
    std::string dbName;        // Имя базы данных
    std::string dbUser;        // Имя пользователя базы данных
    std::string dbPassword;    // Пароль базы данных

    std::string startUrl;      // Начальный URL для сканирования
    int maxDepth;              // Максимальная глубина сканирования
    int timeout;               // Таймаут запроса
    bool filterStopwords;      // Флаг, указывающий на необходимость фильтрации стоп-слов

    int serverPort;            // Порт сервера

    bool logToConsole;         // Флаг, указывающий на вывод логов в консоль
    bool logToFile;            // Флаг, указывающий на вывод логов в файл
    std::string logDir;        // Директория для логов
};
