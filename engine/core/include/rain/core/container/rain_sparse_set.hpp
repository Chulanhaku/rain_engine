#pragma once

#include<utility>
#include <vector>

#include<rain/core/assert.hpp>
#include<rain/core/types.hpp>

namespace rain{
    template<typename id_type,typename value_type>
    class rain_sparse_set{
    public:
        bool contains(id_type id)const{
            if(id.index >= sparse_indices_.size()){
                return false;
            }
            const u32 dense_index = sparse_indices_[id.index];

            if(dense_index == invalid_u32 || dense_index >= dense_ids_.size()){
                return false;
            }

            return dense_ids_[dense_index] == id;
        }

        value_type&get(id_type id){
            rain_assert(contains(id));
            return dense_values_[sparse_indices_[id.index]];
        }

        const value_type& get(id_type id)const{
            rain_assert(contains(id));
            return dense_values_[sparse_indices_[id.index]];
        }

        template<typename... args_type>
        value_type&emplace(id_type id,args_type&&... args){
            if(id.index >= sparse_indices_.size()){
                sparse_indices_.resize(static_cast<usize>(id.index)+1,invalid_u32);
            }

            if(contains(id)){
                value_type&value = get(id);
                value = value_type{std::forward<args_type>(args)...};
                return value;
            }

            const u32 dense_index = static_cast<u32>(dense_values_.size());
            sparse_indices_[id.index] = dense_index;
            dense_ids_.push_back(id);
            dense_values_.emplace_back(std::forward<args_type>(args)...);

            return dense_values_.back();
        }

        bool remove(id_type id){
            if(!contains(id)){
                return false;
            }

            const u32 dense_index = sparse_indices_[id.index];
            const u32 last_index = static_cast<u32>(dense_values_.size()-1);

            if(dense_index != last_index){
                dense_values_[dense_index] = std::move(dense_values_[last_index]);
                dense_ids_[dense_index] = dense_ids_[last_index];
                sparse_indices_[dense_ids_[dense_index].index] = dense_index;
            }

            dense_values_.pop_back();
            dense_ids_.pop_back();
            sparse_indices_[id.index] = invalid_u32;

            return true;
        }

        void clear(){
            dense_values_.clear();
            dense_ids_.clear();
            sparse_indices_.clear();
        }

        [[nodiscard]]usize size() const{
            return dense_values_.size();
        }

        [[nodiscard]]std::vector<id_type>& ids() {
            return dense_ids_;
        }

        [[nodiscard]]const std::vector<id_type>& ids() const{
            return dense_ids_;
        }

        [[nodiscard]]std::vector<value_type>& values() {
            return dense_values_;
        }

        [[nodiscard]]const std::vector<value_type>& values() const{
            return dense_values_;
        }

    private:
        std::vector<u32> sparse_indices_;
        std::vector<id_type> dense_ids_;
        std::vector<value_type> dense_values_;
    };
}