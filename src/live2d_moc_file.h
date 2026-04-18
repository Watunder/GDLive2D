#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/variant/variant.h"
#endif

class Live2DMocFile : public Resource {
	GDCLASS(Live2DMocFile, Resource);

private:
	PackedByteArray data;

	String path_to_file;

protected:
	static void _bind_methods();

public:
	PackedByteArray get_data() const;

	Error load(String p_path);
	String get_load_path() const;
};

/**************************************************************************/

class ResourceFormatLoaderLive2DModelData : public ResourceFormatLoader {
	GDCLASS(ResourceFormatLoaderLive2DModelData, ResourceFormatLoader);

protected:
	static void _bind_methods() {}

public:
#ifdef GDEXTENSION
	virtual Variant _load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const override;
	virtual PackedStringArray _get_recognized_extensions() const override;
	virtual bool _handles_type(const StringName &p_type) const override;
	virtual String _get_resource_type(const String &p_path) const override;
#elif defined(GODOT_MODULE)
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
#endif
};
