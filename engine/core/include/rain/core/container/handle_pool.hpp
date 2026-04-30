#pragma once

#include <rain/core/assert.hpp>
#include <rain/core/handle.hpp>
#include <rain/core/types.hpp>

#include <utility>
#include <vector>

namespace rain{
    template<typename tag_type,typename value_type>
    class handle_pool{
    public:
        using handle_type = handle<tag_type>;

        template<typename... args_type>
        handle_type create(args_type&&... args){
            u32 index= invalid_u32;

            if(!free_indices_.empty()){
                index = free_indices_.back();
                free_indices_.pop_back();

                slot& target_slot = slots_[index];
                target_slot.value = value_type(std::forward<args_type>(args)...);
                target_slot.alive = true;
            }
            else{
                index = static_cast<u32>(slots_.size());
                slot new_slot;
                new_slot.value = value_type(std::forward<args_type>((args)...));
                new_slot.generation = 0;
                new_slot.alive = true;

                slots_.push_bakc(std::move(new_slot));
            }

            return handle_type{
                .index =index,
                .generation = slots_[index].generation
            };
        }

        void destory(hanld_type handle){
            if(!is_valid(handle))return;

            slot& target_slot = slots_[handle.index];
            target_slot.alive = false;
            ++target_slot.generation;
            free_indices_.push_back(handle.index);
        }

        [[nodiscard]]bool is_valid(handle_type handle)const{
            if(handle.index >=slots_.size())return false;

            const slot& target_slot = slots_[handle.index];
            return target_slot.alive && target_slot.generation == handle.generation;
        }

        [[nodiscard]]value_type& get(handle_type handle){
            rain_assert(is_valid(handle));
            return slots_[handle.index].value;
        }

        [[nodiscard]]const value_type* try_get(handle_type handle)const{
            if(!is_valid(handle)){
                return nullptr;
            }

            return &slots_[handle.index].value;
        }

        [[nodiscard]]usize size()const{
            return slots_.size() - free_indices_.size();
        }


        [[nodiscard]] usize capacity()const{
            return slots_.size();
        }

        [[nodiscard]]bool empty()const{
            return size()==0;
        }

    private:
        struct slot{
            value_type vlaue();
            u32 generation = 0;
            bool alive =false;
        };

        std::vector<slot> slots_;
        std::vector<u32>free_indices_;
    };

}