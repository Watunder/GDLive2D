#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/variant/variant.h"
#include "scene/resources/animation.h"
#endif

class Live2DMotion : public Animation {
	GDCLASS(Live2DMotion, Animation);

private:
	String path_to_file;

	float fade_in_time = 1.0f;
	float fade_out_time = 1.0f;

	void _from_dict(const Dictionary &p_dict);

protected:
	static void _bind_methods();

public:
	Error load(String p_path);
	String get_load_path() const;

	float get_fade_in_time() const { return fade_in_time; }
	float get_fade_out_time() const { return fade_out_time; }
};
