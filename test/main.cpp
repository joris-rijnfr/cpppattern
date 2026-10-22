#include "../cpppattern.hpp"
#include <vector>
#include <iostream>



// constexpr auto dogsAndCatsMatcher = Pattern::concat(
//     Pattern::anyLongest(
//         Pattern::concat("dog"),
//         Pattern::concat("cat")
//     ),
//     Pattern::optional("s"),
//     Pattern::repeatZeroOrMoreTimes(
//         Pattern::concat(
//             Pattern::concat(" and "),
//             Pattern::anyLongest(
//                 "dog",
//                 "cat"
//             ),
//             Pattern::optional("s")
//         )
//     )
// );



int main(int argc, char* argv[]) {
    std::string text = "i have dogs and cats and dog and cat and cats and dogs";
    if(argc > 1) {
        text = argv[1];
        for(int i = 2; i < argc; i++) {
            text += " " + std::string(argv[i]);
        }
    }
    constexpr auto matcher = Pattern::optional(Pattern::optional(Pattern::optional("bruh")));
    matcher.printDebugInfo();
    
    std::cout << "\n\nText: " << text << "\n\n";
    std::vector<Pattern::Result> results = matcher.matchAll(text);
    if (results.empty()) {
        std::cout << "Found no match\n";
    } else {
        std::cout << "Found " << results.size() << " match" << (results.size() == 1 ? "" : "es") << " in a text with " << text.length() << " character" << (text.length() == 1 ? "" : "s") << ":\n";
        for(const Pattern::Result& result : results) {
            std::cout << "  Index " << result.index << ", length " << result.length << ": " << text.substr(result.index, result.length) << "\n";
        }
    }
}
