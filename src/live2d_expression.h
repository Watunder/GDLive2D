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

	void _from_dict(const Dictionary &p_dict);

protected:
	static void _bind_methods();

public:
	Error load(String p_path);
	String get_load_path() const;
};
