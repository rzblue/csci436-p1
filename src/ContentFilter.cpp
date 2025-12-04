#include "ContentFilter.hpp"
#include <fstream>
#include <cctype>
#include <iostream>

#include "StringUtils.hpp"

using namespace utils;

// ====================================================================================================
// Constructor
// ====================================================================================================
ContentFilter::ContentFilter(const std::string& filename) {
    loadFromFile(filename);
}

// ====================================================================================================
// Public Methods
// ====================================================================================================

bool ContentFilter::loadFromFile(const std::string& filename) {
    forbidden_words.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ContentFilter] Failed to open " << filename << "\n";
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {
        // Trim whitespace
        std::string word = trim(line);

        // Skip empty lines
        if (word.empty()) {
            continue;
        }

        // Skip comment lines (starting with #)
        if (word[0] == '#') {
            continue;
        }

        // Add to forbidden words list
        forbidden_words.push_back(word);
        std::cout << "[ContentFilter] Loaded: '" << word << "'\n";
    }

    file.close();

    std::cout << "[ContentFilter] Loaded " << forbidden_words.size() 
              << " forbidden words from " << filename << "\n";

    return true;
}

bool ContentFilter::containsForbiddenContent(const std::string& content,
                                              std::vector<std::string>& matches) const {
    matches.clear();

    // Convert content to lowercase for case-insensitive matching
    std::string lower_content = toLower(content);

    // Check each forbidden word
    for (const std::string& word : forbidden_words) {
        std::string lower_word = toLower(word);

        // Search for forbidden word in content
        if (lower_content.find(lower_word) != std::string::npos) {
            matches.push_back(word);  // Store original casing
        }
    }

    return !matches.empty();
}

// ====================================================================================================
// Private Helper Methods
// ====================================================================================================

std::string ContentFilter::trim(const std::string& str) {
    // Find first non-whitespace character
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";  // String is all whitespace
    }

    // Find last non-whitespace character
    size_t end = str.find_last_not_of(" \t\r\n");

    // Extract substring
    return str.substr(start, end - start + 1);
}