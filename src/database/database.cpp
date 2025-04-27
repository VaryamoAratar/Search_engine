#include "database.hpp"

// Конструктор класса Database, инициализирует соединение с базой данных
Database::Database(const Config& config, Logger& logger)
    : connection(config.getDbConnectionString()), logger(logger) {
    logger.info("Подключено к базе данных.");
}

// Метод для инициализации таблиц в базе данных
void Database::init() {
    pqxx::work txn(connection); // Начинаем транзакцию
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS pages (
            id SERIAL PRIMARY KEY,
            url TEXT UNIQUE
        );
        CREATE TABLE IF NOT EXISTS words (
            id SERIAL PRIMARY KEY,
            word TEXT UNIQUE
        );
        CREATE TABLE IF NOT EXISTS index (
            page_id INTEGER REFERENCES pages(id),
            word_id INTEGER REFERENCES words(id),
            frequency INTEGER,
            PRIMARY KEY (page_id, word_id)
        );
    )");  // Выполняем SQL-запрос на создание таблиц
    txn.commit();  // Завершаем транзакцию
    logger.info("Таблицы инициализированы.");
}

// Метод для сохранения документа в базе данных
void Database::saveDocument(const std::string& url, const std::unordered_map<std::string, int>& words) {
    pqxx::work txn(connection); // Начинаем транзакцию

    try {
        // Вставляем URL страницы в таблицу pages, если его нет
        txn.exec_params("INSERT INTO pages (url) VALUES ($1) ON CONFLICT (url) DO NOTHING", url);

        // Получаем ID страницы по URL
        pqxx::result pageRes = txn.exec_params("SELECT id FROM pages WHERE url = $1", url);
        if (pageRes.empty()) {
            logger.error("Не удалось получить ID страницы для URL: " + url);
            return;
        }
        int pageId = pageRes[0][0].as<int>(); // Извлекаем ID страницы

        // Для каждого слова, сохраняем его в таблице words, а затем индексируем на соответствующей странице
        for (const auto& [word, freq] : words) {
            txn.exec_params("INSERT INTO words (word) VALUES ($1) ON CONFLICT (word) DO NOTHING", word); // Вставляем слово
            pqxx::result wordRes = txn.exec_params("SELECT id FROM words WHERE word = $1", word); // Получаем ID слова
            if (wordRes.empty()) {
                logger.error("Не удалось получить ID слова для: " + word);
                continue; // Если ID слова не найдено, продолжаем с другим словом
            }
            int wordId = wordRes[0][0].as<int>(); // Извлекаем ID слова

            // Вставляем или обновляем запись в индексе (связь между страницей и словом с учётом частоты)
            txn.exec_params(R"(
                INSERT INTO index (page_id, word_id, frequency)
                VALUES ($1, $2, $3)
                ON CONFLICT (page_id, word_id) DO UPDATE SET frequency = $3
            )", pageId, wordId, freq);
        }

        txn.commit();  // Завершаем транзакцию
        logger.info("Сохранён документ: " + url);
    }
    catch (const std::exception& e) {
        logger.error("Ошибка при сохранении документа: " + std::string(e.what()));
        txn.abort(); // Если произошла ошибка, откатываем транзакцию
    }
}

// Метод для поиска страниц по запросу
std::vector<std::pair<std::string, int>> Database::search(const std::vector<std::string>& queryWords) {
    pqxx::work txn(connection);  // Начинаем транзакцию
    std::vector<std::pair<std::string, int>> results;  // Результаты поиска

    if (queryWords.empty()) return results;  // Если нет запроса, возвращаем пустой результат

    std::ostringstream whereStream;
    // Создаём часть WHERE для SQL-запроса на основе слов из запроса
    for (size_t i = 0; i < queryWords.size(); ++i) {
        if (i > 0) whereStream << " OR "; // Добавляем OR между словами
        whereStream << "w.word = " << txn.quote(queryWords[i]); // Экранируем слово
    }

    // Формируем SQL-запрос
    std::string sql =
        "SELECT p.url, SUM(i.frequency) AS total " // Выбираем URL и суммируем частоты слов
        "FROM pages p "
        "JOIN index i ON p.id = i.page_id "
        "JOIN words w ON w.id = i.word_id ";

    if (!queryWords.empty()) {
        sql += "WHERE " + whereStream.str() + " "; // Добавляем фильтрацию по словам
    }

    sql +=
        "GROUP BY p.url "  // Группируем по URL
        "HAVING COUNT(DISTINCT w.word) = " + std::to_string(queryWords.size()) + " " // Отбираем страницы, которые содержат все слова запроса
        "ORDER BY total DESC "  // Сортируем по убыванию суммы частот
        "LIMIT 10";  // Ограничиваем результат 10 страницами

    pqxx::result r = txn.exec(sql);  // Выполняем запрос

    // Добавляем найденные результаты в вектор
    for (const auto& row : r) {
        std::string url = row["url"].as<std::string>();  // Извлекаем URL
        int score = row["total"].as<int>();  // Извлекаем сумму частот
        results.emplace_back(url, score);  // Добавляем результат в вектор
    }

    return results;  // Возвращаем результаты поиска
}
