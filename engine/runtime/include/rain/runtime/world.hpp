#pragma once

#include <rain/core/assert.hpp>
#include <rain/runtime/component_registry.hpp>
#include <rain/runtime/entity.hpp>
#include<rain/core/string_id.hpp>
#include<rain/core/tag/tag.hpp>
#include<rain/core/tag/tag_container.hpp>
#include<rain/core/tag/tag_query.hpp>


#include <utility>
#include <vector>

namespace rain
{
    struct world_entity_desc {
        string_id name;
        bool active = true;
    };

    struct world_entity_meta {
        string_id name;
        bool active = true;
    };


    class world
    {
    public:
        entity_id create_entity();

        entity_id create_entity(const world_entity_desc& desc);

        bool destroy_entity(entity_id entity);


        bool is_alive(entity_id entity) const;
        bool is_entity_active(entity_id entity)const;

        void set_entity_active(entity_id entity,bool active);

        void set_entity_name(entity_id entity, string_id name);

        [[nodiscard]] string_id entity_name(entity_id entity)const;

        [[nodiscard]] entity_id find_entity_by_name(string_id name)const;

        [[nodiscard]] u32 living_entity_count() const;

        void add_tag(entity_id entity, tag_id tag);
        bool remove_tag(entity_id entity, tag_id tag);

        [[nodiscard]] bool has_tag(entity_id entity, tag_id tag)const;
        [[nodiscard]] u32 tag_count(entity_id entity, tag_id tag)const;
        [[nodiscard]] bool matches_tags(entity_id entity, const tag_query& query)const;

        [[nodiscard]] tag_container* try_get_tags(entity_id entity);
        [[nodiscard]] const tag_container* try_get_tags(entity_id entity)const;

        template <typename component_type, typename... args_type>
        component_type& add_component(entity_id entity, args_type&&... args)
        {
            rain_assert(is_alive(entity));
            return components_.add<component_type>(entity, std::forward<args_type>(args)...);
        }

        template <typename component_type>
        bool remove_component(entity_id entity)
        {
            if (!is_alive(entity))
            {
                return false;
            }

            return components_.remove<component_type>(entity);
        }

        template <typename component_type>
        [[nodiscard]] bool has_component(entity_id entity) const
        {
            if (!is_alive(entity))
            {
                return false;
            }

            return components_.has<component_type>(entity);
        }

        template <typename component_type>
        [[nodiscard]] component_type& get_component(entity_id entity)
        {
            rain_assert(is_alive(entity));
            return components_.get<component_type>(entity);
        }

        template <typename component_type>
        [[nodiscard]] const component_type& get_component(entity_id entity) const
        {
            rain_assert(is_alive(entity));
            return components_.get<component_type>(entity);
        }

        template <typename component_type>
        [[nodiscard]] component_type* try_get_component(entity_id entity)
        {
            if (!is_alive(entity))
            {
                return nullptr;
            }

            return components_.try_get<component_type>(entity);
        }

        template <typename component_type>
        [[nodiscard]] const component_type* try_get_component(entity_id entity) const
        {
            if (!is_alive(entity))
            {
                return nullptr;
            }

            return components_.try_get<component_type>(entity);
        }

        template <typename component_type>
        [[nodiscard]] component_pool<component_type>& get_or_create_component_pool()
        {
            return components_.get_or_create_pool<component_type>();
        }

        template <typename component_type>
        [[nodiscard]] component_pool<component_type>* try_get_component_pool()
        {
            return components_.try_get_pool<component_type>();
        }

        template <typename component_type>
        [[nodiscard]] const component_pool<component_type>* try_get_component_pool() const
        {
            return components_.try_get_pool<component_type>();
        }

    private:
        struct entity_record {
            u32 generation = 0;
            bool alive = false;
        };

    private:
        std::vector<entity_record>records_;
        std::vector<world_entity_meta>metadatas_;

        std::vector<u32> free_indices_;
        u32 living_count_ = 0;

        component_registry components_;
    };
}