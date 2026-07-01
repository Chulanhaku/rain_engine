#include <rain/runtime/world.hpp>
#include<rain/runtime/tag_component.hpp>

namespace rain{
    entity_id world::create_entity(){
        return create_entity(world_entity_desc{});
    }

    entity_id world::create_entity(const world_entity_desc& desc) {
        u32 index = invalid_u32;
        if (!free_indices_.empty()) {
            index = free_indices_.back();
            free_indices_.pop_back();

            entity_record& record = records_[index];
            record.alive = true;

            metadatas_[index] = world_entity_meta{
                .name = desc.name,
                .active = desc.active
            };
        }
        else {
            index = static_cast<u32>(records_.size());

            records_.push_back(entity_record{
                .generation = 0,
                .alive = true
            });

            metadatas_.push_back(world_entity_meta{
                .name = desc.name,
                .active = desc.active
            });
        }

        ++living_count_;

        return entity_id{
            .index = index,
            .generation = records_[index].generation
        };
    }


    bool world::destroy_entity(entity_id entity){
        if(!is_alive(entity))return false;

        entity_record& record = records_[entity.index];

        record.alive = false;

        ++record.generation;

        metadatas_[entity.index] = world_entity_meta{};

        components_.remove_all(entity);
        free_indices_.push_back(entity.index);
        --living_count_;
        return true;
    }

    bool world::is_alive(entity_id entity) const
    {
        if (!entity.is_valid() || entity.index >= records_.size())
        {
            return false;
        }

        const entity_record& record = records_[entity.index];

        return record.alive && record.generation == entity.generation;
    }

    bool world::is_entity_active(entity_id entity) const
    {
        if (!is_alive(entity))
        {
            return false;
        }

        return metadatas_[entity.index].active;
    }

    void world::set_entity_active(entity_id entity, bool active) {
        if (!is_alive(entity))return;

        metadatas_[entity.index].active = active;
    }

    void world::set_entity_name(entity_id entity, string_id name) {
        if (!is_alive(entity))return;

        metadatas_[entity.index].name = name;
    }


    string_id world::entity_name(entity_id entity)const {
        if (!is_alive(entity))return string_id{};

        return metadatas_[entity.index].name;
    }

    entity_id world::find_entity_by_name(string_id name)const {
        for (u32 index = 0; index < static_cast<u32>(records_.size()); ++index) {
            const entity_record& record = records_[index];

            if (!record.alive)continue;

            const world_entity_meta& meta = metadatas_[index];

            if (!meta.active)continue;

            if (meta.name == name) {
                return entity_id{
                    .index = index,
                    .generation = record.generation
                };
            }
        }
        return entity_id{};
    }

    u32 world::living_entity_count()const{
        return living_count_;
    }

    void world::add_tag(entity_id entity, tag_id tag) {
        if(!is_alive(entity)||!tag.is_valid())return ;
        tag_component* component =try_get_component<tag_component>(entity);

        if(component==nullptr)component = &add_component<tag_component>(entity);

        component->tags.add(tag);
    }



    bool world::remove_tag(entity_id entity, tag_id tag) {
        if (!is_alive(entity) || !tag.is_valid())return false;
        tag_component* component = try_get_component<tag_component>(entity);
        if (component == nullptr)return false;
        return component->tags.remove(tag);
    }

    bool world::has_tag(entity_id entity, tag_id tag)const {
        if (!is_alive(entity) || !tag.is_valid())return false;

        const tag_component* component = try_get_component<tag_component>(entity);

        if (component == nullptr)return false;

        return component->tags.has(tag);

    }

    u32 world::tag_count(entity_id entity, tag_id tag)const {
        if (!is_alive(entity) || !tag.is_valid())return 0;

        const tag_component* component = try_get_component<tag_component>(entity);

        if (component == nullptr)return 0;

        return component->tags.count(tag);
    }

    bool world::matches_tags(entity_id entity, const tag_query& query)const {
        if (!is_alive(entity))return false;

        const tag_component* component = try_get_component<tag_component>(entity);

        if (component == nullptr)return query.matches(tag_container{});

        return query.matches(component->tags);
    }

    tag_container* world::try_get_tags(entity_id entity) {
        if (!is_alive(entity))return nullptr;

        tag_component* component = try_get_component<tag_component>(entity);

        if (component == nullptr)return nullptr;

        return &component->tags;
    }

    const tag_container* world::try_get_tags(entity_id entity)const {
        if (!is_alive(entity))return nullptr;

        const tag_component* component = try_get_component<tag_component>(entity);

        if (component == nullptr)return nullptr;

        return &component->tags;
    }


    bool world::has_component(entity_id entity, type_id component_type_id)const {
        if (!is_alive(entity))return false;

        return components_.has(entity, component_type_id);
    }

    entity_query_result world::query_entities(const entity_query_desc& desc)const {
        entity_query_result result;

        const component_pool_base* primary_pool = nullptr;

        if (!desc.required_components.empty()) {
            primary_pool = components_.try_get_pool(desc.required_components.front());
            if (primary_pool == nullptr)return result;
        }

        const auto matches_entity = [&](entity_id entity)->bool {
            if (desc.require_alive && !is_alive(entity))return false;

            if (desc.require_active && !is_entity_active(entity))return false;

            for (type_id component_type_id : desc.required_components) {
                if (!has_component(entity, component_type_id))return false;
            }

            if (!desc.required_tags.all_tags().empty() ||
                !desc.required_tags.any_tags().empty() ||
                !desc.required_tags.none_tags().empty()) {
                if (!matches_tags(entity, desc.required_tags))return false;
            }
            return true;

        };

        if (primary_pool != nullptr) {
            for(usize i =0;i<primary_pool->size();++i){
                const entity_id entity = primary_pool->entity_at(i);

                if (matches_entity(entity)) {
                    result.entities.push_back(entity);
                }

            }

            return result;
        }

        for (u32 index = 0; index < static_cast<u32>(records_.size()); ++index) {
            const entity_record& record = records_[index];

            if (!record.alive)continue;

            const entity_id entity{
                .index = index,
                .generation = record.generation
            };

            if (matches_entity(entity))result.entities.push_back(entity);
        }

        return result;
    }
}
  