#include "../patrn.hpp"
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
    constexpr auto matcher = Patrn::chain(
        Patrn::character('t'),
        Patrn::character('e'),
        Patrn::character('s'),
        Patrn::character('t'),
        Patrn::optional(
            Patrn::alteration(
                Patrn::chain(
                    Patrn::character('i'),
                    Patrn::character('n'),
                    Patrn::character('g')
                ),
                Patrn::character('s')
            )
        )
    );

    matcher.printAsDOT();

    std::cout<< text << "\n\n";
    std::vector<Patrn::Result> results = matcher.matchAll(text);
    if (results.empty()) {
        std::cout << "Found no match\n";
    } else {
        for(const Patrn::Result& result : results) {
            std::cout << "Match at index " << result.index << " with length " << result.length << ": " << text.substr(result.index, result.length) << "\n";
        }
    }
}
