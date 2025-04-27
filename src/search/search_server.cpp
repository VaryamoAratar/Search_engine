#include "search_server.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/locale.hpp>

#include <fstream>
#include <sstream>
#include <thread>
#include <algorithm>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

// Конструктор SearchServer: инициализация с конфигурацией, логгером, базой данных и флагом работы сервера
SearchServer::SearchServer(const Config& config, Logger& logger, Database& db, std::atomic<bool>& running)
    : config(config), logger(logger), db(db), running(running) {
}

// Метод запуска сервера
void SearchServer::run() {
    startServer();
}

// Метод для старта сервера, включает настройки и запуск потоков
void SearchServer::startServer() {
    try {
        boost::asio::io_context ioc{ 1 };

        // Ловим SIGINT и SIGTERM для корректного завершения
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([this, &ioc](boost::system::error_code /*ec*/, int /*signo*/) {
            logger.info("Получен сигнал остановки. Завершаем сервер...");
            running = false;
            ioc.stop(); // Останавливаем io_context
            });

        // Создаём TCP-акцептор для прослушивания порта сервера
        tcp::acceptor acceptor{ ioc, {tcp::v4(), static_cast<unsigned short>(config.getServerPort())} };
        logger.info("HTTP-сервер запущен на порту " + std::to_string(config.getServerPort()));

        // Пул потоков для обработки запросов
        boost::asio::thread_pool pool(std::thread::hardware_concurrency());

        // Лямбда-функция для асинхронного приёма подключений
        std::function<void()> do_accept;
        do_accept = [&]() {
            auto socket = std::make_shared<tcp::socket>(ioc);
            acceptor.async_accept(*socket, [&, socket](boost::system::error_code ec) {
                if (!ec) {
                    boost::asio::post(pool, [this, socket]() mutable {
                        handleSession(std::move(*socket)); // Обрабатываем подключение
                        });
                }
                if (running) {
                    do_accept(); // Ожидаем следующее подключение
                }
                });
            };

        do_accept(); // Старт первого accept'а

        // Запускаем цикл событий
        ioc.run();

        pool.join(); // Ждём завершения всех задач в пуле
        logger.info("Сервер остановлен.");
    }
    catch (const std::exception& ex) {
        logger.error(std::string("Ошибка запуска сервера: ") + ex.what());
    }
}

// Метод обработки HTTP-сессии
void SearchServer::handleSession(boost::asio::ip::tcp::socket socket) {
    try {
        boost::beast::flat_buffer buffer; // Буфер для чтения данных
        http::request<http::string_body> req; // HTTP-запрос
        http::read(socket, buffer, req); // Чтение запроса

        http::response<http::string_body> res; // HTTP-ответ
        res.version(req.version()); // Установка версии HTTP
        res.keep_alive(false); // Отключение поддержки keep-alive

        // Обработка GET-запроса на главную страницу
        if (req.method() == http::verb::get && req.target() == "/") {
            std::ifstream file("html/search_form.html"); // Чтение HTML-шаблона формы
            std::stringstream ss;
            ss << file.rdbuf();
            res.result(http::status::ok); // Устанавливаем статус OK
            res.set(http::field::content_type, "text/html"); // Устанавливаем тип контента
            res.body() = ss.str(); // Тело ответа

        }
        // Обработка POST-запроса на поиск
        else if (req.method() == http::verb::post && req.target() == "/search") {
            // Функция декодирования URL
            auto urlDecode = [](const std::string& str) -> std::string {
                std::string result;
                std::istringstream iss(str);
                char ch;
                while (iss.get(ch)) {
                    if (ch == '%') {
                        std::string hex;
                        if (iss.get(ch)) hex += ch;
                        if (iss.get(ch)) hex += ch;
                        result += static_cast<char>(std::stoi(hex, nullptr, 16)); // Декодируем символы
                    }
                    else if (ch == '+') {
                        result += ' '; // Заменяем "+" на пробел
                    }
                    else {
                        result += ch;
                    }
                }
                return result;
                };

            std::string query = req.body(); // Получаем тело запроса
            std::string cleaned;
            std::istringstream in(query);
            std::string key, val;
            if (std::getline(in, key, '=') && std::getline(in, val)) {
                cleaned = urlDecode(val); // Декодируем параметры
            }

            logger.info("Тело запроса (decoded) = [" + cleaned + "]");

            // Разделяем запрос на слова
            std::istringstream wordStream(cleaned);
            std::string w;
            std::vector<std::string> words;
            while (wordStream >> w) {
                words.push_back(w);
            }

            // Нормализуем слова
            std::vector<std::string> normalizedWords;
            for (auto& word : words) {
                normalizedWords.push_back(
                    boost::locale::normalize(
                        boost::locale::to_lower(word), boost::locale::norm_default
                    )
                );
            }

            // Логируем нормализованные слова
            for (const auto& word : normalizedWords) {
                logger.info("Поисковое слово после нормализации: [" + word + "]");
            }

            // Выполняем поиск по базе данных
            auto results = db.search(normalizedWords);

            // Чтение HTML-шаблона для результатов поиска
            std::ifstream templateFile("html/search_results.html");
            std::stringstream ss;
            ss << templateFile.rdbuf();
            std::string templateHtml = ss.str();

            // Формирование результатов поиска
            std::ostringstream resultsHtml;
            if (results.empty()) {
                resultsHtml << "<p><em>Ничего не найдено.</em></p>";
            }
            else {
                resultsHtml << "<ul>";
                for (const auto& [url, score] : results) {
                    resultsHtml << "<li><a href='" << url << "'>" << url << "</a> — рейтинг: " << score << "</li>";
                }
                resultsHtml << "</ul>";
            }

            // Вставляем результаты в шаблон
            size_t pos = templateHtml.find("<!--RESULTS-->");
            if (pos != std::string::npos) {
                templateHtml.replace(pos, std::string("<!--RESULTS-->").length(), resultsHtml.str());
            }

            // Формируем ответ
            res.result(http::status::ok);
            res.set(http::field::content_type, "text/html");
            res.body() = templateHtml;
        }
        else {
            // Если путь не найден, возвращаем 404
            res.result(http::status::not_found);
            res.set(http::field::content_type, "text/html");
            res.body() = "404 Not Found";
        }

        res.prepare_payload(); // Подготавливаем тело ответа
        http::write(socket, res); // Отправляем ответ
    }
    catch (const std::exception& e) {
        logger.error(std::string("Ошибка обработки запроса: ") + e.what()); // Логируем ошибку
    }
}
