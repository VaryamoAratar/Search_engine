#pragma once

#include "config.hpp"
#include "logger.hpp"

#include <unordered_map>
#include <string>
#include <unordered_set>

// ����� ��� ���������� ����������: ���������� ���� � ��������� ����-����
class Indexer {
public:
    // �����������, ������� �������������� ���������� � ������������� � �������
    Indexer(const Config& config, Logger& logger);

    // ����� ��� ���������� ���� �� HTML �������� � �������� �� �������
    std::unordered_map<std::string, int> extractWords(const std::string& html);

private:
    // ��������� ����-����, ������� �� ����� ����������� ��� ����������
    std::unordered_set<std::string> stopwords;

    // ����, �����������, ����� �� ������������ ����-�����
    bool useStopwords;

    // ������ �� ������ ������ ��� ������ �����
    Logger& logger;

    // ����� ��� �������� ����-���� (��������, �� ����������������� �����)
    void loadStopwords();

    // ����� ��� ������� HTML �������� �� ����� � �������� ��������
    std::string cleanHtml(const std::string& html);
};
