#pragma once

#include <cstddef>
#include <array>



class Pattern {
public:
    template <typename V, size_t Capacity>
    class Matcher;


    template <typename Iterator>
    struct Result {
        Iterator matchStart{};
        bool matched = false;
        size_t length = 0;
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
};
