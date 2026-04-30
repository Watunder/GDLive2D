#include "live2d_model_instance.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/texture2d.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "scene/animation/animation_library.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/texture.h"
#endif

#include "live2d_expression.h"
#include "live2d_motion.h"

#include <CubismModelSettingJson.hpp>
#include <Math/CubismModelMatrix.hpp>

static String _get_rel_dir(const char *p_cstr) {
	if (!p_cstr || p_cstr[0] == '\0') {
		return String();
	}
	String s = String::utf8(p_cstr);
	s = s.replace("\\", "/");
	while (s.begins_with("../")) {
		s = s.substr(3);
	}
	if (s.begins_with("./")) {
		s = s.substr(2);
	}
	return s;
}

void Live2DModelInstance::_update_model_properties() {
	parameter_ids.clear();
	parameter_values.clear();
	parameter_mins.clear();
	parameter_maxs.clear();

	if (!user_model || !user_model->GetModel()) {
		notify_property_list_changed();
		return;
	}

	Csm::csmVector<Csm::csmString> temp_parameter_ids;
	user_model->GetModelParameterIds(temp_parameter_ids);
	for (Csm::csmUint32 i = 0; i < temp_parameter_ids.GetSize(); ++i) {
		const String parameter_id = String::utf8(temp_parameter_ids[i].GetRawString());
		if (parameter_id.is_empty()) {
			continue;
		}
		parameter_ids.push_back(parameter_id);
		parameter_values[parameter_id] = user_model->GetModelParameterValue(parameter_id.utf8().get_data());
		Csm::csmFloat32 min_value = 0.0f;
		Csm::csmFloat32 max_value = 1.0f;
		if (user_model->GetModelParameterRange(parameter_id.utf8().get_data(), min_value, max_value)) {
			parameter_mins[parameter_id] = min_value;
			parameter_maxs[parameter_id] = max_value;
		}
	}

	notify_property_list_changed();
}

void Live2DModelInstance::_update_pose_groups(const String &p_model_dir) {
	pose_groups.clear();

	if (!user_model) {
		return;
	}
	Csm::CubismModelSettingJson *setting = user_model->GetSettingJson();
	if (!setting) {
		return;
	}

	const String pose_rel = String::utf8(setting->GetPoseFileName());
	if (pose_rel.is_empty()) {
		return;
	}
	const String pose_path = p_model_dir.path_join(pose_rel);
	PackedByteArray pose_data = FileAccess::get_file_as_bytes(pose_path);
	if (pose_data.is_empty()) {
		return;
	}

	const Variant parsed = JSON::parse_string(pose_data.get_string_from_utf8());
	if (parsed.get_type() != Variant::DICTIONARY) {
		return;
	}
	const Dictionary pose_dict = parsed;
	const Array groups = pose_dict.get("Groups", Array());
	for (int32_t gi = 0; gi < groups.size(); ++gi) {
		if (groups[gi].get_type() != Variant::ARRAY) {
			continue;
		}
		const Array group = groups[gi];
		if (group.size() < 2) {
			continue;
		}
		PackedStringArray members;
		for (int32_t pi = 0; pi < group.size(); ++pi) {
			if (group[pi].get_type() != Variant::DICTIONARY) {
				continue;
			}
			const Dictionary part = group[pi];
			const String part_id = part.get("Id", "");
			if (!part_id.is_empty()) {
				members.push_back(part_id);
			}
		}
		if (members.size() < 2) {
			continue;
		}
		const String group_key = vformat("PoseGroups/%d", gi);
		pose_groups[group_key] = members;
	}
}

void Live2DModelInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			ERR_BREAK(!user_model);

			const Array keys = parameter_values.keys();
			for (int32_t i = 0; i < keys.size(); ++i) {
				const String parameter_id = keys[i];
				const Variant value = parameter_values[parameter_id];
				user_model->SetModelParameterValue(parameter_id.utf8().get_data(), static_cast<float>(value));
			}

			const float delta = static_cast<float>(get_process_delta_time());
			user_model->Update(delta);

			queue_redraw();
		} break;

		case NOTIFICATION_DRAW: {
			ERR_BREAK(!user_model);

			Csm::CubismMatrix44 projection;
			_update_projection(projection);

			user_model->Draw(projection);
		} break;

		default:
			break;
	}
}

void Live2DModelInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_model_entry", "model_entry"), &Live2DModelInstance::set_model_entry);
	ClassDB::bind_method(D_METHOD("get_model_entry"), &Live2DModelInstance::get_model_entry);

	ClassDB::bind_method(D_METHOD("get_moc_file"), &Live2DModelInstance::get_moc_file);
	ClassDB::bind_method(D_METHOD("get_textures"), &Live2DModelInstance::get_textures);

	ClassDB::bind_method(D_METHOD("_reset_model_properties"), &Live2DModelInstance::_reset_model_properties);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "model_entry", PROPERTY_HINT_FILE, "*.model3.json"), "set_model_entry", "get_model_entry");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "moc_file", PROPERTY_HINT_RESOURCE_TYPE, "Live2DMocFile", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "", "get_moc_file");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "textures", PROPERTY_HINT_ARRAY_TYPE, "Texture2D", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "", "get_textures");
}

void Live2DModelInstance::_setup_moc_file(const String &p_model_dir) {
	ERR_FAIL_COND(!user_model);

	Csm::CubismModelSettingJson *setting = user_model->GetSettingJson();
	ERR_FAIL_COND(!setting);

	const String moc_path = p_model_dir.path_join(setting->GetModelFileName());

#ifdef GDEXTENSION
	Ref<Resource> res = ResourceLoader::get_singleton()->load(moc_path, "Live2DMocFile");
#elif defined(GODOT_MODULE)
	Ref<Resource> res = ResourceLoader::load(moc_path, "Live2DMocFile");
#endif

	moc_file = res;
	if (moc_file.is_null()) {
		ERR_PRINT_ED(String("[GDLive2D] Moc file failed to load: ") + moc_path);
		return;
	}

	PackedByteArray moc_data = moc_file->get_data();
	user_model->LoadModelFromMoc3(moc_data.ptr(), moc_data.size());
}

void Live2DModelInstance::_setup_textures(const String &p_model_dir) {
	ERR_FAIL_COND(!user_model);

	Csm::CubismModelSettingJson *setting = user_model->GetSettingJson();
	ERR_FAIL_COND(!setting);

	const int32_t tex_count = setting->GetTextureCount();
	ERR_FAIL_COND(tex_count <= 0);

	textures.clear();

	for (int32_t i = 0; i < tex_count; ++i) {
		const String rel = _get_rel_dir(setting->GetTextureFileName(i));
		const String tex_path = rel.is_empty() ? String() : p_model_dir.path_join(rel);

#ifdef GDEXTENSION
		Ref<Resource> res = ResourceLoader::get_singleton()->load(tex_path, "Texture2D");
#elif defined(GODOT_MODULE)
		Ref<Resource> res = ResourceLoader::load(tex_path, "Texture2D");
#endif

		const Ref<Texture2D> tex = res;
		if (tex.is_null()) {
			ERR_PRINT_ED("[GDLive2D] Texture failed to load: " + tex_path);
			continue;
		}

		user_model->BindTexture(i, tex->get_rid().get_id());
		textures.push_back(tex);
	}
}

void Live2DModelInstance::_update_animation_player(const String &p_model_dir) {
	ERR_FAIL_COND(!user_model);

	AnimationPlayer *animation_player = nullptr;
	for (int32_t i = get_child_count() - 1; i >= 0; --i) {
		Node *child = get_child(i);
		if (Object::cast_to<AnimationPlayer>(child)) {
			animation_player = Object::cast_to<AnimationPlayer>(child);
		}
	}
	if (!animation_player) {
		animation_player = memnew(AnimationPlayer);
		animation_player->set_name("AnimationPlayer");
		add_child(animation_player);
		animation_player->set_owner(this);
	}
	ERR_FAIL_COND(!animation_player);

	Csm::CubismModelSettingJson *setting = user_model->GetSettingJson();
	ERR_FAIL_COND(!setting);

	Ref<AnimationLibrary> library;
	library.instantiate();

	String default_animation_name;

	for (Csm::csmInt32 i = 0; i < setting->GetMotionGroupCount(); ++i) {
		const Csm::csmChar *group = setting->GetMotionGroupName(i);
		const String group_name = String::utf8(group);
		for (Csm::csmInt32 j = 0; j < setting->GetMotionCount(group); ++j) {
			const String motion_path = p_model_dir.path_join(String::utf8(setting->GetMotionFileName(group, j)));
			const String animation_name = vformat("%s_%d", group_name, j);

#ifdef GDEXTENSION
			Ref<Resource> res = ResourceLoader::get_singleton()->load(motion_path, "Live2DMotion");
#elif defined(GODOT_MODULE)
			Ref<Resource> res = ResourceLoader::load(motion_path, "Live2DMotion");
#endif

			Ref<Live2DMotion> motion = res;
			if (motion.is_null()) {
				continue;
			}
			library->add_animation(animation_name, motion);

			if (default_animation_name.is_empty() || animation_name.begins_with("idle_")) {
				default_animation_name = animation_name;
			}
		}
	}

	for (Csm::csmInt32 i = 0; i < setting->GetExpressionCount(); ++i) {
		const String expression_path = p_model_dir.path_join(String::utf8(setting->GetExpressionFileName(i)));
		const String expression_name = String::utf8(setting->GetExpressionName(i));
		const String animation_name = expression_name;

#ifdef GDEXTENSION
		Ref<Resource> res = ResourceLoader::get_singleton()->load(expression_path, "Live2DExpression");
#elif defined(GODOT_MODULE)
		Ref<Resource> res = ResourceLoader::load(expression_path, "Live2DExpression");
#endif

		Ref<Live2DExpression> expression = res;
		if (expression.is_null()) {
			continue;
		}

		library->add_animation(animation_name, expression);
	}

	if (animation_player->has_animation_library(StringName())) {
		animation_player->remove_animation_library(StringName());
	}

	animation_player->add_animation_library(StringName(), library);
	if (!default_animation_name.is_empty()) {
		animation_player->set_autoplay(default_animation_name);
	}
}

// refer to Camera2D::_get_camera_screen_size
Size2 Live2DModelInstance::_get_screen_size() {
	if (is_part_of_edited_scene()) {
#ifdef GDEXTENSION
		return Size2(ProjectSettings::get_singleton()->get_setting_with_override("display/window/size/viewport_width"), ProjectSettings::get_singleton()->get_setting_with_override("display/window/size/viewport_height"));
#elif defined(GODOT_MODULE)
		return Size2(GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_width"), GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_height"));
#endif
	}
	return get_viewport_rect().size;
}

void Live2DModelInstance::_update_projection(Csm::CubismMatrix44 &r_projection) {
	ERR_FAIL_COND(!user_model);

	const CubismUserModelExtend::CanvasInfo canvas_info = user_model->GetModelCanvasInfo();
	const Vector2 screen_size = _get_screen_size();

	float model_scale = 1.0f;

	if (!Math::is_zero_approx(canvas_info.pixelsPerUnit) && !screen_size.is_zero_approx()) {
		float canvas_width = canvas_info.sizeInPixels.X / canvas_info.pixelsPerUnit;
		float canvas_height = canvas_info.sizeInPixels.Y / canvas_info.pixelsPerUnit;
		if (!Math::is_zero_approx(canvas_width) && !Math::is_zero_approx(canvas_height)) {
			const float scale_x = screen_size.x / canvas_width;
			const float scale_y = screen_size.y / canvas_height;
			model_scale = MAX(MAX(scale_x, scale_y), 1.0f);
		}
	}

	r_projection.Scale(model_scale, model_scale);
}

void Live2DModelInstance::set_model_entry(const String &p_model_entry_path) {
	ERR_FAIL_COND(!user_model);

	if (p_model_entry_path == model_entry_path) {
		return;
	}

	String path = p_model_entry_path;
	if (path.begins_with("uid://")) {
		path = ResourceUID::uid_to_path(path);
	}
	ERR_FAIL_COND(!path.ends_with(".model3.json"));

	user_model->LoadSettingJson(path.utf8().get_data());

	Csm::CubismModelSettingJson *setting = user_model->GetSettingJson();
	ERR_FAIL_COND(!setting);

	const String model_dir = path.get_base_dir();

	_setup_moc_file(model_dir);
	ERR_FAIL_COND(!user_model->GetModel());

	_setup_textures(model_dir);
	ERR_FAIL_COND(textures.is_empty());

	user_model->SetupConfigs(model_dir.utf8().get_data());
	ERR_FAIL_COND(!user_model->IsInitialized());

	_update_pose_groups(model_dir);

	if (is_inside_tree() || is_importing) {
		_update_animation_player(model_dir);
	}

	_update_model_properties();

	user_model->CreateRenderer();
	set_process(true);

	model_entry_path = path;
}

String Live2DModelInstance::get_model_entry() const {
	return model_entry_path;
}

Ref<Live2DMocFile> Live2DModelInstance::get_moc_file() const {
	return moc_file;
}

TypedArray<Texture2D> Live2DModelInstance::get_textures() const {
	TypedArray<Texture2D> ret;
	for (int64_t i = 0; i < textures.size(); ++i) {
		ret.push_back(textures[i]);
	}
	return ret;
}

void Live2DModelInstance::_reset_model_properties() {
	if (!user_model || !user_model->GetModel()) {
		return;
	}

	for (int32_t i = 0; i < parameter_ids.size(); ++i) {
		const String parameter_id = parameter_ids[i];
		parameter_values[parameter_id] = user_model->GetModelParameterDefaultValue(parameter_id.utf8().get_data());
	}

	const Array group_keys = pose_groups.keys();
	for (int32_t i = 0; i < group_keys.size(); ++i) {
		const String group_key = group_keys[i];
		const PackedStringArray members = pose_groups[group_key];
		for (int32_t mi = 0; mi < members.size(); ++mi) {
			const bool visible = (mi == 0);
			user_model->SetModelPartVisible(members[mi].utf8().get_data(), visible);
		}
	}
}

bool Live2DModelInstance::_set(const StringName &p_name, const Variant &p_value) {
	if (parameter_values.has(p_name)) {
		parameter_values[p_name] = p_value;
		return true;
	}

	if (pose_groups.has(p_name)) {
		const PackedStringArray members = pose_groups[p_name];
		if (members.is_empty()) {
			return true;
		}
		int32_t selected = static_cast<int32_t>(p_value);
		selected = CLAMP(selected, 0, members.size() - 1);
		for (int32_t i = 0; i < members.size(); ++i) {
			const String part_id = members[i];
			const bool visible = (i == selected);
			if (user_model && user_model->GetModel()) {
				user_model->SetModelPartVisible(part_id.utf8().get_data(), visible);
			}
		}
		return true;
	}

	const Array group_keys = pose_groups.keys();
	for (int32_t gi = 0; gi < group_keys.size(); ++gi) {
		const String group_key = group_keys[gi];
		const PackedStringArray members = pose_groups[group_key];
		for (int32_t mi = 0; mi < members.size(); ++mi) {
			if (members[mi] != String(p_name)) {
				continue;
			}
			bool active = false;
			if (p_value.get_type() == Variant::BOOL) {
				active = static_cast<bool>(p_value);
			} else {
				active = static_cast<float>(p_value) >= 0.5f;
			}
			if (!active) {
				return true;
			}
			for (int32_t i = 0; i < members.size(); ++i) {
				const bool visible = (i == mi);
				if (user_model && user_model->GetModel()) {
					user_model->SetModelPartVisible(members[i].utf8().get_data(), visible);
				}
			}
			return true;
		}
	}

	return false;
}

bool Live2DModelInstance::_get(const StringName &p_name, Variant &r_ret) const {
	if (parameter_values.has(p_name)) {
		r_ret = parameter_values[p_name];
		return true;
	}

	if (pose_groups.has(p_name)) {
		const PackedStringArray members = pose_groups[p_name];
		if (members.is_empty()) {
			r_ret = 0;
			return true;
		}
		int32_t selected = 0;
		float best_value = -1.0f;
		for (int32_t i = 0; i < members.size(); ++i) {
			const String part_id = members[i];
			if (!user_model || !user_model->GetModel()) {
				continue;
			}
			const float value = user_model->GetModelParameterValue(part_id.utf8().get_data());
			if (value > best_value) {
				best_value = value;
				selected = i;
			}
		}
		r_ret = selected;
		return true;
	}

	const Array group_keys = pose_groups.keys();
	for (int32_t gi = 0; gi < group_keys.size(); ++gi) {
		const String group_key = group_keys[gi];
		const PackedStringArray members = pose_groups[group_key];
		for (int32_t mi = 0; mi < members.size(); ++mi) {
			if (members[mi] == String(p_name)) {
				bool active = false;
				if (user_model && user_model->GetModel()) {
					const float value = user_model->GetModelParameterValue(members[mi].utf8().get_data());
					active = value >= 0.5f;
				}
				r_ret = active;
				return true;
			}
		}
	}

	return false;
}

void Live2DModelInstance::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::NIL, "Parameters", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
	for (int32_t i = 0; i < parameter_ids.size(); ++i) {
		const String parameter_id = parameter_ids[i];
		double min_value = -1.0;
		double max_value = 1.0;
		if (parameter_mins.has(parameter_id)) {
			min_value = static_cast<double>(parameter_mins[parameter_id]);
		}
		if (parameter_maxs.has(parameter_id)) {
			max_value = static_cast<double>(parameter_maxs[parameter_id]);
		}
		const String hint = vformat("%s,%s,0.001,or_less,or_greater", String::num_real(min_value), String::num_real(max_value));
		p_list->push_back(PropertyInfo(Variant::FLOAT, parameter_id, PROPERTY_HINT_RANGE, hint));
	}

	p_list->push_back(PropertyInfo(Variant::NIL, "PoseGroups", PROPERTY_HINT_NONE, "PoseGroups/", PROPERTY_USAGE_GROUP));
	const Array group_keys = pose_groups.keys();
	for (int32_t i = 0; i < group_keys.size(); ++i) {
		const String group_key = group_keys[i];
		const PackedStringArray members = pose_groups[group_key];
		if (members.is_empty()) {
			continue;
		}
		String labels;
		for (int32_t mi = 0; mi < members.size(); ++mi) {
			if (mi > 0) {
				labels += ",";
			}
			labels += members[mi];
		}
		p_list->push_back(PropertyInfo(Variant::INT, group_key, PROPERTY_HINT_ENUM, labels));
	}

	PackedStringArray hidden_part_ids;
	for (int32_t gi = 0; gi < group_keys.size(); ++gi) {
		const String group_key = group_keys[gi];
		const PackedStringArray members = pose_groups[group_key];
		for (int32_t mi = 0; mi < members.size(); ++mi) {
			const String part_id = members[mi];
			if (!hidden_part_ids.has(part_id)) {
				hidden_part_ids.push_back(part_id);
			}
		}
	}
	for (int32_t i = 0; i < hidden_part_ids.size(); ++i) {
		const String part_id = hidden_part_ids[i];
		p_list->push_back(PropertyInfo(Variant::BOOL, part_id, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NO_EDITOR));
	}
}

Live2DModelInstance::Live2DModelInstance(bool p_is_importing) {
	is_importing = p_is_importing;

	user_model = memnew(CubismUserModelExtend);

	user_model->BindBase(get_canvas_item().get_id());
}

Live2DModelInstance::~Live2DModelInstance() {
	if (user_model) {
		memdelete(user_model);
	}
}
