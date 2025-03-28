#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <memory>
// Use X DevAPI headers instead
#include <mysqlx/xdevapi.h>

class Database {
public:
    // Constructor & Destructor
    Database();
    ~Database();
    
    // Connection management
    bool connect();
    bool isConnected() const;
    void disconnect();
    
    // Translation operations
    bool storeTranslation(const std::string& originalText,
                         const std::string& englishMeaning,
                         const std::string& pinyinMandarin,
                         const std::string& jyutpingCantonese,
                         const std::string& equivalentCantonese,
                         int audioFileId = -1);
                         
    bool getTranslation(const std::string& originalText,
                       std::string& englishMeaning,
                       std::string& pinyinMandarin,
                       std::string& jyutpingCantonese,
                       std::string& equivalentCantonese,
                       int& audioFileId);
    
    // Audio file operations
    int storeAudioFile(const std::string& mimeType, const std::string& audioFilePath);
    bool getAudioData(int audioFileId, std::string& mimeType, std::string& audioData);

private:
    // Database connection properties
    std::unique_ptr<mysqlx::Session> session;
    
    // Connection parameters (read from environment or config)
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    int port;
    
    // Helper methods
    void loadConfig();
    bool executeQuery(const std::string& query);
};

#endif // DATABASE_H 