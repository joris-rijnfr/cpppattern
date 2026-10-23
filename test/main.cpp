#include "../cpppattern.hpp"
#include "./utils.hpp"



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
    constexpr auto matcher = Pattern::concat(Pattern::optional("dog"), Pattern::optional("cat"));
    matcher.printDebugInfo();

    auto result = matcher.matchAll(text);
    reportResult(text, result);
}
