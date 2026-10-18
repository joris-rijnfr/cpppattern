#include "../cpppattern.hpp"
#include <iostream>


int main(int argc, char* argv[]) {
    std::string text = "The quick brown fox jumps over the lazy dog!";
    if(argc > 1) {
        text = argv[1];
        for(int i = 2; i < argc; i++) {
            text += " " + std::string(argv[i]);
        }
    }

    constexpr auto matcher = Pattern::make(
        Pattern::plus(
            Pattern::word()
        ),
        Pattern::character('@'),
        Pattern::plus(
            Pattern::word()
        ),
        Pattern::character('.'),
        Pattern::plus(
            Pattern::word()
        )
    );
    std::cout<< text << "\n\n";
    if (Pattern::Result result = matcher(text); result.matched) {
        std::cout << "Match: " << text.substr(result.index, result.length) << "\n";
        std::cout << "Found match at index " << result.index << " with length " << result.length << "\n";
    } else {
        std::cout << "Found no match\n";
    }
}
