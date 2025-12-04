#ifndef CONTENT_FILTER_HPP
#define CONTENT_FILTER_HPP

#include <string>
#include <vector>

/**
 * ContentFilter - Manages forbidden word filtering
 * 
 * Responsibilities:
 * - Load forbidden words from configuration file
 * - Check text content for forbidden words (case-insensitive)
 * - Report which words were matched
 */
class ContentFilter {
public:
    /**
     * Constructor - loads forbidden words from default file
     * @param filename Path to forbidden words file (default: "forbidden.txt")
     */
    explicit ContentFilter(const std::string& filename = "forbidden.txt");

    /**
     * Load (or reload) forbidden words from file
     * @param filename Path to forbidden words file
     * @return true on success, false on failure
     */
    bool loadFromFile(const std::string& filename);

    /**
     * Check if content contains any forbidden words
     * 
     * Performs case-insensitive matching.
     * 
     * @param content Text content to check
     * @param matches Output vector of matched forbidden words (original casing)
     * @return true if any forbidden words were found
     */
    bool containsForbiddenContent(const std::string& content,
                                   std::vector<std::string>& matches) const;

    /**
     * Get count of loaded forbidden words
     * @return Number of forbidden words currently loaded
     */
    size_t getWordCount() const { return forbidden_words.size(); }

    /**
     * Get list of all loaded forbidden words
     * @return Vector of forbidden words
     */
    const std::vector<std::string>& getForbiddenWords() const { 
        return forbidden_words; 
    }

    /**
     * Check if filter is empty (no words loaded)
     * @return true if no forbidden words are loaded
     */
    bool isEmpty() const { return forbidden_words.empty(); }

    /**
     * Clear all forbidden words
     */
    void clear() { forbidden_words.clear(); }

private:
    std::vector<std::string> forbidden_words;

    /**
     * Trim whitespace from both ends of string
     * @param str Input string
     * @return Trimmed string
     */
    static std::string trim(const std::string& str);
};

#endif // CONTENT_FILTER_HPP