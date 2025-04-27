#include "crawler.hpp"
#include "indexer.hpp"
#include "utils.hpp"

#include <boost/beast/core.hpp>        // ��� ������ � core ������������ Boost.Beast
#include <boost/beast/http.hpp>        // ��� ������ � HTTP ���������
#include <boost/beast/version.hpp>     // ��� ��������� ���������� � ������ Boost.Beast
#include <boost/asio/connect.hpp>      // ��� ����������� � �������������� Boost.Asio
#include <boost/asio/ip/tcp.hpp>       // ��� ������ � TCP-�������� ����� Boost.Asio
#include <regex>                       // ��� ������ � ����������� �����������
#include <unordered_set>               // ��� ������������� unordered_set
#include <condition_variable>          // ��� ������������� condition_variable
#include <atomic>                      // ��� ��������� ����������

using tcp = boost::asio::ip::tcp;    // ���������� ��� ������ � TCP
namespace http = boost::beast::http; // ������������ ���� ��� HTTP ��������

// ����������� ������ Crawler �������������� ��� � �������������, �������, ����� ������ � ������ ������
Crawler::Crawler(const Config& config, Logger& logger, Database& db, std::atomic<bool>& running)
    : config(config), logger(logger), db(db), running(running) {
}

// ����� ������� ��������
void Crawler::start() {
    while (running)
    {
        // ��������� ��������� URL � ������� � �������� ��� ��� ����������
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            urlQueue.push({ config.getStartUrl(), 1 });
            visited.insert(config.getStartUrl());
        }

        logger.info("Starting crawl from: " + config.getStartUrl());
        logger.info("Timeout set to: " + std::to_string(config.getTimeout()) + "ms");

        std::condition_variable queueCv;       // ��� ������������� �������
        std::mutex queueCvMutex;               // ������� ��� ���������� �������

        std::vector<std::thread> workers;      // ������ ��� �������� ������� �������
        int numThreads = std::thread::hardware_concurrency(); // �������� ���������� ��������� �������
        for (int i = 0; i < numThreads; ++i) {
            // ������ ������� ������
            workers.emplace_back([this, &queueCv, &queueCvMutex]() {
                while (true) {
                    std::pair<std::string, int> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        // �������, ���� ������� �� ������ ������ ��� �� ���������� ������
                        queueCv.wait(lock, [this]() { return !urlQueue.empty() || !running; });

                        if (!running && urlQueue.empty())
                            return; // �����, ���� ������ ���������

                        task = urlQueue.front(); // ���� ������ �� �������
                        urlQueue.pop();
                    }

                    // ���������� ������, ������� ��� ������, ��� ������������ �������
                    if (task.second > config.getMaxDepth()) continue;

                    try {
                        logger.info("Fetching page: " + task.first);
                        std::string html = fetchPage(task.first); // ��������� ��������
                        if (html.empty()) {
                            logger.error("Failed to fetch page: " + task.first);
                            continue; // ���� �������� �� ���������, ��������� � ���������
                        }

                        indexPage(task.first, html); // ����������� ��������

                        // ��������� ��� ������ �� ��������
                        for (const auto& link : extractLinks(html, task.first)) {
                            int nextDepth = task.second + 1;
                            if (nextDepth > config.getMaxDepth()) continue;

                            {
                                std::lock_guard<std::mutex> lock(queueMutex);
                                if (visited.contains(link)) continue; // ���������� ��� ���������� ������
                                visited.insert(link);
                                urlQueue.emplace(link, nextDepth); // ��������� ����� ������ � �������
                            }
                            queueCv.notify_one(); // ��������� ������ ������
                            logger.info("Extracted link: " + link); // �������� ����������� ������
                        }
                    }
                    catch (const std::exception& ex) {
                        logger.error("Error crawling " + task.first + ": " + ex.what()); // �������� ������
                    }
                }
                });
        }

        // ������� ���������� ��������� ���� ����� � �������
        while (true) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (urlQueue.empty()) break; // �����, ���� ������� �����
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // ��������
        }

        running = false; // ������������� �������
        queueCv.notify_all(); // ��������� ��� ������ � ���������� ������

        // ������� ���������� ���� ������� �������
        for (auto& worker : workers) {
            if (worker.joinable())
                worker.join();
        }
        if (!running) {
            break; // ��������� ����, ���� running == false
        }
    }
    logger.info("Crawling finished."); // �������� ���������� ������
}

// ����� ��� ��������� HTML-�������� �������� � �������� ���������
std::string Crawler::fetchPage(const std::string& url) {
    auto future = std::async(std::launch::async, [&]() {
        return Utils::httpGet(url, config.getTimeout()); // ��������� HTTP GET-������
        });

    // ������� ���������� ������� � �������� ��������
    if (future.wait_for(std::chrono::milliseconds(config.getTimeout())) == std::future_status::ready) {
        return future.get(); // ���������� ���������� ���������
    }
    else {
        logger.warn("Timeout occurred while fetching: " + url); // �������� �������
        return ""; // ���������� ������ ������ � ������ ��������
    }
}

// ����� ��� ���������� ������ �� HTML-�������� ��������
std::vector<std::string> Crawler::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    std::regex hrefRegex(R"(<a\s+(?:[^>]*?\s+)?href=["'](.*?)["'])", std::regex::icase); // ���������� ��������� ��� ������ ������
    std::smatch match;
    std::string::const_iterator searchStart(html.cbegin());

    // ���� ��� ������������ ����������� ���������
    while (std::regex_search(searchStart, html.cend(), match, hrefRegex)) {
        std::string link = match[1]; // ��������� ������
        if (Utils::isHttpUrl(link)) {
            links.push_back(link); // ���������� ������
        }
        else if (Utils::isRelativeUrl(link)) {
            links.push_back(Utils::resolveRelativeUrl(baseUrl, link)); // ����������� ������������� ������ � ����������
        }
        searchStart = match.suffix().first;
    }
    return links; // ���������� ��� ����������� ������
}

// ����� ��� ���������� �������� � ���������� � � ���� ������
void Crawler::indexPage(const std::string& url, const std::string& content) {
    logger.info("Indexing: " + url);
    Indexer indexer(config, logger); // ������ ����������
    auto words = indexer.extractWords(content); // ��������� ����� �� ��������

    if (words.empty()) {
        logger.error("No words extracted from: " + url); // ���� ���� �� ���������, �������� ������
        return;
    }

    logger.info("Extracted words count: " + std::to_string(words.size())); // �������� ���������� ����������� ����
    db.saveDocument(url, words); // ��������� ������ � ����
}
