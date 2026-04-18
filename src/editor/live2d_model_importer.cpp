#include "live2d_model_importer.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#elif defined(GODOT_MODULE)
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/packed_scene.h"
#endif

#include "../live2d_model_instance.h"

#include <CubismModelSettingJson.hpp>

#ifdef GDEXTENSION
String Live2DModelImporter::_get_importer_name() const {
	return "live2d_model";
}

String Live2DModelImporter::_get_visible_name() const {
	return "Live2D Model (GDLive2D)";
}

int32_t Live2DModelImporter::_get_preset_count() const {
	return 1;
}

String Live2DModelImporter::_get_preset_name(int32_t p_preset_index) const {
	ERR_FAIL_COND_V(p_preset_index != 0, "Default");
	return "Default";
}

PackedStringArray Live2DModelImporter::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("model3.json");
	return extensions;
}

TypedArray<Dictionary> Live2DModelImporter::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	TypedArray<Dictionary> opts;
	return opts;
}

String Live2DModelImporter::_get_save_extension() const {
	return "tscn";
}

String Live2DModelImporter::_get_resource_type() const {
	return "PackedScene";
}

float Live2DModelImporter::_get_priority() const {
	return 1.0f;
}

int32_t Live2DModelImporter::_get_import_order() const {
	return ResourceImporter::IMPORT_ORDER_SCENE;
}

int32_t Live2DModelImporter::_get_format_version() const {
	return 1;
}

bool Live2DModelImporter::_get_option_visibility(const String &p_path, const StringName &p_option_name, const Dictionary &p_options) const {
	return true;
}

Error Live2DModelImporter::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const {
	if (!p_source_file.ends_with(".model3.json")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	Live2DModelInstance *instance = memnew(Live2DModelInstance);
	instance->set_name(p_source_file.get_base_dir().get_file());
	instance->set_model_entry(p_source_file);

	Ref<PackedScene> packed;
	packed.instantiate();
	Error pack_err = packed->pack(instance);
	memdelete(instance);
	if (pack_err != OK) {
		ERR_PRINT(String("[GDLive2D] failed to pack scene for: ") + p_source_file);
		return pack_err;
	}

	const String out_path = p_save_path + String(".") + _get_save_extension();
	return ResourceSaver::get_singleton()->save(packed, out_path);
}

bool Live2DModelImporter::_can_import_threaded() const {
	return false;
}
#elif defined(GODOT_MODULE)
String Live2DModelImporter::get_importer_name() const {
	return "live2d_model";
}

String Live2DModelImporter::get_visible_name() const {
	return "Live2D Model (GDLive2D)";
}

int Live2DModelImporter::get_preset_count() const {
	return 1;
}

String Live2DModelImporter::get_preset_name(int p_idx) const {
	ERR_FAIL_COND_V_MSG(p_idx != 0, String(), vformat("Invalid preset index: %d", p_idx));
	return "Default";
}

void Live2DModelImporter::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("model3.json");
}

void Live2DModelImporter::get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options, int p_preset) const {
}

String Live2DModelImporter::get_save_extension() const {
	return "tscn";
}

String Live2DModelImporter::get_resource_type() const {
	return "PackedScene";
}

float Live2DModelImporter::get_priority() const {
	return 1.0f;
}

int Live2DModelImporter::get_import_order() const {
	return ResourceImporter::IMPORT_ORDER_SCENE;
}

int Live2DModelImporter::get_format_version() const {
	return 1;
}

bool Live2DModelImporter::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	return true;
}

Error Live2DModelImporter::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	if (!p_source_file.ends_with(".model3.json")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	Live2DModelInstance *instance = memnew(Live2DModelInstance);
	instance->set_name(p_source_file.get_base_dir().get_basename());
	instance->set_model_entry(p_source_file);

	Ref<PackedScene> packed;
	packed.instantiate();
	Error pack_err = packed->pack(instance);
	memdelete(instance);
	if (pack_err != OK) {
		ERR_PRINT(vformat("[GDLive2D] failed to pack scene for: %s", p_source_file));
		return pack_err;
	}

	const String out_path = p_save_path + "." + get_save_extension();
	return ResourceSaver::save(packed, out_path);
}

bool Live2DModelImporter::can_import_threaded() const {
	return false;
}
#endif
