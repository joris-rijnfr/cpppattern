#pragma once

#include <cstddef>
#include <array>
#include <cstdint>



class Pattern {
public:
    template <typename V, size_t Capacity>
    class Matcher;


    Pattern(const Pattern&) = delete;
    Pattern(Pattern&&) = delete;
    Pattern& operator=(const Pattern&) = delete;
    Pattern& operator=(Pattern&&) = delete;


    template <typename Iterator>
    struct Result {
        Iterator matchStart{};
        bool matched = false;
        size_t length = 0;
    };


    template <typename V>
    static constexpr auto literal(const V literal) {
        Matcher<V, 1> matcher;
        matcher.pushState({ literal, 1 });
        return matcher;
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const Matcher<V, N>& matcher, const T&... rest) {
        if constexpr (sizeof...(T) == 0) {
            return matcher;
        } else {
            auto combinedMatcherRest = concat(rest...);
            constexpr size_t matcherCapacity = N + combinedMatcherRest.states.size();
            Matcher<V, matcherCapacity> newMatcher(matcher);
            newMatcher.pushStates(combinedMatcherRest);
            return newMatcher;
        }
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const std::array<V, N>& param, const T&... rest) {
        Matcher<V, N> matcher;
        for(StateIndex i = 0; i < N; i++) {
            matcher.pushState({ param[i], i + 1 });
        }
        return concat(matcher, rest...);
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const V (&param)[N], const T&... rest) {
        constexpr size_t length = std::is_same_v<char, V> ? N - 1 : N;
        return concat(toStdArray<V, N, length>(param, std::make_index_sequence<length>{}), rest...);
    }


    template <typename V, typename... T>
    static constexpr auto concat(const V param, const T&... rest) {
        return concat(literal(param), rest...);
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const Matcher<V, N>& matcher, const T&... rest) {
        if constexpr (sizeof...(T) == 0) {
            return matcher;
        } else {
            auto combinedMatcherRest = anyLongest(rest...);
            if(matcher.states[0].epsilonSize > combinedMatcherRest.states[0].epsilonSize) {
                return combineAnyLongest(matcher, combinedMatcherRest);
            }
            return combineAnyLongest(combinedMatcherRest, matcher);
        }
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const std::array<V, N>& param, const T&... rest) {
        return anyLongest(concat(param), rest...);
    }


    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const V (&param)[N], const T&... rest) {
        return anyLongest(concat(param), rest...);
    }


    template <typename V, typename... T>
    static constexpr auto anyLongest(const V param, const T&... rest) {
        return anyLongest(literal(param), rest...);
    }


    template <typename V, size_t N>
    static constexpr auto optional(const Matcher<V, N>& matcher) {
        Matcher<V, N + 1> newMatcher(matcher);
        if(!newMatcher.canHaveEmptyMatches() && !newMatcher.tryAddEpsilon(0, newMatcher.size)) {
            newMatcher.insertState(0, { {}, NO_STATE, 2, {1, newMatcher.size + 1} });
        }
        return newMatcher;
    }


    template <typename V, size_t N>
    static constexpr auto optional(const std::array<V, N>& param) {
        return optional(concat(param));
    }


    template <typename V, size_t N>
    static constexpr auto optional(const V (&param)[N]) {
        return optional(concat(param));
    }


    template <typename V>
    static constexpr auto optional(const V param) {
        return optional(literal(param));
    }


    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N + 2> newMatcher(matcher);
        bool shouldAddRepeatState = !newMatcher.hasEpsilonTransitionsToTargets(newMatcher.size - 1, {0, newMatcher.size});
        if(StateIndex repeatState = shouldAddRepeatState ? newMatcher.size : newMatcher.size - 1; !newMatcher.canHaveEmptyMatches() && !newMatcher.tryAddEpsilon(0, repeatState)) {
            newMatcher.insertState(0, { {}, NO_STATE, 2, {1, repeatState + 1} });
        }
        if(shouldAddRepeatState) {
            newMatcher.pushState({ {}, NO_STATE, 2, {0, newMatcher.size + 1} });
        }
        return newMatcher;
    }


    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const std::array<V, N>& param) {
        return repeatZeroOrMoreTimes(concat(param));
    }


    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const V (&param)[N]) {
        return repeatZeroOrMoreTimes(concat(param));
    }


    template <typename V>
    static constexpr auto repeatZeroOrMoreTimes(const V param) {
        return repeatZeroOrMoreTimes(literal(param));
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
    static constexpr auto repeatNTimes(const std::array<V, N>& param) {
        return repeatNTimes<n>(concat(param));
    }


    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const V (&param)[N]) {
        return repeatNTimes<n>(concat(param));
    }


    template <size_t n, typename V>
    static constexpr auto repeatNTimes(const V param) {
        return repeatNTimes<n>(literal(param));
    }


private:
    using StateIndex = uint32_t;
    inline static constexpr StateIndex NO_STATE = UINT32_MAX;
    inline static constexpr size_t MaxEpsilon = 2;
    using EpsilonSize = uint8_t;

    template <typename V>
    struct State {
        V literal;
        StateIndex literalTarget = NO_STATE;
        EpsilonSize epsilonSize = 0;
        std::array<StateIndex, MaxEpsilon> epsilon = { NO_STATE };
    };


    template <typename V, size_t N, size_t Length, size_t... I>
    static constexpr auto toStdArray(const V (&param)[N], std::index_sequence<I...>) {
        return std::array<V, Length>{ param[I]... };
    }


    template <typename V, size_t N1, size_t N2>
    static constexpr auto combineAnyLongest(const Matcher<V, N1>& a, const Matcher<V, N2>& b) {
        Matcher<V, N1 + N2 + 1> newMatcher;
        newMatcher.pushStates(a);
        newMatcher.pushStates(b);
        newMatcher.updateTransitions(a.size, newMatcher.size);
        if(!newMatcher.tryAddEpsilon(0, a.size)) {
            newMatcher.insertState(0, { {}, NO_STATE, 2, {1, a.size + 1} });
        }
        return newMatcher;
    }
};
