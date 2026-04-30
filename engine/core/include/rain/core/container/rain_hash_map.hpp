#pragma once

#include <rain/core/assert.hpp>
#include <rain/core/types.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace rain
{
    template <
        typename key_type,
        typename value_type,
        typename hash_type = std::hash<key_type>,
        typename equal_type = std::equal_to<key_type>>
    class rain_hash_map
    {
    public:
        rain_hash_map()
        {
            rehash(16);
        }

        explicit rain_hash_map(usize initial_capacity)
        {
            rehash(initial_capacity);
        }

        [[nodiscard]] bool contains(const key_type& key) const
        {
            return find_bucket_index(key).has_value();
        }

        [[nodiscard]] value_type* find(const key_type& key)
        {
            const std::optional<usize> bucket_index = find_bucket_index(key);

            if (!bucket_index.has_value())
            {
                return nullptr;
            }

            return &buckets_[*bucket_index].value;
        }

        [[nodiscard]] const value_type* find(const key_type& key) const
        {
            const std::optional<usize> bucket_index = find_bucket_index(key);

            if (!bucket_index.has_value())
            {
                return nullptr;
            }

            return &buckets_[*bucket_index].value;
        }

        value_type& operator[](const key_type& key)
        {
            value_type* existing_value = find(key);

            if (existing_value != nullptr)
            {
                return *existing_value;
            }

            return insert(key, value_type{}).first;
        }

        std::pair<value_type&, bool> insert(const key_type& key, const value_type& value)
        {
            maybe_grow();

            usize bucket_index = find_insert_bucket_index(key);

            bucket& target_bucket = buckets_[bucket_index];

            if (target_bucket.state == bucket_state::occupied)
            {
                target_bucket.value = value;
                return {target_bucket.value, false};
            }

            if (target_bucket.state == bucket_state::deleted)
            {
                --deleted_count_;
            }

            target_bucket.key = key;
            target_bucket.value = value;
            target_bucket.state = bucket_state::occupied;

            ++size_;

            return {target_bucket.value, true};
        }

        std::pair<value_type&, bool> insert(key_type&& key, value_type&& value)
        {
            maybe_grow();

            usize bucket_index = find_insert_bucket_index(key);

            bucket& target_bucket = buckets_[bucket_index];

            if (target_bucket.state == bucket_state::occupied)
            {
                target_bucket.value = std::move(value);
                return {target_bucket.value, false};
            }

            if (target_bucket.state == bucket_state::deleted)
            {
                --deleted_count_;
            }

            target_bucket.key = std::move(key);
            target_bucket.value = std::move(value);
            target_bucket.state = bucket_state::occupied;

            ++size_;

            return {target_bucket.value, true};
        }

        template <typename... args_type>
        std::pair<value_type&, bool> emplace(const key_type& key, args_type&&... args)
        {
            maybe_grow();

            usize bucket_index = find_insert_bucket_index(key);

            bucket& target_bucket = buckets_[bucket_index];

            if (target_bucket.state == bucket_state::occupied)
            {
                return {target_bucket.value, false};
            }

            if (target_bucket.state == bucket_state::deleted)
            {
                --deleted_count_;
            }

            target_bucket.key = key;
            target_bucket.value = value_type(std::forward<args_type>(args)...);
            target_bucket.state = bucket_state::occupied;

            ++size_;

            return {target_bucket.value, true};
        }

        bool erase(const key_type& key)
        {
            const std::optional<usize> bucket_index = find_bucket_index(key);

            if (!bucket_index.has_value())
            {
                return false;
            }

            bucket& target_bucket = buckets_[*bucket_index];
            target_bucket.state = bucket_state::deleted;

            --size_;
            ++deleted_count_;

            return true;
        }

        void clear()
        {
            for (bucket& current_bucket : buckets_)
            {
                current_bucket.state = bucket_state::empty;
            }

            size_ = 0;
            deleted_count_ = 0;
        }

        void reserve(usize desired_capacity)
        {
            const usize required_capacity = static_cast<usize>(static_cast<float>(desired_capacity) / max_load_factor_) + 1;

            if (required_capacity > buckets_.size())
            {
                rehash(next_power_of_two(required_capacity));
            }
        }

        void rehash(usize new_capacity)
        {
            new_capacity = next_power_of_two(new_capacity);

            std::vector<bucket> old_buckets = std::move(buckets_);

            buckets_.clear();
            buckets_.resize(new_capacity);

            size_ = 0;
            deleted_count_ = 0;

            for (bucket& old_bucket : old_buckets)
            {
                if (old_bucket.state == bucket_state::occupied)
                {
                    insert(std::move(old_bucket.key), std::move(old_bucket.value));
                }
            }
        }

        [[nodiscard]] usize size() const
        {
            return size_;
        }

        [[nodiscard]] usize capacity() const
        {
            return buckets_.size();
        }

        [[nodiscard]] bool empty() const
        {
            return size_ == 0;
        }

        [[nodiscard]] float load_factor() const
        {
            if (buckets_.empty())
            {
                return 0.0f;
            }

            return static_cast<float>(size_) / static_cast<float>(buckets_.size());
        }

    private:
        enum class bucket_state : u8
        {
            empty,
            occupied,
            deleted
        };

        struct bucket
        {
            key_type key{};
            value_type value{};
            bucket_state state = bucket_state::empty;
        };

        [[nodiscard]] static usize next_power_of_two(usize value)
        {
            usize result = 1;

            while (result < value)
            {
                result <<= 1;
            }

            return result;
        }

        [[nodiscard]] usize bucket_mask() const
        {
            return buckets_.size() - 1;
        }

        [[nodiscard]] usize hash_key(const key_type& key) const
        {
            return static_cast<usize>(hash_type{}(key));
        }

        [[nodiscard]] std::optional<usize> find_bucket_index(const key_type& key) const
        {
            if (buckets_.empty())
            {
                return std::nullopt;
            }

            usize bucket_index = hash_key(key) & bucket_mask();

            for (usize probe_count = 0; probe_count < buckets_.size(); ++probe_count)
            {
                const bucket& current_bucket = buckets_[bucket_index];

                if (current_bucket.state == bucket_state::empty)
                {
                    return std::nullopt;
                }

                if (
                    current_bucket.state == bucket_state::occupied &&
                    equal_type{}(current_bucket.key, key))
                {
                    return bucket_index;
                }

                bucket_index = (bucket_index + 1) & bucket_mask();
            }

            return std::nullopt;
        }

        [[nodiscard]] usize find_insert_bucket_index(const key_type& key)
        {
            rain_assert(!buckets_.empty());

            usize bucket_index = hash_key(key) & bucket_mask();
            usize first_deleted_index = invalid_u32;

            for (usize probe_count = 0; probe_count < buckets_.size(); ++probe_count)
            {
                bucket& current_bucket = buckets_[bucket_index];

                if (
                    current_bucket.state == bucket_state::occupied &&
                    equal_type{}(current_bucket.key, key))
                {
                    return bucket_index;
                }

                if (
                    current_bucket.state == bucket_state::deleted &&
                    first_deleted_index == invalid_u32)
                {
                    first_deleted_index = bucket_index;
                }

                if (current_bucket.state == bucket_state::empty)
                {
                    if (first_deleted_index != invalid_u32)
                    {
                        return first_deleted_index;
                    }

                    return bucket_index;
                }

                bucket_index = (bucket_index + 1) & bucket_mask();
            }

            rain_assert(false);
            return 0;
        }

        void maybe_grow()
        {
            if (buckets_.empty())
            {
                rehash(16);
                return;
            }

            const float current_load_factor =
                static_cast<float>(size_ + deleted_count_ + 1) /
                static_cast<float>(buckets_.size());

            if (current_load_factor >= max_load_factor_)
            {
                rehash(buckets_.size() * 2);
            }
        }

    private:
        std::vector<bucket> buckets_;
        usize size_ = 0;
        usize deleted_count_ = 0;
        float max_load_factor_ = 0.70f;
    };
}