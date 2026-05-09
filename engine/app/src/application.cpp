#include<rain/app/application.hpp>
#include<rain/core/log.hpp>
#include<chrono>

namespace rain {
	application::application(const application_desc& desc) :main_window_(window_desc{
		.title = desc.title,
		.width = desc.width,
		.height = desc.height,
		.resizable = desc.resizable
		}) 
	{
		rain::log_info("app start");
	}

	application::~application() {
		application_context context = make_context(0.0f);

		for (auto layer_iter =layers_.rbegin(); layer_iter != layers_.rend(); layer_iter++) {
			(*layer_iter)->on_detach(context);
		}

		rain::log_info("app down");
	}

	void application::push_layer(std::unique_ptr<layer>new_layer) {
		application_context context = make_context(0.0f);
		new_layer->on_attach(context);

		layers_.push_back(std::move(new_layer));
	}

	int application::run() {
		using clock = std::chrono::high_resolution_clock;

		running_ = true;

		auto previous_time = clock::now();

		while (running_ && !main_window_.should_close()) {
			const auto current_time = clock::now();

			const std::chrono::duration<f32>delta_duration = current_time - previous_time;

			previous_time = current_time;

			const f32 delta_seconds = delta_duration.count();

			main_window_.poll_events();
			application_context context = make_context(delta_seconds);

			for (std::unique_ptr<layer>& current_layer : layers_) {
				current_layer->on_update(context);
			}

			scheduler_.run(target_world_, events_, delta_seconds, frame_index_);

			events_.dispatch_all_queued();

			main_window_.present();

			++frame_index_;
		}

		running_ = false;

		return 0;
	}

	void application::request_close() {
		running_ = false;
		main_window_.request_close();
	}

	rain_window& application::main_window() {
		return main_window_;
	}
	const rain_window& application::main_window()const {
		return main_window_;
	}
	
	world& application::target_world() {
		return target_world_;
	}

	const world& application::target_world() const {
		return target_world_;
	}

	event_system& application::events() {
		return events_;
	}

	const event_system& application::events() const {
		return events_;
	}

	system_scheduler& application::scheduler()
    {
        return scheduler_;
    }

    const system_scheduler& application::scheduler() const
    {
        return scheduler_;
    }

	application_context application::make_context(f32 delta_seconds)
    {
        return application_context{
            .main_window = &main_window_,
            .target_world = &target_world_,
            .events = &events_,
            .scheduler = &scheduler_,
            .delta_seconds = delta_seconds,
            .frame_index = frame_index_
        };
    }
}