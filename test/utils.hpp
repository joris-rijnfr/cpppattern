#pragma once

#include "../cpppattern.hpp"



template <typename Iterator>
void reportResult(std::string_view text, Pattern::Result<Iterator> result, bool success) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (success) {
        std::cout << "Found a match in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        std::cout << "  Length " << result.length << ": " << std::string(result.start, result.start + result.length) << "\n";
    } else {
        std::cout << "Found no match\n";
    }
}


template <typename Iterator>
void reportResult(std::string_view text, const std::vector<Pattern::Result<Iterator>>& results, bool success) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (success) {
        std::cout << "Found " << results.size() << " match" << (results.size() == 1 ? "" : "es") << " in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        for(Pattern::Result<Iterator> result : results) {
            std::cout << "  Length " << result.length << ": " << std::string(result.start, result.start + result.length) << "\n";
        }
    } else {
        std::cout << "Found no match\n";
    }
}
