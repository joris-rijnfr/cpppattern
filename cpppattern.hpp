#pragma once

#include <string_view>
#include <cassert>



class Pattern {
public:
    struct Result {
        size_t index = 0;
        size_t length = 0;
        bool matched = false;
    };


    template <typename F1, typename... F2>
    static constexpr auto make(const F1& pattern, const F2&... patternRest) {
        auto matchFunction = nonCapturingGroup(pattern, patternRest...);
        return [matchFunction](std::string_view text) {
            Context ctx { text };
            for(ctx.result.index = 0; ctx.result.index < text.length(); ctx.result.index++) {
                if(matchFunction(ctx)) {
                    ctx.result.matched = true;
                    return ctx.result;
                }
            }
            ctx.result.index = 0;
            ctx.result.length = 0;
            return ctx.result;
        };
    };


    static constexpr auto character(char pattern) {
        return [pattern](Context& ctx) {
            if(currentIs(ctx, pattern)) {
                return success(ctx);
            }
            return false;
        };
    }


    template <size_t I = 0, size_t N>
    static constexpr auto characterSet(const char (&pattern)[N]) {
        static_assert(N - 1 > 0, "Must have at least one character in character set");
        return [pattern](Context& ctx) {
            if(currentIs(ctx, pattern[I], I)) {
                return success(ctx);
            }
            if constexpr (I + 1 < N - 1) {
                return characterSet<I + 1>(pattern)(ctx);
            } else {
                return false;
            }
        };
    }


    template <size_t I = 0, size_t N>
    static constexpr auto negatedSet(const char (&pattern)[N]) {
        static_assert(N - 1 > 0, "Must have at least one character in negated set");
        return [pattern](Context& ctx) {
            if(currentIs(ctx, pattern[I], I)) {
                return false;
            }
            if constexpr (I + 1 < N - 1) {
                return negatedSet<I + 1>(pattern)(ctx);
            } else {
                return success(ctx);
            }
        };
    }


    static constexpr auto range(char low, char high) {
        assert(low <= high);
        return [low, high](Context& ctx) {
            if(currentIsInRange(ctx, low, high)) {
                return success(ctx);
            }
            return false;
        };
    }


    static constexpr auto dot() {
        return [](Context& ctx) {
            if(size_t index = ctx.result.index + ctx.offset; index < ctx.text.length() && ctx.text[index] != '\n' && ctx.text[index] != '\r') {
                return success(ctx);
            }
            return false;
        };
    }


    static constexpr auto word() {
        return [](Context& ctx) {
            if(size_t index = ctx.result.index + ctx.offset; index < ctx.text.length() && isWordChar(ctx.text[index])) {
                return success(ctx);
            }
            return false;
        };
    }


    static constexpr auto notWord() {
        return [](Context& ctx) {
            if(size_t index = ctx.result.index + ctx.offset; index < ctx.text.length() && isWordChar(ctx.text[index])) {
                return false;
            }
            return success(ctx);
        };
    }


    static constexpr auto digit() {
        return [](Context& ctx) {
            if(currentIsInRange(ctx, '0', '9')) {
                return success(ctx);
            }
            return false;
        };
    }


    static constexpr auto notDigit() {
        return [](Context& ctx) {
            if(currentIsInRange(ctx, '0', '9')) {
                return false;
            }
            return success(ctx);
        };
    }


    static constexpr auto whitespace() {
        return [](Context& ctx) {
            if(size_t index = ctx.result.index + ctx.offset; index < ctx.text.length()) {
                switch (ctx.text[index]) {
                    case ' ':
                    case '\t':
                    case '\n':
                    case '\r':
                    case '\f':
                    case '\v':
                        return success(ctx);
                    default:
                        return false;
                }
            }
            return false;
        };
    }


    static constexpr auto notWhitespace() {
        return [](Context& ctx) {
            if(size_t index = ctx.result.index + ctx.offset; index < ctx.text.length()) {
                switch (ctx.text[index]) {
                    case ' ':
                    case '\t':
                    case '\n':
                    case '\r':
                    case '\f':
                    case '\v':
                        return false;
                    default:
                        return success(ctx);
                }
            }
            return false;
        };
    }


    static constexpr auto beginning() {
        return [](const Context& ctx) {
            return ctx.result.index + ctx.offset == 0;
        };
    }


    static constexpr auto end() {
        return [](const Context& ctx) {
            return ctx.result.index + ctx.offset == ctx.text.length();
        };
    }


    static constexpr auto wordBoundary() {
        return [](const Context& ctx) {
            size_t index = ctx.result.index + ctx.offset;
            if(index < ctx.text.length()) {
                if(index == 0) {
                    return isWordChar(ctx.text[index]);
                }
                return isWordChar(ctx.text[index - 1]) != isWordChar(ctx.text[index]);
            }
            return index > 0 && isWordChar(ctx.text[index - 1]);
        };
    }


    static constexpr auto notWordBoundary() {
        return [](const Context& ctx) {
            size_t index = ctx.result.index + ctx.offset;
            if(index < ctx.text.length()) {
                if(index == 0) {
                    return !isWordChar(ctx.text[index]);
                }
                return isWordChar(ctx.text[index - 1]) == isWordChar(ctx.text[index]);
            }
            return index == 0 || !isWordChar(ctx.text[index - 1]);
        };
    }


    template <typename F1, typename... F2>
    static constexpr auto nonCapturingGroup(const F1& pattern, const F2&... patternRest) {
        if constexpr (sizeof...(F2) == 0) {
            return pattern;
        } else {
            return [pattern, combined = nonCapturingGroup(patternRest...)](Context& ctx) {
                size_t offset = ctx.offset;
                size_t resultLength = ctx.result.length;
                if(pattern(ctx) && combined(ctx)) {
                    return true;
                }
                ctx.offset = offset;
                ctx.result.length = resultLength;
                return false;
            };
        }
    }


    template <size_t I = 0, size_t N>
    static constexpr auto nonCapturingGroup(const char (&pattern)[N]) {
        static_assert(N - 1 > 0, "Must have at least one character in non-capturing group");
        return [pattern](Context& ctx) {
            if(currentIs(ctx, pattern[I], I)) {
                if constexpr (I + 1 < N - 1) {
                    return nonCapturingGroup<I + 1>(pattern)(ctx);
                } else {
                    return success(ctx, N - 1);
                }
            }
            return false;
        };
    }


    template<typename F>
    static constexpr auto plus(const F& pattern) {
        return [pattern](Context& ctx) {
            if(!pattern(ctx)) {
                return false;
            }
            while(pattern(ctx)) { /* greedy match */ }
            return true;
        };
    }


    template<typename F>
    static constexpr auto star(const F& pattern) {
        return [pattern](Context& ctx) {
            while(pattern(ctx)) { /* greedy match */ }
            return true;
        };
    }


    template<typename F>
    static constexpr auto quantifier(size_t minimum, size_t maximum, const F& pattern) {
        assert(minimum <= maximum);
        return [minimum, maximum, pattern](Context& ctx) {
            size_t offset = ctx.offset;
            size_t resultLength = ctx.result.length;
            for(size_t i = 0; i < minimum; i++) {
                if(!pattern(ctx)) {
                    ctx.offset = offset;
                    ctx.result.length = resultLength;
                    return false;
                }
            }
            for(size_t i = 0; i < maximum - minimum; i++) {
                if(!pattern(ctx)) {
                    return true;
                }
            }
            return true;
        };
    }


    template<typename F>
    static constexpr auto quantifier(size_t quantity, const F& pattern) {
        return [quantity, pattern](Context& ctx) {
            size_t offset = ctx.offset;
            size_t resultLength = ctx.result.length;
            for(size_t i = 0; i < quantity; i++) {
                if(!pattern(ctx)) {
                    ctx.offset = offset;
                    ctx.result.length = resultLength;
                    return false;
                }
            }
            return true;
        };
    }


    template<typename F>
    static constexpr auto optional(const F& pattern) {
        return [pattern](Context& ctx) {
            pattern(ctx);
            return true;
        };
    }


    template <typename F1, typename... F2>
    static constexpr auto alteration(const F1& pattern, const F2&... patternRest) {
        if constexpr (sizeof...(F2) == 0) {
            return pattern;
        } else {
            return [pattern, combined = alteration(patternRest...)](Context& ctx) {
                return pattern(ctx) || combined(ctx);
            };
        }
    }


private:
    struct Context {
        std::string_view text;
        size_t offset = 0;
        Result result = {};
    };


    static constexpr bool success(Context& ctx, size_t matchLength = 1) {
        ctx.offset += matchLength;
        ctx.result.length += matchLength;
        return true;
    }
    
    
    static constexpr bool isInRange(char c, char low, char high) {
        return (low <= c && c <= high);
    }


    static constexpr bool currentIsInRange(const Context& ctx, char low, char high) {
        size_t index = ctx.result.index + ctx.offset;
        return index < ctx.text.length() && isInRange(ctx.text[index], low, high);
    }


    static constexpr bool isWordChar(char c) {
        return isInRange(c, 'A', 'Z') || isInRange(c, 'a', 'z') || isInRange(c, '0', '9') || c == '_';
    }


    static constexpr bool currentIs(const Context& ctx, char c, size_t offset = 0) {
        size_t index = ctx.result.index + ctx.offset + offset;
        return index < ctx.text.length() && ctx.text[index] == c;
    }
};
