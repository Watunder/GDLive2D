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

	fade_in_time = static_cast<float>(static_cast<double>(p_dict.get("FadeInTime", 1.0)));
	fade_out_time = static_cast<float>(static_cast<double>(p_dict.get("FadeOutTime", 1.0)));
	blend_modes.clear();

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

		blend_modes[id] = param.get("Blend", "Add");

		const NodePath path = NodePath(vformat(".:%s", id));

		const int32_t track_index = add_track(Animation::TYPE_VALUE);
		track_set_path(track_index, path);
		value_track_set_update_mode(track_index, Animation::UPDATE_CONTINUOUS);

		const double value = static_cast<double>(param.get("Value", 0.0));

		track_insert_key(track_index, 0.0, value);
	}
}

void Live2DExpression::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_load_path"), &Live2DExpression::get_load_path);
	ClassDB::bind_method(D_METHOD("load", "path"), &Live2DExpression::load);
	ClassDB::bind_method(D_METHOD("get_fade_in_time"), &Live2DExpression::get_fade_in_time);
	ClassDB::bind_method(D_METHOD("get_fade_out_time"), &Live2DExpression::get_fade_out_time);
	ClassDB::bind_method(D_METHOD("get_blend_modes"), &Live2DExpression::get_blend_modes);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "load_path", PROPERTY_HINT_FILE, "*.exp3.json"), "load", "get_load_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_in_time"), "", "get_fade_in_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_out_time"), "", "get_fade_out_time");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "blend_modes"), "", "get_blend_modes");
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

#ifdef GDEXTENSION
Variant ResourceFormatLoaderLive2DExpression::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	Ref<Live2DExpression> expression;
	expression.instantiate();

	if (expression->load(p_path) != OK) {
		return Ref<Resource>();
	}

	return expression;
}

PackedStringArray ResourceFormatLoaderLive2DExpression::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("exp3.json");
	return extensions;
}

bool ResourceFormatLoaderLive2DExpression::_handles_type(const StringName &p_type) const {
	return ClassDB::is_parent_class(p_type, "Live2DExpression");
}

String ResourceFormatLoaderLive2DExpression::_get_resource_type(const String &p_path) const {
	if (p_path.ends_with(".exp3.json")) {
		return "Live2DExpression";
	}
	return "";
}
#elif defined(GODOT_MODULE)
Ref<Resource> ResourceFormatLoaderLive2DExpression::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	Ref<Live2DExpression> expression;
	expression.instantiate();

	if (expression->load(p_path) != OK) {
		return Ref<Resource>();
	}

	return expression;
}

void ResourceFormatLoaderLive2DExpression::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("exp3.json");
}

bool ResourceFormatLoaderLive2DExpression::handles_type(const String &p_type) const {
	return ClassDB::is_parent_class(p_type, "Live2DExpression");
}

String ResourceFormatLoaderLive2DExpression::get_resource_type(const String &p_path) const {
	if (p_path.ends_with(".exp3.json")) {
		return "Live2DExpression";
	}
	return "";
}
#endif
