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
		,renderer_(create_d3d11_render_backend(main_window_))
		,clear_color_(desc.clear_color)
	{
		assets_ = std::make_unique<texture_asset_registry>(*renderer_);
		meshes_3d_ = std::make_unique<mesh_3d_registry>(*renderer_);
		materials_3d_ = std::make_unique<material_3d_registry>();

		rain::log_info("app start");
	}

	application::~application() {
		application_context context = make_context(0.0f);

		for (auto layer_iter =layers_.rbegin(); layer_iter != layers_.rend(); layer_iter++) {
			(*layer_iter)->on_detach(context);
		}
		layers_.clear();

		materials_3d_.reset();
		meshes_3d_.reset();
		assets_.reset();
		if (renderer_ != nullptr)renderer_->flush_resource_destruction();

		renderer_.reset();

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

			input_.update(main_window_);

			application_context context = make_context(delta_seconds);

			for (std::unique_ptr<layer>& current_layer : layers_) {
				current_layer->on_update(context);
			}

			const system_run_info variable_run{
				.delta_seconds = delta_seconds,
				.interpolation_alpha = fixed_step_state_.interpolation_alpha,
				.frame_index = frame_index_,
				.fixed_tick_index = fixed_step_state_.tick_index,
				.tick_type = system_tick_type::varible
			}


			scheduler_.run_phase(system_phase::pre_update,target_world_, events_, variable_run);
			scheduler_.run_phase(system_phase::input, target_world_, events_, variable_run);
			scheduler_.run_phase(system_phase::gameplay, target_world_, events_, variable_run);
			scheduler_.run_phase(system_phase::movement, target_world_, events_, variable_run);


			//physics
			const f32 physics_frame_delta = std::min(delta_seconds,fixed_step_settings_.max_frame_delta );
			const f32 max_accumulator = fixed_step_settings_.delta_seconds * static_cast<f32>(fixed_step_settings_.max_substeps);
			fixed_step_state_.accumulator = std::min(fixed_step_state_.accumulator + physics_frame_delta, max_accumulator);

			u32 substep_count = 0;

			while (fixed_step_state_.accumulator >= fixed_step_settings_.delta_seconds && substep_count < fixed_step_settings_.max_substeps) {
				const system_run_info fixed_run{
					.delta_seconds = fixed_step_settings_.delta_seconds,
					.interpolation_alpha = 0.0f,
					.frame_index = frame_index,
					.fixed_tick_index = fixed_step_state_.tick_index,
					.tick_type = system_tick_type::fixed
				};

				scheduler_.run_phase(system_phase::physics, target_world_, events_, variable_run);
				scheduler_.run_phase(system_phase::post_physics, target_world_, events_, variable_run);

				fixed_step_state_.accumulator -= fixed_step_settings_.delta_seconds;
				++fixed_step_state_.tick_index;
				++substep_count;
			}

			//
			fixed_step_state_.interpolation_alpha = fixed_step_state_.accumulator / fixed_step_settings_.delta_seconds;

			const system_run_info post_run{  
				.delta_seconds = delta_seconds,
				.interpolation_alpha = fixed_step_state_.interpolation_alpha,
				.frame_index = frame_index_,
				.fixed_tick_index = fixed_step_state_.tick_index,
				.tick_type = system_tick_type::variable
			};

			scheduler_.run_phase(system_phase::animation, target_world_, events_, post_run);
			scheduler_.run_phase(system_phase::post_update, target_world_, events_, post_run);
			scheduler_.run_phase(system_phase::render_prepare, target_world_, events_, post_run);

			events_.dispatch_all_queued();

			renderer_->begin_frame();
			renderer_->clear(clear_color_);
			renderer_->clear_depth(1.0f);

			for (std::unique_ptr<layer>& current_layer : layers_) {
				current_layer->on_render(context);
			}
			renderer_->end_frame();

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

	render_backend& application::renderer()
	{
		return *renderer_;
	}

	const render_backend& application::renderer() const
	{
		return *renderer_;
	}

	input_action_map& application::input() {
		return input_;
	}

	const input_action_map& application::input() const{
		return input_;
	}

	texture_asset_registry& application::assets() {
		return *assets_;
	}
	const texture_asset_registry& application::assets()const {
		return *assets_;
	}
	material_2d_registry& application::materials() {
		return materials_;
	}

	const material_2d_registry& application::materials()const {
		return materials_;
	}

	application_context application::make_context(f32 delta_seconds)
    {
		return application_context{
			.main_window = &main_window_,
			.target_world = &target_world_,
			.events = &events_,
			.scheduler = &scheduler_,
			.renderer = renderer_.get(),
			.assets = assets_.get(),
			.materials = &materials_,
			.meshes_3d = meshes_3d_.get(),
			.materials_3d = materials_3d_.get(),
            .delta_seconds = delta_seconds,
            .frame_index = frame_index_,
			.input = &input_
        };
    }
}