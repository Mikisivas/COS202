#pragma once
// Small string helpers. Matching is written by hand so that the search
// module does not depend on library search functions.

#include <cctype>
#include <string>

namespace scapms::utils {

inline std::string toLower(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

inline std::string toUpper(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return text;
}

inline std::string trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

// Case-insensitive "does text contain term" written with two nested loops.
inline bool containsIgnoreCase(const std::string& text, const std::string& term) {
    if (term.empty()) {
        return true;
    }
    const std::string haystack = toLower(text);
    const std::string needle = toLower(term);
    if (needle.size() > haystack.size()) {
        return false;
    }
    for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        std::size_t j = 0;
        while (j < needle.size() && haystack[i + j] == needle[j]) {
            ++j;
        }
        if (j == needle.size()) {
            return true;
        }
    }
    return false;
}

} // namespace scapms::utils
