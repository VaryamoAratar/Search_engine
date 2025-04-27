#pragma once

#include <string>

namespace Utils {

    // ¬ыполн€ет HTTP(S) GET-запрос на указанный URL с таймаутом (по умолчанию 5000 мс)
    std::string httpGet(const std::string& url, int timeoutMs = 5000);

    // ѕроверка, €вл€етс€ ли URL абсолютным (http:// или https://)
    bool isHttpUrl(const std::string& url);

    // ѕроверка, €вл€етс€ ли URL относительным (начинаетс€ с '/')
    bool isRelativeUrl(const std::string& url);

    // ѕреобразует относительный URL в абсолютный на основе переданного базового URL
    std::string resolveRelativeUrl(const std::string& base, const std::string& relative);

    // Ёкранирует HTML-символы в строке (замен€ет &, <, >, ", ' на соответствующие сущности)
    std::string escapeHtml(const std::string& input);

} // namespace Utils
