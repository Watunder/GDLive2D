#include "live2d_motion_importer.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/resource_saver.hpp>
#elif defined(GODOT_MODULE)
#include "core/io/resource_saver.h"
#endif

#include "../live2d_motion.h"

#ifdef GDEXTENSION
String Live2DMotionImporter::_get_importer_name() const {
	return "live2d_motion";
}

String Live2DMotionImporter::_get_visible_name() const {
	return "Live2D Motion (GDLive2D)";
}

int32_t Live2DMotionImporter::_get_preset_count() const {
	return 1;
}

String Live2DMotionImporter::_get_preset_name(int32_t p_preset_index) const {
	ERR_FAIL_COND_V(p_preset_index != 0, "Default");
	return "Default";
}

PackedStringArray Live2DMotionImporter::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("motion3.json");
	return extensions;
}

TypedArray<Dictionary> Live2DMotionImporter::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	TypedArray<Dictionary> opts;
	return opts;
}

String Live2DMotionImporter::_get_save_extension() const {
	return "res";
}

String Live2DMotionImporter::_get_resource_type() const {
	return "Live2DMotion";
}

float Live2DMotionImporter::_get_priority() const {
	return 1.0f;
}

int32_t Live2DMotionImporter::_get_import_order() const {
	return ResourceImporter::IMPORT_ORDER_DEFAULT;
}

int32_t Live2DMotionImporter::_get_format_version() const {
	return 1;
}

bool Live2DMotionImporter::_get_option_visibility(const String &p_path, const StringName &p_option_name, const Dictionary &p_options) const {
	return true;
}

Error Live2DMotionImporter::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const {
	if (!p_source_file.ends_with(".motion3.json")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	Ref<Live2DMotion> motion_file;
	motion_file.instantiate();
	Error load_err = motion_file->load(p_source_file);
	if (load_err != OK) {
		ERR_PRINT(String("[GDLive2D] failed to load motion file: ") + p_source_file);
		return load_err;
	}

	const String out_path = p_save_path + String(".") + _get_save_extension();
	return ResourceSaver::get_singleton()->save(motion_file, out_path);
}

bool Live2DMotionImporter::_can_import_threaded() const {
	return false;
}
#elif defined(GODOT_MODULE)
String Live2DMotionImporter::get_importer_name() const {
	return "live2d_motion";
}

String Live2DMotionImporter::get_visible_name() const {
	return "Live2D Motion (GDLive2D)";
}

int Live2DMotionImporter::get_preset_count() const {
	return 1;
}

String Live2DMotionImporter::get_preset_name(int p_idx) const {
	ERR_FAIL_COND_V_MSG(p_idx != 0, String(), vformat("Invalid preset index: %d", p_idx));
	return "Default";
}

void Live2DMotionImporter::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("motion3.json");
}

void Live2DMotionImporter::get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options, int p_preset) const {
}

String Live2DMotionImporter::get_save_extension() const {
	return "res";
}

String Live2DMotionImporter::get_resource_type() const {
	return "Live2DMotion";
}

float Live2DMotionImporter::get_priority() const {
	return 1.0f;
}

int Live2DMotionImporter::get_import_order() const {
	return ResourceImporter::IMPORT_ORDER_DEFAULT;
}

int Live2DMotionImporter::get_format_version() const {
	return 1;
}

bool Live2DMotionImporter::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	return true;
}

Error Live2DMotionImporter::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	if (!p_source_file.ends_with(".motion3.json")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	Ref<Live2DMotion> motion_file;
	motion_file.instantiate();
	Error load_err = motion_file->load(p_source_file);
	if (load_err != OK) {
		ERR_PRINT(vformat("[GDLive2D] failed to load motion file: %s", p_source_file));
		return load_err;
	}

	const String out_path = p_save_path + "." + get_save_extension();
	return ResourceSaver::save(motion_file, out_path);
}

bool Live2DMotionImporter::can_import_threaded() const {
	return false;
}
#endif
