#pragma once

#include "../src/pattern.hpp"
#include <string>
#include <vector>
#include <iostream>



template <typename Iterator>
void reportResult(std::string_view text, Pattern::Result<Iterator> result) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (result.matched) {
        std::cout << "Found a match in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        std::cout << "  Length " << result.length << ": " << std::string(result.matchStart, result.matchStart + result.length) << "\n";
    } else {
        std::cout << "Found no match\n";
    }
}


template <typename Iterator>
void reportResult(std::string_view text, const std::vector<Pattern::Result<Iterator>>& results) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (results.empty()) {
        std::cout << "Found no match\n";
    } else {
        std::cout << "Found " << results.size() << " match" << (results.size() == 1 ? "" : "es") << " in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        for(Pattern::Result<Iterator> result : results) {
            std::cout << "  Length " << result.length << ": " << std::string(result.matchStart, result.matchStart + result.length) << "\n";
        }
    }
}
