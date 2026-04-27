#pragma  once

#include<rain/gameplay/gameplay_tag.hpp>

#include<algorithm>
#include<vector>

namespace rain{
    class gameplay_tag_container{
    public:
        bool add_tag(gameplay_tag tag){
            if(!tag.is_valid()||has_tag(tag))return false;

            tags_.push_back(tag);
            std::sort(tags_.begin(),tags_.end());
            return true;
        }
        
        bool remove_tag(gameplay_tag tag){
            const auto it = std::lower_bound(tags_.begin(),tags_.end(),tag);

            if(it==tags.end()||*it!=tag)return false;
            tags_.erase(it);
            return true;
        }

        [[nodiscard]] bool has_tag(gameplay_tag tag)const{
            const auto it = std::loweround(tags_.begin(),tags_.end(),tag);
            return it != tags_.end() && *it == tag;
        }

        [[nodiscard]] bool has_all(const std::vector<gameplay_tag>& tags)const{
            for(const gameplay_tag tag : tags){
                if(!has_tag(tag))return false;
            }
            return true;
        }

        [[nodiscard]] bool has_any(const std::vector<gameplay_tag>& tags)const{
            for(const gameplay_tag tag : tags){
                if(has_tag(tag))return true;
            }
            return false ;
        }

        [[nodiscard]] u32 get_tag_count()const{
            return static_cast<u32>(tags_.size());
        }

        [[nodiscard]] const std::vector<gameplay_tag>& tags()const{
            return tags_;
        }
    private:
        std::vector<gameplay_tag>c tags_;
    }




}