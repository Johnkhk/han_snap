#include "../include/database.h"
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <fstream>
#include <cstdio>
#include <cstdlib>

// Function to run SQL script
bool runSqlScript(const std::string& scriptPath, const std::string& database) {
    // Use the hansnap login path
    std::string command = "mysql --login-path=hansnap " + database + " < " + scriptPath;
    
    int result = system(command.c_str());
    return (result == 0);
}

// Function to create a temporary audio file for testing
std::string createTestAudioFile() {
    std::string tempFilePath = "/tmp/test_audio.mp3";
    std::ofstream audioFile(tempFilePath, std::ios::binary);
    
    // Create a simple "audio" file with some test data
    const char* testData = "THIS IS A TEST AUDIO FILE";
    audioFile.write(testData, strlen(testData));
    audioFile.close();
    
    return tempFilePath;
}

// Test fixture for database testing
class DatabaseTest : public ::testing::Test {
protected:
    std::string dbName;
    Database db;
    
    void SetUp() override {
        // Setup test database
        dbName = "hansnap_test_db";
        
        std::cout << "Setting up test database: " << dbName << std::endl;
        
        // Create test database using login path
        std::string createDbCmd = "mysql --login-path=hansnap -e \"CREATE DATABASE IF NOT EXISTS " + dbName + "\"";
        system(createDbCmd.c_str());
        
        // Run migrations
        std::cout << "Running migrations down..." << std::endl;
        runSqlScript("../db/01_down.sql", dbName);
        std::cout << "Running migrations up..." << std::endl;
        runSqlScript("../db/02_up.sql", dbName);
        
        // The Database class should also use the same login path mechanism
        // or environment variables that have been set externally
    }
    
    void TearDown() override {
        // Clean up: drop test database
        std::string dropDbCmd = "mysql --login-path=hansnap -e \"DROP DATABASE IF EXISTS " + dbName + "\"";
        system(dropDbCmd.c_str());
        std::cout << "Test database dropped: " << dbName << std::endl;
    }
};

TEST_F(DatabaseTest, TestConnection) {
    std::cout << "Testing database connection..." << std::endl;
    ASSERT_TRUE(db.connect());
    ASSERT_TRUE(db.isConnected());
    db.disconnect();
    ASSERT_FALSE(db.isConnected());
    std::cout << "Connection test passed!" << std::endl;
}

TEST_F(DatabaseTest, TestTranslations) {
    std::cout << "Testing translation operations..." << std::endl;
    
    // Re-connect if needed
    if (!db.isConnected()) {
        db.connect();
    }
    
    // Test data
    std::string originalText = "你好";
    std::string englishMeaning = "Hello";
    std::string pinyinMandarin = "Nǐ hǎo";
    std::string jyutpingCantonese = "nei5 hou2";
    std::string equivalentCantonese = "你好";
    int audioFileId = -1; // No audio initially
    
    // Test storing translation
    ASSERT_TRUE(db.storeTranslation(
        originalText, 
        englishMeaning, 
        pinyinMandarin, 
        jyutpingCantonese, 
        equivalentCantonese, 
        audioFileId
    ));
    
    // Test retrieving translation
    std::string retrievedEnglish;
    std::string retrievedPinyin;
    std::string retrievedJyutping;
    std::string retrievedCantonese;
    int retrievedAudioId;
    
    ASSERT_TRUE(db.getTranslation(
        originalText,
        retrievedEnglish,
        retrievedPinyin,
        retrievedJyutping,
        retrievedCantonese,
        retrievedAudioId
    ));
    
    EXPECT_EQ(retrievedEnglish, englishMeaning);
    EXPECT_EQ(retrievedPinyin, pinyinMandarin);
    EXPECT_EQ(retrievedJyutping, jyutpingCantonese);
    EXPECT_EQ(retrievedCantonese, equivalentCantonese);
    EXPECT_EQ(retrievedAudioId, audioFileId);
    
    // Test retrieving non-existent translation
    std::string nonExistentEnglish;
    std::string nonExistentPinyin;
    std::string nonExistentJyutping;
    std::string nonExistentCantonese;
    int nonExistentAudioId;
    
    ASSERT_FALSE(db.getTranslation(
        "不存在的文本",
        nonExistentEnglish,
        nonExistentPinyin,
        nonExistentJyutping,
        nonExistentCantonese,
        nonExistentAudioId
    ));
    
    // Test updating existing translation
    std::string updatedEnglish = "Hello there";
    std::string updatedPinyin = "Nǐ hǎo a";
    
    ASSERT_TRUE(db.storeTranslation(
        originalText, 
        updatedEnglish, 
        updatedPinyin, 
        jyutpingCantonese, 
        equivalentCantonese, 
        audioFileId
    ));
    
    // Verify the update
    std::string retrievedUpdatedEnglish;
    std::string retrievedUpdatedPinyin;
    std::string retrievedUpdatedJyutping;
    std::string retrievedUpdatedCantonese;
    int retrievedUpdatedAudioId;
    
    ASSERT_TRUE(db.getTranslation(
        originalText,
        retrievedUpdatedEnglish,
        retrievedUpdatedPinyin,
        retrievedUpdatedJyutping,
        retrievedUpdatedCantonese,
        retrievedUpdatedAudioId
    ));
    
    EXPECT_EQ(retrievedUpdatedEnglish, updatedEnglish);
    EXPECT_EQ(retrievedUpdatedPinyin, updatedPinyin);
    
    std::cout << "Translation operations test passed!" << std::endl;
}

TEST_F(DatabaseTest, TestAudioFiles) {
    std::cout << "Testing audio file operations..." << std::endl;
    
    // Re-connect if needed
    if (!db.isConnected()) {
        db.connect();
    }
    
    // Create a test audio file
    std::string audioFilePath = createTestAudioFile();
    std::string mimeType = "audio/mpeg";
    
    // Test storing audio file
    int audioFileId = db.storeAudioFile(mimeType, audioFilePath);
    ASSERT_GT(audioFileId, 0); // Should return a valid ID
    
    // Test retrieving audio data
    std::string retrievedMimeType;
    std::string retrievedAudioData;
    ASSERT_TRUE(db.getAudioData(audioFileId, retrievedMimeType, retrievedAudioData));
    EXPECT_EQ(retrievedMimeType, mimeType);
    EXPECT_FALSE(retrievedAudioData.empty());
    
    // Test retrieving non-existent audio file
    std::string nonExistentMimeType;
    std::string nonExistentAudioData;
    ASSERT_FALSE(db.getAudioData(99999, nonExistentMimeType, nonExistentAudioData));
    
    // Test linking audio to translation
    std::string originalText = "谢谢";
    std::string englishMeaning = "Thank you";
    std::string pinyinMandarin = "Xièxiè";
    std::string jyutpingCantonese = "ze6 ze6";
    std::string equivalentCantonese = "唔該";
    
    // Store translation with audio ID
    ASSERT_TRUE(db.storeTranslation(
        originalText,
        englishMeaning,
        pinyinMandarin,
        jyutpingCantonese,
        equivalentCantonese,
        audioFileId
    ));
    
    // Verify audio ID is correctly associated
    std::string retrievedEnglish;
    std::string retrievedPinyin;
    std::string retrievedJyutping;
    std::string retrievedCantonese;
    int retrievedAudioId;
    
    ASSERT_TRUE(db.getTranslation(
        originalText,
        retrievedEnglish,
        retrievedPinyin,
        retrievedJyutping,
        retrievedCantonese,
        retrievedAudioId
    ));
    
    EXPECT_EQ(retrievedAudioId, audioFileId);
    
    // Clean up test file
    remove(audioFilePath.c_str());
    
    std::cout << "Audio file operations test passed!" << std::endl;
}