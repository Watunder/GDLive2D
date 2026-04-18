#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/variant/variant.h"
#include "scene/2d/node_2d.h"
#include "scene/resources/texture.h"
#endif

#include "cubism_user_model_extend.h"
#include "live2d_moc_file.h"

class Live2DModelInstance : public Node2D {
	GDCLASS(Live2DModelInstance, Node2D);

private:
	CubismUserModelExtend *user_model = nullptr;

	String model_entry_path;

	Ref<Live2DMocFile> moc_file;
	Vector<Ref<Texture2D>> textures;

	void _setup_moc_file(const String &p_model_dir);
	void _setup_textures(const String &p_model_dir);

	Size2 _get_screen_size();
	void _update_projection(Csm::CubismMatrix44 &r_projection);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_model_entry(const String &p_model_entry_path);
	String get_model_entry() const;

	Ref<Live2DMocFile> get_moc_file() const;
	TypedArray<Texture2D> get_textures() const;

	Live2DModelInstance();
	~Live2DModelInstance();
};
