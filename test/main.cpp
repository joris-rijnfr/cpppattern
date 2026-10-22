#include "../cpppattern.hpp"
#include <vector>
#include <iostream>



int main(int argc, char* argv[]) {
    std::string text = "trying to test tests by testing";
    if(argc > 1) {
        text = argv[1];
        for(int i = 2; i < argc; i++) {
            text += " " + std::string(argv[i]);
        }
    }
    constexpr auto matcher = Pattern::concat(
        Pattern::literal('t'),
        Pattern::literal('e'),
        Pattern::literal('s'),
        Pattern::literal('t'),
        Pattern::optional(
            Pattern::anyLongest(
                Pattern::concat(
                    Pattern::literal('i'),
                    Pattern::literal('n'),
                    Pattern::literal('g')
                ),
                Pattern::literal('s')
            )
        )
    );

    matcher.printAsDOT();

    std::cout<< text << "\n\n";
    std::vector<Pattern::Result> results = matcher.matchAll(text);
    if (results.empty()) {
        std::cout << "Found no match\n";
    } else {
        for(const Pattern::Result& result : results) {
            std::cout << "Match at index " << result.index << " with length " << result.length << ": " << text.substr(result.index, result.length) << "\n";
        }
    }
}
