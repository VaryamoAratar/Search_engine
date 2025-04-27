#pragma once
#include <string>

// ����� ��� ������ � ������������� ���������
class Config {
public:
    // �����������, ������� ��������� ��� ����� ������������
    Config(const std::string& filename);

    // ������ ��� ��������� �������� ���������� ������������
    std::string getDbHost() const { return dbHost; }           // �������� ���� ���� ������
    int getDbPort() const { return dbPort; }                   // �������� ���� ���� ������
    std::string getDbName() const { return dbName; }           // �������� ��� ���� ������
    std::string getDbUser() const { return dbUser; }           // �������� ��� ������������ ��� ���� ������
    std::string getDbPassword() const { return dbPassword; }   // �������� ������ ��� ���� ������
    std::string getDbConnectionString() const;                 // �������� ������ ����������� ��� ���� ������

    std::string getStartUrl() const { return startUrl; }       // �������� ��������� URL ��� ������������
    int getMaxDepth() const { return maxDepth; }               // �������� ������������ ������� ������������
    int getTimeout() const { return timeout; }                 // �������� ������� �������
    bool shouldFilterStopwords() const { return filterStopwords; }  // ���������, ����� �� ����������� ����-�����

    int getServerPort() const { return serverPort; }           // �������� ���� �������

    bool isConsoleLoggingEnabled() const { return logToConsole; } // ���������, ������� �� ����� � �������
    bool isFileLoggingEnabled() const { return logToFile; }     // ���������, ������� �� ����� � ����
    std::string getLogDir() const { return logDir; }           // �������� ���������� ��� �����

private:
    // ����������, �������� ��������� ������������
    std::string dbHost;        // ���� ���� ������
    int dbPort;                // ���� ���� ������
    std::string dbName;        // ��� ���� ������
    std::string dbUser;        // ��� ������������ ���� ������
    std::string dbPassword;    // ������ ���� ������

    std::string startUrl;      // ��������� URL ��� ������������
    int maxDepth;              // ������������ ������� ������������
    int timeout;               // ������� �������
    bool filterStopwords;      // ����, ����������� �� ������������� ���������� ����-����

    int serverPort;            // ���� �������

    bool logToConsole;         // ����, ����������� �� ����� ����� � �������
    bool logToFile;            // ����, ����������� �� ����� ����� � ����
    std::string logDir;        // ���������� ��� �����
};
