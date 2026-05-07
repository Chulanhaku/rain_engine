#include <rain/runtime/world.hpp>

namespace rain{
    entity_id world::create_entity(){
        u32 index = invalid_u32;
        if(!free_indices_.empty()){
            index = free_indices_.bakc();
            free_indices_.pop_back();
        }
        else{
            index = static_cast<u32>(generations_.size());
            generations_.push_back(0);
        }

        ++living_count_;

        return entity_id{
            .index = index,
            .generation = generations_[index]
        };
    }

    bool world::destory_entity(entity_id entity){
        if(!is_alive(entity))return false;
        components_.remove_all(entity);
        ++generations_[entity.index];
        free_indices_.push_back(entity.index);
        --living_count_;
        return true;
    }

    bool world::is_alive(entity_id entity)const{
        return entity.index<generations_.size()&&generations_[entity.index]==entity.generation;
    }

    u32 world::living_entity_count()const{
        return living_count_;
    }

}