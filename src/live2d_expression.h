#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/variant/variant.h"
#include "scene/resources/animation.h"
#endif

class Live2DExpression : public Animation {
	GDCLASS(Live2DExpression, Animation);

private:
	String path_to_file;

	float fade_in_time = 1.0f;
	float fade_out_time = 1.0f;

	// The "Blend" parameter (Add/Multiply/Overwrite)
	Dictionary blend_modes;

	void _from_dict(const Dictionary &p_dict);

protected:
	static void _bind_methods();

public:
	Error load(String p_path);
	String get_load_path() const;

	float get_fade_in_time() const { return fade_in_time; }
	float get_fade_out_time() const { return fade_out_time; }
	Dictionary get_blend_modes() const { return blend_modes.duplicate(); }
};

class ResourceFormatLoaderLive2DExpression : public ResourceFormatLoader {
	GDCLASS(ResourceFormatLoaderLive2DExpression, ResourceFormatLoader);

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
