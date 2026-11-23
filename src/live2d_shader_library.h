#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/shader.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "scene/resources/shader.h"
#endif

#include <Live2DCubismCore.hpp>
#include <Rendering/csmBlendMode.hpp>

class Live2DShaderLibrary {
private:
	static constexpr int32_t COLOR_BLEND_COUNT = Live2D::Cubism::Core::csmColorBlendType_Color + 1;
	static constexpr int32_t ALPHA_BLEND_COUNT = Live2D::Cubism::Core::csmAlphaBlendType_DisjointOver + 1;
	static constexpr int32_t SHADER_COUNT = COLOR_BLEND_COUNT * ALPHA_BLEND_COUNT * 2;

	static Ref<Shader> shaders[SHADER_COUNT];

	static Ref<Shader> setup_mask_shader;

	static int32_t _resolve_shader_index(int32_t &r_color_blend, int32_t &r_alpha_blend, bool p_masked);

	static Ref<Shader> _get_shader(int32_t p_color_blend, int32_t p_alpha_blend, bool p_masked);

public:
	static Ref<Shader> get_drawable_shader(const Csm::csmBlendMode &p_blend_mode, bool p_masked);

	static Ref<Shader> get_mask_shader();

	static bool requires_backbuffer(const Csm::csmBlendMode &p_blend_mode);

	static void release();
};
