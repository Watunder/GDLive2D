#include "live2d_expression_importer.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/resource_saver.hpp>
#elif defined(GODOT_MODULE)
#include "core/io/resource_saver.h"
#endif

#include "../live2d_expression.h"

#ifdef GDEXTENSION
String Live2DExpressionImporter::_get_importer_name() const {
	return "live2d_expression";
}

String Live2DExpressionImporter::_get_visible_name() const {
	return "Live2D Expression (GDLive2D)";
}

int32_t Live2DExpressionImporter::_get_preset_count() const {
	return 1;
}

String Live2DExpressionImporter::_get_preset_name(int32_t p_preset_index) const {
	ERR_FAIL_COND_V(p_preset_index != 0, "Default");
	return "Default";
}

PackedStringArray Live2DExpressionImporter::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("exp3.json");
	return extensions;
}

TypedArray<Dictionary> Live2DExpressionImporter::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	TypedArray<Dictionary> opts;
	return opts;
}

String Live2DExpressionImporter::_get_save_extension() const {
	return "res";
}

String Live2DExpressionImporter::_get_resource_type() const {
	return "Live2DExpression";
}

float Live2DExpressionImporter::_get_priority() const {
	return 1.0f;
}

int32_t Live2DExpressionImporter::_get_import_order() const {
	return ResourceImporter::IMPORT_ORDER_DEFAULT;
}

int32_t Live2DExpressionImporter::_get_format_version() const {
	return 1;
}

bool Live2DExpressionImporter::_get_option_visibility(const String &p_path, const StringName &p_option_name, const Dictionary &p_options) const {
	return true;
}

Error Live2DExpressionImporter::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const {
	if (!p_source_file.ends_with(".exp3.json")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	Ref<Live2DExpression> expression;
	expression.instantiate();
	const Error load_err = expression->load(p_source_file);
	if (load_err != OK) {
		ERR_PRINT(String("[GDLive2D] failed to load expression file: ") + p_source_file);
		return load_err;
	}

	const String out_path = p_save_path + String(".") + _get_save_extension();
	return ResourceSaver::get_singleton()->save(expression, out_path);
}

bool Live2DExpressionImporter::_can_import_threaded() const {
	return false;
}
#elif defined(GODOT_MODULE)
String Live2DExpressionImporter::get_importer_name() const {
	return "live2d_expression";
}

String Live2DExpressionImporter::get_visible_name() const {
	return "Live2D Expression (GDLive2D)";
}

int Live2DExpressionImporter::get_preset_count() const {
	return 1;
}

String Live2DExpressionImporter::get_preset_name(int p_idx) const {
	ERR_FAIL_COND_V_MSG(p_idx != 0, String(), vformat("Invalid preset index: %d", p_idx));
	return "Default";
}

void Live2DExpressionImporter::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("exp3.json");
}

void Live2DExpressionImporter::get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options, int p_preset) const {
}

String Live2DExpressionImporter::get_save_extension() const {
	return "res";
}

String Live2DExpressionImporter::get_resource_type() const {
	return "Live2DExpression";
}

float Live2DExpressionImporter::get_priority() const {
	return 1.0f;
}

int Live2DExpressionImporter::get_import_order() const {
	return ResourceImporter::IMPORT_ORDER_DEFAULT;
}

int Live2DExpressionImporter::get_format_version() const {
	return 1;
}

bool Live2DExpressionImporter::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	return true;
}

Error Live2DExpressionImporter::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	if (!p_source_file.ends_with(".exp3.json")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	Ref<Live2DExpression> expression;
	expression.instantiate();
	const Error load_err = expression->load(p_source_file);
	if (load_err != OK) {
		ERR_PRINT(vformat("[GDLive2D] failed to load expression file: %s", p_source_file));
		return load_err;
	}

	const String out_path = p_save_path + "." + get_save_extension();
	return ResourceSaver::save(expression, out_path);
}

bool Live2DExpressionImporter::can_import_threaded() const {
	return false;
}
#endif
