#pragma once

#include <iterator>
#include <type_traits>
#include <vector>
#include <array>
#include <iostream>
#include <cassert>




class Pattern {
public:
    template <typename Iterator>
    struct Result {
        Iterator start{};
        size_t length = 0;
    };


    template <typename V, size_t NumStates>
    class Matcher {
    public:
        Matcher(const Matcher&) = default;
        Matcher& operator=(const Matcher&) = default;


        template <typename Iterator>
        constexpr Result<Iterator> matchFirst(Iterator begin, Iterator end) const {
            for(; begin != end; begin++) {
                Result<Iterator> result = matchAt(begin, end);
                if(result.start != end) {
                    return result;
                }
            }
            return Result<Iterator> { std::move(end), 0 };
        }


        template <typename T>
        constexpr auto matchFirst(const T& input) const {
            using std::begin;
            using std::end;
            using std::next;
            return matchFirst(begin(input), end(input));
        }


        template <typename Iterator>
        constexpr Result<Iterator> matchLast(Iterator begin, Iterator end) const {
            Result<Iterator> current = end;
            current--;
            for(; current != begin; current--) {
                Result<Iterator> result = matchAt(current, end);
                if(result.start != end) {
                    return result;
                }
            }
            return Result<Iterator> { std::move(end), 0 };
        }


        template <typename T>
        constexpr auto matchLast(const T& input) const {
            using std::begin;
            using std::end;
            using std::next;
            return matchLast(begin(input), end(input));
        }


        template <typename Iterator>
        std::vector<Result<Iterator>> matchAll(Iterator begin, Iterator end) const {
            std::vector<Result<Iterator>> results;
            for(; begin != end; begin++) {
                Result<Iterator> result = matchAt(begin, end);
                if(result.start != end) {
                    results.push_back(result);
                }
            }
            return results;
        }


        template <typename T>
        constexpr auto matchAll(const T& input) const {
            using std::begin;
            using std::end;
            return matchAll(begin(input), end(input));
        }


        template <typename Iterator>
        constexpr auto matchAt(Iterator begin, Iterator end) const {
            std::array<bool, NumStates + 1> currentStates = { false };
            followEmptyTransitions(0, currentStates);
            currentStates[0] = true;
            Result<Iterator> match = {
                begin,
                0
            };
            bool matched = false;
            size_t matchLength = 0;
            for(; begin != end; begin++) {
                std::array<bool, NumStates + 1> nextStates = { false };
                bool transitionHappened = false;
                matchLength++;
                V currentLiteral = *begin;
                for(size_t j = 0; j < NumStates; j++) {
                    if(currentStates[j]) {
                        transitionHappened = followAllTransitions(j, currentLiteral, nextStates) || transitionHappened;
                    }
                }
                if(nextStates[NumStates]) {
                    matched = true;
                    match.length = matchLength;
                }
                if(!transitionHappened) {
                    break;
                }
                currentStates = nextStates;
            }
            if(!matched) {
                match.start = end;
            }
            return match;
        }


        template <typename T>
        constexpr auto matchAt(const T& input, size_t index) const {
            using std::begin;
            using std::end;
            using std::next;
            return matchAt(next(begin(input), index), end(input));
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
    static constexpr auto concat(const V (&param)[N], const T&... paramRest) {
        if constexpr (sizeof...(T) == 0) {
            constexpr size_t realLength = std::is_same_v<char, V> ? N - 1 : N;
            Matcher<V, realLength> matcher;
            for(size_t i = 0; i < realLength; i++) {
                matcher.states[i] = {
                    {true, param[i], i + 1}
                };
            }
            return matcher;
        } else {
            return concat(concat(param), concat(paramRest...));
        }
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


    template <typename T1, typename... T2>
    static constexpr auto anyLongest(const T1& param, const T2&... paramRest) {
        if constexpr (sizeof...(T2) == 0) {
            return concat(param);
        } else {
            return anyLongest(concat(param), concat(paramRest)...);
        }
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


    template <typename T>
    static constexpr auto optional(const T& param) {
        return optional(concat(param));
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


    template <typename T>
    static constexpr auto repeatZeroOrMoreTimes(const T& param) {
        return repeatZeroOrMoreTimes(concat(param));
    }


    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N * n> newMatcher;
        for(size_t i = 0; i < n; i++) {
            insertStatesAt(i * N, newMatcher, matcher);
        }
        return newMatcher;
    }


    template <size_t n, typename T>
    static constexpr auto repeatNTimes(const T& param) {
        return repeatNTimes<n>(concat(param));
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