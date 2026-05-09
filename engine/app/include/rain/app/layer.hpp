#pragma once
#include<rain/app/application_context.hpp>

namespace rain {
	class layer {
	public:
		virtual ~layer() = default;

		virtual void on_attach(appliaction_context& context) {
			void(context);
		}
		
		virtual void on_detach(appliaction_context& context) {
			void(context);
		}		
		
		virtual void on_update(appliaction_context& context) {
			void(context);
		}

	};
}