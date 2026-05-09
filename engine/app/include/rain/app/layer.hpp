#pragma once
#include<rain/app/application_context.hpp>

namespace rain {
	class layer {
	public:
		virtual ~layer() = default;

		virtual void on_attach(application_context& context) {
			(void)context;
		}
		
		virtual void on_detach(application_context& context) {
			(void)context;
		}		
		
		virtual void on_update(application_context& context) {
			(void)context;
		}

	};
}