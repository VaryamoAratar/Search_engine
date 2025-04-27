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

// ����� ��� ���������� �������� (���������� ������), ������� ����� �������� �������� � ������
class Crawler {
public:
    // �����������, �������������� ������� � �������������, �������, ����� ������ � ������ ������
    explicit Crawler(const Config& config, Logger& logger, Database& db, std::atomic<bool>& running);

    // ����� ��� ������� ������ ��������
    void start();

private:
    // ����� ��� ��������� ����������� �������� �� URL
    std::string fetchPage(const std::string& url);

    // ����� ��� ���������� ���� ������ �� ����������� ��������
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl);

    // ����� ��� ���������� �������� (���������� ���������� � ���� ������)
    void indexPage(const std::string& url, const std::string& content);

    // ������ �� ������ ������������
    const Config& config;

    // ������ �� ������ ������ ��� ������ �����
    Logger& logger;

    // ������ �� ������ ���� ������ ��� �������� ������
    Database& db;

    // ������� ��� ������������� ������� � ������� URL
    std::mutex queueMutex;

    // ������� ��� �������� URL-�� � ������ �� �������
    std::queue<std::pair<std::string, int>> urlQueue;

    // ������ �� ����, ������� ��������� ���������� ������ ��������
    std::atomic<bool>& running;

    // ��������� ���������� URL-�� ��� �������������� ���������� ���������
    std::unordered_set<std::string> visited;
};
