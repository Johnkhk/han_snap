-- Table for metadata / references
CREATE TABLE translations (
    id INT AUTO_INCREMENT PRIMARY KEY,
    original_text VARCHAR(512) NOT NULL,
    english_meaning VARCHAR(512) NOT NULL,
    pinyin_mandarin VARCHAR(512) NOT NULL,
    jyutping_cantonese VARCHAR(512) NOT NULL,
    equivalent_cantonese VARCHAR(512) NOT NULL,
    audio_file_id INT DEFAULT NULL,            -- Points to audio_files table
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Table for audio blobs
CREATE TABLE audio_files (
    id INT AUTO_INCREMENT PRIMARY KEY,
    mime_type VARCHAR(100) NOT NULL,           -- e.g. "audio/mpeg" or "audio/wav"
    audio_data LONGBLOB NOT NULL,              -- Your actual BLOB data
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
