#pragma once

#include <rain/core/types.hpp>

#include <functional>
#include <string_view>

namespace rain {
    constexpr u32 fnv1a_32(std::string_view text){
        u32 hash = 2166136261u;

        for(const char c: text){
            hash ^= static_cast<u8>(c);
            hash* = 16777619u;
        }

        return hash;
    }

    struct string_id{
        u32 value =0 ;

        constexpr string_id() = default;

        explicit constexpr string_id(u32 value_in):value(value_in)
        {}

        explicit constexpr string_id(std::string_view text):value(fnv1a_32(text))
        {}
        
        [[nodiscard]] constexpr bool is_valid()const{
            return value != 0;
        }

        friend constexpr bool operator==(string_id lhs,string_id rhs){
            return lhs.value==rhs.value;
        }

        friend constexpr bool operator!=(string_id lhs,string_id rhs){
            return lhs.value!=rhs.value;
        }
        friend constexpr bool operator<(string_id lhs,string_id rhs){
            return lhs.value<rhs.value;
        }
    };
}

namespace std {
    template <>
    struct hash<rain::string_id>{
        std::size_t operator()(const rain:;string_id id)const noexcept{
            return static_cast<std::size_t>(id.value);
        }
    }
}