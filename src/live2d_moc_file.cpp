#include "live2d_moc_file.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
#elif defined(GODOT_MODULE)
#include "core/io/file_access.h"
#endif

void Live2DMocFile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_data"), &Live2DMocFile::get_data);
	ClassDB::bind_method(D_METHOD("get_load_path"), &Live2DMocFile::get_load_path);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "load_path", PROPERTY_HINT_FILE, "*.moc3", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "", "get_load_path");
}

PackedByteArray Live2DMocFile::get_data() const {
	return data;
}

Error Live2DMocFile::load(String p_path) {
#ifdef GDEXTENSION
	if (!FileAccess::file_exists(p_path))
#elif defined(GODOT_MODULE)
	if (!FileAccess::exists(p_path))
#endif
	{
		return ERR_FILE_NOT_FOUND;
	}

	data = FileAccess::get_file_as_bytes(p_path);
	if (!data.is_empty()) {
		path_to_file = p_path;
		return OK;
	}

	return FAILED;
}

String Live2DMocFile::get_load_path() const {
	return path_to_file;
}

/**************************************************************************/

#ifdef GDEXTENSION
Variant ResourceFormatLoaderLive2DMocFile::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	Ref<Live2DMocFile> live2d_model_data;
	live2d_model_data.instantiate();

	if (live2d_model_data->load(p_path) != OK) {
		return Ref<Resource>();
	}

	return live2d_model_data;
}

PackedStringArray ResourceFormatLoaderLive2DMocFile::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("moc3");
	return extensions;
}

bool ResourceFormatLoaderLive2DMocFile::_handles_type(const StringName &p_type) const {
	return ClassDB::is_parent_class(p_type, "Live2DMocFile");
}

String ResourceFormatLoaderLive2DMocFile::_get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "moc3") {
		return "Live2DMocFile";
	}
	return "";
}
#elif defined(GODOT_MODULE)
Ref<Resource> ResourceFormatLoaderLive2DMocFile::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	Ref<Live2DMocFile> live2d_model_data;
	live2d_model_data.instantiate();

	if (live2d_model_data->load(p_path) != OK) {
		return Ref<Resource>();
	}

	return live2d_model_data;
}

void ResourceFormatLoaderLive2DMocFile::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("moc3");
}

bool ResourceFormatLoaderLive2DMocFile::handles_type(const String &p_type) const {
	return ClassDB::is_parent_class(p_type, "Live2DMocFile");
}

String ResourceFormatLoaderLive2DMocFile::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "moc3") {
		return "Live2DMocFile";
	}
	return "";
}
#endif
