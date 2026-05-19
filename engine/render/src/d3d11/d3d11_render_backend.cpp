#include <rain/render/d3d11/d3d11_render_backend.hpp>

#include<rain/core/assert.hpp>
#include<rain/core/log.hpp>
#include<rain/platform/window.hpp>

#define WIN32_LEAN_AND_MEAN

#include<window.h>

#include<d3d11.h>
#include<dxgi.h>

#include<utility>

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

		rain::log_info("d3d11 render init");
	}

	void d3d11_render_backend::shutdown() {
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
}