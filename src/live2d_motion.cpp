#include "live2d_motion.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#elif defined(GODOT_MODULE)
#include "core/io/file_access.h"
#include "core/io/json.h"
#endif

void Live2DMotion::_from_dict(const Dictionary &p_dict) {
	while (get_track_count() > 0) {
		remove_track(get_track_count() - 1);
	}

	set_length(0.0);
	set_loop_mode(Animation::LOOP_NONE);
	fade_in_time = 1.0f;
	fade_out_time = 1.0f;

	double duration = 0.0;
	bool is_loop = false;

	const Dictionary meta = p_dict.get("Meta", Dictionary());
	if (!meta.is_empty()) {
		duration = static_cast<double>(meta.get("Duration", 0.0));
		is_loop = static_cast<bool>(meta.get("Loop", false));
		fade_in_time = static_cast<float>(static_cast<double>(meta.get("FadeInTime", 1.0)));
		fade_out_time = static_cast<float>(static_cast<double>(meta.get("FadeOutTime", 1.0)));
		set_loop_mode(is_loop ? Animation::LOOP_LINEAR : Animation::LOOP_NONE);
	}

	const Array curves = p_dict.get("Curves", Array());
	if (curves.is_empty()) {
		set_length(duration);
		return;
	}

	for (int32_t ci = 0; ci < curves.size(); ++ci) {
		if (curves[ci].get_type() != Variant::DICTIONARY) {
			continue;
		}

		const Dictionary curve = curves[ci];
		const String id = curve.get("Id", "");
		const String target = curve.get("Target", "");
		const Array segments = curve.get("Segments", Array());
		if (id.is_empty() || target.is_empty() || segments.size() < 2) {
			continue;
		}

		const NodePath path = NodePath(vformat(".:%s", id));

		const int32_t track_index = add_track(Animation::TYPE_BEZIER);
		track_set_path(track_index, path);

		int32_t si = 0;
		double current_time = static_cast<double>(segments[si++]);
		double current_value = static_cast<double>(segments[si++]);

		int32_t previous_key_index = bezier_track_insert_key(track_index, current_time, current_value, Vector2(0, 0), Vector2(0, 0));

		auto connect_linear = [&](int32_t p_from_key, double p_from_time, double p_from_value, int32_t p_to_key, double p_to_time, double p_to_value) {
			const double dt = p_to_time - p_from_time;
			const double dv = p_to_value - p_from_value;
			bezier_track_set_key_out_handle(track_index, p_from_key, Vector2(dt / 3.0, dv / 3.0));
			bezier_track_set_key_in_handle(track_index, p_to_key, Vector2(-dt / 3.0, -dv / 3.0));
		};

		while (si < segments.size()) {
			const int32_t segment_type = static_cast<int32_t>(segments[si++]);
			switch (segment_type) {
				case 0: { // Linear
					if (si + 1 >= segments.size()) {
						si = segments.size();
						break;
					}
					const double end_time = static_cast<double>(segments[si++]);
					const double end_value = static_cast<double>(segments[si++]);
					const int32_t end_key_index = bezier_track_insert_key(track_index, end_time, end_value, Vector2(0, 0), Vector2(0, 0));
					connect_linear(previous_key_index, current_time, current_value, end_key_index, end_time, end_value);

					current_time = end_time;
					current_value = end_value;
					previous_key_index = end_key_index;
				} break;
				case 1: { // Bezier
					if (si + 5 >= segments.size()) {
						si = segments.size();
						break;
					}
					const double cp1_time = static_cast<double>(segments[si++]);
					const double cp1_value = static_cast<double>(segments[si++]);
					const double cp2_time = static_cast<double>(segments[si++]);
					const double cp2_value = static_cast<double>(segments[si++]);
					const double end_time = static_cast<double>(segments[si++]);
					const double end_value = static_cast<double>(segments[si++]);

					const int32_t end_key_index = bezier_track_insert_key(track_index, end_time, end_value, Vector2(0, 0), Vector2(0, 0));
					bezier_track_set_key_out_handle(track_index, previous_key_index, Vector2(cp1_time - current_time, cp1_value - current_value));
					bezier_track_set_key_in_handle(track_index, end_key_index, Vector2(cp2_time - end_time, cp2_value - end_value));

					current_time = end_time;
					current_value = end_value;
					previous_key_index = end_key_index;
				} break;
				case 2: { // Stepped
					if (si + 1 >= segments.size()) {
						si = segments.size();
						break;
					}
					const double end_time = static_cast<double>(segments[si++]);
					const double end_value = static_cast<double>(segments[si++]);
					const double dt = end_time - current_time;

					if (dt > 0.0) {
						const double hold_time = end_time - MIN(0.001, dt * 0.5);
						if (hold_time > current_time) {
							const int32_t hold_key_index = bezier_track_insert_key(track_index, hold_time, current_value, Vector2(0, 0), Vector2(0, 0));
							connect_linear(previous_key_index, current_time, current_value, hold_key_index, hold_time, current_value);
							previous_key_index = hold_key_index;
							current_time = hold_time;
						}
					}

					const int32_t end_key_index = bezier_track_insert_key(track_index, end_time, end_value, Vector2(0, 0), Vector2(0, 0));
					bezier_track_set_key_out_handle(track_index, previous_key_index, Vector2(0, 0));
					bezier_track_set_key_in_handle(track_index, end_key_index, Vector2(0, 0));

					current_time = end_time;
					current_value = end_value;
					previous_key_index = end_key_index;
				} break;
				case 3: { // InverseStepped
					if (si + 1 >= segments.size()) {
						si = segments.size();
						break;
					}
					const double end_time = static_cast<double>(segments[si++]);
					const double end_value = static_cast<double>(segments[si++]);
					const double dt = end_time - current_time;

					if (dt > 0.0) {
						const double jump_time = current_time + MIN(0.001, dt * 0.5);
						const int32_t jump_key_index = bezier_track_insert_key(track_index, jump_time, end_value, Vector2(0, 0), Vector2(0, 0));
						bezier_track_set_key_out_handle(track_index, previous_key_index, Vector2(0, 0));
						bezier_track_set_key_in_handle(track_index, jump_key_index, Vector2(0, 0));
						previous_key_index = jump_key_index;
						current_time = jump_time;
						current_value = end_value;
					}

					const int32_t end_key_index = bezier_track_insert_key(track_index, end_time, end_value, Vector2(0, 0), Vector2(0, 0));
					connect_linear(previous_key_index, current_time, current_value, end_key_index, end_time, end_value);

					current_time = end_time;
					current_value = end_value;
					previous_key_index = end_key_index;
				} break;
				default:
					si = segments.size();
					break;
			}
		}
	}

	set_length(duration);
}

void Live2DMotion::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_load_path"), &Live2DMotion::get_load_path);
	ClassDB::bind_method(D_METHOD("load", "path"), &Live2DMotion::load);
	ClassDB::bind_method(D_METHOD("get_fade_in_time"), &Live2DMotion::get_fade_in_time);
	ClassDB::bind_method(D_METHOD("get_fade_out_time"), &Live2DMotion::get_fade_out_time);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "load_path", PROPERTY_HINT_FILE, "*.motion3.json"), "load", "get_load_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_in_time"), "", "get_fade_in_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_out_time"), "", "get_fade_out_time");
}

String Live2DMotion::get_load_path() const {
	return path_to_file;
}

Error Live2DMotion::load(String p_path) {
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
