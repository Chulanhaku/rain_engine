#pragma once

#include <memory>
#include<utility>
#include<vector>

#include<rain/core/assert.hpp>
#include<rain/core/container/rain_hash_map.hpp>
#include<rain/core/type_id.hpp>
#include<rain/core/types.hpp>
#include<rain/runtime/component_pool.hpp>
#include<rain/runtime/entity.hpp>

namespace rain{
    class component_registry{
    private:
        struct component_pools_base{
            virtual ~component_pools_base()=default;
            virtual bool remove_entity(entity_id entity)=0;
            [[nodiscard]] virtual component_pool_base* pool_base_ptr()=0;
            [[nodiscard]] virtual const component_pool_base* pool_base_ptr()const=0;
        };

        template <typename component_type>
        struct component_pools_model;

    public:
        template<typename component_type,typename... args_type>
        component_type&add(entity_id entity,args_type&&... args){
            component_pool<component_type>&pool = get_or_create_pool<component_type>();
            return pool.add(entity,std::forward<args_type>(args)...);
        }

        template<typename component_type>
        bool remove(entity_id entity){
            component_pool<component_type>*pool = try_get_pool<component_type>();

            if(pool ==nullptr)return false;

            return pool->remove(entity);
        }

        void remove_all(entity_id entity){
            for(std::unique_ptr<component_pools_base>&pool :pools_){
                pool->remove_entity(entity);
            }
        }

        template<typename component_type>
        [[nodiscard]]bool has (entity_id entity)const{
            const component_pool<component_type> *pool = try_get_pool<component_type>();
            if(pool==nullptr)return false;
            
            return pool->has(entity);
        }

        [[nodiscard]] bool has(entity_id entity,type_id component_type_id)const {
            const component_pool_base* pool = try_get_pool(component_type_id);
            if (pool == nullptr)return false;

            return pool->has_entity(entity);
        }

        template<typename component_type>
        [[nodiscard]]component_type&get(entity_id entity){
            component_pool<component_type>*pool = try_get_pool<component_type>();
            rain_assert(pool!= nullptr);
            rain_assert(pool->has(entity));

            return pool->get(entity);
        }

        template<typename component_type>
        [[nodiscard]]const component_type&get(entity_id entity)const{
            const component_pool<component_type>*pool = try_get_pool<component_type>();
            rain_assert(pool!= nullptr);
            rain_assert(pool->has(entity));

            return pool->get(entity);
        }

        template<typename component_type>
        [[nodiscard]]component_type* try_get(entity_id entity){
            component_pool<component_type>*pool = try_get_pool<component_type>();
            if(pool==nullptr||!pool->has(entity))return nullptr;

            return &pool->get(entity);
        }

        template<typename component_type>
        [[nodiscard]]const component_type* try_get(entity_id entity)const{
            const component_pool<component_type>*pool = try_get_pool<component_type>();
            if(pool==nullptr||!pool->has(entity))return nullptr;

            return &pool->get(entity);
        }

        template<typename component_type>
        [[nodiscard]]component_pool<component_type>&get_or_create_pool(){
            const type_id id = get_type_id<component_type>();
            const usize* pools_index = type_to_pools_index_.find(id);

            if(pools_index!=nullptr){
                return *static_cast<component_pools_model<component_type>*>(pools_[*pools_index].get())->pools_ptr();
            }

            const usize new_pools_index = pools_.size();

            auto pool = std::make_unique<component_pools_model<component_type>>();
            component_pool<component_type>*raw_pool = pool->pools_ptr();

            pools_.push_back(std::move(pool));
            type_to_pools_index_.insert(id,new_pools_index);

            return *raw_pool;
        }

        template<typename component_type>
        [[nodiscard]]component_pool<component_type>* try_get_pool(){
            const type_id id = get_type_id<component_type>();
            const usize* pools_index = type_to_pools_index_.find(id);

            if(pools_index ==nullptr){
                return nullptr;
            }

            return static_cast<component_pools_model<component_type>*>(pools_[*pools_index].get())->pools_ptr();            
        }

        template<typename component_type>
        [[nodiscard]]const component_pool<component_type>* try_get_pool()const{
            const type_id id = get_type_id<component_type>();
            const usize* pools_index = type_to_pools_index_.find(id);

            if(pools_index ==nullptr){
                return nullptr;
            }

            return static_cast<const component_pools_model<component_type>*>(pools_[*pools_index].get())->pools_ptr();            
        }

        [[nodiscard]] component_pool_base* try_get_pool(type_id id) {
            const usize* pool_index = type_to_pools_index_.find(id);

            if (pool_index == nullptr)return nullptr;

            return pools_[*pool_index]->pool_base_ptr();
        }

        [[nodiscard]]const component_pool_base* try_get_pool(type_id id)const {
            const usize* pool_index = type_to_pools_index_.find(id);

            if (pool_index == nullptr)return nullptr;

            return pools_[*pool_index]->pool_base_ptr();
        }

    private:
        template <typename component_type>
        struct component_pools_model final:component_pools_base{
            bool remove_entity(entity_id entity)override{
                return pool.remove(entity);
            }

            [[nodiscard]]component_pool<component_type>* pools_ptr(){
                return &pool;
            }

            [[nodiscard]]const component_pool<component_type>* pools_ptr()const {
                return &pool;
            }

            [[nodiscard]] component_pool_base* pool_base_ptr()override{
                return &pool;
            }

            [[nodiscard]] const component_pool_base* pool_base_ptr()const override{
                return &pool;
            }

            component_pool<component_type>pool;
        };

    private:
        rain_hash_map<type_id,usize>type_to_pools_index_;
        std::vector<std::unique_ptr<component_pools_base>>pools_;
    };
}