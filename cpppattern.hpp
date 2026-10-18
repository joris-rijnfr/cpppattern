#pragma once

#include <string_view>



class Pattern {
public:
    struct Result {
        bool matched = false;
        size_t index = 0;
        size_t length = 0;
    };


    template <typename F1, typename... F2>
    static constexpr auto make(const F1& pattern, const F2&... patternRest) {
        return [pattern, patternRest...](std::string_view text) {
            Pattern::Context ctx { text };
            for(ctx.result.index = 0; ctx.result.index < text.length(); ctx.result.index++) {
                if constexpr (sizeof...(F2) == 0) {
                    if(pattern(ctx)) {
                        ctx.result.matched = true;
                        return ctx.result;
                    }
                } else {
                    if(all(pattern, patternRest...)(ctx)) {
                        ctx.result.matched = true;
                        return ctx.result;
                    }
                }
            }
            ctx.result.index = 0;
            ctx.result.length = 0;
            return ctx.result;
        };
    };


    static constexpr auto literal(std::string_view pattern) {
        return [pattern](Context& ctx) {
            size_t index = ctx.result.index + ctx.offset;
            for(size_t i = 0; i < pattern.length(); i++) {
                if(index + i >= ctx.text.length() || ctx.text[index + i] != pattern[i]) {
                    return false;
                }
            }
            ctx.result.length += pattern.length();
            ctx.offset += pattern.length();
            return true;
        };
    }


    static constexpr auto literal(char pattern) {
        return [pattern](Context& ctx) {
            size_t index = ctx.result.index + ctx.offset;
            if(index < ctx.text.length() && ctx.text[index] == pattern) {
                ctx.result.length++;
                ctx.offset++;
                return true;
            }
            return false;
        };
    }


    template <typename F1, typename... F2>
    static constexpr auto any(const F1& pattern, const F2&... patternRest) {
        return [pattern, patternRest...](Context& ctx) {
            if constexpr (sizeof...(F2) == 0) {
                return pattern(ctx);
            } else {
                return pattern(ctx) || any(patternRest...)(ctx);
            }
        };
    }


    template <typename F1, typename... F2>
    static constexpr auto all(const F1& pattern, const F2&... patternRest) {
        return [pattern, patternRest...](Context& ctx) {
            if constexpr (sizeof...(F2) == 0) {
                return pattern(ctx);
            } else {
                return pattern(ctx) && all(patternRest...)(ctx);
            }
        };
    }


private:
    struct Context {
        std::string_view text;
        size_t offset = 0;
        Result result = {};
    };
};
