#pragma once

#include "config.hpp"
#include "logger.hpp"

#include <unordered_map>
#include <string>
#include <unordered_set>

// Класс для индексации документов: извлечение слов и обработка стоп-слов
class Indexer {
public:
    // Конструктор, который инициализирует индексатор с конфигурацией и логером
    Indexer(const Config& config, Logger& logger);

    // Метод для извлечения слов из HTML контента и подсчета их частоты
    std::unordered_map<std::string, int> extractWords(const std::string& html);

private:
    // Множество стоп-слов, которые не будут учитываться при индексации
    std::unordered_set<std::string> stopwords;

    // Флаг, указывающий, нужно ли использовать стоп-слова
    bool useStopwords;

    // Ссылка на объект логера для записи логов
    Logger& logger;

    // Метод для загрузки стоп-слов (например, из конфигурационного файла)
    void loadStopwords();

    // Метод для очистки HTML контента от тегов и ненужных символов
    std::string cleanHtml(const std::string& html);
};
