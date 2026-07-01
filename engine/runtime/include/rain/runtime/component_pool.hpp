#pragma once

#include <rain/core/container/rain_sparse_set.hpp>
#include <rain/core/types.hpp>
#include <rain/runtime/entity.hpp>

#include <utility>

namespace rain
{
    class component_pool_base
    {
    public:
        virtual ~component_pool_base() = default;

        virtual bool remove_entity(entity_id entity) = 0;
        [[nodiscard]] virtual bool has_entity(entity_id entity) const = 0;

        [[nodiscard]] virtual usize size() const = 0;
        [[nodiscard]] virtual entity_id entity_at(usize index) const = 0;
    };

    template <typename component_type>
    class component_pool final : public component_pool_base
    {
    public:
        template <typename... args_type>
        component_type& add(entity_id entity, args_type&&... args)
        {
            return components_.emplace(
                entity,
                std::forward<args_type>(args)...
            );
        }

        bool remove(entity_id entity)
        {
            return components_.remove(entity);
        }

        bool remove_entity(entity_id entity) override
        {
            return remove(entity);
        }

        [[nodiscard]] bool has(entity_id entity) const
        {
            return components_.contains(entity);
        }

        [[nodiscard]] bool has_entity(entity_id entity) const override
        {
            return has(entity);
        }

        [[nodiscard]] component_type& get(entity_id entity)
        {
            return components_.get(entity);
        }

        [[nodiscard]] const component_type& get(entity_id entity) const
        {
            return components_.get(entity);
        }

        [[nodiscard]] component_type* try_get(entity_id entity)
        {
            return components_.try_get(entity);
        }

        [[nodiscard]] const component_type* try_get(entity_id entity) const
        {
            return components_.try_get(entity);
        }

        [[nodiscard]] usize size() const override
        {
            return components_.size();
        }

        [[nodiscard]] entity_id entity_at(usize index) const override
        {
            return components_.keys()[index];
        }

        [[nodiscard]] const auto& entities() const
        {
            return components_.keys();
        }

        [[nodiscard]] const auto& values() const
        {
            return components_.values();
        }

    private:
        rain_sparse_set<entity_id, component_type> components_;
    };
}