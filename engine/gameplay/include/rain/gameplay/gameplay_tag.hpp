#pragma once

#include <string>
#include <string_view>

#include <rain/core/container/rain_hash_map.hpp>
#include <rain/core/string_id.hpp>

namespace rain{

    struct gameplay_tag{
        string_id id;

        [[nodiscard]] bool is_valid()const{
            return id.is_valid();
        }

        friend bool operator==(gameplay_tag lhs,gameplay_tag rhs){
            return lhs.id==rhs.id;
        }
        
        friend bool operator!=(gameplay_tag lhs,gameplay_tag rhs){
            return !(lhs.id==rhs.id);
        }

        friend bool operator<(gameplay_tag lhs,gameplay_tag rhs){
            return lhs.id <rhs.id;
        }

    };

    class gameplay_tag_registry{
    public:
        gameplay_tag register_tag(std::string_view name){
            const string_id id{name};
            name_map_.insert(id,std::string{name});
            return gameplay_tag{.id=id};
        }


        [[nodiscard]] std::string_view get_name(gameplay_tag tag)const{
            const std::string * name = name_map_.find(tag.id);
            if(name==nullptr)return {};
            return *name;
        }

    private:
        rain_hash_map<string_id,std::string> name_map_;
    };
}

template<>
struct std::hash<rain::gameplay_tag>{
    std::size_t operator()(rain::gameplay_tag tag)const noexcept{
        return static_cast<std::size_t>(tag.id.value);
    }
};