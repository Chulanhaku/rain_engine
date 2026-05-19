#pragma once

#include<rain/core/types.hpp>
#include<rain/render/render_clear_color.hpp>

namespace rain {
	class render_backend {
	public:
		virtual~render_backend() = default;
		virtual void begin_frame() = 0;
		virtual void clear(const render_clear_color& color) = 0;
		virtual void end_frame() = 0;

		virtual void resize() = 0;

		[[nodiscard]] virtual u32 width()const = 0;
		[[nodiscard]] virtual u32 height()const = 0;
	};
}