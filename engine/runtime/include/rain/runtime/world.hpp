#pragma once

#include<rain/runtime/entity.hpp>

#include<vector>

namespace rain{
    class world{
    public:
        entity_id create_entity();
        bool destory_entity(entity_id entity);
        bool is_alive(entity_id entity)const;

        [[nodiscard]] u32 living_entity_count()const;

    private:
        std::vector<u32> generations_;
        std::vector<u32> free_indices_;
        u32 living_count_ =0;
    }
}