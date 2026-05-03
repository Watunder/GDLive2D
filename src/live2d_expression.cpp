#include "live2d_expression.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#elif defined(GODOT_MODULE)
#include "core/io/file_access.h"
#include "core/io/json.h"
#endif

void Live2DExpression::_from_dict(const Dictionary &p_dict) {
	while (get_track_count() > 0) {
		remove_track(get_track_count() - 1);
	}

	set_loop_mode(Animation::LOOP_NONE);

	Array parameters;
	if (p_dict.has("Parameters") && p_dict["Parameters"].get_type() == Variant::ARRAY) {
		parameters = p_dict["Parameters"];
	}

	for (int32_t pi = 0; pi < parameters.size(); ++pi) {
		if (parameters[pi].get_type() != Variant::DICTIONARY) {
			continue;
		}

		const Dictionary param = parameters[pi];
		const String id = param.get("Id", "");
		if (id.is_empty()) {
			continue;
		}

		const NodePath path = NodePath(vformat(".:%s", id));

		const int32_t track_index = add_track(Animation::TYPE_VALUE);
		track_set_path(track_index, path);
		value_track_set_update_mode(track_index, Animation::UPDATE_CONTINUOUS);

		const int32_t value = static_cast<int32_t>(param.get("Value", 0));

		track_insert_key(track_index, 0.0, value);
	}
}

void Live2DExpression::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_load_path"), &Live2DExpression::get_load_path);
	ClassDB::bind_method(D_METHOD("load", "path"), &Live2DExpression::load);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "load_path", PROPERTY_HINT_FILE, "*.exp3.json"), "load", "get_load_path");
}

Error Live2DExpression::load(String p_path) {
#ifdef GDEXTENSION
	if (!FileAccess::file_exists(p_path))
#elif defined(GODOT_MODULE)
	if (!FileAccess::exists(p_path))
#endif
	{
		return ERR_FILE_NOT_FOUND;
	}

	const String content = FileAccess::get_file_as_string(p_path);
	if (content.is_empty()) {
		return FAILED;
	}

	const Variant parsed = JSON::parse_string(content);
	if (parsed.get_type() != Variant::DICTIONARY) {
		return ERR_PARSE_ERROR;
	}

	path_to_file = p_path;

	_from_dict(parsed);
	return OK;
}

String Live2DExpression::get_load_path() const {
	return path_to_file;
}
