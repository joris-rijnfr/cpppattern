# CPPPattern

CPPPattern is a single-header C++ library for defining and searching for regex-like patterns in sequences of arbitrary types. It allows you to describe a pattern and then find sequences in a container that match that pattern.

The library is not limited to text or characters. It works with an arbitrary type `V`. So you can use it to search for text in a string, but also to find a specific sequence of values in an array, detect sequences of events in an event log, or something completely else.

The matcher is implemented as an [NFA](https://en.wikipedia.org/wiki/Nondeterministic_finite_automaton). This means that matching does not require backtracking. Searching for matches takes linear time with respect to the input length, avoiding explosive execution times that can occur with backtracking-based pattern matching.

Pattern creation and most matching functions are implemented as `constexpr`. This allows the compiler to construct the internal NFA at compile-time for constant patterns, resulting in zero runtime overhead for pattern creation. If the input is also constant, the compiler can even evaluate the complete matching operation at compile-time.


### Sections

- [Examples](#examples)
- [Pattern API](#pattern-api)
  - [Functions](#functions)
    - [`literal`](#literal)
    - [`concat`](#concat)
    - [`anyLongest`](#anylongest)
    - [`optional`](#optional)
    - [`repeatZeroOrMoreTimes`](#repeatzeroormoretimes)
    - [`repeatNTimes`](#repeatntimes)
- [Matcher API](#matcher-api)
  - [Functions](#functions-1)
    - [`matchFirst`](#matchfirst)
    - [`matchLast`](#matchlast)
    - [`matchAll`](#matchall)
    - [`matchAt`](#matchat)
    - [`canHaveEmptyMatches`](#canhaveemptymatches)
  - [Result](#result)



## Examples

Finding all matches of a pattern in a string.

```cpp
// Type of a pattern is deduced from the type of the given parameters.
// In this case, the given parameters have char as (inner) type.
// This pattern can be described as regex "(Hello|Goodbye) Worlds?".
constexpr auto matcher = Pattern::concat(
    Pattern::anyLongest(
        "Hello "
        "Goodbye "
    ),
    "World",
    Pattern::optional('s')
);

// Find all matches in the input.
auto results = matcher.matchAll("Goodbye Worlds and Hello World!");


// Show results.
for(const auto& result : results) {
    // result.matchStart is an iterator at the start of the match in the input.
    std::string matchText(result.matchStart, result.matchStart + result.length);
    std::cout << "Match of length " << result.length << ": " << matchText << "\n";
}
// Output:
// Match of length 14: Goodbye Worlds
// Match of length 11: Hello World
```


Matching a specific sequence of booleans in an array.

```cpp
// In this case, the given parameters have bool as (inner) type.
// This pattern can be described as [false, true at least 3 times, false].
constexpr auto matcher = Pattern::concat(
    false,                                  // Parameter can just be a single element.
    std::array{true, true, true},           // Parameter can be an array of elements.
    Pattern::repeatZeroOrMoreTimes(true),   // Parameter can be a pattern.
    false                                   // (This is just a single element again).
);

static constexpr std::array input = {
    false, true, true, false, true, true, true, true, false, true, true
};
// Find the first match of the pattern in this input.
constexpr auto result = matcher.matchFirst(input);
// ^ constexpr, result is actually computed at compile-time.


// Show result.
if(result.matched) {
    auto iterator = result.matchStart;
    std::string matchText;
    for(size_t i = 0; i < result.length; i++) {
        matchText += (*iterator ? " true" : " false");
        iterator++;
    }
    std::cout << "Match of length " << result.length << ":" << matchText << "\n";
} else {
    std::cout << "No match found\n";
}
// Output:
// Match of length 6: false true true true true false
```



## Pattern API

The Pattern API provides a collection of static functions for defining patterns and creating matchers.

Each pattern function returns a matcher representing the specified pattern. You can combine patterns by passing the matchers returned by these functions as parameters to other pattern functions. This allows you to build complex patterns from smaller, reusable ones.

The available pattern functions are described in [Functions](#functions).


### Functions

Each pattern function takes one or more parameters. The parameters describe the patterns that the function should operate on.

For any Pattern API function except `literal`, a parameter can be any of the following:

- **A matcher that matches on elements of type `V`**

  The function applies its operation to the given matcher and returns a new matcher. The original matcher remains unchanged.

  Example: `Pattern::optional(Pattern::literal(42))`, the matcher parameter is `Pattern::literal(42)`.

- **An array with elements of type `V`**

  An array is interpreted as a concatenation of its elements. This is equivalent to `Pattern::concat(arr[0], arr[1], arr[2], ..., arr[N-1])`, where `N` is the length of the array.

  An array can be given as either `std::array<V, N>` or `V[N]`.

  Example: `Pattern::optional(std::array{1, 2, 3})`, the array parameter is `std::array{1, 2, 3}`.

- **A value of type `V`**

  Any value that is neither a matcher nor an array is interpreted as a literal pattern. This is equivalent to `Pattern::literal(value)`.

  Example: `Pattern::optional(42)`, the value parameter is `42`.

For pattern functions that accept multiple parameters, each parameter can be a different type. You can combine matchers, arrays, and individual values in any order. For example, the first two parameters can be matchers, followed by an array and then an individual value.

All matchers use longest-match semantics. When searching for a match at a particular starting position, the matcher continues exploring possible matches even after finding a valid one. It returns the longest valid match found at that position.


#### `literal`

```cpp
constexpr auto Pattern::literal(const V value)
```

Returns a matcher that matches any single element that is equal to `value`.

This is the most basic type of pattern. Every successful match has a length of exactly 1.

Unlike the other pattern functions, `literal` does not interpret a given matcher or array parameter as pattern. Its parameter is always treated as a literal value. For example, if an array or matcher is passed to `literal`, the matcher will look for elements that are themselves equal to that array or matcher.


#### `concat`

```cpp
constexpr auto Pattern::concat(param1, param2, param3, ...)
```

Returns a matcher that matches all given parameters in sequence.

Each parameter must match immediately after the previous parameter. A successful match consists of the concatenation of the individual matches, in the same order as the parameters.

Each parameter is interpreted as described in [Functions](#functions).

For example, `Pattern::concat(1, 2, 3)` matches the sequence `[1, 2, 3]`.


#### `anyLongest`

```cpp
constexpr auto Pattern::anyLongest(param1, param2, param3, ...)
```

Returns a matcher that matches any one of the given parameters.

If multiple parameters can match at the same starting position, the longest match is returned. The matcher continues looking for longer matches and only finishes when no longer match is possible.

Each parameter is interpreted as described in [Functions](#functions).

For example, `Pattern::anyLongest(1, Pattern::concat(1, 2))` matches `[1, 2]` as a whole rather than stopping after matching `[1]`.


#### `optional`

```cpp
constexpr auto Pattern::optional(param)
```

Returns a matcher that matches the given parameter either zero or one time.

A successful match can consist of either the match of the parameter or an empty sequence (a match of length 0). The longest-match semantics of the matcher garuantee that it will match the parameter if possible.

The parameter is interpreted as described in [Functions](#functions).

For example, `Pattern::optional(42)` preferably matches `[42]` or otherwise `[]`.


#### `repeatZeroOrMoreTimes`

```cpp
constexpr auto Pattern::repeatZeroOrMoreTimes(param)
```

Returns a matcher that matches the given parameter zero or more times in sequence.

The matcher returns the longest valid match, consisting of as many consecutive repetitions as possible. It can also match an empty sequence if the parameter cannot be matched at all.

The parameter is interpreted as described in [Functions](#functions).

For example, `Pattern::repeatZeroOrMoreTimes(42)` matches `[]`, `[42]`, `[42, 42]`, `[42, 42, 42]`, and so on.


#### `repeatNTimes`

```cpp
template <size_t n>
constexpr auto Pattern::repeatNTimes<n>(param)
```

Returns a matcher that matches the given parameter exactly `n` times in sequence.

A successful match contains all `n` repetitions, with each repetition immediately following the previous one.

The parameter is interpreted as described in [Functions](#functions).

For example, `Pattern::repeatNTimes<3>(42)` matches exactly `[42, 42, 42]`.



## Matcher API

The Matcher API consists of the member functions available on the matcher objects returned by the Pattern API.

Once you have defined a pattern and created a matcher, you can use these functions to search an input for sequences that match the pattern.


### Functions

The matching functions accept either in input container (array, vector, list, etc.) or a pair of iterators defining the input range. The iterator overloads allow you to specify exactly which part of a container should be searched.

The available functions are described below.


#### `matchFirst`

```cpp
constexpr Result<Iterator> matchFirst(const T& input)
constexpr Result<Iterator> matchFirst(Iterator begin, Iterator end)
```

Finds the first match of the pattern in the input.

The matcher searches the input from beginning and returns the longest match at the earliest starting position where a match is possible.

If no match is found, the returned result has `matched == false`.


#### `matchLast`

```cpp
constexpr Result<Iterator> matchLast(const T& input)
constexpr Result<Iterator> matchLast(Iterator begin, Iterator end)
```

Finds the last match of the pattern in the input.

The matcher searches for the latest starting position at which a match is possible and returns the longest match at that position.

If no match is found, the returned result has `matched == false`.


#### `matchAll`

```cpp
std::vector<Result<Iterator>> matchAll(const T& input)
std::vector<Result<Iterator>> matchAll(Iterator begin, Iterator end)
```

Finds all matches of the pattern in the input.

The returned vector contains one result for every starting position at which a match is possible. Each result represents the longest match starting at that position. By definition, each result in the vector always has `matched == true`.

Matches are returned in order of their starting positions, from beginning to end of the input. Unlike many conventional search functions, this function also returns overlapping matches.

If no matches are found, the returned vector is empty.


#### `matchAt`

```cpp
constexpr Result<Iterator> matchAt(const T& input, size_t index)
constexpr Result<Iterator> matchAt(Iterator begin, Iterator end)
```

Attempts to match the pattern at a specific starting position.

The iterator overload treats `begin` as the starting position and searches up to `end`. The container overload uses `index` as the starting position within the input.

If a match is possible at that position, the longest valid match is returned. Otherwise, the returned result has `matched == false`.


#### `canHaveEmptyMatches`

```cpp
constexpr bool canHaveEmptyMatches()
```

Returns `true` if the matcher can produce a successful match with a length of 0, and `false` otherwise.

This function can be useful when determining whether a pattern can match an empty sequence.



### Result

All matching functions that return a single match return a `Result<Iterator>` object. The `matchAll` function returns `std::vector<Result<Iterator>>`.

The result is a small struct containing the following members:

```cpp
template <typename Iterator>
struct Result {
    Iterator matchStart;
    bool matched;
    size_t length;
};
```

- **`matchStart`**

  An iterator pointing to the first element of the match in the input.

- **`matched`**

  Indicates whether a match was found. If `true`, the other members describe the match. If `false`, no valid match was found.

- **`length`**

  The number of elements in the match.
