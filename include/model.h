#pragma once

#include <string>
#include <nlohmann/json.hpp>

// Use the nlohmann json namespace
using json = nlohmann::json;



/**
 * Translation data structure
 */
struct Translation {
    std::string meaning_english;
    std::string pinyin_mandarin;
    std::string jyutping_cantonese;
    std::string equivalent_cantonese;

    // Enable JSON serialization and deserialization
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Translation, meaning_english, pinyin_mandarin, jyutping_cantonese, equivalent_cantonese)

    // inline void to_json(json& j) const {
    //     j = json{
    //         {"meaning_english", meaning_english},
    //         {"pinyin_mandarin", pinyin_mandarin},
    //         {"jyutping_cantonese", jyutping_cantonese},
    //         {"equivalent_cantonese", equivalent_cantonese}
    //     };
    // }
    // Static method that returns the JSON Schema for Translation
    static json responseSchema() {
        return json{
            {"type", "object"},
            {"properties", {
                {"meaning_english", {{"type", "string"}}},
                {"pinyin_mandarin", {{"type", "string"}}},
                {"jyutping_cantonese", {{"type", "string"}}},
                {"equivalent_cantonese", {{"type", "string"}}}
            }},
            {"required", {"meaning_english", "pinyin_mandarin", "jyutping_cantonese", "equivalent_cantonese"}},
            {"additionalProperties", false}
        };
    }
};

// Add any other data structures here 