#pragma once

#include <functional>

#include<rain/core/types.hpp>

namespace rain{
    struct type_id{
        u32 value = invalid_u32;

        [[nodiscard]]bool is_valid()const{
            return value!= invalid_u32;
        }

        friend bool operator==(type_id lhs,type_id rhs){
            return lhs.value==rhs.value;
        }

        friend bool operator!=(type_id lhs,type_id rhs){
            return !(lhs==rhs);
        }

        friend bool operator<(type_id lhs,type_id rhs){
            return lhs.value<rhs.value;
        }
    };

    class type_id_generator{
    public:
        static type_id next(){
            static u32 next_value = 0;

            type_id result{
                .value = next_value
            };

            ++next_value;

            return result;
        }
    };

    template <typename value_type>
    [[nodiscard]] type_id get_type_id(){
        static const type_id id = type_id_generator::next();
        return id;
    }
}

namespace std{
    template<>
    struct hash<rain::type_id>{
        std::size_t operator()(rain::type_id id)const noexcept{
            return static_cast<std::size_t>(id.value);
        }
    };
}