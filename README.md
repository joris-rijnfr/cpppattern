# CPPPattern

CPPPattern is a single-header C++17 library for defining and searching for regex-like patterns in sequences of arbitrary types. It allows you to describe a pattern and then find sequences in a container that match that pattern.

The library is not limited to text or characters. It works with an arbitrary type `V`. So you can use it to search for text in a string, but also to find a specific sequence of values in an array, detect sequences of events in an event log, or something completely else.

The matcher is implemented as an [NFA](https://en.wikipedia.org/wiki/Nondeterministic_finite_automaton). This means that matching does not require backtracking. Searching for matches takes linear time with respect to the input length, avoiding explosive execution times that can occur with backtracking-based pattern matching.

Pattern creation and most matching functions are implemented as `constexpr`. This allows the compiler to construct the internal NFA at compile-time for constant patterns, resulting in zero runtime overhead for pattern creation. If the input is also constant, the compiler can even evaluate the complete matching operation at compile-time.



### Example

The pattern `(Hello|Goodbye) Worlds?` (regex notation) matched against a string:

```cpp
// A pattern's element type is deduced from its arguments.
// In this case, the arguments have char as (inner) type.
// (string literals are just char arrays)
constexpr auto matcher = Pattern::concat(
    Pattern::anyLongest(
        "Hello",
        "Goodbye"
    ),
    " World",
    Pattern::optional("s")
);

// Find all matches in the input.
auto results = matcher.matchAll("Goodbye Worlds and Hello World!");

// Show the results.
for(const auto& result : results) {
    // result.matchStart is an iterator to the start of the match.
    std::string matchText(result.matchStart, result.matchStart + result.length);
    std::cout << "Match of length " << result.length << ": " << matchText << "\n";
}
// Output:
// Match of length 14: Goodbye Worlds
// Match of length 11: Hello World
```


The same way of building patterns works with other types without any change:

```cpp
// In this case, the arguments have bool as (inner) type.
// Described as a sequence: [false, true exactly 3 times, true zero or more times, false].
constexpr auto matcher = Pattern::concat(
    false,                                  // A single value.
    std::array{true, true, true},           // An array of values (treated as a sequence).
    Pattern::repeatZeroOrMoreTimes(true),   // A pattern.
    false
);

static constexpr std::array input = {
    false, true, true, false, true, true, true, true, false, true, true
};

// Find the first match of the pattern in this input.
constexpr auto result = matcher.matchFirst(input);
// ^ constexpr, so result is actually computed at compile-time.

// Show the result.
if(result.matched) {
    std::cout << "Match of length " << result.length << "\n";
} else {
    std::cout << "No match found\n";
}
// Output:
// Match of length 6
```


### Sections

- [Creating patterns](#creating-patterns)
  - [Longest-match semantics](#longest-match-semantics)
  - [Function reference](#function-reference)
    - [`literal`](#literal)
    - [`concat`](#concat)
    - [`anyLongest`](#anylongest)
    - [`optional`](#optional)
    - [`repeatZeroOrMoreTimes`](#repeatzeroormoretimes)
    - [`repeatOneOrMoreTimes`](#repeatoneormoretimes)
    - [`repeatNTimes`](#repeatntimes)
- [Finding matches](#finding-matches)
  - [Function reference](#function-reference-1)
    - [`matchFirst`](#matchfirst)
    - [`matchLast`](#matchlast)
    - [`matchAll`](#matchall)
    - [`matchAt`](#matchat)
    - [`canHaveEmptyMatches`](#canhaveemptymatches)
  - [Result](#result)



## Creating patterns

All pattern functions are static member functions of the `Pattern` class. They return a `Pattern::Matcher<V, N>` object, which you rarely need to spell out. Use `auto` and the type is deduced from your arguments.

Every function takes "pattern-like" arguments, and each argument can take one of three forms:
- a **matcher** (the result of another pattern function): used as a pattern
- an **array** (`std::array` or a C-style array): its elements are concatenated into a sequence and used as a pattern
- a **single value**: interpreted as a one-element pattern.

The exception is `Pattern::literal`, which takes a single value and always treats it as a literal element. A matcher or array is not interpreted as pattern.



### Longest-match semantics

A matcher does not stop as soon as it has found some valid match. It keeps consuming elements as long as a longer match is still possible, and only returns the longest match it found once no longer match can be completed. As a consequence:
- `Pattern::concat(A, Pattern::optional(B))` find the match that includes B whenever B can match.
- `repeatZeroOrMoreTimes` is "greedy": it consumes as many repetitions as possible.
- With `anyLongest`, the longest alternative wins regardless of argument order.



### Function reference

#### `literal`

```cpp
constexpr auto Pattern::literal(value)
```

Creates a matcher that matches any single element that is equal to `value`.

Unlike the other pattern functions, `literal` does not interpret a matcher or array argument as a pattern. Its argument is always treated as a literal value. For example, if an array or matcher is passed to `literal`, the matcher will look for elements that are themselves equal to that array or matcher.

For example, `Pattern::literal(42)` matches only the sequence `[42]`.


#### `concat`

```cpp
constexpr auto Pattern::concat(param1, param2, param3, ...)
```

Creates a matcher that matches all arguments in sequence.

Each argument must match immediately after the previous argument. A successful match consists of the concatenation of the individual matches, in the same order as the arguments.

For example, `Pattern::concat(1, 2, 3)` matches only the sequence `[1, 2, 3]`.


#### `anyLongest`

```cpp
constexpr auto Pattern::anyLongest(param1, param2, param3, ...)
```

Creates a matcher that matches any of the arguments.

The longest-match semantics of the matcher ensure that the longest match is found. The order of the arguments do not matter. It has no influence on the longest match between any of the arguments.

For example, `Pattern::anyLongest(1, std::array{1, 2})` matches only the sequence `[1]` or `[1, 2]`. Preferably `[1, 2]` if possible because it is the longest match.


#### `optional`

```cpp
constexpr auto Pattern::optional(param)
```

Creates a matcher that optionally matches the argument.

A successful match can consist of either the match of the argument or an empty sequence (a match of length 0). The longest-match semantics of the matcher garuantee that it will match the argument if possible.

For example, `Pattern::optional(42)` matches only the sequence `[]` or `[42]`. Preferably `[42]` if possible because it is the longest match.


#### `repeatZeroOrMoreTimes`

```cpp
constexpr auto Pattern::repeatZeroOrMoreTimes(param)
```

Creates a matcher that matches the argument zero or more times.

The matcher returns the longest valid match, consisting of as many consecutive repetitions as possible. It can also match an empty sequence if the argument cannot be matched at all.

For example, `Pattern::repeatZeroOrMoreTimes(42)` matches `[]`, `[42]`, `[42, 42]`, `[42, 42, 42]`, etc.


#### `repeatOneOrMoreTimes`

```cpp
constexpr auto Pattern::repeatOneOrMoreTimes(param)
```

Creates a matcher that matches the argument one or more times.

The matcher returns the longest valid match, consisting of as many consecutive repetitions as possible. The argument must be matched at least once.

For example, `Pattern::repeatOneOrMoreTimes(42)` matches `[42]`, `[42, 42]`, `[42, 42, 42]`, `[42, 42, 42, 42]` etc.


#### `repeatNTimes`

```cpp
template <size_t n>
constexpr auto Pattern::repeatNTimes<n>(param)
```

Creates a matcher that matches the argument exactly `n` times.

A successful match contains all `n` repetitions, with each repetition immediately following the previous one.

For example, `Pattern::repeatNTimes<3>(42)` matches only the sequence `[42, 42, 42]`.



## Finding matches

The Matcher API consists of the member functions available on the matcher objects returned by the Pattern API.

Once you have defined a pattern and created a matcher, you can use these functions to search an input for sequences that match the pattern.


Every pattern function returns a `Pattern::Matcher` object (type `Pattern::Matcher<V, N>`, where `V` is the element type and `N` the number of internal states). This object holds the fully built pattern and provides the search functions below.

Almost all of them are `constexpr`. This means that for constant patterns, the compiler builds the internal NFA at compile time, so pattern construction costs nothing at runtime. When the input is a constant expression as well, the compiler can evaluate the whole match at compile time (see the second example above). The only exception is `matchAll`, which allocates a `std::vector` and therefore is not `constexpr`.

Each search function comes in two overloads:
- a container overload: pass any container (`std::string`, `std::vector`, std::array, ...) that supports `begin()`/`end()`
- an iterator overload: pass a begin iterator and an end iterator of the range to search. Use this to search only part of a container, or a range that is not itself a container.



### Function reference

#### `matchFirst`

```cpp
constexpr Result<Iterator> matchFirst(const T& input)
constexpr Result<Iterator> matchFirst(Iterator begin, Iterator end)
```

Finds the match with the earliest starting position in the given input.

The matcher searches the input from beginning and returns the longest match at the earliest starting position where a match is possible.

If no match is found, the returned result has `matched == false`.


#### `matchLast`

```cpp
constexpr Result<Iterator> matchLast(const T& input)
constexpr Result<Iterator> matchLast(Iterator begin, Iterator end)
```

Finds the match with the latest starting position in the given input.

The matcher searches for the latest starting position at which a match is possible and returns the longest match at that position. The input must support bidirectional iterators.

If no match is found, the returned result has `matched == false`.


#### `matchAll`

```cpp
std::vector<Result<Iterator>> matchAll(const T& input)
std::vector<Result<Iterator>> matchAll(Iterator begin, Iterator end)
```

Finds all matches in the given input.

For every starting position, it tries to find a match. Consequently, matches with nearby starting positions and of sufficient length may have partially overlapping results. The returned vector only contains matches found. Every result in the vector has `matched == true`.

If no matches are found, the returned vector is empty.


#### `matchAt`

```cpp
constexpr Result<Iterator> matchAt(const T& input, size_t index)
constexpr Result<Iterator> matchAt(Iterator begin, Iterator end)
```

Finds a match at the given position in the input.

The iterator overload treats `begin` as the starting position and searches up to `end`. The container overload uses `index` as the starting position within the input.

If a match is possible at that position, the longest valid match is returned. Otherwise, the returned result has `matched == false`.


#### `canHaveEmptyMatches`

```cpp
constexpr bool canHaveEmptyMatches()
```

Determines whether the matcher can match an empty sequence.

Returns `true` if the matcher can produce a successful match with a length of 0, and `false` otherwise.



### Result

All matching functions that return a single match return a `Pattern::Result<Iterator>` object. The `matchAll` function returns `std::vector<Pattern::Result<Iterator>>`.

The result is a small struct containing the following members:

```cpp
template <typename Iterator>
struct Result {
    Iterator matchStart;  // An iterator pointing to the first element of the match in the input.
    bool matched;         // Indicates whether a match was found.
    size_t length;        // The number of elements in the match.
};
```

When no match is found, `matched` is false and `matchStart` points to the end of the input.

> [!TIP]
> `matchStart` is an iterator so it works with any container. However, you may want the match start as an index instead. You can compute the index of the first element by doing the following:
> ```cpp
> size_t index = std::distance(std::begin(input), result.matchStart);
> ```
> The complexity is constant for random access iterators like arrays or vectors. However, for containers like linked lists, this takes linear time and an index might not even be as useful.
