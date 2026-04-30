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

	const Dictionary meta = p_dict.get("Meta", Dictionary());
	if (!meta.is_empty()) {
		set_length(static_cast<double>(meta.get("Duration", 0.0)));
		set_loop_mode(static_cast<bool>(meta.get("Loop", false)) ? Animation::LOOP_LINEAR : Animation::LOOP_NONE);
	}

	const Array curves = p_dict.get("Curves", Array());
	if (curves.is_empty()) {
		return;
	}

	for (int32_t ci = 0; ci < curves.size(); ++ci) {
		if (curves[ci].get_type() != Variant::DICTIONARY) {
			continue;
		}

		const Dictionary curve = curves[ci];
		const String id = curve.get("Id", "");
		const Array segments = curve.get("Segments", Array());
		if (id.is_empty() || segments.is_empty()) {
			continue;
		}
		const String target = curve.get("Target", "");
		if (target.is_empty()) {
			continue;
		}
		if (segments.size() < 2) {
			continue;
		}

		const NodePath path = NodePath(vformat(".:%s", id));
		const bool is_part_opacity = (target == "PartOpacity");

		if (is_part_opacity) {
			const int32_t track_index = add_track(Animation::TYPE_VALUE);
			track_set_path(track_index, path);
			value_track_set_update_mode(track_index, Animation::UPDATE_DISCRETE);

			int32_t si = 0;
			double key_time = static_cast<double>(segments[si++]);
			double key_value = static_cast<double>(segments[si++]);
			track_insert_key(track_index, key_time, key_value >= 0.5 ? true : false);

			while (si < segments.size()) {
				const int32_t segment_type = static_cast<int32_t>(segments[si++]);
				double end_time = key_time;
				double end_value = key_value;

				switch (segment_type) {
					case 0: // linear
					case 2: // stepped
					case 3: { // inverse stepped
						if (si + 1 >= segments.size()) {
							si = segments.size();
							continue;
						}
						end_time = static_cast<double>(segments[si++]);
						end_value = static_cast<double>(segments[si++]);
					} break;
					case 1: { // bezier
						if (si + 5 >= segments.size()) {
							si = segments.size();
							continue;
						}
						si += 4;
						end_time = static_cast<double>(segments[si++]);
						end_value = static_cast<double>(segments[si++]);
					} break;
					default:
						si = segments.size();
						continue;
				}

				key_time = end_time;
				key_value = end_value;
				track_insert_key(track_index, key_time, key_value >= 0.5 ? true : false);
			}
			continue;
		}

		const int32_t track_index = add_track(Animation::TYPE_BEZIER);
		track_set_path(track_index, path);

		int32_t si = 0;
		double current_time = static_cast<double>(segments[si++]);
		double current_value = static_cast<double>(segments[si++]);
		int32_t previous_key_index = bezier_track_insert_key(track_index, current_time, current_value);

		while (si < segments.size()) {
			const int32_t segment_type = static_cast<int32_t>(segments[si++]);
			switch (segment_type) {
				case 0: { // linear
					if (si + 1 >= segments.size()) {
						si = segments.size();
						break;
					}
					const double end_time = static_cast<double>(segments[si++]);
					const double end_value = static_cast<double>(segments[si++]);
					const int32_t end_key_index = bezier_track_insert_key(track_index, end_time, end_value);

					const double dt = end_time - current_time;
					const double dv = end_value - current_value;
					bezier_track_set_key_out_handle(track_index, previous_key_index, Vector2(dt / 3.0, dv / 3.0));
					bezier_track_set_key_in_handle(track_index, end_key_index, Vector2(-dt / 3.0, -dv / 3.0));

					current_time = end_time;
					current_value = end_value;
					previous_key_index = end_key_index;
				} break;
				case 1: { // bezier
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

					const int32_t end_key_index = bezier_track_insert_key(track_index, end_time, end_value);
					bezier_track_set_key_out_handle(track_index, previous_key_index, Vector2(cp1_time - current_time, cp1_value - current_value));
					bezier_track_set_key_in_handle(track_index, end_key_index, Vector2(cp2_time - end_time, cp2_value - end_value));

					current_time = end_time;
					current_value = end_value;
					previous_key_index = end_key_index;
				} break;
				case 2: // stepped
				case 3: { // inverse stepped
					if (si + 1 >= segments.size()) {
						si = segments.size();
						break;
					}
					const double end_time = static_cast<double>(segments[si++]);
					const double end_value = static_cast<double>(segments[si++]);

					if (end_time > current_time) {
						const double hold_time = end_time - MIN(0.001, (end_time - current_time) * 0.5);
						if (hold_time > current_time) {
							const int32_t hold_key_index = bezier_track_insert_key(track_index, hold_time, current_value);
							const double hold_dt = hold_time - current_time;
							bezier_track_set_key_out_handle(track_index, previous_key_index, Vector2(hold_dt / 3.0, 0.0));
							bezier_track_set_key_in_handle(track_index, hold_key_index, Vector2(-hold_dt / 3.0, 0.0));
							previous_key_index = hold_key_index;
						}
					}

					const int32_t end_key_index = bezier_track_insert_key(track_index, end_time, end_value);
					const double dt = end_time - current_time;
					const double dv = end_value - current_value;
					bezier_track_set_key_out_handle(track_index, previous_key_index, Vector2(dt / 3.0, dv / 3.0));
					bezier_track_set_key_in_handle(track_index, end_key_index, Vector2(-dt / 3.0, -dv / 3.0));

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
}

void Live2DMotion::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_load_path"), &Live2DMotion::get_load_path);
	ClassDB::bind_method(D_METHOD("load", "path"), &Live2DMotion::load);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "load_path", PROPERTY_HINT_FILE, "*.motion3.json"), "load", "get_load_path");
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

	PackedByteArray data = FileAccess::get_file_as_bytes(p_path);
	if (data.is_empty()) {
		return FAILED;
	}

	const Variant parsed = JSON::parse_string(data.get_string_from_utf8());
	if (parsed.get_type() != Variant::DICTIONARY) {
		return ERR_PARSE_ERROR;
	}

	path_to_file = p_path;

	_from_dict(parsed);
	return OK;
}
