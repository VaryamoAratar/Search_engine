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

    // ��������� ������� ��� ���������� HTTP-������� (� ���������� ��� TCP, ��� � SSL)
    template<typename Socket>
    std::string performRequest(Socket& socket, const std::string& host, const std::string& target, int timeoutMs);

    // ������� ��� ��������, �������� �� ������ URL � ���������� HTTP ��� HTTPS
    bool isHttpUrl(const std::string& url) {
        return boost::algorithm::starts_with(url, "http://") || boost::algorithm::starts_with(url, "https://");
    }

    // ������� ��� ��������, �������� �� ������ ������������� URL (���������� � '/')
    bool isRelativeUrl(const std::string& url) {
        return !url.empty() && url[0] == '/';
    }

    // ������� ��� ���������� �������������� URL �� ������ �������� URL
    std::string resolveRelativeUrl(const std::string& base, const std::string& relative) {
        std::regex re(R"(^(https?://[^/]+))");
        std::smatch match;
        if (std::regex_search(base, match, re) && match.size() > 1) {
            return match[1].str() + relative; // ��������� ������������� ���� � �������� URL
        }
        return base + relative; // ���� ��� ����������� URL, ���������� ���������� ������� URL � ������������� ����
    }

    // ������� ��� ���������� HTTP GET �������
    std::string httpGet(const std::string& url, int timeoutMs) {
        try {
            auto pos = url.find("://"); // ������� ����� � URL
            if (pos == std::string::npos) throw std::runtime_error("Invalid URL: " + url);

            auto scheme = url.substr(0, pos); // �������� (http ��� https)
            auto host_and_path = url.substr(pos + 3); // ���� � ����
            auto slash_pos = host_and_path.find('/'); // ���� ������ ���� ��� ���������� ����� � ����
            std::string host = (slash_pos == std::string::npos) ? host_and_path : host_and_path.substr(0, slash_pos);
            std::string target = (slash_pos == std::string::npos) ? "/" : host_and_path.substr(slash_pos);

            // ������� io_context ��� ����������� ��������
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);

            stream.expires_after(std::chrono::milliseconds(timeoutMs)); // ������������� ����-���

            // �������� ����
            auto const results = resolver.resolve(host, scheme == "https" ? "443" : "80");
            stream.connect(results);

            // ���� HTTPS, ������ SSL-�����
            if (scheme == "https") {
                ssl::context ctx(ssl::context::sslv23_client);
                ctx.set_default_verify_paths(); // ������������� ���� ��� ����������� ������������
                ssl::stream<beast::tcp_stream> ssl_stream(std::move(stream), ctx);
                beast::get_lowest_layer(ssl_stream).expires_after(std::chrono::milliseconds(timeoutMs));

                ssl_stream.handshake(ssl::stream_base::client); // ��������� ����������� SSL
                return performRequest(ssl_stream, host, target, timeoutMs); // ��������� ������ ����� SSL
            }
            else {
                return performRequest(stream, host, target, timeoutMs); // ��������� ������ ����� TCP
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[httpGet] Error: " << e.what() << std::endl; // �������� ������
            return "";
        }
    }

    // ��������� ������� ��� ���������� ������� � �������������� ������
    template<typename Socket>
    std::string performRequest(Socket& socket, const std::string& host, const std::string& target, int timeoutMs) {
        http::request<http::empty_body> req{ http::verb::get, target, 11 };
        req.set(http::field::host, host); // ������������� ��������� Host
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING); // ������������� ��������� User-Agent

        http::write(socket, req); // ���������� ������

        beast::flat_buffer buffer; // ����� ��� ��������� ������
        http::response<http::string_body> res;
        http::read(socket, buffer, res); // ������ �����

        // ��������� ���������� (���� ������� ������ ���������������)
        int redirect_count = 0;
        while (res.result() == http::status::moved_permanently || res.result() == http::status::found) {
            if (++redirect_count > 10) {
                throw std::runtime_error("Too many redirects"); // �������������� ������������ ����������
            }

            if (!res.base().count(http::field::location)) {
                throw std::runtime_error("Redirect without Location header"); // ������, ���� ��� ��������� Location
            }

            auto new_url = std::string(res[http::field::location]); // �������� ����� URL ��� ���������
            return httpGet(new_url, timeoutMs); // ��������� ����������� ������ �� ������ URL
        }

        // ���� �� OK ������, ���������� ������
        if (res.result() != http::status::ok) {
            throw std::runtime_error("Received non-200 response: " + std::to_string(static_cast<int>(res.result())));
        }

        return res.body(); // ���������� ���� ������
    }

    // ������� ��� ������������� HTML ��������
    std::string escapeHtml(const std::string& input) {
        std::ostringstream escaped;
        for (char c : input) {
            switch (c) {
            case '&': escaped << "&amp;"; break;
            case '<': escaped << "&lt;"; break;
            case '>': escaped << "&gt;"; break;
            case '\"': escaped << "&quot;"; break;
            case '\'': escaped << "&#39;"; break;
            default: escaped << c; break; // ��� ��������� ������� ��������� ��� ���������
            }
        }
        return escaped.str(); // ���������� ������ � ��������������� ���������
    }

} // namespace Utils
