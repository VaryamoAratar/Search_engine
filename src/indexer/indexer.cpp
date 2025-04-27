#include "indexer.hpp"
#include <boost/locale.hpp>
#include <regex>
#include <fstream>
#include <sstream>
#include <cctype>

// Конструктор класса Indexer, принимает настройки конфигурации и логгер
Indexer::Indexer(const Config& config, Logger& logger)
    : logger(logger), useStopwords(config.shouldFilterStopwords()) {  // Определяем, нужно ли использовать стоп-слова
    if (useStopwords) {
        loadStopwords();  // Загружаем стоп-слова, если это указано в конфигурации
    }
}

// Метод для загрузки стоп-слов из файла
void Indexer::loadStopwords() {
    std::ifstream in("stopwords.txt");  // Открываем файл стоп-слов
    std::string word;
    while (std::getline(in, word)) {  // Читаем файл строка за строкой
        stopwords.insert(boost::locale::to_lower(word));  // Преобразуем слово в нижний регистр и добавляем в множество
    }
    logger.info("Загружено стоп-слов: " + std::to_string(stopwords.size()));  // Логируем количество загруженных стоп-слов
}

// Метод для очистки HTML-кода от тегов и лишних символов
std::string Indexer::cleanHtml(const std::string& html) {
    std::string text = std::regex_replace(html, std::regex("<[^>]*>"), " ");  // Убираем HTML-теги
    text = std::regex_replace(text, std::regex(R"([\n\r\t.,!?:;"'(){}[\]\\/@#$%^&*+=<>`~|])"), " ");  // Убираем лишние символы
    return text;  // Возвращаем очищенный текст
}

// Метод для извлечения слов из HTML-кода
std::unordered_map<std::string, int> Indexer::extractWords(const std::string& html) {
    std::unordered_map<std::string, int> wordFreq;  // Мап для хранения частот слов
    std::string cleanText = cleanHtml(html);  // Очищаем HTML

    std::istringstream iss(cleanText);  // Преобразуем очищенный текст в поток
    std::string word;

    // Проходим по каждому слову в потоке
    while (iss >> word) {
        try {
            word = boost::locale::to_lower(word);  // Преобразуем слово в нижний регистр
        }
        catch (const std::exception& ex) {
            logger.error("Ошибка в boost::locale::to_lower: " + std::string(ex.what()));  // Логируем ошибку в случае исключения
            continue;
        }

        // Пропускаем слова слишком короткие или слишком длинные
        if (word.length() < 3 || word.length() > 32) continue;

        // Пропускаем слова, которые являются стоп-словами (если это указано)
        if (useStopwords && stopwords.count(word)) continue;

        wordFreq[word]++;  // Увеличиваем частоту найденного слова
    }

    return wordFreq;  // Возвращаем частоты слов
}
