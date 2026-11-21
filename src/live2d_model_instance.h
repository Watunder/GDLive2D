#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/resource_loader.h"
#include "core/variant/variant.h"
#include "scene/2d/node_2d.h"
#include "scene/animation/animation_player.h"
#include "scene/main/viewport.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/texture.h"
#endif

#include "live2d_moc_file.h"
#include "live2d_user_model.h"

class Live2DModelInstance : public Node2D {
	GDCLASS(Live2DModelInstance, Node2D);

private:
	Live2DUserModel *user_model = nullptr;

	AnimationPlayer *animation_player = nullptr;
	SubViewport *mask_viewport = nullptr;
	Node2D *mask_container = nullptr;

	String model_entry_path;

	Ref<Live2DMocFile> moc_file;
	Vector<Ref<Texture2D>> textures;

	bool is_importing = false;

	PackedStringArray parameter_ids;
	PackedStringArray part_ids;
	Vector<PackedStringArray> pose_groups;

	void _setup_moc_file(const String &p_model_dir);
	void _setup_textures(const String &p_model_dir);
	void _setup_pose_groups(const String &p_model_dir);

	void _update_animation_player(const String &p_model_dir);
	void _update_model_properties();

	Size2 _get_screen_size();
	void _update_projection(Csm::CubismMatrix44 &r_projection);

protected:
	void _notification(int p_what);
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	void set_model_entry(const String &p_model_entry_path);
	String get_model_entry() const;

	Ref<Live2DMocFile> get_moc_file() const;
	TypedArray<Texture2D> get_textures() const;

	static Ref<PackedScene> create_packed_scene(const String &p_model_entry_path);

	Live2DModelInstance(bool p_is_importing = false);
	~Live2DModelInstance();
};

class ResourceFormatLoaderLive2DModel : public ResourceFormatLoader {
	GDCLASS(ResourceFormatLoaderLive2DModel, ResourceFormatLoader);

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
