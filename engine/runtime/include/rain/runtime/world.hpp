#pragma once

#include <rain/core/assert.hpp>
#include <rain/runtime/component_registry.hpp>
#include <rain/runtime/entity.hpp>

#include <utility>
#include <vector>

namespace rain
{
    class world
    {
    public:
        entity_id create_entity();
        bool destroy_entity(entity_id entity);
        bool is_alive(entity_id entity) const;

        [[nodiscard]] u32 living_entity_count() const;

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
        std::vector<u32> generations_;
        std::vector<u32> free_indices_;
        u32 living_count_ = 0;

        component_registry components_;
    };
}