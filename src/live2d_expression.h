#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/variant/variant.h"
#include "scene/resources/animation.h"
#endif

class Live2DExpression : public Animation {
	GDCLASS(Live2DExpression, Animation);

private:
	String path_to_file;

	float fade_in_time = 1.0f;
	float fade_out_time = 1.0f;

	// the "Blend" parameter (Add/Multiply/Overwrite)
	Dictionary blend_modes;

	void _from_dict(const Dictionary &p_dict);

protected:
	static void _bind_methods();

public:
	Error load(String p_path);
	String get_load_path() const;

	float get_fade_in_time() const { return fade_in_time; }
	float get_fade_out_time() const { return fade_out_time; }
	Dictionary get_blend_modes() const { return blend_modes.duplicate(); }
};
