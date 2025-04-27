#include "utils.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <regex>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace beast = boost::beast;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace http = beast::http;

using tcp = net::ip::tcp;

namespace Utils {

    // Шаблонная функция для выполнения HTTP-запроса (с поддержкой как TCP, так и SSL)
    template<typename Socket>
    std::string performRequest(Socket& socket, const std::string& host, const std::string& target, int timeoutMs);

    // Функция для проверки, является ли строка URL с протоколом HTTP или HTTPS
    bool isHttpUrl(const std::string& url) {
        return boost::algorithm::starts_with(url, "http://") || boost::algorithm::starts_with(url, "https://");
    }

    // Функция для проверки, является ли строка относительным URL (начинается с '/')
    bool isRelativeUrl(const std::string& url) {
        return !url.empty() && url[0] == '/';
    }

    // Функция для разрешения относительного URL на основе базового URL
    std::string resolveRelativeUrl(const std::string& base, const std::string& relative) {
        std::regex re(R"(^(https?://[^/]+))");
        std::smatch match;
        if (std::regex_search(base, match, re) && match.size() > 1) {
            return match[1].str() + relative; // Добавляем относительный путь к базовому URL
        }
        return base + relative; // Если нет подходящего URL, возвращаем соединённый базовый URL и относительный путь
    }

    // Функция для выполнения HTTP GET запроса
    std::string httpGet(const std::string& url, int timeoutMs) {
        try {
            auto pos = url.find("://"); // Находим схему в URL
            if (pos == std::string::npos) throw std::runtime_error("Invalid URL: " + url);

            auto scheme = url.substr(0, pos); // Протокол (http или https)
            auto host_and_path = url.substr(pos + 3); // Хост и путь
            auto slash_pos = host_and_path.find('/'); // Ищем первый слэш для разделения хоста и пути
            std::string host = (slash_pos == std::string::npos) ? host_and_path : host_and_path.substr(0, slash_pos);
            std::string target = (slash_pos == std::string::npos) ? "/" : host_and_path.substr(slash_pos);

            // Создаем io_context для асинхронных операций
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);

            stream.expires_after(std::chrono::milliseconds(timeoutMs)); // Устанавливаем тайм-аут

            // Резолвим хост
            auto const results = resolver.resolve(host, scheme == "https" ? "443" : "80");
            stream.connect(results);

            // Если HTTPS, создаём SSL-стрим
            if (scheme == "https") {
                ssl::context ctx(ssl::context::sslv23_client);
                ctx.set_default_verify_paths(); // Устанавливаем пути для верификации сертификатов
                ssl::stream<beast::tcp_stream> ssl_stream(std::move(stream), ctx);
                beast::get_lowest_layer(ssl_stream).expires_after(std::chrono::milliseconds(timeoutMs));

                ssl_stream.handshake(ssl::stream_base::client); // Выполняем рукопожатие SSL
                return performRequest(ssl_stream, host, target, timeoutMs); // Выполняем запрос через SSL
            }
            else {
                return performRequest(stream, host, target, timeoutMs); // Выполняем запрос через TCP
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[httpGet] Error: " << e.what() << std::endl; // Логируем ошибку
            return "";
        }
    }

    // Шаблонная функция для выполнения запроса с использованием сокета
    template<typename Socket>
    std::string performRequest(Socket& socket, const std::string& host, const std::string& target, int timeoutMs) {
        http::request<http::empty_body> req{ http::verb::get, target, 11 };
        req.set(http::field::host, host); // Устанавливаем заголовок Host
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING); // Устанавливаем заголовок User-Agent

        http::write(socket, req); // Отправляем запрос

        beast::flat_buffer buffer; // Буфер для получения ответа
        http::response<http::string_body> res;
        http::read(socket, buffer, res); // Читаем ответ

        // Обработка редиректов (если получен статус перенаправления)
        int redirect_count = 0;
        while (res.result() == http::status::moved_permanently || res.result() == http::status::found) {
            if (++redirect_count > 10) {
                throw std::runtime_error("Too many redirects"); // Предотвращение зацикливания редиректов
            }

            if (!res.base().count(http::field::location)) {
                throw std::runtime_error("Redirect without Location header"); // Ошибка, если нет заголовка Location
            }

            auto new_url = std::string(res[http::field::location]); // Получаем новый URL для редиректа
            return httpGet(new_url, timeoutMs); // Выполняем рекурсивный запрос по новому URL
        }

        // Если не OK статус, генерируем ошибку
        if (res.result() != http::status::ok) {
            throw std::runtime_error("Received non-200 response: " + std::to_string(static_cast<int>(res.result())));
        }

        return res.body(); // Возвращаем тело ответа
    }

    // Функция для экранирования HTML символов
    std::string escapeHtml(const std::string& input) {
        std::ostringstream escaped;
        for (char c : input) {
            switch (c) {
            case '&': escaped << "&amp;"; break;
            case '<': escaped << "&lt;"; break;
            case '>': escaped << "&gt;"; break;
            case '\"': escaped << "&quot;"; break;
            case '\'': escaped << "&#39;"; break;
            default: escaped << c; break; // Все остальные символы добавляем без изменений
            }
        }
        return escaped.str(); // Возвращаем строку с экранированными символами
    }

} // namespace Utils
