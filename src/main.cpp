#include "config.hpp"
#include "logger.hpp"
#include "database.hpp"
#include "crawler.hpp"
#include "search_server.hpp"
#include <boost/locale.hpp>
#include <csignal>  
#include <iostream>
#include <string>
#include <atomic> 

// Используем атомарную переменную для отслеживания состояния работы приложения
std::atomic<bool> running(true); // Переменная для отслеживания состояния работы

// Обработчик сигнала прерывания
void signalHandler(int signum) {
    std::cout << "\nПолучен сигнал прерывания (" << signum << "). Завершение работы...\n";
    running = false; // Устанавливаем флаг завершения работы программы
}

int main(int argc, char* argv[]) {
    // Настроим локаль для корректного отображения русских символов
    boost::locale::generator gen;
    std::locale::global(gen("ru_RU.UTF-8"));  // Настроим локаль "ru_RU.UTF-8"
    std::cout.imbue(std::locale("ru_RU.UTF-8"));
    std::cout << "Текущая локаль: " << std::locale().name() << std::endl; // Выводим текущую локаль

    // Проверяем количество аргументов командной строки
    if (argc < 3) {
        std::cerr << "Используйте: " << argv[0] << " <config.ini> <crawler|server>\n";
        return 1; // Выход с ошибкой, если аргументы не заданы
    }

    // Читаем конфигурационный файл и режим работы из аргументов командной строки
    std::string configFile = argv[1];
    std::string mode = argv[2];

    // Регистрация обработчика сигнала прерывания (SIGINT)
    std::signal(SIGINT, signalHandler);

    try {
        // Инициализируем конфигурацию, логгер и базу данных
        Config config(configFile);
        Logger logger(config);
        Database db(config, logger);

        // В зависимости от режима запускаем соответствующий компонент
        if (mode == "crawler") {
            logger.info("Режим: Краулер");
            db.init();  // Инициализация базы данных
            Crawler crawler(config, logger, db, running); // Создаём объект краулера
            crawler.start(); // Запускаем краулер
        }
        else if (mode == "server") {
            logger.info("Режим: Сервер");
            SearchServer server(config, logger, db, running); // Создаём объект сервера
            server.run(); // Запускаем сервер
        }
        else {
            std::cerr << "Неизвестный режим: " << mode << "\n"; // Если режим не указан правильно
            return 1;
        }

    }
    catch (const std::exception& ex) {
        // Ловим исключения и выводим сообщение об ошибке
        std::cerr << "Ошибка: " << ex.what() << "\n";
        return 1; // Выход с ошибкой
    }

    return 0; // Успешное завершение программы
}
