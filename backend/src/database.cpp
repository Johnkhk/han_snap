#include "../include/database.h"
#include "../../common/include/logger.h"
#include <iostream>
#include <cstdlib> // For getenv
#include <stdexcept>
#include <fstream>

// Module-level static logger initialization
static std::shared_ptr<spdlog::logger> getDBLogger() {
    static std::shared_ptr<spdlog::logger> logger = hansnap::Logger::getInstance().createLogger("database");
    return logger;
}

// Convenience macros
#define DB_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(getDBLogger(), __VA_ARGS__)
#define DB_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(getDBLogger(), __VA_ARGS__)
#define DB_LOG_INFO(...) SPDLOG_LOGGER_INFO(getDBLogger(), __VA_ARGS__)
#define DB_LOG_WARNING(...) SPDLOG_LOGGER_WARN(getDBLogger(), __VA_ARGS__)
#define DB_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(getDBLogger(), __VA_ARGS__)
#define DB_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(getDBLogger(), __VA_ARGS__)

Database::Database() : port(33060) {
    // Constructor code
    DB_LOG_DEBUG("Database instance created");
    
    loadConfig();
}

Database::~Database() {
    disconnect();
}

void Database::loadConfig() {
    // Load database configuration from environment variables
    const char* db_host = std::getenv("MYSQL_HOST");
    const char* db_user = std::getenv("MYSQL_USER");
    const char* db_password = std::getenv("MYSQL_PASSWORD");
    const char* db_name = std::getenv("MYSQL_DATABASE");
    const char* db_port = std::getenv("MYSQL_PORT");
    
    // Set defaults if not found
    host = db_host ? db_host : "127.0.0.1";
    user = db_user ? db_user : "hansnap_user";
    password = db_password ? db_password : "";
    database = db_name ? db_name : "hansnap_db";
    port = db_port ? std::stoi(db_port) : 33060; // X Protocol default port
    
    DB_LOG_DEBUG("Database config loaded: host={}, user={}, database={}, port={}",
                host, user, database, port);
}

bool Database::connect() {
    try {
        // Create X DevAPI session
        session = std::make_unique<mysqlx::Session>(
            mysqlx::SessionOption::HOST, host,
            mysqlx::SessionOption::PORT, port,
            mysqlx::SessionOption::USER, user,
            mysqlx::SessionOption::PWD, password,
            mysqlx::SessionOption::DB, database
        );
        
        DB_LOG_INFO("Connected to MySQL database: {}", database);
        return true;
    }
    catch (const std::exception &e) {
        DB_LOG_ERROR("Connection error: {}", e.what());
        return false;
    }
}

bool Database::isConnected() const {
    return session != nullptr;
}

void Database::disconnect() {
    if (isConnected()) {
        session.reset();
        DB_LOG_INFO("Disconnected from MySQL database");
    }
}

bool Database::executeQuery(const std::string& query) {
    if (!isConnected() && !connect()) {
        return false;
    }
    
    try {
        session->sql(query).execute();
        return true;
    }
    catch (const std::exception &e) {
        DB_LOG_ERROR("SQL error: {}", e.what());
        return false;
    }
}

bool Database::storeTranslation(const std::string& originalText,
                              const std::string& englishMeaning,
                              const std::string& pinyinMandarin,
                              const std::string& jyutpingCantonese,
                              const std::string& equivalentCantonese,
                              int audioFileId) {
    if (!isConnected() && !connect()) {
        return false;
    }
    
    try {
        // Using SQL instead of Collections to match schema
        std::string query = "INSERT INTO translations "
                            "(original_text, english_meaning, pinyin_mandarin, "
                            "jyutping_cantonese, equivalent_cantonese, audio_file_id) "
                            "VALUES (?, ?, ?, ?, ?, ?) "
                            "ON DUPLICATE KEY UPDATE "
                            "english_meaning = VALUES(english_meaning), "
                            "pinyin_mandarin = VALUES(pinyin_mandarin), "
                            "jyutping_cantonese = VALUES(jyutping_cantonese), "
                            "equivalent_cantonese = VALUES(equivalent_cantonese), "
                            "audio_file_id = VALUES(audio_file_id)";
                            
        auto stmt = session->sql(query)
                         .bind(originalText)
                         .bind(englishMeaning)
                         .bind(pinyinMandarin)
                         .bind(jyutpingCantonese)
                         .bind(equivalentCantonese)
                         .bind(audioFileId);
        
        stmt.execute();
        DB_LOG_INFO("Successfully stored translation");
        return true;
    }
    catch (const std::exception &e) {
        DB_LOG_ERROR("Error in storeTranslation: {}", e.what());
        return false;
    }
}

bool Database::getTranslation(const std::string& originalText,
                            std::string& englishMeaning,
                            std::string& pinyinMandarin,
                            std::string& jyutpingCantonese,
                            std::string& equivalentCantonese,
                            int& audioFileId) {
    if (!isConnected() && !connect()) {
        return false;
    }
    
    try {
        std::string query = "SELECT english_meaning, pinyin_mandarin, "
                            "jyutping_cantonese, equivalent_cantonese, audio_file_id "
                            "FROM translations WHERE original_text = ?";
                            
        auto result = session->sql(query).bind(originalText).execute();
        
        if (result.count() > 0) {
            auto row = result.fetchOne();
            englishMeaning = row[0].get<std::string>();
            pinyinMandarin = row[1].get<std::string>();
            jyutpingCantonese = row[2].get<std::string>();
            equivalentCantonese = row[3].get<std::string>();
            audioFileId = row[4].isNull() ? -1 : row[4].get<int>();
            return true;
        } else {
            return false; // No translation found
        }
    }
    catch (const std::exception &e) {
        DB_LOG_ERROR("Error in getTranslation: {}", e.what());
        return false;
    }
}

int Database::storeAudioFile(const std::string& mimeType, const std::string& audioFilePath) {
    if (!isConnected() && !connect()) {
        return -1;
    }
    
    try {
        // Read audio data from file
        std::ifstream audioFile(audioFilePath, std::ios::binary);
        if (!audioFile) {
            DB_LOG_ERROR("Could not open audio file: {}", audioFilePath);
            return -1;
        }
        
        // Read file into string buffer
        std::string audioData((std::istreambuf_iterator<char>(audioFile)),
                              std::istreambuf_iterator<char>());
        audioFile.close();
        
        // Use SQL to insert audio blob
        std::string query = "INSERT INTO audio_files (mime_type, audio_data) VALUES (?, ?)";
        auto stmt = session->sql(query).bind(mimeType).bind(audioData);
        auto result = stmt.execute();
        
        // Get the last inserted ID
        auto idResult = session->sql("SELECT LAST_INSERT_ID()").execute();
        auto row = idResult.fetchOne();
        int audioFileId = row[0].get<int>();
        
        DB_LOG_INFO("Successfully stored audio file with ID: {}", audioFileId);
        return audioFileId;
    }
    catch (const std::exception &e) {
        DB_LOG_ERROR("Error in storeAudioFile: {}", e.what());
        return -1;
    }
}

bool Database::getAudioData(int audioFileId, std::string& mimeType, std::string& audioData) {
    if (!isConnected() && !connect()) {
        return false;
    }
    
    try {
        std::string query = "SELECT mime_type, audio_data FROM audio_files WHERE id = ?";
        auto result = session->sql(query).bind(audioFileId).execute();
        
        if (result.count() > 0) {
            auto row = result.fetchOne();
            mimeType = row[0].get<std::string>();
            audioData = row[1].get<std::string>();
            return true;
        } else {
            return false; // No audio file found
        }
    }
    catch (const std::exception &e) {
        DB_LOG_ERROR("Error in getAudioData: {}", e.what());
        return false;
    }
} 