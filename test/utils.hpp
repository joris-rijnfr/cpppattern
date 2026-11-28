#pragma once

#include "../src/pattern.hpp"
#include <string>
#include <vector>
#include <iostream>



template <typename Iterator>
void reportResult(const std::string& text, Pattern::Result<Iterator> result) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (result.matched) {
        size_t index = std::distance(std::begin(text), result.matchStart);
        std::cout << "Found a match in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        std::cout << "  Length " << result.length << " (index " << index << "): " << std::string(result.matchStart, result.matchStart + result.length) << "\n";
    } else {
        std::cout << "Found no match\n";
    }
}


template <typename Iterator>
void reportResult(const std::string& text, const std::vector<Pattern::Result<Iterator>>& results) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (results.empty()) {
        std::cout << "Found no match\n";
    } else {
        std::cout << "Found " << results.size() << " match" << (results.size() == 1 ? "" : "es") << " in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        for(Pattern::Result<Iterator> result : results) {
            size_t index = std::distance(std::begin(text), result.matchStart);
            std::cout << "  Length " << result.length << " (index " << index << "): " << std::string(result.matchStart, result.matchStart + result.length) << "\n";
        }
    }
}
