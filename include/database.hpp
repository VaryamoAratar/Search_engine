#pragma once

#include "config.hpp"
#include "logger.hpp"

#include <pqxx/pqxx>  // Библиотека для работы с PostgreSQL
#include <string>
#include <vector>
#include <unordered_map>

// Класс для работы с базой данных, включая создание таблиц, сохранение документов и выполнение поиска
class Database {
public:
    // Конструктор, который инициализирует базу данных с конфигурацией и логером
    Database(const Config& config, Logger& logger);

    // Метод для создания таблиц в базе данных
    void init();

    // Метод для сохранения документа в базе данных с URL и частотами слов
    void saveDocument(const std::string& url, const std::unordered_map<std::string, int>& words);

    // Метод для выполнения поиска по запросу (список слов) в базе данных
    std::vector<std::pair<std::string, int>> search(const std::vector<std::string>& queryWords);

private:
    pqxx::connection connection;  // Соединение с базой данных PostgreSQL
    Logger& logger;               // Логер для записи логов
};
