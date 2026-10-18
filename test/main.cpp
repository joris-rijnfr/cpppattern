#include "../cpppattern.hpp"
#include <iostream>


int main(int argc, char* argv[]) {
    std::string text = "xxxtestxxx";
    if(argc == 2) {
        text = argv[1];
    }


    constexpr auto matcher = Pattern::make(
        Pattern::any(
            Pattern::literal("test"),
            Pattern::literal("xxx")
        )
    );
    if (Pattern::Result result = matcher(text); result.matched) {
        std::cout << "Match at index " << result.index << " of length " << result.length << "\n";
    } else {
        std::cout << "No match :(\n";
    }
}
