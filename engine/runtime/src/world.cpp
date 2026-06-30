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

        if (component == nullptr)return false;

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
}
