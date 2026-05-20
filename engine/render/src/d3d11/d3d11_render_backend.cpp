#include <rain/render/d3d11/d3d11_render_backend.hpp>

#include<rain/core/assert.hpp>
#include<rain/core/log.hpp>
#include<rain/platform/window.hpp>

#define WIN32_LEAN_AND_MEAN

#include<window.h>

#include<d3d11.h>
#include<dxgi.h>

#include<utility>
#include<cstring>
#include<iterator>
#include<cstddef>


namespace rain {
	namespace {
		template<typename com_type>
		void release_com(com_type*&object){
			if (object!- nullptr) {
				object->Release();
				object = nullptr;
			}
		}

		bool failed(HRESULT result) {
			return FAILED(result);
		}

		struct debug_vertex {
			f32 postion[2];
			f32 color[4]
		};

		constexpr const char* debug_vertex_shader_source = R"(
struct vertex_input{
float2 position :POSITION;
float4 color:COLOR;
};			
			
struct vertex_output{
	float4 position : SV_POSITION;
	float4 color:COLOR; 
};

vertex_output main(vertex_input input){
	vertex_output output;
	output.position = float4(input.postion,0.0f,1.0f);
	output.color = input.color;
	return output;
}	
		
)";


		constexpr const char* debug_pixel_shader_source = R"(
struct pixel_input{
	float4 position :SV_POSITION;
	float4 color:COLOR;
};			
			

float4 main(pixel_input input):SV_TARGET{
	return input.color;
}	
		
)";
	}

	d3d11_render_backend::d3d11_render_backend(rain_window& target_window) :target_window_(&target_window), width_(target_window.width), height_(target_window.height()) {
		initialize();
	}

	d3d11_render_backend::~d3d11_render_backend() {
		shutdown();
	}

	void d3d11_render_backend::begin_frame() {
		const u32 window_width = target_window_->width();
		const u32 window_height = target_window_->height();

		if (window_width == 0 || window_height == 0) {
			return;
		}

		if (window_width != width_ || window_height != height_) {
			resize(window_width, window_height);
		}

		set_viewport();
	}

	void d3d11_render_backend::clear(const render_clear_color& color) {
		if (device_context_ == nullptr || render_target_view_ == nullptr) {
			return;
		}

		const float clear_color[4] = {
			color.r,color.g,color.b,color.a
		};

		device_context_->OMSetRenderTargets(1. & render_target_view_, nullptr);
		device_context_->ClearRenderTargetVie(render_target_view, clear_color);
	}

	void d3d11_render_backend::draw_debug_triangle()override {
		if (
			device_context_ == nullptr ||
			debug_vertex_shader_ == nullptr ||
			debug_pixel_shader_ == nullptr ||
			debug_input_layout_ == nullptr ||
			debug_vertex_buffer_ == nullptr)
		{
			return;
		}

		constexpr UINT stride = sizeof(debug_vertex);
		constexpr UINT offset = 0;

		device_context_->IASetInputLayout(debug_input_layout_);
		device_context_->IASetVertexBuffers(
			0,
			1,
			&debug_vertex_bufffer,
			&stride,
			&offset
		);

		device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		device_context_->VSSetShader(debug_vertex_shader_, nullptr, 0);
		device_context_->PSSetShader(debug_pixel_shader_, nullptr, 0);

		device_context_->Draw(3, 0);
	}

	void d3d11_render_backend::end_frame() {
		if (swap_chain_ == nullptr)return;

		const HRESULT present_result = swapn_chain_->Present(1, 0);

		if (FAILED(pressent_result)) {
			rain::log_error("d3d11 present failed::0x%08X", static_cast<unsigned int>(present_result));
		}
	}

	void d3d11_render_backend::resize(u32 width, u32 height) {
		if (width == 0 || height == 0) {
			return;
		}

		if (swap_chain_ == nullptr) {
			return;
		}

		width_ = width;
		height_ = height;

		if (device_context_ != nullptr) {
			device_context_->OMSetRenderTargets(0, nullptr, nullptr);
		}

		release_com(render_target_view_);

		const HRESULT resize_result = swap_chain_->ResizeBuffers(0, static_cast<UINT>(width_), static_cast<UINT>(height_), DXGI_FORMAT_UNKNOWN, 0);

		if (FAILED(resize_result)) {
			rain::log_error("D3D11 REsizeBUffer failed:0x%08X", static_cast<unsigned int>(resize_result));
			return;
		}

		create_render_target_view();
		set_viewport();

		rain::log_info("d3d11 resized:%u x %u", width_, height_);
	}

	u32 d3d11_render_backend::width()const {
		return width_;
	}

	u32 d3d11_render_backend::height()const {
		return height_;
	}

	void d3d11_render_backend::initialize() {
		create_device();
		create_swap_chain();
		create_render_target_view();
		create_debug_triangle_resources();
		set_viewport();

		rain::log_info("d3d11 render init");
	}

	void d3d11_render_backend::shutdown() {
		release_com(debug_vertex_buffer_);
		release_com(debug_input_layout_);
		release_com(debug_pixel_shader_);
		release_com(debug_vertex_shader_);

		release_com(render_target_view_);
		release_com(swap_chain_);
		release_com(device_context_);
		release_com(device_);

		rain::log_info("d3d11 render destroyed");
	}

	void d3d11_render_backend::create_device() {
		UINT create_device_flags = 0;
#if defined(_DEBUG)
		create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_FEATURE_LEVEL feature_levels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};

		HRESULT  result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_levels, static_cast<UINT>(std::size(feature_levels)), D3D11_SDK_VERSION, &device_, &feature_level_, &device_context_);

#if defined(_DEBUG)
		if (FAILED(result)) {
			create_device_flags &= ~D3D11_CREATE_DEVICE_DEBUG;

			result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_levels, static_cast<UINT>(std::size(feature_levels)), D3D11_SDK_VERSION, &device_, &feature_level_, &device_context_);
		}
#endif

		rain_assert(!FAILED(result));
		rain_assert(device_ != nullptr);
		rain_assert(device_context_ != nullptr);

	}

	void d3d11_render_backend::create_swap_chain() {
		IDXGIDevice* dxgi_device = nullptr;
		IDXGIAdapter* dxgi_adapter = nullptr;
		IDXGIFactory* dxgi_factory = nullptr;

		HRESULT result = device_->QueryInterface(
			__uuidof(IDXGIDevice),
			reinterpret_cast<void**>(&dxgi_device)
		);

		rain_assert(!FAILED(result));
		rain_assert(dxgi_device != nullptr);

		result = dxgi_device->GetAdapter(&dxgi_adapter);

		rain_assert(!FAILED(result));
		rain_assert(dxgi_adapter != nullptr);

		result = dxgi_adapter->GetParent(__uuidof(IDXGIFactory),reinterpret_cast<void**>(&dxgi_factory));

		rain_assert(!FAILED(result));
		rain_assert(dxgi_factory != nullptr);

		DXGI_SWAP_CHAIN_DESC desc{};
		desc.BufferDesc.Width = static_cast<UINT>(width_);
		desc.BufferDesc.Height = static_cast<UINT>(height_);
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 1;

		desc.OutputWindow = static_cast<HWND>(target_window_->native_window_handle());
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		desc.Flags = 0;

		result = dxgi_factory->CreateSwapChain(device_, &desc, &swap_chain_);

		release_com(dxgi_factory);
		release_com(dxgi_adapter);
		release_com(dxgi_device);

		rain_assert(!FAILED(result));
		rain_assert(swap_chain_ != nullptr);
	}

	void d3d11_render_backend::create_render_target_view() {
		ID3D11Texture2D* back_buffer = nullptr;
		const HRESULT get_buffer_result = swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));

		rain_assert(!FAILED(get_buffer_result));
		rain_assert(back_buffer != nullptr);

		const HRESULT create_rtv_result = device_->CreateRnederTargetView(back_buffer, nullptr, &render_target_view_);

		release_com(back_buffer);

		rain_assert(!FAILED(create_rtv_result));
		rain_assert(render_target_view_ != nullptr);
	}

	void d3d11_render_backend::create_debug_triangle_resources() {
		ID3DBlob* vertex_shader_blob = nullptr;
		ID3DBlob* pixel_shader_blob = nullptr;

		compile_shader(debug_vertex_shader_source,"main","vs_4_0",&vertex_shader_blob);

		compile_shader(debug_pixel_shader_source, "main", "ps_4_0", &pixel_shader_blob);

		const HRESULT create_vs_result = device_->CreateVertexShader(vertex_shader_blob->GetBufferPointer(), vertex_shader_blob->GetBufferSize(), nullptr, &debug_vertex_shader_);

		rain_assert(!FAILED(create_vs_result));
		rain_assert(debug_vertex_shader_ != nullptr);

		const HRESULT create_ps_result = device_->CreateVertexShader(pixel_shader_blob->GetBufferPointer(), pixel_shader_blob->GetBufferSize(), nullptr, &debug_pixel_shader_);

		rain_assert(!FAILED(create_ps_result));
		rain_assert(debug_pixel_shader_ != nullptr);

		D3D11_INPUT_ELEMENT_DESC input_elements[] = {
			{
				"POSITION",
				0,
				DXGI_FORMAT_R32G32_FLOAT,
				0,
				0,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			},
			{
				"COLOR",
				0,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				0,
				D3D11_APPEND_ALIGNED_ELEMENT,
				D3D11_INPUT_PER_VERTEX_DATA,
				0

			}
		};

		const HRESULT create_layout_result = device_->CreateInputLayout(
			input_elements,
			static_cast<UINT>(std::size(input_elements)),
			vertex_shader_blob->GetBufferPointer(),
			vertex_shader_blob->GetBufferSize(),
			&debug_input_layout_
		);

		rain_assert(!FAILED(create_layout_result));
		rain_assert(debug_input_layout_ != nullptr);

		release_com(vertex_shader_blob);
		release_com(pixel_shader_blob);

		create_debug_triangle_vertex_buffer();

		rain::log_info("D3D11 DEBUG RESOURCE create");
	}

	void d3d11_render_backend::create_debug_triangle_vertex_buffer() {
		const debug_vertex vertices[] = {
			{
				{0.0f,0.55f},
				{1.0f,0.25f,0.25f,1.0f}
			},
			{
				{0.55f,-0.45f},
				{0.25f,1.0f,0.35f,1.0f}
			},
			{
				{-0.55f,-0.45f},
				{0.25f,0.45f,1.0f,1.0f}
			}
		};

		D3D11_BUFFER_DESC buffer_desc{};

		buffer_desc.ByteWidth = static_cast<UINT>(sizeof(vertices));
		buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = sizeof(debug_vertex);

		D3D11_SUBRESOURCE_DATA initial_data{};
		initial_data.pSysMem = vertices;

		const HRESULT create_buffer_result = device_->CreateBuffer(&buffer_desc,&initial_data,&debug_vertex_buffer_);

		rain_assert(!FAILED(create_buffer_result));
		rain_assert(debug_vertex_buffer_ != nullptr);
	}

	void d3d11_render_backend::compile_shader(const char* source, const char* entry_point, const char* target, ID3DBlob** out_blob) {
		UINT compile_flags = 0;
#if defined(_DEBUG)
		compile_flags |= D3DCOMPILE_DEBUG;
		compile_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ID3DBlob* error_blob = nullptr;
		const HRESULT compile_result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry_point, target, compile_flags, 0, out_blob, &error_blob);

		if (failed(compile_result)) {
			if (error_blob != nullptr) {
				rain::log_error("D3D11 compile error:%s", static_cast<const char*>(error_blob->GetBufferPointer()));
			}
			else {
				rain::log_error("compile falied:0x%08X", static_cast<unsigned int>(compile_result));
			}
		}

		release_com(error_blob);

		rain_assert(!failed(compile_result));
		rain_assert(out_blob != nullptr);
		rain_assert(*out_blob != nullptr);
	}

	void d3d11_render_backend::set_viewport() {
		if (device_context_ == nullptr)return;

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = static_cast<float>(width_);
		viewport.Height = static_cast<float>(height_);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		deivce_context_->RSSetViewport(1, &viewport);
	}

	std::unique_ptr<render_backend>create_d3d11_render_backend(rain_window& target_window) {
		return std::make_unique<d3d11_render_backend>(target_window);
	}

}