#pragma once
#include<rain/core/types.hpp>
#include<rain/render/render_backend.hpp>
#include<memory>
#include<d3d11.h>
#include<dxgi.h>


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
		void compile_shader(const char* source, const char* entry_point, const char* target, ID3DBlob** out_blob);
	private:
		void initialize();
		void shutdown();

		void create_device();
		void create_swap_chain();
		void create_render_tagrte_view;
	private:
		rain_window* tagrte_window_ = nullptr;

		u32 width_ = 0;
		u32 height_ = 0;

		ID3D11Device* device_ = nullptr;
		ID3D11DeviceContext* device_context_ = nullptr;
		IDXGISwapChain* swap_chain_ = nullptr;
		ID3D11RenderTargetView* render_target_view_ = nullptr;

		ID3D11VertexShader* debug_vertex_shader_ = nullptr;
		ID3D11PixelShader* debug_pixel_shader_ = nullptr;
		ID3D11InputLayout* debug_input_layout_ = nullptr;
		ID3D11Buffer* debug_vertext_buffer_ = nullptr;

		D3D_FEATURE_LEVEL feature_level_ = D3D_FEATURE_LEVEL_11_0;
	};


}