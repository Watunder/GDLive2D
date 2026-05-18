#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/editor_import_plugin.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/resource_uid.h"
#include "editor/import/editor_import_plugin.h"
#endif

class Live2DExpressionImporter : public EditorImportPlugin {
	GDCLASS(Live2DExpressionImporter, EditorImportPlugin);

protected:
	static void _bind_methods() {}

public:
#ifdef GDEXTENSION
	virtual String _get_importer_name() const override;
	virtual String _get_visible_name() const override;
	virtual int32_t _get_preset_count() const override;
	virtual String _get_preset_name(int32_t p_preset_index) const override;
	virtual PackedStringArray _get_recognized_extensions() const override;
	virtual TypedArray<Dictionary> _get_import_options(const String &p_path, int32_t p_preset_index) const override;
	virtual String _get_save_extension() const override;
	virtual String _get_resource_type() const override;
	virtual float _get_priority() const override;
	virtual int32_t _get_import_order() const override;
	virtual int32_t _get_format_version() const override;
	virtual bool _get_option_visibility(const String &p_path, const StringName &p_option_name, const Dictionary &p_options) const override;
	virtual Error _import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &p_platform_variants, const TypedArray<String> &p_gen_files) const override;
	virtual bool _can_import_threaded() const override;
#elif defined(GODOT_MODULE)
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual void get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options, int p_preset) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual float get_priority() const override;
	virtual int get_import_order() const override;
	virtual int get_format_version() const override;
	virtual bool get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const override;
	virtual Error import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata = nullptr) override;
	virtual bool can_import_threaded() const override;
#endif
};
