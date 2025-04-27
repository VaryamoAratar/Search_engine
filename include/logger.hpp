#pragma once

#include "config.hpp"
#include <fstream>  // ��� ������ � �������� �������
#include <mutex>    // ��� ������������� �������
#include <string>

// ����� ��� ����������� ��������� (����������, ������, ��������������)
class Logger {
public:
    // �����������, ������� �������������� ����� � �������������
    Logger(const Config& config);

    // ����������, ������� ��������� ���� (���� ����������� � ���� ��������)
    ~Logger();

    // ����� ��� ������ ��������������� ���������
    void info(const std::string& message);

    // ����� ��� ������ ��������� �� ������
    void error(const std::string& message);

    // ����� ��� ������ ���������������� ���������
    void warn(const std::string& message);

private:
    // ����� ��� ������ ��������� � ����������� ������� �����������
    void log(const std::string& level, const std::string& message);

    // ����, �����������, ����� �� �������� ���� � �������
    bool logToConsole;

    // ����, �����������, ����� �� �������� ���� � ����
    bool logToFile;

    // ����� ��� ������ ����� � ���� (���� logToFile = true)
    std::ofstream fileStream;

    // ������� ��� ������������� ����������� � ������������� �����
    std::mutex logMutex;
};
