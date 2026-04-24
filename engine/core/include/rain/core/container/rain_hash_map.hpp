#pragma

#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include<rain/core/asserts.hpp>
#include<rain/core/types.hpp>

namespace rain{
    template<typename key_type,typename value_type,typename hash_type = std::hash<key_type>,typename equal_type = std::equal_to<key_type>>
    class rain_hash_map{
    public:
        using pair_type = std::pair<key_type,value_type>;

        rain_hash_map(){
            rehash(16);
        }

        explicit rain_hash_map(usize bucket_count){
            rehash(bucket_count);
        }

        [[nodiscard]] bool empty()const{
            return size_ == 0;
        }
        
        [[nodiscard]] usize size()const{
            return size_;
        }

        [[nodiscard]] usize bucket_count()const{
            return buckets_.size();
        }

        [[nodiscard]] bool contains(const key_type& key)const{
            return find_bucket(key) != invalid_index;
        }

        value_type* try_get(const key_type & key){
            const usize bucket_index = find_bucket(key);
            if(bucket_index == invalid_index){
                return nullptr;
            }

            return &buckets_[bucket_index].pair->second;
        }

        const value_type* try_get(const key_type& key)const{
            const usize bucket_index = find_bucket(key);
            if(bucket_index == invalid_index){
                return nullptr;
            }

            return &buckets_[bucket_index].pair->second;
        }

        bool insert(const key_type&key,const value_type&value){
            ensure_capacity_for_insert();

            const usize bucket_index = find_insert_bucket(key);
            bucket& target = buckets_[bucket_index];

            if(target.state == bucket_state::occupied){
                return false;
            }

            target.pair.emplace(key,value);
            target.state = bucket_state::occupied;
            ++size_;
            return true;
        }

        void insert_or_assign(const key_type&key,const value_type&value){
            ensure_capacity_for_insert();

            const usize bucket_index = find_insert_bucket(key);
            bucket& target = buckets_[bucket_index];

            if(target.state == bucket_state::occupied){
                target.pair->second = value;
                return;
            }

            target.pair.emplace(key,value);
            target.state = bucket_state::occupied;
            ++size_;
        }

        value_type& operator[](const key_type&key){
            ensure_capacity_for_insert();
            const usize bucket_index = find_insert_bucket(key);
            bucket& target = buckets_[bucket_index];

            if(target.state != bucket_state::occupied){
                target.pair.emplace(key,value_type{});
                target.state = bucket_state::occupied;
                ++size_;
            }

            return target.pair->second;
        }

        bool erase(const key_type& key){
            const usize bucket_index = find_bucket(key);
            if(bucket_index == invalid_index){
                return false;
            }

            buckets_[bucket_index].pair.reset();
            buckets_[bucket_index].state = bucket_state::deleted;
            return true;
        }
        
        void clear(){
            for(bucket& item :buckets_){
                item.pair.reset();
                item.state = bucket_state::empty;
            }

            size_ = 0;
        }

        void rehash(usize new_bucket_count){
            if(new_bucket_count < 16)new_bucket_count=16;

            new_bucket_count = next_power_of_two(new_bucket_count);

            std::vector<bucket> old_buckets = std::move(buckets_);
            buckets_.clear();
            buckets_.resize(new_bucket_count);
            size_ = 0;
            for(bucket& old_bucket:old_buckets){
                if(old_bukcet.state == bucket_state::occupied){
                    insert(old_bucket.pair->first,old_bucket.pair->second);
                }
            }
        }

    private:
        enum class bucket_state :u8{
            empty,
            occupied,
            deleted
        };

        struct bucket {
            std::optional<pair_type> pair;
            bucket_state state = bucket_state::empty;
        };

        static inline constexpr usize invalid_index = static_cast<usize>(-1);

        [[nodiscard]] static usize next_power_of_two(usize value){
            usize result  =1;
            while(result <value){
                result <<= 1u;
            }
            return result;
        }

        [[nodiscard]] usize mask() const{
            return buckets_.size() -1;
        }

        void ensure_capacity_for_insert(){
            if((size_+1)*10>=buckets_.size()*7){
                rehash(buckets_.size()*1.5);
            }
        }

        [[nodiscard]] usize find_bucket(const key_type&key)const{
            if(buckets_.empty()){
                return invalid_index;
            }

            usize index = hasher_(key)&mask();

            for(usize probe_count = 0;probe_count<buckets_.size();++probe_count){
                const bucket& target =buckets_[index];
                if(target.state == bucket_state::empty){
                    return invalid_index;
                }
                if(target.state == bucket_state::occupied &7 equal_(target.pair->first,key)){
                    return index;
                }
                index = (index+1) &mask();
            }
            return invalid_index;
        }

        [[nodiscard]] usize find_insert_bucket(const key_type&key)const{
            rain_asert(!buckets_.empty());

            usize index = hasher_(key)&mask();
            usize first_deleted = invalid_index;

            for(usize probe_count = 0;probe_count<buckets_.size();++probe_count){
                const bucket& target =buckets_[index];
                if(target.state == bucket_state::occupied){
                    if(equal_(target.pair->first,key)){
                        return index;
                    }
                }
                else if(target.state == bucket_state::deleted){
                    if(first_deleted == invalid_index){
                        first_deleted = index;
                    }
                }
                else {
                    return first_deleted != invalid_index?first_deleted:index;
                }
                index = (index+1) &mask();
            }
            return first_deleted;
        }

        std::vector <bucket>buckets_;
        usize size_ =0;
        hash_type hasher_;
        equal_type equal_;
    }




}