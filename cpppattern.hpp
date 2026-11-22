// Copyright (c) 2026 Joris Rijnfrank
// Licensed under the MIT License.
// See https://github.com/joris-rijnfr/cpppattern/blob/main/LICENSE

// Version: 0.1.0
// Source: https://github.com/joris-rijnfr/cpppattern

#pragma once

// Begin src/matcher.hpp
#include <vector>
#include <array>

// Begin src/pattern.hpp
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
    static constexpr auto literal(const V value) {
        Matcher<V, 1> matcher;
        matcher.pushState({ value, 1 });
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
        Matcher<V, N * n> newMatcher;
        for(size_t i = 0; i < n; i++) {
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
// End src/pattern.hpp

template <typename V, size_t Capacity>
class Pattern::Matcher {
public:
    friend class Pattern;
    template <typename Iterator>
    constexpr Result<Iterator> matchFirst(Iterator begin, Iterator end) const {
        if(begin == end || size == 0) {
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
        if(begin == end || size == 0) {
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
        if(begin == end || size == 0) {
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
        if(begin == end || size == 0) {
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
        if constexpr (Capacity == 0) {
            return true;
        } else {
            std::array<bool, Capacity + 1> currentStates = { false };
            followEmptyTransitions(0, currentStates);
            return currentStates[size];
        }
    }
private:
    constexpr Matcher() = default;
    constexpr Matcher(const Matcher&) = default;
    constexpr Matcher(Matcher&&) noexcept = default;
    constexpr Matcher& operator=(const Matcher&) = default;
    constexpr Matcher& operator=(Matcher&&) noexcept = default;
    template <size_t N>
    explicit constexpr Matcher(const Matcher<V, N>& matcher) : size(matcher.size) {
        static_assert(N <= Capacity, "Matcher capacity must be equal or greater than given matcher");
        for(StateIndex i = 0; i < matcher.size; i++) {
            states[i] = matcher.states[i];
        }
    }
    std::array<State<V>, Capacity> states = {};
    StateIndex size = 0;
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
            for(StateIndex j = 0; j < size; j++) {
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
    constexpr bool followAllTransitions(StateIndex index, V literal, std::array<bool, Capacity + 1>& nextStates) const {
        State state = states[index];
        if(state.literalTarget == NO_STATE || state.literal != literal) {
            return false;
        }
        followEmptyTransitions(state.literalTarget, nextStates);
        nextStates[state.literalTarget] = true;
        return true;
    }
    constexpr void followEmptyTransitions(StateIndex index, std::array<bool, Capacity + 1>& nextStates) const {
        State state = states[index];
        if(nextStates[index]) {
            return;
        }
        nextStates[index] = true;
        if(index == size) {
            return;
        }
        for(EpsilonSize i = 0; i < state.epsilonSize; i++) {
            followEmptyTransitions(state.epsilon[i], nextStates);
        }
    }
    constexpr void insertState(StateIndex index, const State<V> &state) {
        for(StateIndex i = 0; i < size - index; i++) {
            setState(size - i, states[size - i - 1], 1);
        }
        setState(index, state, index);
        size++;
    }
    constexpr void pushState(const State<V>& state) {
        states[size] = state;
        size++;
    }
    template <size_t N>
    constexpr void pushStates(const Matcher<V, N>& matcher) {
        for(StateIndex i = 0; i < matcher.size; i++) {
            setState(size + i, matcher.states[i], size);
        }
        size += matcher.size;
    }
    constexpr void setState(StateIndex index, const State<V>& state, StateIndex transitionOffset = 0) {
        State<V>& thisState = states[index];
        thisState = state;
        if(thisState.literalTarget != NO_STATE) {
            thisState.literalTarget += transitionOffset;
        }
        EpsilonSize numberOfTransitions = thisState.epsilonSize;
        for(EpsilonSize i = 0; i < numberOfTransitions; i++) {
            thisState.epsilon[i] += transitionOffset;
        }
    }
    constexpr void updateTransitions(StateIndex currentIndex, StateIndex newIndex) {
        for(State<V>& state : states) {
            if(state.literalTarget == currentIndex) {
                state.literalTarget = newIndex;
            }
            EpsilonSize numberOfTransitions = state.epsilonSize;
            for(EpsilonSize i = 0; i < numberOfTransitions; i++) {
                if(state.epsilon[i] == currentIndex) {
                    state.epsilon[i] = newIndex;
                }
            }
        }
    }
    constexpr bool tryAddEpsilon(StateIndex index, StateIndex to) {
        if(State<V>& state = states[index]; state.epsilonSize < MaxEpsilon) {
            state.epsilon[state.epsilonSize] = to;
            state.epsilonSize++;
            return true;
        }
        return false;
    }
    template <size_t N>
    constexpr bool hasEpsilonTransitionsToTargets(StateIndex index, const StateIndex (&targets)[N]) {
        if constexpr (N > MaxEpsilon) {
            return false;
        }
        State<V> state = states[index];
        std::array<bool, N> seen = {false};
        EpsilonSize leftToBeSeen = N;
        for(EpsilonSize i = 0; i < state.epsilonSize; i++) {
            StateIndex target = state.epsilon[i];
            for(size_t j = 0; j < N; j++) {
                if(target == targets[j] && !seen[j]) {
                    seen[j] = true;
                    leftToBeSeen--;
                    break;
                }
            }
            if(leftToBeSeen == 0) {
                return true;
            }
        }
        return leftToBeSeen == 0;
    }
};
// End src/matcher.hpp

