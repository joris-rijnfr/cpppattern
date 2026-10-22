#pragma once

#include <type_traits>
#include <vector>
#include <array>
#include <iostream>
#include <cassert>




class Pattern {
public:
    struct Result {
        size_t index = 0;
        size_t length = 0;
        bool matched = false;
    };


    template <typename V, size_t NumStates>
    class Matcher {
    public:
        Matcher(const Matcher&) = default;
        Matcher& operator=(const Matcher&) = default;


        template <typename T>
        constexpr Result matchFirst(const T& input, size_t offset = 0) const {
            for(size_t i = offset; i < input.size(); i++) {
                Result result = matchAt(input, i);
                if(result.matched) {
                    return result;
                }
            }
            return Result {};
        }


        template <typename T>
        constexpr Result matchLast(const T& input, size_t offset = 0) const {
            for(size_t i = offset; i < input.size(); i++) {
                Result result = matchAt(input, input.size() - i);
                if(result.matched) {
                    return result;
                }
            }
            return Result {};
        }


        template <typename T>
        std::vector<Result> matchAll(const T& input, size_t offset = 0) const {
            std::vector<Result> results;
            for(size_t i = offset; i < input.size(); i++) {
                Result result = matchAt(input, i);
                if(result.matched) {
                    results.push_back(result);
                }
            }
            return results;
        }


        template <typename T>
        constexpr Result matchAt(const T& input, size_t index) const {
            std::array<bool, NumStates + 1> currentStates = { false };
            followEmptyTransitions(0, currentStates);
            currentStates[0] = true;
            bool match = currentStates[NumStates];
            size_t longestMatchEnd = index;
            for(size_t i = index; i < input.size(); i++) {
                std::array<bool, NumStates + 1> nextStates = { false };
                bool transitionHappened = false;
                V currentLiteral = input[i];
                for(size_t j = 0; j < NumStates; j++) {
                    if(currentStates[j]) {
                        transitionHappened = followAllTransitions(j, currentLiteral, nextStates) || transitionHappened;
                    }
                }
                if(nextStates[NumStates]) {
                    match = true;
                    longestMatchEnd = i + 1;
                }
                if(!transitionHappened) {
                    break;
                }
                currentStates = nextStates;
                for(size_t j = 0; j < NumStates + 1; j++) {
                    nextStates[j] = false;
                }
            }
            if(match) {
                return Result {
                    index,
                    longestMatchEnd - index,
                    true
                };
            }
            return Result {};
        }


        constexpr bool canHaveEmptyMatches() const {
            std::array<bool, NumStates + 1> currentStates = { false };
            followEmptyTransitions(0, currentStates);
            return currentStates[NumStates];
        }


        void printDebugInfo() const {
            std::cout << "digraph NFA {\n";
            std::cout << "    rankdir=\"LR\";\n    node [shape=box, style=rounded];\n";
            for(size_t i = 0; i < states.size(); i++) {
                State state = states[i];
                if(state.literalTransition.hasTransition) {
                    std::cout << "    " << i << " -> " << state.literalTransition.result << " [label=\"'" << state.literalTransition.literal << "'\"];\n";
                }
                for(size_t j = 0; j < state.epsilonTransitions.length; j++) {
                    std::cout << "    " << i << " -> " << state.epsilonTransitions.elements[j] << " [arrowhead=\"onormal\"];\n";
                }
            }
            std::cout << "    " << states.size() << " [label=\"ACCEPT\", shape=circle];\n";
            std::cout << "}\n\n";
            std::cout << "State size in bytes: " << sizeof(State) << "\n";
            std::cout << "Number of states: " << NumStates << "\n";
            std::cout << "States array size in bytes: " << sizeof(states) << "\n";
            std::cout << "Can have empty matches? " << (canHaveEmptyMatches() ? "Yes" : "No") << "\n";
        }


    private:
        constexpr Matcher() = default;

        friend class Pattern;

        struct State {
            struct {
                bool hasTransition;
                V literal;
                size_t result;
            } literalTransition;
            struct {
                std::array<size_t, 2> elements;
                size_t length = 0;
            } epsilonTransitions = {};
        };

        std::array<State, NumStates> states = {};


        constexpr bool followAllTransitions(size_t stateIndex, V literal, std::array<bool, NumStates + 1>& nextStates) const {
            State state = states[stateIndex];
            if(!state.literalTransition.hasTransition || state.literalTransition.literal != literal) {
                return false;
            }
            followEmptyTransitions(state.literalTransition.result, nextStates);
            nextStates[state.literalTransition.result] = true;
            return true;
        }


        constexpr void followEmptyTransitions(size_t stateIndex, std::array<bool, NumStates + 1>& nextStates) const {
            State state = states[stateIndex];
            if(state.epsilonTransitions.length == 0 || nextStates[stateIndex] || stateIndex == NumStates) {
                return;
            }
            for(size_t i = 0; i < state.epsilonTransitions.length; i++) {
                size_t nextStateIndex = state.epsilonTransitions.elements[i];
                followEmptyTransitions(nextStateIndex, nextStates);
                nextStates[nextStateIndex] = true;
            }
        }
    };


    template <typename V>
    static constexpr auto literal(V literal) {
        Matcher<V, 1> matcher;
        matcher.states[0] = {
            {true, literal, 1}
        };
        return matcher;
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const Matcher<V, N>& matcher, const T&... matcherRest) {
        if constexpr (sizeof...(T) == 0) {
            return matcher;
        } else {
            auto combinedMatcherRest = concat(matcherRest...);
            constexpr size_t matcherSize = N + combinedMatcherRest.states.size();
            Matcher<V, matcherSize> newMatcher;
            insertStatesAt(0, newMatcher, matcher);
            insertStatesAt(N, newMatcher, combinedMatcherRest);
            return newMatcher;
        }
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const std::array<V, N>& literalArray, const T&... literalArrayRest) {
        if constexpr (sizeof...(T) == 0) {
            static_assert(N > 0, "Must have at least one literal in array");
            Matcher<V, N> newMatcher;
            for(size_t i = 0; i < N; i++) {
                newMatcher.states[i] = {
                    {true, literalArray[i], i + 1}
                };
            }
            return newMatcher;
        } else {
            return concat(concat(literalArray), concat(literalArrayRest)...);
        }
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const V (&literalArray)[N], const T&... literalArrayRest) {
        constexpr size_t realLength = std::is_same_v<V, char> ? N - 1 : N;
        static_assert(realLength > 0, "Must have at least one literal in array");
        std::array<V, realLength> arr = {};
        for(size_t i = 0; i < realLength; i++) {
            arr[i] = literalArray[i];
        }
        return concat(arr, concat(literalArrayRest)...);
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const Matcher<V, N>& matcher, const T&... matcherRest) {
        if constexpr (sizeof...(T) == 0) {
            return matcher;
        } else {
            auto combinedMatcherRest = anyLongest(matcherRest...);
            constexpr size_t matcherSize = N + combinedMatcherRest.states.size() + 1;
            Matcher<V, matcherSize> newMatcher;
            newMatcher.states[0] = {
                {},
                { {1, N + 1}, 2 }
            };
            insertStatesAt(1, newMatcher, matcher, matcherSize);
            insertStatesAt(N + 1, newMatcher, combinedMatcherRest);
            return newMatcher;
        }
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const V (&literalArray)[N], const T&... literalArrayRest) {
        return anyLongest(concat(literalArray), concat(literalArrayRest)...);
    }


    template <typename V, size_t N>
    static constexpr auto optional(const Matcher<V, N>& matcher) {
        Matcher<V, N + 1> newMatcher;
        newMatcher.states[0] = {
            {},
            { {1, N + 1}, 2 }
        };
        insertStatesAt(1, newMatcher, matcher);
        return newMatcher;
    }


    template <typename V, size_t N>
    static constexpr auto optional(const V (&literalArray)[N]) {
        return optional(concat(literalArray));
    }


    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N + 2> newMatcher;
        newMatcher.states[0] = {
            {},
            { {1, N + 2}, 2 }
        };
        insertStatesAt(1, newMatcher, matcher);
        newMatcher.states[N + 1] = {
            {},
            { {1, N + 2}, 2 }
        };
        return newMatcher;
    }


    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const V (&literalArray)[N]) {
        return repeatZeroOrMoreTimes(concat(literalArray));
    }


    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N * n> newMatcher;
        for(size_t i = 0; i < n; i++) {
            insertStatesAt(i * N, newMatcher, matcher);
        }
        return newMatcher;
    }


    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const V (&literalArray)[N]) {
        return repeatNTimes<n>(concat(literalArray));
    }


private:
    template <typename V, size_t N1, size_t N2>
    static constexpr void insertStatesAt(size_t index, Matcher<V, N1>& matcherTo, const Matcher<V, N2>& matcherFrom) {
        assert(index + N2 <= N1);
        for(size_t i = 0; i < N2; i++) {
            typename Matcher<V, N2>::State state = matcherFrom.states[i];
            matcherTo.states[i + index] = {
                {
                    state.literalTransition.hasTransition,
                    state.literalTransition.literal,
                    state.literalTransition.result + index
                },
                {
                    {
                        state.epsilonTransitions.elements[0] + index,
                        state.epsilonTransitions.elements[1] + index
                    },
                    state.epsilonTransitions.length
                }
            };
        }
    }


    template <typename V, size_t N1, size_t N2>
    static constexpr void insertStatesAt(size_t index, Matcher<V, N1>& matcherTo, const Matcher<V, N2>& matcherFrom, size_t newAcceptState) {
        assert(index + N2 <= N1);
        for(size_t i = 0; i < N2; i++) {
            typename Matcher<V, N2>::State state = matcherFrom.states[i];
            size_t literalTransitionResult = state.literalTransition.result + index;
            if(state.literalTransition.result == N2) {
                literalTransitionResult = newAcceptState;
            }
            size_t epsilonTransitions0 = state.epsilonTransitions.elements[0] + index;
            if(state.epsilonTransitions.elements[0] == N2) {
                epsilonTransitions0 = newAcceptState;
            }
            size_t epsilonTransitions1 = state.epsilonTransitions.elements[1] + index;
            if(state.epsilonTransitions.elements[1] == N2) {
                epsilonTransitions1 = newAcceptState;
            }
            matcherTo.states[i + index] = {
                {
                    state.literalTransition.hasTransition,
                    state.literalTransition.literal,
                    literalTransitionResult
                },
                {
                    {
                        epsilonTransitions0,
                        epsilonTransitions1
                    },
                    state.epsilonTransitions.length
                }
            };
        }
    }
};