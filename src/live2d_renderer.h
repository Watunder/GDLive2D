#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/templates/vector.h"
#include "core/variant/variant.h"
#include "scene/resources/material.h"
#endif

#include <CubismFramework.hpp>
#include <Math/CubismMatrix44.hpp>
#include <Model/CubismModel.hpp>
#include <Rendering/CubismRenderer.hpp>

class Live2DClippingContext;
class Live2DClippingManager;

struct Live2DRenderContext {
	RID base_rid;
	RID mask_viewport_rid;
	RID mask_container_rid;
	Vector<RID> texture_rids;

	RID get_texture_rid(int32_t p_index) const {
		if (p_index < 0 || p_index >= static_cast<int32_t>(texture_rids.size())) {
			return RID();
		}
		return texture_rids[p_index];
	}
};

class Live2DRenderer {
public:
	static constexpr int32_t MASK_TARGET_SIZE = 256;

	struct DrawableRenderData {
		int32_t drawable_index = -1;
		bool masked = false;

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
	Live2DRenderContext render_context;

	PackedFloat32Array mvp_matrix;

	Vector<DrawableRenderData> drawables;

	Live2DClippingManager *clipping_manager = nullptr;
	Vector<MaskSourceRenderData> mask_sources;

	RID mask_background_rid;
	RID mask_texture_rid;

	bool _has_mask_target() const;

	void _release_masks();
	void _build_masks(Csm::CubismModel &p_model);

	void _update_mask_texture();

	void _release_mask_target();
	void _build_mask_target(Csm::CubismModel &p_model);
	void _update_mask_target(Csm::CubismModel &p_model);

	void _release_drawables();
	void _build_drawables(Csm::CubismModel &p_model);

	void _render_drawable(Csm::CubismModel &p_model, int32_t p_drawable_index);

public:
	Live2DRenderer(const Live2DRenderContext &p_context);
	virtual ~Live2DRenderer();

	void initialize(Csm::CubismModel &p_model, int32_t p_mask_buffer_count = 1);

	void update_masks(Csm::CubismModel &p_model);

	void update_model(Csm::CubismModel &p_model);

	void set_mvp_matrix(Csm::CubismMatrix44 *p_matrix);

	void draw_model(Csm::CubismModel &p_model);
};
