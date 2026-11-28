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


    /**
     * @brief The result of a matching operation.
     */
    template <typename Iterator>
    struct Result {
        Iterator matchStart{};  // An iterator pointing to the first element of the match in the input.
        bool matched = false;   // Indicates whether a match was found.
        size_t length = 0;      // The number of elements in the match.
    };


    /**
     * @brief Creates a matcher that matches any single element that is equal to `value`.
     * 
     * @param value The value to match on.
     * @return A matcher that matches any single element that is equal to `value`.
     */
    template <typename V>
    static constexpr auto literal(const V value) {
        Matcher<V, 1> matcher;
        matcher.pushState({ value, 1 });
        return matcher;
    }


    /**
     * @brief Creates a matcher that matches all arguments in sequence.
     * 
     * @param matcher The initial matcher.
     * @param rest Additional matchers or values to concatenate.
     * @return A matcher that matches all arguments in sequence.
     */
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


    /**
     * @brief Creates a matcher that matches all arguments in sequence.
     * 
     * @param param An array whose elements will be concatenated.
     * @param rest Additional matchers or values to concatenate.
     * @return A matcher that matches all arguments in sequence.
     */
    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const std::array<V, N>& param, const T&... rest) {
        Matcher<V, N> matcher;
        for(StateIndex i = 0; i < N; i++) {
            matcher.pushState({ param[i], i + 1 });
        }
        return concat(matcher, rest...);
    }


    /**
     * @brief Creates a matcher that matches all arguments in sequence.
     * 
     * @param param An array whose elements will be concatenated.
     * @param rest Additional matchers or values to concatenate.
     * @return A matcher that matches all arguments in sequence.
     */
    template <typename V, size_t N, typename... T>
    static constexpr auto concat(const V (&param)[N], const T&... rest) {
        constexpr size_t length = std::is_same_v<char, V> ? N - 1 : N;
        return concat(toStdArray<V, N, length>(param, std::make_index_sequence<length>{}), rest...);
    }


    /**
     * @brief Creates a matcher that matches all arguments in sequence.
     * 
     * @param param The initial value.
     * @param rest Additional matchers or values to concatenate.
     * @return A matcher that matches all arguments in sequence.
     */
    template <typename V, typename... T>
    static constexpr auto concat(const V param, const T&... rest) {
        return concat(literal(param), rest...);
    }


    /**
     * @brief Creates a matcher that matches any of the arguments.
     *
     * The longest-match semantics of the matcher ensure that the longest
     * match is found. The order of the arguments do not matter. It has no
     * influence on the longest match between any of the arguments.
     *
     * @param matcher A matcher to use as possible matcher.
     * @param rest Additional matchers or values use as possible matchers.
     * @return A matcher that matches the longest possible match among the arguments.
     */
    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const Matcher<V, N>& matcher, const T&... rest) {
        if constexpr (sizeof...(T) == 0) {
            return matcher;
        } else {
            auto combinedMatcherRest = anyLongest(rest...);
            if(matcher.size > 0 && combinedMatcherRest.size > 0 && matcher.states[0].epsilonSize > combinedMatcherRest.states[0].epsilonSize) {
                return combineAnyLongest(matcher, combinedMatcherRest);
            }
            return combineAnyLongest(combinedMatcherRest, matcher);
        }
    }


    /**
     * @brief Creates a matcher that matches any of the arguments.
     *
     * The longest-match semantics of the matcher ensure that the longest
     * match is found. The order of the arguments do not matter. It has no
     * influence on the longest match between any of the arguments.
     *
     * @param param An array whose elements will be concatenated and seen as a single sequence.
     * @param rest Additional matchers or values use as possible matchers.
     * @return A matcher that matches the longest possible match among the arguments.
     */
    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const std::array<V, N>& param, const T&... rest) {
        return anyLongest(concat(param), rest...);
    }


    /**
     * @brief Creates a matcher that matches any of the arguments.
     *
     * The longest-match semantics of the matcher ensure that the longest
     * match is found. The order of the arguments do not matter. It has no
     * influence on the longest match between any of the arguments.
     *
     * @param param An array whose elements will be concatenated and seen as a single sequence.
     * @param rest Additional matchers or values use as possible matchers.
     * @return A matcher that matches the longest possible match among the arguments.
     */
    template <typename V, size_t N, typename... T>
    static constexpr auto anyLongest(const V (&param)[N], const T&... rest) {
        return anyLongest(concat(param), rest...);
    }


    /**
     * @brief Creates a matcher that matches any of the arguments.
     *
     * The longest-match semantics of the matcher ensure that the longest
     * match is found. The order of the arguments do not matter. It has no
     * influence on the longest match between any of the arguments.
     *
     * @param param A single value to possibly match.
     * @param rest Additional matchers or values use as possible matchers.
     * @return A matcher that matches the longest possible match among the arguments.
     */
    template <typename V, typename... T>
    static constexpr auto anyLongest(const V param, const T&... rest) {
        return anyLongest(literal(param), rest...);
    }


    /**
     * @brief Creates a matcher that optionally matches the argument.
     *
     * @param matcher The matcher to make optional.
     * @return A matcher that matches the argument zero or one times.
     */
    template <typename V, size_t N>
    static constexpr auto optional(const Matcher<V, N>& matcher) {
        Matcher<V, N + 1> newMatcher(matcher);
        if(!newMatcher.canHaveEmptyMatches() && !newMatcher.tryAddEpsilon(0, newMatcher.size)) {
            newMatcher.insertState(0, { {}, NO_STATE, 2, {1, newMatcher.size + 1} });
        }
        return newMatcher;
    }


    /**
     * @brief Creates a matcher that optionally matches the argument.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument zero or one times.
     */
    template <typename V, size_t N>
    static constexpr auto optional(const std::array<V, N>& param) {
        return optional(concat(param));
    }


    /**
     * @brief Creates a matcher that optionally matches the argument.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument zero or one times.
     */
    template <typename V, size_t N>
    static constexpr auto optional(const V (&param)[N]) {
        return optional(concat(param));
    }


    /**
     * @brief Creates a matcher that optionally matches the argument.
     *
     * @param param The value to make optional.
     * @return A matcher that matches the argument zero or one times.
     */
    template <typename V>
    static constexpr auto optional(const V param) {
        return optional(literal(param));
    }


    /**
     * @brief Creates a matcher that matches the argument zero or more times.
     *
     * @param matcher The matcher to repeat.
     * @return A matcher that matches the argument zero or more times.
     */
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


    /**
     * @brief Creates a matcher that matches the argument zero or more times.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument zero or more times.
     */
    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const std::array<V, N>& param) {
        return repeatZeroOrMoreTimes(concat(param));
    }


    /**
     * @brief Creates a matcher that matches the argument zero or more times.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument zero or more times.
     */
    template <typename V, size_t N>
    static constexpr auto repeatZeroOrMoreTimes(const V (&param)[N]) {
        return repeatZeroOrMoreTimes(concat(param));
    }


    /**
     * @brief Creates a matcher that matches the argument zero or more times.
     *
     * @param param The value to repeat.
     * @return A matcher that matches the argument zero or more times.
     */
    template <typename V>
    static constexpr auto repeatZeroOrMoreTimes(const V param) {
        return repeatZeroOrMoreTimes(literal(param));
    }


    /**
     * @brief Creates a matcher that matches the argument one or more times.
     *
     * @param matcher The matcher to repeat.
     * @return A matcher that matches the argument one or more times.
     */
    template <typename V, size_t N>
    static constexpr auto repeatOneOrMoreTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N + 1> newMatcher(matcher);
        if(!newMatcher.hasEpsilonTransitionsToTargets(newMatcher.size - 1, {0, newMatcher.size})) {
            newMatcher.pushState({ {}, NO_STATE, 2, {0, newMatcher.size + 1} });
        }
        return newMatcher;
    }


    /**
     * @brief Creates a matcher that matches the argument one or more times.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument one or more times.
     */
    template <typename V, size_t N>
    static constexpr auto repeatOneOrMoreTimes(const std::array<V, N>& param) {
        return repeatOneOrMoreTimes(concat(param));
    }


    /**
     * @brief Creates a matcher that matches the argument one or more times.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument one or more times.
     */
    template <typename V, size_t N>
    static constexpr auto repeatOneOrMoreTimes(const V (&param)[N]) {
        return repeatOneOrMoreTimes(concat(param));
    }


    /**
     * @brief Creates a matcher that matches the argument one or more times.
     *
     * @param param The value to repeat.
     * @return A matcher that matches the argument one or more times.
     */
    template <typename V>
    static constexpr auto repeatOneOrMoreTimes(const V param) {
        return repeatOneOrMoreTimes(literal(param));
    }


    /**
     * @brief Creates a matcher that matches the argument exactly `n` times.
     *
     * @tparam n The number of times to repeat the argument.
     *
     * @param matcher The matcher to repeat.
     * @return A matcher that matches the argument exactly n times.
     */
    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const Matcher<V, N>& matcher) {
        Matcher<V, N * n> newMatcher;
        for(size_t i = 0; i < n; i++) {
            newMatcher.pushStates(matcher);
        }
        return newMatcher;
    }


    /**
     * @brief Creates a matcher that matches the argument exactly `n` times.
     *
     * @tparam n The number of times to repeat the argument.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument exactly n times.
     */
    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const std::array<V, N>& param) {
        return repeatNTimes<n>(concat(param));
    }


    /**
     * @brief Creates a matcher that matches the argument exactly `n` times.
     *
     * @tparam n The number of times to repeat the argument.
     *
     * @param param An array whose elements will be concatenated.
     * @return A matcher that matches the argument exactly n times.
     */
    template <size_t n, typename V, size_t N>
    static constexpr auto repeatNTimes(const V (&param)[N]) {
        return repeatNTimes<n>(concat(param));
    }


    /**
     * @brief Creates a matcher that matches the argument exactly `n` times.
     *
     * @tparam n The number of times to repeat the argument.
     *
     * @param param The value to repeat.
     * @return A matcher that matches the argument exactly n times.
     */
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
        Matcher<V, N1 + N2 + 1> newMatcher(a);
        // Must happen before b is pushed otherwise transitions to 0 (in b) will suddenly point to ACCEPT
        newMatcher.updateTransitions(a.size, a.size + b.size);
        newMatcher.pushStates(b);
        if(!newMatcher.tryAddEpsilon(0, a.size)) {
            newMatcher.insertState(0, { {}, NO_STATE, 2, {1, a.size + 1} });
        }
        return newMatcher;
    }
};
