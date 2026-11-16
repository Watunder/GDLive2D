#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"
#include "scene/resources/material.h"
#endif

#include <CubismFramework.hpp>
#include <Math/CubismMatrix44.hpp>
#include <Model/CubismModel.hpp>
#include <Rendering/CubismRenderer.hpp>

class Live2DUserModel;
class Live2DClippingContext;
class Live2DClippingManager;

class Live2DRenderer {
public:
	static constexpr int32_t MASK_TARGET_SIZE = 256;

	struct DrawableRenderData {
		int32_t drawable_index = -1;
		bool masked = false;
		Live2DClippingContext *mask_context = nullptr;

		RID canvas_item_rid;
		RID backbuffer_copy_rid;
		RID texture_rid;
		RID mesh_rid;
		Ref<ShaderMaterial> material;
	};

	struct MaskSourceRenderData {
		int32_t context_index = -1;
		int32_t drawable_index = -1;

		RID canvas_item_rid;
		RID texture_rid;
		RID mesh_rid;
		Ref<ShaderMaterial> material;
	};

private:
	Live2DUserModel *user_model = nullptr;

	PackedFloat32Array mvp_matrix;

	Vector<DrawableRenderData> drawables;

	Live2DClippingManager *clipping_manager = nullptr;
	Vector<MaskSourceRenderData> mask_sources;

	RID mask_viewport_rid;
	RID mask_container_rid;
	RID mask_background_rid;
	RID mask_texture_rid;

	bool _has_mask_target() const;

	void _build_masks();
	void _release_masks();
	void _update_mask_texture();

	void _build_mask_target();
	void _release_mask_target();
	void _update_mask_target();

	void _build_drawables();
	void _release_drawables();

	void _render_drawable(int32_t p_drawable_index);

public:
	Live2DRenderer(uint32_t p_width, uint32_t p_height);
	virtual ~Live2DRenderer();

	void initialize(Live2DUserModel *p_user_model);
	void initialize(Live2DUserModel *p_user_model, int32_t p_mask_buffer_count);

	void bind_mask_targets(const RID &p_viewport_rid, const RID &p_mask_container_rid);

	void update_masks();

	void update_model();

	void set_mvp_matrix(Csm::CubismMatrix44 *p_matrix);

	void draw_model();
};
