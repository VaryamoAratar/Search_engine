#pragma once

#include "config.hpp"
#include "logger.hpp"
#include "database.hpp"

#include <atomic>                  // Для атомарных переменных
#include <boost/asio/ip/tcp.hpp>   // Для работы с TCP-сокетами через Boost.Asio
#include <memory>                  // Для умных указателей

// Класс для реализации поискового сервера
class SearchServer {
public:
    // Конструктор, который инициализирует сервер с конфигурацией, логером, базой данных и флагом работы
    SearchServer(const Config& config, Logger& logger, Database& db, std::atomic<bool>& running);

    // Метод для запуска сервера
    void run();

private:
    // Ссылка на объект конфигурации
    const Config& config;

    // Ссылка на объект логера для записи логов
    Logger& logger;

    // Ссылка на объект базы данных для хранения данных
    Database& db;

    // Ссылка на флаг, который управляет состоянием работы сервера
    std::atomic<bool>& running;

    // Метод для инициализации и старта TCP-сервера
    void startServer();

    // Метод для обработки клиентских сессий (взаимодействие с клиентом через TCP-сокет)
    void handleSession(boost::asio::ip::tcp::socket socket);
};
