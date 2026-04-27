#pragma once

#include<array>
#include<cstddef>
#include<initializer_list>
#include<new>
#include<type_traits>
#include<utility>

#include<rain/core/assert.hpp>
#include<rain/core/types.hpp>

namespace rain{
    template<typename value_type,usize capacity_value>
    class rain_vector{
    public:
        rain_vector() = default;
        rain_vector(std:initializer_list<value_type> values){
            rain_asert(values.size()<=capacity_value);

            for(const value_type& value:values){
                push_back(value);
            }
        }

        rain_vector(const rain_vector&other){
            for(const value_type& value:other){
                push_back(value);
            }
        }

        rain_vector& operator=(const rain_vector&other){
            if(this == &other){
                return *this;
            }
            clear();

            for(const value_type& value:other){
                push_back(value);
            }

            return *this;
        }

        rain_vector(rain_vector&&other)noexcept{
            for(value_type& value:other){
                push_back(std::move(value));
            }
            other.clear();
        }

        rain_vector& operator=(rain_vector&other)noexcept{
            if(this==&other){
                return *this;
            }
            clear();
            for(value_type&value:other){
                push_back(std::move(value));
            }
            other.clear();

            return *this;
        }


        ~rain_vector(){
            clear();
        }
        
        template <typename... args_type>
        value_type& emplace_back(args_type&&... args)
        {
            rain_assert(size_ < capacity_value);

            void* address = static_cast<void*>(&storage_[size_]);
            value_type* value = new (address) value_type(std::forward<args_type>(args)...);

            ++size_;

            return *value;
        }

        void push_back(const value_type& value)
        {
            emplace_back(value);
        }

        void push_back(value_type&& value)
        {
            emplace_back(std::move(value));
        }

        void pop_back()
        {
            rain_assert(size_ > 0);

            --size_;
            data()[size_].~value_type();
        }

        void clear()
        {
            for (usize i = 0; i < size_; ++i)
            {
                data()[i].~value_type();
            }

            size_ = 0;
        }

        [[nodiscard]] value_type& operator[](usize index)
        {
            rain_assert(index < size_);
            return data()[index];
        }

        [[nodiscard]] const value_type& operator[](usize index) const
        {
            rain_assert(index < size_);
            return data()[index];
        }

        [[nodiscard]] value_type& front()
        {
            rain_assert(size_ > 0);
            return data()[0];
        }

        [[nodiscard]] const value_type& front() const
        {
            rain_assert(size_ > 0);
            return data()[0];
        }

        [[nodiscard]] value_type& back()
        {
            rain_assert(size_ > 0);
            return data()[size_ - 1];
        }

        [[nodiscard]] const value_type& back() const
        {
            rain_assert(size_ > 0);
            return data()[size_ - 1];
        }

        [[nodiscard]] value_type* data()
        {
            return reinterpret_cast<value_type*>(storage_.data());
        }

        [[nodiscard]] const value_type* data() const
        {
            return reinterpret_cast<const value_type*>(storage_.data());
        }

        [[nodiscard]] value_type* begin()
        {
            return data();
        }

        [[nodiscard]] value_type* end()
        {
            return data() + size_;
        }

        [[nodiscard]] const value_type* begin() const
        {
            return data();
        }

        [[nodiscard]] const value_type* end() const
        {
            return data() + size_;
        }

        [[nodiscard]] usize size() const
        {
            return size_;
        }

        [[nodiscard]] constexpr usize capacity() const
        {
            return capacity_value;
        }

        [[nodiscard]] bool empty() const
        {
            return size_ == 0;
        }

        [[nodiscard]] bool full() const
        {
            return size_ == capacity_value;
        }

    private:
#ifdef cpp23
        struct storage_type{
            alignas(value_type) std::byte bytes[sizeof(value_type)];
        };
#elif
        using storage_type = std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
#endif
        
        std::array<storage_type, capacity_value> storage_{};
        usize size_ = 0;


    };






}