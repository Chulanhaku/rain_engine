#pragma once
#include<rain/core/types.hpp>
#include<rain/render/render_backend.hpp>
#include<memory>
#include<d3d11.h>
#include<dxgi.h>
#include<d3dcompiler.h>
#include<vector>

namespace rain {
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

		void draw(u32 vertex_count, u32 start_vertex)override;

	private:
		void initialize();
		void shutdown();

		void create_device();
		void create_swap_chain();
		void create_render_target_view();
		void release_render_resources();
		void set_viewport();
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
			u32 stride_bytes=0;
		};

		struct d3d11_pipeline_state {
			std::string name;
			shader_program_handle shader;
			ID3D11InputLayout* input_layout = nullptr;
			primitive_topology topology = primitive_topology::triangle_list;
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

		std::vector<d3d11_shader_program> shader_programs_;
		std::vector<d3d11_render_buffer>buffers_;
		std::vector<d3d11_pipeline_state>pipeline_states_;

		shader_program_handle debug_triangle_shader_;
		render_buffer_handle debug_triangle_vertex_buffer_;
		pipeline_state_handle debug_triangle_pipeline_;

		D3D_FEATURE_LEVEL feature_level_ = D3D_FEATURE_LEVEL_11_0;
	};


}