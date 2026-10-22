#pragma once

#include "../cpppattern.hpp"



void reportResult(const Pattern::Result& result, std::string_view text) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (result.matched) {
        std::cout << "Found a match in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        std::cout << "  Index " << result.index << ", length " << result.length << ": " << text.substr(result.index, result.length) << "\n";
    } else {
        std::cout << "Found no match\n";
    }
}


void reportResult(const std::vector<Pattern::Result>& results, std::string_view text) {
    std::cout << "\n\nText: " << text << "\n\n";
    if (results.empty()) {
        std::cout << "Found no match\n";
    } else {
        std::cout << "Found " << results.size() << " match" << (results.size() == 1 ? "" : "es") << " in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        for(const Pattern::Result& result : results) {
            std::cout << "  Index " << result.index << ", length " << result.length << ": " << text.substr(result.index, result.length) << "\n";
        }
    }
}
