#include <rain/render/d3d11/d3d11_render_backend.hpp>

#include<rain/core/assert.hpp>
#include<rain/core/log.hpp>
#include<rain/platform/window.hpp>

#define WIN32_LEAN_AND_MEAN

#include<windows.h>

#include<d3d11.h>
#include<dxgi.h>

#include<utility>
#include<cstring>
#include<iterator>
#include<cstddef>
#include<vector>


namespace rain {
	namespace {
		template<typename com_type>
		void release_com(com_type*&object){
			if (object!= nullptr) {
				object->Release();
				object = nullptr;
			}
		}

		bool failed(HRESULT result) {
			return FAILED(result);
		}

		[[nodiscard]] const char* to_d3d_shader_target(shader_target target) {
			switch (target) {
			case shader_target::vertex_shader_4_0:
				return"vs_4_0";
			case shader_target::pixel_shader_4_0:
				return"ps_4_0";
			}

			return"";
		}

		[[nodiscard]] DXGI_FORMAT to_dxgi_format(vertex_attribute_format format) {
			switch (format)
			{
			case vertex_attribute_format::r32g32_float:
				return DXGI_FORMAT_R32G32_FLOAT;
			case vertex_attribute_format::r32g32b32_float:
				return DXGI_FORMAT_R32G32B32_FLOAT;
			case vertex_attribute_format::r32g32b32a32_float:
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			return DXGI_FORMAT_UNKNOWN;

		}

		[[nodiscard]] D3D11_PRIMITIVE_TOPOLOGY to_d3d_topology(primitive_topology topology)
		{
			switch (topology)
			{
			case primitive_topology::triangle_list:
				return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case primitive_topology::line_list:
				return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			}

			return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

		[[nodiscard]] D3D11_USAGE to_d3d_usage(render_buffer_usage usage)
		{
			switch (usage)
			{
			case render_buffer_usage::immutable:
				return D3D11_USAGE_IMMUTABLE;
			case render_buffer_usage::dynamic:
				return D3D11_USAGE_DYNAMIC;
			}

			return D3D11_USAGE_DEFAULT;
		}

		[[nodiscard]] UINT to_d3d_bind_flags(render_buffer_bind bind)
		{
			switch (bind)
			{
			case render_buffer_bind::vertex_buffer:
				return D3D11_BIND_VERTEX_BUFFER;
			case render_buffer_bind::index_buffer:
				return D3D11_BIND_INDEX_BUFFER;
			}

			return 0;
		}

		struct debug_vertex {
			f32 position[2];
			f32 color[4];
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
	output.position = float4(input.position,0.0f,1.0f);
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

	d3d11_render_backend::d3d11_render_backend(rain_window& target_window) :target_window_(&target_window), width_(target_window.width()), height_(target_window.height()) {
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

		device_context_->OMSetRenderTargets(1, & render_target_view_, nullptr);
		device_context_->ClearRenderTargetView(render_target_view_, clear_color);
	}

	void d3d11_render_backend::draw_debug_triangle() {
		set_pipeline_state(debug_triangle_pipeline_);
		set_vertex_buffer(debug_triangle_vertex_buffer_);
		draw(3, 0);
	}

	void d3d11_render_backend::end_frame() {
		if (swap_chain_ == nullptr)return;

		const HRESULT present_result = swap_chain_->Present(1, 0);

		if (FAILED(present_result)) {
			rain::log_error("d3d11 present failed::0x%08X");
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
			rain::log_error("D3D11 REsizeBUffer failed:0%08X");
			return;
		}

		create_render_target_view();
		set_viewport();

		rain::log_info("d3d11 resized xu");
	}

	shader_program_handle d3d11_render_backend::create_shader_program(const shader_program_desc& desc) {
		d3d11_shader_program shader_program;
		shader_program.name = desc.name;

		ID3DBlob* vertex_blob = nullptr;
		ID3DBlob* pixel_blob = nullptr;

		compile_shader(
			desc.vertex_source.c_str(),
			desc.vertex_source.size(),
			desc.vertex_entry.c_str(),
			to_d3d_shader_target(desc.vertex_target),
			&vertex_blob
		);

		compile_shader(
			desc.pixel_source.c_str(),
			desc.pixel_source.size(),
			desc.pixel_entry.c_str(),
			to_d3d_shader_target(desc.pixel_target),
			&pixel_blob
		);

		const HRESULT create_vs_result = device_->CreateVertexShader(
			vertex_blob->GetBufferPointer(),
			vertex_blob->GetBufferSize(),
			nullptr,
			&shader_program.vertex_shader
		);

		rain_assert(!failed(create_vs_result));
		rain_assert(shader_program.vertex_shader!=nullptr);

		const HRESULT create_ps_result = device_->CreatePixelShader(
			pixel_blob->GetBufferPointer(),
			pixel_blob->GetBufferSize(),
			nullptr,
			&shader_program.pixel_shader
		);

		rain_assert(!failed(create_ps_result));
		rain_assert(shader_program.pixel_shader != nullptr);

		shader_program.vertex_shader_blob = vertex_blob;
		shader_program.pixel_shader_blob = pixel_blob;

		const u32 index = static_cast<u32>(shader_programs_.size());
		shader_programs_.push_back(shader_program);

		rain::log_info("D3D11 SHADER PROGRAM CREATED");

		return shader_program_handle{
			.index = index,
			.generation=0
		};
	}

	render_buffer_handle d3d11_render_backend::create_vertex_buffer(const render_buffer_desc& desc) {
		rain_assert(desc.bind==render_buffer_bind::vertex_buffer);
		rain_assert(desc.size_bytes > 0);
		rain_assert(desc.stride_bytes>0);

		if (desc.usage == render_buffer_usage::immutable) {
			rain_assert(desc.initial_data != nullptr);
		}
		else {

		}
		d3d11_render_buffer buffer;
		buffer.name = desc.name;
		buffer.bind = desc.bind;
		buffer.stride_bytes = desc.stride_bytes;
		buffer.usage = desc.usage;
		buffer.size_bytes = desc.size_bytes;


		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.ByteWidth = static_cast<UINT>(desc.size_bytes);
		buffer_desc.Usage = to_d3d_usage(desc.usage);
		buffer_desc.BindFlags = to_d3d_bind_flags(desc.bind);
		buffer_desc.CPUAccessFlags = desc.usage == render_buffer_usage::dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = desc.stride_bytes;

		D3D11_SUBRESOURCE_DATA initial_data{};
		D3D11_SUBRESOURCE_DATA* initial_data_ptr = nullptr;

		if (desc.initial_data != nullptr)
		{
			initial_data.pSysMem = desc.initial_data;
			initial_data_ptr = &initial_data;
		}

		const HRESULT create_buffer_result = device_->CreateBuffer(&buffer_desc, initial_data_ptr,&buffer.buffer);

		rain_assert(!failed(create_buffer_result));
		rain_assert(buffer.buffer != nullptr);

		const u32 index = static_cast<u32>(buffers_.size());
		buffers_.push_back(buffer);

		rain::log_info("d3d11 buffer created");

		return render_buffer_handle{
			.index = index,
			.generation = 0
		};

	}

	void d3d11_render_backend::update_buffer(render_buffer_handle handle,const void* data,usize size_bytes) {
		if (!handle.is_valid() || handle.index >= buffers_.size())return;

		if (data == nullptr || size_bytes == 0) return;

		d3d11_render_buffer& buffer = buffers_[handle.index];

		rain_assert(buffer.buffer!=nullptr);
		rain_assert(buffer.usage == render_buffer_usage::dynamic);
		rain_assert(size_bytes <= buffer.size_bytes);

		D3D11_MAPPED_SUBRESOURCE mapped_resource{};

		const HRESULT map_result = device_context_->Map(buffer.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

		rain_assert(!failed(map_result));
		rain_assert(mapped_resource.pData != nullptr);

		std::memcpy(mapped_resource.pData, data, size_bytes);

		device_context_->Unmap(buffer.buffer, 0);
	}

	pipeline_state_handle d3d11_render_backend::create_pipeline_state(const pipeline_state_desc& desc) {
		rain_assert(desc.shader.is_valid());
		rain_assert(desc.shader.index < shader_programs_.size());
		rain_assert(!desc.vertex_attributes.empty());

		d3d11_pipeline_state pipeline;
		pipeline.name = desc.name;
		pipeline.shader = desc.shader;
		pipeline.topology = desc.topology;

		d3d11_shader_program& shader_program = shader_programs_[desc.shader.index];

		std::vector<D3D11_INPUT_ELEMENT_DESC>input_elements;
		input_elements.reserve(desc.vertex_attributes.size());

		for (const vertex_attribute_desc& attribute : desc.vertex_attributes) {
			D3D11_INPUT_ELEMENT_DESC element{};
			element.SemanticName = attribute.semantic_name.c_str();
			element.SemanticIndex = attribute.semantic_index;
			element.Format = to_dxgi_format(attribute.format);
			element.InputSlot = attribute.input_slot;
			element.AlignedByteOffset = attribute.offset_bytes;
			element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;

			input_elements.push_back(element);

		}

		const HRESULT create_layout_result = device_->CreateInputLayout(
			input_elements.data(),
			static_cast<UINT>(input_elements.size()),
			shader_program.vertex_shader_blob->GetBufferPointer(),
			shader_program.vertex_shader_blob->GetBufferSize(),
			&pipeline.input_layout
		);

		rain_assert(!failed(create_layout_result));
		rain_assert(pipeline.input_layout != nullptr);

		const u32 index = static_cast<u32>(pipeline_states_.size());
		pipeline_states_.push_back(pipeline);

		rain::log_info("D3D11 pipeline state created");

		return pipeline_state_handle{
			.index = index,
			.generation = 0
		};

	}

	void d3d11_render_backend::set_pipeline_state(pipeline_state_handle handle) {
		if (!handle.is_valid() || handle.index >= pipeline_states_.size())return;

		d3d11_pipeline_state& pipeline = pipeline_states_[handle.index];

		if (!pipeline.shader.is_valid() || pipeline.shader.index >= shader_programs_.size())return;

		d3d11_shader_program& shader_program = shader_programs_[pipeline.shader.index];

		device_context_->IASetInputLayout(pipeline.input_layout);
		device_context_->IASetPrimitiveTopology(to_d3d_topology(pipeline.topology));
		device_context_->VSSetShader(shader_program.vertex_shader, nullptr, 0);
		device_context_->PSSetShader(shader_program.pixel_shader, nullptr, 0);
	}

	void d3d11_render_backend::set_vertex_buffer(render_buffer_handle handle) {
		if (!handle.is_valid() || handle.index >= buffers_.size())return;

		d3d11_render_buffer& buffer = buffers_[handle.index];

		if (buffer.buffer == nullptr)return;

		const UINT stride = buffer.stride_bytes;
		constexpr UINT offset = 0;

		device_context_->IASetVertexBuffers(
			0,
			1,
			&buffer.buffer,
			&stride,
			&offset
		);

	}

	void d3d11_render_backend::draw(u32 vertex_count, u32 start_vertex){
		device_context_->Draw(
			static_cast<UINT>(vertex_count),
			static_cast<UINT>(start_vertex)
		);
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
		release_render_resources();

		release_com(render_target_view_);
		release_com(swap_chain_);
		release_com(device_context_);
		release_com(device_);

		rain::log_info("d3d11 render destroyed");
	}

	void d3d11_render_backend::release_render_resources() {
		for (d3d11_pipeline_state& pipeline : pipeline_states_) {
			release_com(pipeline.input_layout);
		}

		for (d3d11_render_buffer& buffer : buffers_) {
			release_com(buffer.buffer);
		}

		for (d3d11_shader_program& shader_program : shader_programs_) {
			release_com(shader_program.pixel_shader);
			release_com(shader_program.vertex_shader);
			release_com(shader_program.pixel_shader_blob);
			release_com(shader_program.vertex_shader_blob);
		}

		pipeline_states_.clear();
		buffers_.clear();
		shader_programs_.clear();
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

		const HRESULT create_rtv_result = device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_view_);

		release_com(back_buffer);

		rain_assert(!FAILED(create_rtv_result));
		rain_assert(render_target_view_ != nullptr);
	}

	void d3d11_render_backend::create_debug_triangle_resources() {
		debug_triangle_shader_ = create_shader_program(shader_program_desc{
			.name = "debug_triangle_shader",
			.vertex_source = debug_vertex_shader_source,
			.vertex_entry = "main",
			.vertex_target = shader_target::vertex_shader_4_0,
			.pixel_source = debug_pixel_shader_source,
			.pixel_entry = "main",
			.pixel_target = shader_target::pixel_shader_4_0
			});

		const debug_vertex vertices[] = {
			{
				{0.0f, 0.55f},
				{1.0f, 0.25f, 0.25f, 1.0f}
			},
			{
				{0.55f, -0.45f},
				{0.25f, 1.0f, 0.35f, 1.0f}
			},
			{
				{-0.55f, -0.45f},
				{0.25f, 0.45f, 1.0f, 1.0f}
			}
		};

		debug_triangle_vertex_buffer_ = create_vertex_buffer(render_buffer_desc{
			.name = "debug_triangle_vertex_buffer",
			.bind = render_buffer_bind::vertex_buffer,
			.usage = render_buffer_usage::immutable,
			.size_bytes = sizeof(vertices),
			.stride_bytes = sizeof(debug_vertex),
			.initial_data = vertices
			});

		debug_triangle_pipeline_ = create_pipeline_state(pipeline_state_desc{
			.name = "debug_triangle_pipeline",
			.shader = debug_triangle_shader_,
			.vertex_attributes = {
				vertex_attribute_desc{
					.semantic_name = "POSITION",
					.semantic_index = 0,
					.format = vertex_attribute_format::r32g32_float,
					.input_slot = 0,
					.offset_bytes = offsetof(debug_vertex, position)
				},
				vertex_attribute_desc{
					.semantic_name = "COLOR",
					.semantic_index = 0,
					.format = vertex_attribute_format::r32g32b32a32_float,
					.input_slot = 0,
					.offset_bytes = offsetof(debug_vertex, color)
				}
			},
			.topology = primitive_topology::triangle_list
			});

		rain::log_info("D3D11 debug triangle resources created");
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

	void d3d11_render_backend::compile_shader(const char* source,usize source_size, const char* entry_point, const char* target, ID3DBlob** out_blob) {
		UINT compile_flags = 0;
#if defined(_DEBUG)
		compile_flags |= D3DCOMPILE_DEBUG;
		compile_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ID3DBlob* error_blob = nullptr;
		const HRESULT compile_result = D3DCompile(source, source_size, nullptr, nullptr, nullptr, entry_point, target, compile_flags, 0, out_blob, &error_blob);

		if (failed(compile_result)) {
			if (error_blob != nullptr) {
				rain::log_error("D3D11 compile error:s");
			}
			else {
				rain::log_error("compile falied:0x08X");
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

		device_context_->RSSetViewports(1, &viewport);
	}

	std::unique_ptr<render_backend>create_d3d11_render_backend(rain_window& target_window) {
		return std::make_unique<d3d11_render_backend>(target_window);
	}

}