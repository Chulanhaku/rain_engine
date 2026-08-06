#pragma once
#include<rain/core/types.hpp>
#include<rain/render/render_backend.hpp>
#include<memory>
#include<d3d11.h>
#include<dxgi.h>
#include<d3dcompiler.h>
#include<vector>
#include<array>


	namespace rain {
	template <typename handle_type, typename resource_type>
	class d3d11_resource_pool
	{
	private:
		struct resource_slot
		{
			resource_type resource{};

			u32 generation = 0;

			bool alive = false;
			bool pending_destroy = false;
		};

	public:
		[[nodiscard]] handle_type create(resource_type resource)
		{
			u32 index = 0;

			if (!free_indices_.empty())
			{
				index = free_indices_.back();
				free_indices_.pop_back();

				resource_slot& slot = slots_[index];

				slot.resource = std::move(resource);
				slot.alive = true;
				slot.pending_destroy = false;
			}
			else
			{
				index = static_cast<u32>(slots_.size());

				resource_slot slot;
				slot.resource = std::move(resource);
				slot.generation = 0;
				slot.alive = true;
				slot.pending_destroy = false;

				slots_.push_back(std::move(slot));
			}

			return handle_type{
				.index = index,
				.generation = slots_[index].generation
			};
		}

		[[nodiscard]] bool is_valid(handle_type handle) const
		{
			if (!handle.is_valid())
			{
				return false;
			}

			if (handle.index >= slots_.size())
			{
				return false;
			}

			const resource_slot& slot = slots_[handle.index];

			return slot.alive &&
				!slot.pending_destroy &&
				slot.generation == handle.generation;
		}

		[[nodiscard]] resource_type* try_get(handle_type handle)
		{
			if (!is_valid(handle))
			{
				return nullptr;
			}

			return &slots_[handle.index].resource;
		}

		[[nodiscard]] const resource_type* try_get(
			handle_type handle) const
		{
			if (!is_valid(handle))
			{
				return nullptr;
			}

			return &slots_[handle.index].resource;
		}

		bool request_destroy(handle_type handle)
		{
			if (!is_valid(handle))
			{
				return false;
			}

			resource_slot& slot = slots_[handle.index];

			slot.alive = false;
			slot.pending_destroy = true;

			++slot.generation;

			pending_destroy_indices_.push_back(handle.index);

			return true;
		}

		template <typename release_function>
		void flush_pending_destruction(
			release_function&& release_resource)
		{
			for (u32 index : pending_destroy_indices_)
			{
				resource_slot& slot = slots_[index];

				if (!slot.pending_destroy)
				{
					continue;
				}

				release_resource(slot.resource);

				slot.resource = resource_type{};
				slot.pending_destroy = false;

				free_indices_.push_back(index);
			}

			pending_destroy_indices_.clear();
		}

		template <typename release_function>
		void release_all(release_function&& release_resource)
		{
			for (resource_slot& slot : slots_)
			{
				if (slot.alive || slot.pending_destroy)
				{
					release_resource(slot.resource);
				}

				slot.resource = resource_type{};
				slot.alive = false;
				slot.pending_destroy = false;
			}

			slots_.clear();
			free_indices_.clear();
			pending_destroy_indices_.clear();
		}

		template <typename function_type>
		void for_each_alive(function_type&& function) const
		{
			for (u32 index = 0;
				index < static_cast<u32>(slots_.size());
				++index)
			{
				const resource_slot& slot = slots_[index];

				if (!slot.alive || slot.pending_destroy)
				{
					continue;
				}

				const handle_type handle{
					.index = index,
					.generation = slot.generation
				};

				function(handle, slot.resource);
			}
		}

		[[nodiscard]] usize live_count() const
		{
			usize result = 0;

			for (const resource_slot& slot : slots_)
			{
				if (slot.alive && !slot.pending_destroy)
				{
					++result;
				}
			}

			return result;
		}

	private:
		std::vector<resource_slot> slots_;
		std::vector<u32> free_indices_;
		std::vector<u32> pending_destroy_indices_;
	};




	class rain_window;

	[[nodiscard]] std::unique_ptr<render_backend>create_d3d11_render_backend(rain_window& target_window);

	class d3d11_render_backend final :public render_backend {
	public:
		explicit d3d11_render_backend(rain_window& target_window);
		~d3d11_render_backend()override;

		d3d11_render_backend(const d3d11_render_backend&) = delete;
		d3d11_render_backend& operator=(const d3d11_render_backend&) = delete;
	
		void begin_frame()override;
		void clear(const render_clear_color& color)override;
		void end_frame()override;
		void resize(u32 width, u32 height)override;
		u32 width()const override;
		u32 height()const override;
		
		void draw_debug_triangle()override;

		void create_debug_triangle_resources();
		void create_debug_triangle_vertex_buffer();
		void compile_shader(const char* source,usize source_size,const char* entry_point, const char* target, ID3DBlob** out_blob);

		shader_program_handle create_shader_program(const shader_program_desc& desc)override;
		render_buffer_handle create_vertex_buffer(const render_buffer_desc& desc)override;
		pipeline_state_handle create_pipeline_state(const pipeline_state_desc& desc)override;
		void set_pipeline_state(pipeline_state_handle handle)override;
		void set_vertex_buffer(render_buffer_handle handle)override;

		void update_buffer(render_buffer_handle handle,const void* data,usize size_bytes);

		void draw(u32 vertex_count, u32 start_vertex)override;
		[[nodiscard]] texture_2d_handle create_texture_2d(
			const texture_2d_desc& desc) override;

		bool destroy_shader_program(shader_program_handle handle) override;
		bool destroy_render_buffer(render_buffer_handle handle) override;
		bool destroy_pipeline_state(pipeline_state_handle handle) override;
		bool destroy_texture_2d(texture_2d_handle handle) override;

		[[nodiscard]] bool is_valid(shader_program_handle handle) const override;
		[[nodiscard]] bool is_valid(render_buffer_handle handle) const override;
		[[nodiscard]] bool is_valid(pipeline_state_handle handle) const override;
		[[nodiscard]] bool is_valid(texture_2d_handle handle) const override;

		void flush_resource_destruction() override;

		void set_texture_2d(texture_2d_handle handle, u32 slot);

		void draw_indexed(u32 index_count, u32 start_index, i32 base_vertex)override;
	private:
		void initialize();
		void shutdown();

		void create_device();
		void create_swap_chain();
		void create_render_target_view();
		void create_default_sampler();
		void release_render_resources();
		void set_viewport();
		void create_blend_state(render_blend_mode belnd_mode,ID3D11BlendState** out_blend_state);
		void create_depth_stencil();
		void clear_depth(f32 depth)override;
	private:
		struct d3d11_shader_program {
			std::string name;

			ID3DBlob* vertex_shader_blob = nullptr;
			ID3DBlob* pixel_shader_blob = nullptr;

			ID3D11VertexShader* vertex_shader = nullptr;
			ID3D11PixelShader* pixel_shader = nullptr;
		};

		struct d3d11_render_buffer {

			std::string name;

			ID3D11Buffer* buffer = nullptr;
			render_buffer_bind bind = render_buffer_bind::vertex_buffer;
			render_buffer_usage usage = render_buffer_usage::immutable;
			
			usize size_bytes = 0;
			u32 stride_bytes=0;
	
		};

		struct d3d11_pipeline_state {
			std::string name;
			shader_program_handle shader;
			
			ID3D11InputLayout* input_layout = nullptr;
			ID3D11BlendState* blend_state = nullptr;

			primitive_topology topology = primitive_topology::triangle_list;
			render_blend_mode blend_mode = render_blend_mode::opaque;
			ID3D11RasterizerState* rasterizer_state = nullptr;
			ID3D11DepthStencilState* depth_stencil_state = nullptr;
		};

		struct d3d11_texture_2d {
			std::string name;

			ID3D11Texture2D* texture = nullptr;
			ID3D11ShaderResourceView* shader_resource_view = nullptr;

			u32 width = 0;
			u32 height = 0;
			texture_format format = texture_format::rgba8_unorm;
		};
	private:
		rain_window* target_window_ = nullptr;

		u32 width_ = 0;
		u32 height_ = 0;

		ID3D11Device* device_ = nullptr;
		ID3D11DeviceContext* device_context_ = nullptr;
		IDXGISwapChain* swap_chain_ = nullptr;
		ID3D11RenderTargetView* render_target_view_ = nullptr;

		ID3D11VertexShader* debug_vertex_shader_ = nullptr;
		ID3D11PixelShader* debug_pixel_shader_ = nullptr;
		ID3D11InputLayout* debug_input_layout_ = nullptr;
		ID3D11Buffer* debug_vertex_buffer_ = nullptr;

		shader_program_handle debug_triangle_shader_;
		render_buffer_handle debug_triangle_vertex_buffer_;
		pipeline_state_handle debug_triangle_pipeline_;

		d3d11_resource_pool<shader_program_handle,d3d11_shader_program> shader_programs_;
		d3d11_resource_pool<render_buffer_handle, d3d11_render_buffer>buffers_;
		d3d11_resource_pool<pipeline_state_handle, d3d11_pipeline_state>pipeline_states_;

		shader_program_handle current_shader_;
		render_buffer_handle current_vertex_buffer_;
		pipeline_state_handle current_pipeline_;

		D3D_FEATURE_LEVEL feature_level_ = D3D_FEATURE_LEVEL_11_0;

		d3d11_resource_pool<texture_2d_handle, d3d11_texture_2d>textures_;
		ID3D11SamplerState* default_sampler_ = nullptr;

		std::array<texture_2d_handle, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>current_pixel_textures_{};

		ID3D11Texture2D* depth_texture_ = nullptr;
		ID3D11DepthStencilView* depth_stencil_view_ = nullptr;

	};


}