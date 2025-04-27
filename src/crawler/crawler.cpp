#include "crawler.hpp"
#include "indexer.hpp"
#include "utils.hpp"

#include <boost/beast/core.hpp>        // Для работы с core компонентами Boost.Beast
#include <boost/beast/http.hpp>        // Для работы с HTTP запросами
#include <boost/beast/version.hpp>     // Для получения информации о версии Boost.Beast
#include <boost/asio/connect.hpp>      // Для подключения с использованием Boost.Asio
#include <boost/asio/ip/tcp.hpp>       // Для работы с TCP-сокетами через Boost.Asio
#include <regex>                       // Для работы с регулярными выражениями
#include <unordered_set>               // Для использования unordered_set
#include <condition_variable>          // Для использования condition_variable
#include <atomic>                      // Для атомарных переменных

using tcp = boost::asio::ip::tcp;    // Сокращение для работы с TCP
namespace http = boost::beast::http; // Пространство имен для HTTP запросов

// Конструктор класса Crawler инициализирует его с конфигурацией, логером, базой данных и флагом работы
Crawler::Crawler(const Config& config, Logger& logger, Database& db, std::atomic<bool>& running)
    : config(config), logger(logger), db(db), running(running) {
}

// Метод запуска краулера
void Crawler::start() {
    while (running)
    {
        // Добавляем начальный URL в очередь и отмечаем его как посещённый
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            urlQueue.push({ config.getStartUrl(), 1 });
            visited.insert(config.getStartUrl());
        }

        logger.info("Starting crawl from: " + config.getStartUrl());
        logger.info("Timeout set to: " + std::to_string(config.getTimeout()) + "ms");

        std::condition_variable queueCv;       // Для синхронизации потоков
        std::mutex queueCvMutex;               // Мьютекс для блокировки очереди

        std::vector<std::thread> workers;      // Вектор для хранения рабочих потоков
        int numThreads = std::thread::hardware_concurrency(); // Получаем количество доступных потоков
        for (int i = 0; i < numThreads; ++i) {
            // Создаём рабочие потоки
            workers.emplace_back([this, &queueCv, &queueCvMutex]() {
                while (true) {
                    std::pair<std::string, int> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        // Ожидаем, пока очередь не станет пустой или не завершится работа
                        queueCv.wait(lock, [this]() { return !urlQueue.empty() || !running; });

                        if (!running && urlQueue.empty())
                            return; // Выход, если работа завершена

                        task = urlQueue.front(); // Берём задачу из очереди
                        urlQueue.pop();
                    }

                    // Пропускаем ссылки, которые уже глубже, чем максимальная глубина
                    if (task.second > config.getMaxDepth()) continue;

                    try {
                        logger.info("Fetching page: " + task.first);
                        std::string html = fetchPage(task.first); // Загружаем страницу
                        if (html.empty()) {
                            logger.error("Failed to fetch page: " + task.first);
                            continue; // Если страница не загружена, переходим к следующей
                        }

                        indexPage(task.first, html); // Индексируем страницу

                        // Извлекаем все ссылки со страницы
                        for (const auto& link : extractLinks(html, task.first)) {
                            int nextDepth = task.second + 1;
                            if (nextDepth > config.getMaxDepth()) continue;

                            {
                                std::lock_guard<std::mutex> lock(queueMutex);
                                if (visited.contains(link)) continue; // Пропускаем уже посещённые ссылки
                                visited.insert(link);
                                urlQueue.emplace(link, nextDepth); // Добавляем новую ссылку в очередь
                            }
                            queueCv.notify_one(); // Оповещаем другие потоки
                            logger.info("Extracted link: " + link); // Логируем извлечённую ссылку
                        }
                    }
                    catch (const std::exception& ex) {
                        logger.error("Error crawling " + task.first + ": " + ex.what()); // Логируем ошибку
                    }
                }
                });
        }

        // Ожидаем завершения обработки всех задач в очереди
        while (true) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (urlQueue.empty()) break; // Выход, если очередь пуста
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Задержка
        }

        running = false; // Останавливаем краулер
        queueCv.notify_all(); // Оповещаем все потоки о завершении работы

        // Ожидаем завершения всех рабочих потоков
        for (auto& worker : workers) {
            if (worker.joinable())
                worker.join();
        }
        if (!running) {
            break; // Прерываем цикл, если running == false
        }
    }
    logger.info("Crawling finished."); // Логируем завершение работы
}

// Метод для получения HTML-контента страницы с заданным таймаутом
std::string Crawler::fetchPage(const std::string& url) {
    auto future = std::async(std::launch::async, [&]() {
        return Utils::httpGet(url, config.getTimeout()); // Выполняем HTTP GET-запрос
        });

    // Ожидаем завершения запроса в пределах таймаута
    if (future.wait_for(std::chrono::milliseconds(config.getTimeout())) == std::future_status::ready) {
        return future.get(); // Возвращаем полученный результат
    }
    else {
        logger.warn("Timeout occurred while fetching: " + url); // Логируем таймаут
        return ""; // Возвращаем пустую строку в случае таймаута
    }
}

// Метод для извлечения ссылок из HTML-контента страницы
std::vector<std::string> Crawler::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    std::regex hrefRegex(R"(<a\s+(?:[^>]*?\s+)?href=["'](.*?)["'])", std::regex::icase); // Регулярное выражение для поиска ссылок
    std::smatch match;
    std::string::const_iterator searchStart(html.cbegin());

    // Ищем все соответствия регулярному выражению
    while (std::regex_search(searchStart, html.cend(), match, hrefRegex)) {
        std::string link = match[1]; // Извлекаем ссылку
        if (Utils::isHttpUrl(link)) {
            links.push_back(link); // Абсолютные ссылки
        }
        else if (Utils::isRelativeUrl(link)) {
            links.push_back(Utils::resolveRelativeUrl(baseUrl, link)); // Преобразуем относительные ссылки в абсолютные
        }
        searchStart = match.suffix().first;
    }
    return links; // Возвращаем все извлечённые ссылки
}

// Метод для индексации страницы и сохранения её в базе данных
void Crawler::indexPage(const std::string& url, const std::string& content) {
    logger.info("Indexing: " + url);
    Indexer indexer(config, logger); // Создаём индексатор
    auto words = indexer.extractWords(content); // Извлекаем слова из контента

    if (words.empty()) {
        logger.error("No words extracted from: " + url); // Если слов не извлечено, логируем ошибку
        return;
    }

    logger.info("Extracted words count: " + std::to_string(words.size())); // Логируем количество извлечённых слов
    db.saveDocument(url, words); // Сохраняем данные в базе
}
