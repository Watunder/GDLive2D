#include "live2d_model_instance.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/texture2d.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "scene/resources/texture.h"
#endif

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

void Live2DModelInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			ERR_BREAK(!user_model);

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

	const String pose_path = model_dir.path_join(setting->GetPoseFileName());
	if (!pose_path.get_file().is_empty()) {
		user_model->LoadPoseJson(pose_path.utf8().get_data());
	}

	const String physics_path = model_dir.path_join(setting->GetPhysicsFileName());
	if (!physics_path.get_file().is_empty()) {
		user_model->LoadPhysicsJson(physics_path.utf8().get_data());
	}

	const String user_data_path = model_dir.path_join(setting->GetUserDataFile());
	if (!user_data_path.get_file().is_empty()) {
		user_model->LoadUserDataJson(user_data_path.utf8().get_data());
	}

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

Live2DModelInstance::Live2DModelInstance() {
	user_model = memnew(CubismUserModelExtend);

	user_model->BindBase(get_canvas_item().get_id());
}

Live2DModelInstance::~Live2DModelInstance() {
	if (user_model) {
		memdelete(user_model);
	}
}
