#pragma once

#include <string>

namespace Utils {

    // ��������� HTTP(S) GET-������ �� ��������� URL � ��������� (�� ��������� 5000 ��)
    std::string httpGet(const std::string& url, int timeoutMs = 5000);

    // ��������, �������� �� URL ���������� (http:// ��� https://)
    bool isHttpUrl(const std::string& url);

    // ��������, �������� �� URL ������������� (���������� � '/')
    bool isRelativeUrl(const std::string& url);

    // ����������� ������������� URL � ���������� �� ������ ����������� �������� URL
    std::string resolveRelativeUrl(const std::string& base, const std::string& relative);

    // ���������� HTML-������� � ������ (�������� &, <, >, ", ' �� ��������������� ��������)
    std::string escapeHtml(const std::string& input);

} // namespace Utils
