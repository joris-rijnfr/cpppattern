#pragma once

/*** BEGIN - NOT IN SINGLE HEADER ***/
#include <iostream>
/*** END - NOT IN SINGLE HEADER ***/
#include <vector>
#include <array>
#include "./pattern.hpp"



template <typename V, size_t Capacity>
class Pattern::Matcher {
public:
    friend class Pattern;


    /**
     * @brief Finds the match with the earliest starting position in the given input range.
     *
     * @param begin A forward iterator pointing to the beginning of the input range.
     * @param end An iterator pointing to the end of the input range.
     * @return The first match found in the input range, or a result indicating that no match was found.
     */
    template <typename Iterator>
    constexpr Result<Iterator> matchFirst(Iterator begin, Iterator end) const {
        for(; begin != end; begin++) {
            Result<Iterator> result = match(begin, end);
            if(result.matched) {
                return result;
            }
        }
        return { std::move(end) };
    }


    /**
     * @brief Finds the match with the earliest starting position in the given input container.
     *
     * @param input The container to search for a match.
     * @return The first match found in the input container, or a result indicating that no match was found.
     */
    template <typename T>
    constexpr auto matchFirst(const T& input) const {
        using std::begin;
        using std::end;
        return matchFirst(begin(input), end(input));
    }


    /**
     * @brief Finds the match with the latest starting position in the given input range.
     *
     * @param begin An iterator pointing to the beginning of the input range.
     * @param end A bidirectional iterator pointing to the end of the input range.
     * @return The last match found in the input range, or a result indicating that no match was found.
     */
    template <typename Iterator>
    constexpr Result<Iterator> matchLast(Iterator begin, Iterator end) const {
        auto current = end;
        do {
            current--;
            Result<Iterator> result = match(current, end);
            if(result.matched) {
                return result;
            }
        } while(current != begin);
        return { std::move(end) };
    }


    /**
     * @brief Finds the match with the latest starting position in the given input container.
     *
     * @param input The container to search for a match.
     * @return The last match found in the input container, or a result indicating that no match was found.
     */
    template <typename T>
    constexpr auto matchLast(const T& input) const {
        using std::begin;
        using std::end;
        return matchLast(begin(input), end(input));
    }


    /**
     * @brief Finds all matches in the given input range.
     *
     * For every starting position, it tries to find a match.
     * Consequently, matches with nearby starting positions and of
     * sufficient length may have partially overlapping results.
     * The returned vector only contains matches found. Every result
     * in the vector has `matched == true`. If no matches are found,
     * the returned vector is empty.
     *
     * @param begin A forward iterator pointing to the beginning of the input range.
     * @param end An iterator pointing to the end of the input range.
     * @return A vector containing all matches found in the input range.
     */
    template <typename Iterator>
    std::vector<Result<Iterator>> matchAll(Iterator begin, Iterator end) const {
        std::vector<Result<Iterator>> results;
        for(; begin != end; begin++) {
            Result<Iterator> result = match(begin, end);
            if(result.matched) {
                results.push_back(result);
            }
        }
        return results;
    }


    /**
     * @brief Finds all matches in the given input container.
     *
     * For every starting position, it tries to find a match.
     * Consequently, matches with nearby starting positions and of
     * sufficient length may have partially overlapping results.
     * The returned vector only contains matches found. Every result
     * in the vector has `matched == true`. If no matches are found,
     * the returned vector is empty.
     *
     * @param input The container to search for matches.
     * @return A vector containing all matches found in the input container.
     */
    template <typename T>
    constexpr auto matchAll(const T& input) const {
        using std::begin;
        using std::end;
        return matchAll(begin(input), end(input));
    }


    /**
     * @brief Finds a match at the given position in the input range.
     *
     * @param begin A forward iterator pointing to the beginning of the input range.
     * @param end An iterator pointing to the end of the input range.
     * @return The match found at the given position, or a result indicating that no match was found.
     */
    template <typename Iterator>
    constexpr Result<Iterator> matchAt(Iterator begin, Iterator end) const {
        return match(begin, end);
    }


    /**
     * @brief Finds a match at the given index in the input container.
     *
     * @param input The container to search for a match.
     * @param index The zero-based index at which to search for a match.
     * @return The match found at the given index, or a result indicating that no match was found.
     */
    template <typename T>
    constexpr auto matchAt(const T& input, size_t index) const {
        using std::begin;
        using std::end;
        using std::next;
        return matchAt(next(begin(input), index), end(input));
    }


    /**
     * @brief Determines whether the matcher can match an empty sequence.
     *
     * @return `true` if the matcher can match an empty sequence, otherwise `false`.
     */
    constexpr bool canHaveEmptyMatches() const {
        if constexpr (Capacity == 0) {
            return true;
        } else {
            std::array<bool, Capacity + 1> currentStates = { false };
            followEmptyTransitions(0, currentStates);
            return currentStates[size];
        }
    }


    /*** BEGIN - NOT IN SINGLE HEADER ***/
    void printDebugInfo() const {
        std::cout << "digraph NFA {\n";
        std::cout << "    rankdir=\"LR\";\n    node [shape=box, style=rounded];\n";
        for(StateIndex i = 0; i < size; i++) {
            State state = states[i];
            if(state.literalTarget != NO_STATE) {
                std::cout << "    " << i << " -> " << state.literalTarget << " [label=\"'" << state.literal << "'\"];\n";
            }
            for(EpsilonSize j = 0; j < state.epsilonSize; j++) {
                std::cout << "    " << i << " -> " << state.epsilon[j] << " [arrowhead=\"onormal\"];\n";
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
    /*** END - NOT IN SINGLE HEADER ***/


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
        Result<Iterator> result = { begin, currentStates[size] };
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
        if(nextStates[index]) {
            return;
        }
        nextStates[index] = true;
        if(index == size) {
            return;
        }
        State state = states[index];
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
