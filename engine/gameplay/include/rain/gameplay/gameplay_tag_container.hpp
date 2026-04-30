flatrain#pragma once

#include <rain/core/container/rain_vector.hpp>
#include <rain/core/container/rain_hash_map.hpp>
#include <rain/core/types.hpp>
#include <rain/gameplay/gameplay_tag.hpp>
#include <rain/gameplay/gameplay_tag_event.hpp>

#include <vector>

namespace rain
{
    class gameplay_tag_container
    {
    public:
        void add_tag(gameplay_tag tag)
        {
            if (!tag.is_valid())
            {
                return;
            }

            u32* count = tag_counts_.find(tag);

            if (count != nullptr)
            {
                ++(*count);
                return;
            }

            tag_counts_.insert(tag, 1);

            pending_events_.push_back(gameplay_tag_event{
                .type = gameplay_tag_event_type::added,
                .tag = tag,
                .new_count = 1
            });
        }

        void remove_tag(gameplay_tag tag)
        {
            if (!tag.is_valid())
            {
                return;
            }

            u32* count = tag_counts_.find(tag);

            if (count == nullptr)
            {
                return;
            }

            if (*count > 1)
            {
                --(*count);
                return;
            }

            tag_counts_.erase(tag);

            pending_events_.push_back(gameplay_tag_event{
                .type = gameplay_tag_event_type::removed,
                .tag = tag,
                .new_count = 0
            });
        }

        [[nodiscard]] bool has_tag(gameplay_tag tag) const
        {
            return tag_counts_.contains(tag);
        }

        [[nodiscard]] u32 get_tag_count(gameplay_tag tag) const
        {
            const u32* count = tag_counts_.find(tag);

            if (count == nullptr)
            {
                return 0;
            }

            return *count;
        }

        [[nodiscard]] bool has_all(const std::vector<gameplay_tag>& tags) const
        {
            for (gameplay_tag tag : tags)
            {
                if (!has_tag(tag))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool has_any(const std::vector<gameplay_tag>& tags) const
        {
            for (gameplay_tag tag : tags)
            {
                if (has_tag(tag))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool has_none(const std::vector<gameplay_tag>& tags) const
        {
            for (gameplay_tag tag : tags)
            {
                if (has_tag(tag))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] const std::vector<gameplay_tag_event>& get_pending_events() const
        {
            return pending_events_;
        }

        void clear_pending_events()
        {
            pending_events_.clear();
        }

        [[nodiscard]] usize get_unique_tag_count() const
        {
            return tag_counts_.size();
        }

    private:
        rain_hash_map<gameplay_tag, u32> tag_counts_;
        std::vector<gameplay_tag_event> pending_events_;
    };
}