#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <thread>
#include <future>
#include <atomic>
#include "config.hpp"
#include "logger.hpp"
#include "database.hpp"

// Класс для реализации краулера (поискового робота), который будет собирать страницы с сайтов
class Crawler {
public:
    // Конструктор, инициализирует краулер с конфигурацией, логером, базой данных и флагом работы
    explicit Crawler(const Config& config, Logger& logger, Database& db, std::atomic<bool>& running);

    // Метод для запуска работы краулера
    void start();

private:
    // Метод для получения содержимого страницы по URL
    std::string fetchPage(const std::string& url);

    // Метод для извлечения всех ссылок из содержимого страницы
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl);

    // Метод для индексации страницы (сохранение информации в базу данных)
    void indexPage(const std::string& url, const std::string& content);

    // Ссылка на объект конфигурации
    const Config& config;

    // Ссылка на объект логера для записи логов
    Logger& logger;

    // Ссылка на объект базы данных для хранения данных
    Database& db;

    // Мьютекс для синхронизации доступа к очереди URL
    std::mutex queueMutex;

    // Очередь для хранения URL-ов и уровня их глубины
    std::queue<std::pair<std::string, int>> urlQueue;

    // Ссылка на флаг, который управляет состоянием работы краулера
    std::atomic<bool>& running;

    // Множество посещённых URL-ов для предотвращения повторного посещения
    std::unordered_set<std::string> visited;
};
