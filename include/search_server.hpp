#pragma once

#include "config.hpp"
#include "logger.hpp"
#include "database.hpp"

#include <atomic>                  // ��� ��������� ����������
#include <boost/asio/ip/tcp.hpp>   // ��� ������ � TCP-�������� ����� Boost.Asio
#include <memory>                  // ��� ����� ����������

// ����� ��� ���������� ���������� �������
class SearchServer {
public:
    // �����������, ������� �������������� ������ � �������������, �������, ����� ������ � ������ ������
    SearchServer(const Config& config, Logger& logger, Database& db, std::atomic<bool>& running);

    // ����� ��� ������� �������
    void run();

private:
    // ������ �� ������ ������������
    const Config& config;

    // ������ �� ������ ������ ��� ������ �����
    Logger& logger;

    // ������ �� ������ ���� ������ ��� �������� ������
    Database& db;

    // ������ �� ����, ������� ��������� ���������� ������ �������
    std::atomic<bool>& running;

    // ����� ��� ������������� � ������ TCP-�������
    void startServer();

    // ����� ��� ��������� ���������� ������ (�������������� � �������� ����� TCP-�����)
    void handleSession(boost::asio::ip::tcp::socket socket);
};
