#include "config.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

// ����������� ������ Config, ��������� ��������� �� ����������������� �����
Config::Config(const std::string& filename) {
    // ������ ������ ��� �������� INI-�����
    boost::property_tree::ptree pt;
    // ������ ������������ �� ����� � ��������� ������ �������
    boost::property_tree::ini_parser::read_ini(filename, pt);

    // ��������� ��������� ���� ������
    dbHost = pt.get<std::string>("database.host");      // ���� ���� ������
    dbPort = pt.get<int>("database.port");              // ���� ���� ������
    dbName = pt.get<std::string>("database.name");      // ��� ���� ������
    dbUser = pt.get<std::string>("database.user");      // ��� ������������ ��� ���� ������
    dbPassword = pt.get<std::string>("database.password"); // ������ ���� ������

    // ��������� ��������� ��� ��������
    startUrl = pt.get<std::string>("crawler.start_url"); // ��������� URL ��� ������������
    maxDepth = pt.get<int>("crawler.depth");             // ������������ ������� ������������
    timeout = pt.get<int>("crawler.timeout");            // ������� ��� ��������
    filterStopwords = pt.get<bool>("crawler.filter_stopwords"); // ���� ��� ���������� ����-����

    // ��������� ��������� ��� �������
    serverPort = pt.get<int>("server.port");             // ���� �������

    // ��������� ��������� ��� �����������
    logToConsole = pt.get<bool>("logging.console");      // ���� ��� ������ ����� � �������
    logToFile = pt.get<bool>("logging.file");            // ���� ��� ������ ����� � ����
    logDir = pt.get<std::string>("logging.log_dir");     // ���������� ��� ������ �����
}

// ����� ��� ������������ ������ ����������� � ���� ������
std::string Config::getDbConnectionString() const {
    // ������ ������ ����������� � ������� "host=..., port=..., dbname=..., user=..., password=..."
    return "host=" + dbHost + " port=" + std::to_string(dbPort) +
        " dbname=" + dbName + " user=" + dbUser + " password=" + dbPassword;
}
