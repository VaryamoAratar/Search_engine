#include "config.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

// Конструктор класса Config, загружает параметры из конфигурационного файла
Config::Config(const std::string& filename) {
    // Создаём объект для парсинга INI-файла
    boost::property_tree::ptree pt;
    // Читаем конфигурацию из файла и заполняем дерево свойств
    boost::property_tree::ini_parser::read_ini(filename, pt);

    // Загружаем параметры базы данных
    dbHost = pt.get<std::string>("database.host");      // Хост базы данных
    dbPort = pt.get<int>("database.port");              // Порт базы данных
    dbName = pt.get<std::string>("database.name");      // Имя базы данных
    dbUser = pt.get<std::string>("database.user");      // Имя пользователя для базы данных
    dbPassword = pt.get<std::string>("database.password"); // Пароль базы данных

    // Загружаем параметры для краулера
    startUrl = pt.get<std::string>("crawler.start_url"); // Начальный URL для сканирования
    maxDepth = pt.get<int>("crawler.depth");             // Максимальная глубина сканирования
    timeout = pt.get<int>("crawler.timeout");            // Таймаут для запросов
    filterStopwords = pt.get<bool>("crawler.filter_stopwords"); // Флаг для фильтрации стоп-слов

    // Загружаем параметры для сервера
    serverPort = pt.get<int>("server.port");             // Порт сервера

    // Загружаем параметры для логирования
    logToConsole = pt.get<bool>("logging.console");      // Флаг для вывода логов в консоль
    logToFile = pt.get<bool>("logging.file");            // Флаг для записи логов в файл
    logDir = pt.get<std::string>("logging.log_dir");     // Директория для файлов логов
}

// Метод для формирования строки подключения к базе данных
std::string Config::getDbConnectionString() const {
    // Строим строку подключения в формате "host=..., port=..., dbname=..., user=..., password=..."
    return "host=" + dbHost + " port=" + std::to_string(dbPort) +
        " dbname=" + dbName + " user=" + dbUser + " password=" + dbPassword;
}
