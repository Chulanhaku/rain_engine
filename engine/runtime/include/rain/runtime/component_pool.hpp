#pragma once

#include<rain/core/container/rain_sparse_set.hpp>
#include<rain/runtime/entity.hpp>
#include<rain/core/types.hpp>

#include<utility>

namespace rain{
    template<typename component_type>
    class component_pool{
    public:
        template <typename... args_type>
        component_type& add(entity_id entity,args_type&&... args){
            return components_.emplace(entity,std::forward<args_type>(args)...);
        }

        bool remove(entity_id entity){
            return components_.remove(entity);
        }

        bool has(entity_id entity)const{
            return components_.contains(entity);
        }

        component_type& get(entity_id){
            return components_.get(entity);
        }

        const component_type& get(entity_id)const{
            return components_.get(entity);
        }

        [[nodiscard]] auto& entities(){
            return components_.ids();
        }

        [[nodiscard]]const auto& entities()const{
            return components_.ids();
        }

        [[nodiscard]] auto& values(){
            return components_.values();
        }

        [[nodiscard]]const auto& values()const{
            return components_.values();
        }

        [[nodiscard]] usize size()const{
            return components_.size();
        }

        [[nodiscard]]bool empty()const{
            return components_.size()==0;
        }

    private:
        rain_sparse_set<entity_id,component_type> components_;
    };


}


