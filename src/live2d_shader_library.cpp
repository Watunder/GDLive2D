#include "live2d_shader_library.h"

namespace {
static const char *LIVE2D_SHADER_SOURCE = R"(
shader_type canvas_item;

#if defined(GODOT_BLEND_ADD)
render_mode blend_add;
#elif defined(GODOT_BLEND_MULTIPLY)
render_mode blend_mul;
#elif defined(CUSTOM_BLEND_MODE)
render_mode blend_disabled;
#else
render_mode blend_mix;
#endif

uniform vec4 multiply_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec4 screen_color = vec4(0.0, 0.0, 0.0, 0.0);

#ifdef CUSTOM_BLEND_MODE
uniform sampler2D blend_texture : hint_screen_texture, repeat_disable, filter_nearest;
#endif

#ifdef MASKED
uniform sampler2D mask_texture : filter_linear, repeat_disable;
uniform vec4 mask_channel = vec4(1.0, 1.0, 1.0, 1.0);
uniform float mask_inverted = 0.0;
uniform vec4 clip_row0 = vec4(1.0, 0.0, 0.0, 0.0);
uniform vec4 clip_row1 = vec4(0.0, 1.0, 0.0, 0.0);
varying vec2 live2d_mask_uv;
#endif

vec4 live2d_apply_colors(vec4 color) {
    color.rgb *= multiply_color.rgb;
    color.rgb = color.rgb + screen_color.rgb - (color.rgb * screen_color.rgb);
    return color;
}

#ifdef MASKED
void vertex() {
    vec4 position = vec4(VERTEX, 0.0, 1.0);
    live2d_mask_uv = vec2(dot(clip_row0, position), dot(clip_row1, position));
}

float live2d_mask_value() {
    vec4 mask_color = texture(mask_texture, live2d_mask_uv);
    float value = dot(vec4(1.0) - mask_color, mask_channel);
    return mask_inverted > 0.5 ? 1.0 - value : value;
}
#endif

#ifdef CUSTOM_BLEND_MODE
vec4 live2d_premultiplied_to_straight(vec4 source) {
    if (abs(source.a) < 0.00001) {
        source.rgb = vec3(0.0);
    } else {
        source.rgb /= source.a;
    }
    return source;
}

float live2d_color_burn(float source, float destination) {
    if (abs(destination - 1.0) < 0.000001) {
        return 1.0;
    } else if (abs(source) < 0.000001) {
        return 0.0;
    }
    return 1.0 - min(1.0, (1.0 - destination) / source);
}

float live2d_color_dodge(float source, float destination) {
    if (destination <= 0.0) {
        return 0.0;
    } else if (source == 1.0) {
        return 1.0;
    }
    return min(1.0, destination / (1.0 - source));
}

float live2d_overlay(float source, float destination) {
    float mul = 2.0 * source * destination;
    float scr = 1.0 - 2.0 * (1.0 - source) * (1.0 - destination);
    return destination < 0.5 ? mul : scr;
}

float live2d_soft_light(float source, float destination) {
    float val1 = destination - (1.0 - 2.0 * source) * destination * (1.0 - destination);
    float val2 = destination + (2.0 * source - 1.0) * destination * ((16.0 * destination - 12.0) * destination + 3.0);
    float val3 = destination + (2.0 * source - 1.0) * (sqrt(destination) - destination);
    if (source <= 0.5) {
        return val1;
    } else if (destination <= 0.25) {
        return val2;
    }
    return val3;
}

float live2d_hard_light(float source, float destination) {
    float mul = 2.0 * source * destination;
    float scr = 1.0 - 2.0 * (1.0 - source) * (1.0 - destination);
    return source < 0.5 ? mul : scr;
}

float live2d_linear_light(float source, float destination) {
    float burn = max(0.0, 2.0 * source + destination - 1.0);
    float dodge = min(1.0, 2.0 * (source - 0.5) + destination);
    return source < 0.5 ? burn : dodge;
}

float live2d_get_max(vec3 color) {
    return max(color.r, max(color.g, color.b));
}

float live2d_get_min(vec3 color) {
    return min(color.r, min(color.g, color.b));
}

float live2d_saturation(vec3 color) {
    return live2d_get_max(color) - live2d_get_min(color);
}

float live2d_luma(vec3 color) {
    return 0.30 * color.r + 0.59 * color.g + 0.11 * color.b;
}

vec3 live2d_clip_color(vec3 color) {
    float luma = live2d_luma(color);
    float maxv = live2d_get_max(color);
    float minv = live2d_get_min(color);
    vec3 output_color = color;
    output_color = minv < 0.0 ? luma + (output_color - luma) * luma / (luma - minv) : output_color;
    output_color = maxv > 1.0 ? luma + (output_color - luma) * (1.0 - luma) / (maxv - luma) : output_color;
    return output_color;
}

vec3 live2d_set_luma(vec3 color, float luma) {
    return live2d_clip_color(color + (luma - live2d_luma(color)));
}

vec3 live2d_set_saturation(vec3 color, float saturation) {
    float maxv = live2d_get_max(color);
    float minv = live2d_get_min(color);
    float medv = color.r + color.g + color.b - maxv - minv;
    float output_max = minv < maxv ? saturation : 0.0;
    float output_med = minv < maxv ? (medv - minv) * saturation / (maxv - minv) : 0.0;
    float output_min = 0.0;
    if (color.r == maxv) {
        return color.b < color.g ? vec3(output_max, output_med, output_min) : vec3(output_max, output_min, output_med);
    } else if (color.g == maxv) {
        return color.r < color.b ? vec3(output_min, output_max, output_med) : vec3(output_med, output_max, output_min);
    }
    return color.g < color.r ? vec3(output_med, output_min, output_max) : vec3(output_min, output_med, output_max);
}

#ifndef COLOR_BLEND_MODE
#error CUSTOM_BLEND_MODE requires COLOR_BLEND_MODE.
#endif

#if COLOR_BLEND_MODE == 0 // csmColorBlendType_Normal
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return source;
}
#elif COLOR_BLEND_MODE == 3 // csmColorBlendType_Add
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return min(source + destination, vec3(1.0));
}
#elif COLOR_BLEND_MODE == 4 // csmColorBlendType_AddGlow
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return source + destination;
}
#elif COLOR_BLEND_MODE == 5 // csmColorBlendType_Darken
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return min(source, destination);
}
#elif COLOR_BLEND_MODE == 6 // csmColorBlendType_Multiply
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return source * destination;
}
#elif COLOR_BLEND_MODE == 7 // csmColorBlendType_ColorBurn
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return vec3(live2d_color_burn(source.r, destination.r), live2d_color_burn(source.g, destination.g), live2d_color_burn(source.b, destination.b));
}
#elif COLOR_BLEND_MODE == 8 // csmColorBlendType_LinearBurn
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return max(vec3(0.0), source + destination - 1.0);
}
#elif COLOR_BLEND_MODE == 9 // csmColorBlendType_Lighten
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return max(source, destination);
}
#elif COLOR_BLEND_MODE == 10 // csmColorBlendType_Screen
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return source + destination - source * destination;
}
#elif COLOR_BLEND_MODE == 11 // csmColorBlendType_ColorDodge
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return vec3(live2d_color_dodge(source.r, destination.r), live2d_color_dodge(source.g, destination.g), live2d_color_dodge(source.b, destination.b));
}
#elif COLOR_BLEND_MODE == 12 // csmColorBlendType_Overlay
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return vec3(live2d_overlay(source.r, destination.r), live2d_overlay(source.g, destination.g), live2d_overlay(source.b, destination.b));
}
#elif COLOR_BLEND_MODE == 13 // csmColorBlendType_SoftLight
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return vec3(live2d_soft_light(source.r, destination.r), live2d_soft_light(source.g, destination.g), live2d_soft_light(source.b, destination.b));
}
#elif COLOR_BLEND_MODE == 14 // csmColorBlendType_HardLight
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return vec3(live2d_hard_light(source.r, destination.r), live2d_hard_light(source.g, destination.g), live2d_hard_light(source.b, destination.b));
}
#elif COLOR_BLEND_MODE == 15 // csmColorBlendType_LinearLight
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return vec3(live2d_linear_light(source.r, destination.r), live2d_linear_light(source.g, destination.g), live2d_linear_light(source.b, destination.b));
}
#elif COLOR_BLEND_MODE == 16 // csmColorBlendType_Hue
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return live2d_set_luma(live2d_set_saturation(source, live2d_saturation(destination)), live2d_luma(destination));
}
#elif COLOR_BLEND_MODE == 17 // csmColorBlendType_Color
vec3 live2d_color_blend(vec3 source, vec3 destination) {
    return live2d_set_luma(source, live2d_luma(destination));
}
#else
#error Unsupported COLOR_BLEND_MODE.
#endif

vec4 live2d_overlap_rgba(vec3 color, vec3 source, vec3 destination, vec3 parameter) {
    return vec4(color * parameter.x + source * parameter.y + destination * parameter.z, parameter.x + parameter.y + parameter.z);
}

#ifndef ALPHA_BLEND_MODE
#error CUSTOM_BLEND_MODE requires ALPHA_BLEND_MODE.
#endif

#if ALPHA_BLEND_MODE == 0 // csmAlphaBlendType_Over
vec4 live2d_alpha_blend(vec3 color, vec4 source, vec4 destination) {
    vec3 parameter = vec3(source.a * destination.a, source.a * (1.0 - destination.a), destination.a * (1.0 - source.a));
    return live2d_overlap_rgba(color, source.rgb, destination.rgb, parameter);
}
#elif ALPHA_BLEND_MODE == 1 // csmAlphaBlendType_Atop
vec4 live2d_alpha_blend(vec3 color, vec4 source, vec4 destination) {
    vec3 parameter = vec3(source.a * destination.a, 0.0, destination.a * (1.0 - source.a));
    return live2d_overlap_rgba(color, source.rgb, destination.rgb, parameter);
}
#elif ALPHA_BLEND_MODE == 2 // csmAlphaBlendType_Out
vec4 live2d_alpha_blend(vec3 color, vec4 source, vec4 destination) {
    vec3 parameter = vec3(0.0, 0.0, destination.a * (1.0 - source.a));
    return live2d_overlap_rgba(color, source.rgb, destination.rgb, parameter);
}
#elif ALPHA_BLEND_MODE == 3 // csmAlphaBlendType_ConjointOver
vec4 live2d_alpha_blend(vec3 color, vec4 source, vec4 destination) {
    vec3 parameter = vec3(min(source.a, destination.a), max(source.a - destination.a, 0.0), max(destination.a - source.a, 0.0));
    return live2d_overlap_rgba(color, source.rgb, destination.rgb, parameter);
}
#elif ALPHA_BLEND_MODE == 4 // csmAlphaBlendType_DisjointOver
vec4 live2d_alpha_blend(vec3 color, vec4 source, vec4 destination) {
    vec3 parameter = vec3(max(source.a + destination.a - 1.0, 0.0), min(source.a, 1.0 - destination.a), min(destination.a, 1.0 - source.a));
    return live2d_overlap_rgba(color, source.rgb, destination.rgb, parameter);
}
#else
#error Unsupported ALPHA_BLEND_MODE.
#endif
#endif

void fragment() {
    vec4 color = live2d_apply_colors(COLOR);

#ifdef CUSTOM_BLEND_MODE
    vec4 source = color;
#ifdef MASKED
    source.a *= live2d_mask_value();
#endif
    vec4 destination = live2d_premultiplied_to_straight(texture(blend_texture, SCREEN_UV));
    vec3 blended = live2d_color_blend(source.rgb, destination.rgb);
    COLOR = live2d_alpha_blend(blended, source, destination);
#elif defined(GODOT_BLEND_MULTIPLY)
    vec3 factor = color.rgb * color.a + (1.0 - color.a);
#ifdef MASKED
    COLOR = vec4(mix(vec3(1.0), factor, live2d_mask_value()), 1.0);
#else
    COLOR = vec4(factor, 1.0);
#endif
#else
#ifdef MASKED
    color.a *= live2d_mask_value();
#endif
    COLOR = color;
#endif
}
)";

static const char *LIVE2D_SETUP_MASK_SOURCE = R"(
shader_type canvas_item;
render_mode blend_mul;

uniform vec4 channel_flag = vec4(1.0, 0.0, 0.0, 0.0);
uniform vec4 layout_bounds = vec4(0.0, 0.0, 1.0, 1.0);
uniform vec2 mask_size = vec2(256.0, 256.0);

void fragment() {
    vec2 position = VERTEX / mask_size;
    float is_inside = step(layout_bounds.x, position.x) * step(layout_bounds.y, position.y)
        * step(position.x, layout_bounds.z) * step(position.y, layout_bounds.w);
    float coverage = COLOR.a * is_inside;
    COLOR = vec4(vec3(1.0) - channel_flag.rgb * coverage, 1.0 - channel_flag.a * coverage);
}
)";

} //namespace

static Ref<Shader> _make_shader(const String &p_defines, const char *p_source) {
	String code = p_defines;
	code += p_source;

	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code(code);
	return shader;
}

static bool _is_custom_blend(int32_t p_color_blend, int32_t p_alpha_blend) {
	if (p_color_blend == Live2D::Cubism::Core::csmColorBlendType_AddCompatible || p_color_blend == Live2D::Cubism::Core::csmColorBlendType_MultiplyCompatible) {
		return false;
	}
	return !(p_color_blend == Live2D::Cubism::Core::csmColorBlendType_Normal && p_alpha_blend == Live2D::Cubism::Core::csmAlphaBlendType_Over);
}

static String _variant_defines(int32_t p_color_blend, int32_t p_alpha_blend, bool p_masked) {
	String defines;
	if (p_color_blend == Live2D::Cubism::Core::csmColorBlendType_AddCompatible) {
		defines = "#define GODOT_BLEND_ADD\n";
	} else if (p_color_blend == Live2D::Cubism::Core::csmColorBlendType_MultiplyCompatible) {
		defines = "#define GODOT_BLEND_MULTIPLY\n";
	} else if (_is_custom_blend(p_color_blend, p_alpha_blend)) {
		defines = "#define CUSTOM_BLEND_MODE\n";
		defines += "#define COLOR_BLEND_MODE " + String::num_int64(p_color_blend) + "\n";
		defines += "#define ALPHA_BLEND_MODE " + String::num_int64(p_alpha_blend) + "\n";
	}
	if (p_masked) {
		defines += "#define MASKED\n";
	}
	return defines;
}

Ref<Shader> Live2DShaderLibrary::shaders[Live2DShaderLibrary::SHADER_COUNT];

Ref<Shader> Live2DShaderLibrary::setup_mask_shader;

Ref<Shader> Live2DShaderLibrary::_get_shader(int32_t p_color_blend, int32_t p_alpha_blend, bool p_masked) {
	if (p_color_blend < 0 || p_color_blend >= COLOR_BLEND_COUNT) {
		p_color_blend = Live2D::Cubism::Core::csmColorBlendType_Normal;
	}
	if (p_alpha_blend < 0 || p_alpha_blend >= ALPHA_BLEND_COUNT) {
		p_alpha_blend = Live2D::Cubism::Core::csmAlphaBlendType_Over;
	}
	if (!_is_custom_blend(p_color_blend, p_alpha_blend)) {
		p_alpha_blend = Live2D::Cubism::Core::csmAlphaBlendType_Over;
	}

	const int32_t index = (p_color_blend * ALPHA_BLEND_COUNT + p_alpha_blend) * 2 + (p_masked ? 1 : 0);
	Ref<Shader> &shader = shaders[index];
	if (shader.is_null()) {
		shader = _make_shader(_variant_defines(p_color_blend, p_alpha_blend, p_masked), LIVE2D_SHADER_SOURCE);
	}
	return shader;
}

Ref<ShaderMaterial> Live2DShaderLibrary::create_material(const Csm::csmBlendMode &p_blend_mode, bool p_masked, bool p_inverted) {
	Ref<ShaderMaterial> material;
	material.instantiate();
	material->set_shader(_get_shader(p_blend_mode.GetColorBlendType(), p_blend_mode.GetAlphaBlendType(), p_masked));

	if (p_masked) {
		material->set_shader_parameter("mask_inverted", p_inverted ? 1.0f : 0.0f);
	}

	return material;
}

Ref<ShaderMaterial> Live2DShaderLibrary::create_mask_material() {
	if (setup_mask_shader.is_null()) {
		setup_mask_shader = _make_shader("", LIVE2D_SETUP_MASK_SOURCE);
	}

	Ref<ShaderMaterial> material;
	material.instantiate();
	material->set_shader(setup_mask_shader);
	return material;
}

bool Live2DShaderLibrary::requires_backbuffer(const Csm::csmBlendMode &p_blend_mode) {
	return _is_custom_blend(p_blend_mode.GetColorBlendType(), p_blend_mode.GetAlphaBlendType());
}

void Live2DShaderLibrary::release() {
	for (int32_t i = 0; i < SHADER_COUNT; ++i) {
		shaders[i].unref();
	}

	setup_mask_shader.unref();
}
