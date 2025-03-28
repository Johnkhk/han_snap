#include "../include/database.h"
#include <iostream>
#include <cstdlib> // For getenv
#include <stdexcept>
#include <fstream>

Database::Database() : port(33060) {
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
        
        std::cout << "Successfully connected to MySQL database: " << database << std::endl;
        return true;
    }
    catch (const std::exception &e) {
        std::cerr << "Connection Error: " << e.what() << std::endl;
        return false;
    }
}

bool Database::isConnected() const {
    return session != nullptr;
}

void Database::disconnect() {
    if (isConnected()) {
        session.reset();
        std::cout << "Disconnected from MySQL database" << std::endl;
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
        std::cerr << "SQL Error: " << e.what() << std::endl;
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
        std::cout << "Successfully stored translation" << std::endl;
        return true;
    }
    catch (const std::exception &e) {
        std::cerr << "Error in storeTranslation: " << e.what() << std::endl;
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
        std::cerr << "Error in getTranslation: " << e.what() << std::endl;
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
            std::cerr << "Could not open audio file: " << audioFilePath << std::endl;
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
        
        std::cout << "Successfully stored audio file with ID: " << audioFileId << std::endl;
        return audioFileId;
    }
    catch (const std::exception &e) {
        std::cerr << "Error in storeAudioFile: " << e.what() << std::endl;
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
        std::cerr << "Error in getAudioData: " << e.what() << std::endl;
        return false;
    }
} 