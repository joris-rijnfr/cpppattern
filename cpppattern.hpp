#pragma once

#include <vector>
#include <array>
#include <iostream>



class Pattern {
private:
    template <typename V>
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


    template <typename V, size_t N1, size_t N2, template <typename, size_t> typename M>
    static constexpr auto combineAnyLongest(const M<V, N1>& a, const M<V, N2>& b) {
        M<V, N1 + N2 + 1> newMatcher;
        newMatcher.pushStates(a);
        newMatcher.pushStates(b);
        newMatcher.updateTransitions(a.size, newMatcher.size);
        if(!newMatcher.tryAddEpsilon(0, a.size)) {
            newMatcher.insertState(0, {
                {},
                { {1, newMatcher.size + 1}, 2 }
            });
        }
        return newMatcher;
    }


public:
    template <typename Iterator>
    struct Result {
        Iterator matchStart{};
        bool matched = false;
        size_t length = 0;
    };


    template <typename V, size_t Capacity>
    class Matcher {
    public:
        Matcher(const Matcher&) = default;
        Matcher& operator=(const Matcher&) = default;


        template <typename Iterator>
        constexpr Result<Iterator> matchFirst(Iterator begin, Iterator end) const {
            if(begin == end) {
                return { std::move(end), canHaveEmptyMatches() };
            }
            for(; begin != end; begin++) {
                Result<Iterator> result = match(begin, end);
                if(result.matched) {
                    return result;
                }
            }
            return { std::move(end) };
        }


        template <typename T>
        constexpr auto matchFirst(const T& input) const {
            using std::begin;
            using std::end;
            return matchFirst(begin(input), end(input));
        }


        template <typename Iterator>
        constexpr Result<Iterator> matchLast(Iterator begin, Iterator end) const {
            if(begin == end) {
                return { std::move(end), canHaveEmptyMatches() };
            }
            Result<Iterator> current = end;
            current--;
            for(; current != begin; current--) {
                Result<Iterator> result = match(current, end);
                if(result.matched) {
                    return result;
                }
            }
            return { std::move(end) };
        }


        template <typename T>
        constexpr auto matchLast(const T& input) const {
            using std::begin;
            using std::end;
            return matchLast(begin(input), end(input));
        }


        template <typename Iterator>
        std::vector<Result<Iterator>> matchAll(Iterator begin, Iterator end) const {
            if(begin == end) {
                if(canHaveEmptyMatches()) {
                    return { { std::move(end), true } };
                }
                return { };
            }
            std::vector<Result<Iterator>> results;
            for(; begin != end; begin++) {
                Result<Iterator> result = match(begin, end);
                if(result.matched) {
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
        constexpr Result<Iterator> matchAt(Iterator begin, Iterator end) const {
            if(begin == end) {
                return { std::move(end), canHaveEmptyMatches() };
            }
            return match(begin, end);
        }


        template <typename T>
        constexpr auto matchAt(const T& input, size_t index) const {
            using std::begin;
            using std::end;
            using std::next;
            return matchAt(next(begin(input), index), end(input));
        }


        constexpr bool canHaveEmptyMatches() const {
            std::array<bool, Capacity + 1> currentStates = { false };
            followEmptyTransitions(0, currentStates);
            return currentStates[size];
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
            std::cout << "    " << size << " [label=\"ACCEPT\", shape=circle];\n";
            std::cout << "}\n\n";
            std::cout << "State struct size:                  " << sizeof(State<V>) << " bytes\n";
            std::cout << "States array size:                  " << sizeof(states) << " bytes\n";
            std::cout << "Number of states:                   " << size << "\n";
            std::cout << "States array capacity:              " << Capacity << "\n";
            std::cout << "Wasted space on unfilled capacity:  " << (sizeof(states) - (sizeof(State<V>) * size)) << " bytes\n";
            std::cout << "Can have empty matches?             " << (canHaveEmptyMatches() ? "Yes" : "No") << "\n";
        }


    private:
        constexpr Matcher() = default;

        template <typename T, size_t N>
        explicit constexpr Matcher(const Matcher<T, N>& matcher)
            : size(matcher.size){
            static_assert(std::is_same_v<T, V>, "Matcher types must be the same");
            static_assert(N <= Capacity, "Matcher capacity must be equal or greater than given matcher");
            for(size_t i = 0; i < matcher.size; i++) {
                states[i] = matcher.states[i];
            }
        }

        friend class Pattern;

        std::array<State<V>, Capacity> states = {};
        size_t size = 0;


        template <typename Iterator>
        constexpr Result<Iterator> match(Iterator begin, Iterator end) const {
            std::array<bool, Capacity + 1> currentStates = { false };
            followEmptyTransitions(0, currentStates);
            Result<Iterator> result = { begin };
            size_t matchLength = 0;
            for(; begin != end; begin++) {
                matchLength++;
                std::array<bool, Capacity + 1> nextStates = { false };
                bool transitionHappened = false;
                V currentLiteral = *begin;
                for(size_t j = 0; j < size; j++) {
                    if(currentStates[j]) {
                        transitionHappened = followAllTransitions(j, currentLiteral, nextStates) || transitionHappened;
                    }
                }
                if(nextStates[size]) {
                    result.matched = true;
                    result.length = matchLength;
                }
                if(!transitionHappened) {
                    break;
                }
                currentStates = nextStates;
            }
            if(!result.matched) {
                result.matchStart = end;
            }
            return result;
        }


        constexpr bool followAllTransitions(size_t stateIndex, V literal, std::array<bool, Capacity + 1>& nextStates) const {
            State state = states[stateIndex];
            if(!state.literalTransition.hasTransition || state.literalTransition.literal != literal) {
                return false;
            }
            followEmptyTransitions(state.literalTransition.result, nextStates);
            nextStates[state.literalTransition.result] = true;
            return true;
        }


        constexpr void followEmptyTransitions(size_t stateIndex, std::array<bool, Capacity + 1>& nextStates) const {
            State state = states[stateIndex];
            if(nextStates[stateIndex]) {
                return;
            }
            nextStates[stateIndex] = true;
            if(stateIndex == size) {
                return;
            }
            for(size_t i = 0; i < state.epsilonTransitions.length; i++) {
                size_t nextStateIndex = state.epsilonTransitions.elements[i];
                followEmptyTransitions(nextStateIndex, nextStates);
            }
        }


        constexpr void insertState(size_t index, const State<V> &state) {
            for(size_t i = 0; i < size - index; i++) {
                setState(size - i, states[size - i - 1], 1);
            }
            setState(index, state, index);
            size++;
        }


        constexpr void pushState(const State<V>& state) {
            states[size] = state;
            size++;
        }


        template <typename T, size_t N>
        constexpr void pushStates(const Matcher<T, N>& matcher) {
            for(size_t i = 0; i < matcher.size; i++) {
                setState(size + i, matcher.states[i], size);
            }
            size += matcher.size;
        }


        constexpr void setState(size_t index, const State<V>& state, size_t transitionOffset = 0) {
            states[index] = state;
            states[index].literalTransition.result += transitionOffset;
            states[index].epsilonTransitions.elements[0] += transitionOffset;
            states[index].epsilonTransitions.elements[1] += transitionOffset;
        }


        constexpr void updateTransitions(size_t currentIndex, size_t newIndex) {
            for(State<V>& state : states) {
                if(state.literalTransition.result == currentIndex) {
                    state.literalTransition.result = newIndex;
                }
                if(state.epsilonTransitions.elements[0] == currentIndex) {
                    state.epsilonTransitions.elements[0] = newIndex;
                }
                if(state.epsilonTransitions.elements[1] == currentIndex) {
                    state.epsilonTransitions.elements[1] = newIndex;
                }
            }
        }


        constexpr bool tryAddEpsilon(size_t index, size_t to) {
            if(states[index].epsilonTransitions.length < 2) {
                states[index].epsilonTransitions.elements[states[index].epsilonTransitions.length] = to;
                states[index].epsilonTransitions.length++;
                return true;
            }
            return false;
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
            constexpr size_t matcherCapacity = N + combinedMatcherRest.states.size();
            Matcher<V, matcherCapacity> newMatcher(matcher);
            newMatcher.pushStates(combinedMatcherRest);
            return newMatcher;
        }
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const V (&param)[N], T&... paramRest) {
        if constexpr (sizeof...(T) == 0) {
            constexpr size_t length = std::is_same_v<char, V> ? N - 1 : N;
            Matcher<V, length> matcher;
            for(size_t i = 0; i < length; i++) {
                matcher.pushState({
                    {true, param[i], i + 1}
                });
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
            if(matcher.states[0].epsilonTransitions.length > combinedMatcherRest.states[0].epsilonTransitions.length) {
                return combineAnyLongest(matcher, combinedMatcherRest);
            }
            return combineAnyLongest(combinedMatcherRest, matcher);
        }
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const V (&param)[N], const T&... paramRest) {
        if constexpr (sizeof...(T) == 0) {
            return concat(param);
        } else {
            return anyLongest(concat(param), concat(paramRest)...);
        }
    }


    template <typename V, size_t N>
    static constexpr auto optional(const Matcher<V, N>& matcher) {
        Matcher<V, N + 1> newMatcher(matcher);
        if(!newMatcher.canHaveEmptyMatches() && !newMatcher.tryAddEpsilon(0, newMatcher.size)) {
            newMatcher.insertState(0, {
                {},
                { {1, newMatcher.size + 1}, 2 }
            });
        }
        return newMatcher;
    }


    template <typename V, size_t N>
    static constexpr auto optional(const V (&param)[N]) {
        return optional(concat(param));
    }


    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N + 2> newMatcher(matcher);
        if(!newMatcher.canHaveEmptyMatches() && !newMatcher.tryAddEpsilon(0, newMatcher.size)) {
            newMatcher.insertState(0, {
                {},
                { {1, newMatcher.size + 1}, 2 }
            });
        }
        newMatcher.pushState({
            {},
            { {0, newMatcher.size + 1}, 2 }
        });
        return newMatcher;
    }


    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const V (&param)[N]) {
        return repeatZeroOrMoreTimes(concat(param));
    }


    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N * n> newMatcher(matcher);
        for(size_t i = 1; i < n; i++) {
            newMatcher.pushStates(matcher);
        }
        return newMatcher;
    }


    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const V (&param)[N]) {
        return repeatNTimes<n>(concat(param));
    }
};
