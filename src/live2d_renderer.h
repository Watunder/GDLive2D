#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/templates/vector.h"
#include "core/variant/variant.h"
#endif

#include <CubismFramework.hpp>
#include <Math/CubismMatrix44.hpp>
#include <Model/CubismModel.hpp>

class Live2DUserModel;

class Live2DRenderer {
public:
	enum class DrawableObjectType : int32_t {
		Drawable = 0,
		Offscreen = 2,
	};

	enum class CubismBlendMode : int32_t {
		Normal = 0,
		Additive = 1,
		Multiplicative = 2,
		Mask = 3,
		Copy = 4,
	};

private:
	Live2DUserModel *user_model = nullptr;

	Vector<int32_t> sorted_objects_index_list;
	Vector<DrawableObjectType> sorted_objects_type_list;

	PackedFloat32Array mvp_matrix;

	void _draw_sorted();
	void _render_drawable(int32_t drawable_index);
	void _render_offscreen(int32_t offscreen_index);

public:
	Live2DRenderer(uint32_t width, uint32_t height);
	virtual ~Live2DRenderer();

	void initialize(Live2DUserModel *p_user_model);
	void initialize(Live2DUserModel *p_user_model, int32_t mask_buffer_count);

	void set_mvp_matrix(Csm::CubismMatrix44 *p_matrix);

	void draw_model();
};
