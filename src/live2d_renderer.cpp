#include "live2d_renderer.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/rendering_server.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "servers/rendering_server.h"
#endif

#include "live2d_user_model.h"

#include <CubismFramework.hpp>
#include <Model/CubismModel.hpp>

// make the building pass
void Csm::Rendering::CubismRenderer::StaticRelease() {
}

/**************************************************************************/

static float apply_screen_color(float base_value, float screen_value) {
	const float b = CLAMP(base_value, 0.0f, 1.0f);
	const float s = CLAMP(screen_value, 0.0f, 1.0f);
	return 1.0f - (1.0f - b) * (1.0f - s);
}

Live2DRenderer::Live2DRenderer(uint32_t /*width*/, uint32_t /*height*/) {
	mvp_matrix.resize(16);
	float *w = mvp_matrix.ptrw();
	std::memset(w, 0, sizeof(float) * 16);
	w[0] = w[5] = w[10] = w[15] = 1.0f;
}

Live2DRenderer::~Live2DRenderer() {
}

void Live2DRenderer::initialize(Live2DUserModel *p_user_model) {
	initialize(p_user_model, 1);
}

void Live2DRenderer::initialize(Live2DUserModel *p_user_model, int32_t /*mask_buffer_count*/) {
	ERR_FAIL_COND(!p_user_model);
	user_model = p_user_model;

	Csm::CubismModel *model = user_model->get_model();
	if (model && model->IsBlendModeEnabled()) {
		CubismLogInfo("This model uses a high-resolution mask because it operates in blend mode.");
	}
	if (model && model->IsUsingMasking()) {
		WARN_PRINT_ONCE_ED("[GDLive2D] Drawable clipping masks are not supported.");
	}
	if (model && model->IsUsingMaskingForOffscreen()) {
		WARN_PRINT_ONCE_ED("[GDLive2D] Offscreen clipping masks are not supported.");
	}
}

void Live2DRenderer::set_mvp_matrix(Csm::CubismMatrix44 *p_matrix) {
	ERR_FAIL_COND(!p_matrix);
	if (mvp_matrix.size() != 16) {
		mvp_matrix.resize(16);
	}
	const Csm::csmFloat32 *src = p_matrix->GetArray();
	float *dst = mvp_matrix.ptrw();
	std::memcpy(dst, src, sizeof(float) * 16);
}

void Live2DRenderer::draw_model() {
	if (!user_model || !user_model->get_model()) {
		return;
	}
	_draw_sorted();
}

void Live2DRenderer::_render_drawable(int32_t drawable_index) {
	ERR_FAIL_COND(!user_model);
	Csm::CubismModel *model = user_model->get_model();
	ERR_FAIL_COND(!model);

	if (!model->GetDrawableDynamicFlagIsVisible(drawable_index)) {
		return;
	}

	const Csm::csmInt32 vcount = model->GetDrawableVertexCount(drawable_index);
	const Csm::csmInt32 icount = model->GetDrawableVertexIndexCount(drawable_index);
	if (vcount <= 0 || icount <= 0) {
		return;
	}

	const Csm::csmBlendMode blend_mode = model->GetDrawableBlendModeType(drawable_index);
	if (blend_mode.GetColorBlendType() != 0 || blend_mode.GetAlphaBlendType() != 0) {
		WARN_PRINT_ONCE_ED("[GDLive2D] The blend mode of drawable is not implemented right now.");
		return;
	}

	const Csm::csmFloat32 *vertices = model->GetDrawableVertices(drawable_index);
	const Live2D::Cubism::Core::csmVector2 *vertex_uvs = model->GetDrawableVertexUvs(drawable_index);
	const Csm::csmUint16 *vertex_indices = model->GetDrawableVertexIndices(drawable_index);
	ERR_FAIL_COND(!vertices || !vertex_uvs || !vertex_indices);

	PackedVector2Array pts;
	PackedVector2Array uvs;
	PackedColorArray cols;
	pts.resize(vcount);
	uvs.resize(vcount);
	cols.resize(vcount);

	const float opacity = model->GetDrawableOpacity(drawable_index) * model->GetModelOpacity();
	const Live2D::Cubism::Core::csmVector4 mult = model->GetDrawableMultiplyColor(drawable_index);
	const Live2D::Cubism::Core::csmVector4 screen = model->GetDrawableScreenColor(drawable_index);
	const float cr = apply_screen_color(mult.X, screen.X);
	const float cg = apply_screen_color(mult.Y, screen.Y);
	const float cb = apply_screen_color(mult.Z, screen.Z);
	const float ca = CLAMP(opacity * mult.W, 0.0f, 1.0f);
	const Color vertex_color(cr, cg, cb, ca);

	Vector2 *pw = pts.ptrw();
	Vector2 *uw = uvs.ptrw();
	Color *cw = cols.ptrw();

	ERR_FAIL_COND(mvp_matrix.size() != 16);
	const float *mvp = mvp_matrix.ptr();

	for (Csm::csmInt32 vi = 0; vi < vcount; ++vi) {
		float lx = 0.0f, ly = 0.0f;

		const Csm::csmFloat32 vx = vertices[Csm::Constant::VertexOffset + vi * Csm::Constant::VertexStep];
		const Csm::csmFloat32 vy = vertices[Csm::Constant::VertexOffset + vi * Csm::Constant::VertexStep + 1];

		const Csm::csmFloat32 x = mvp[0] * vx + mvp[4] * vy + mvp[12];
		const Csm::csmFloat32 y = mvp[1] * vx + mvp[5] * vy + mvp[13];
		const Csm::csmFloat32 w = mvp[3] * vx + mvp[7] * vy + mvp[15];
		if (w != 0.0f && w != 1.0f) {
			lx = x / w;
			ly = y / w;
		} else {
			lx = x;
			ly = y;
		}

		pw[vi] = Vector2(lx, -ly);
		uw[vi] = Vector2(vertex_uvs[vi].X, 1.0f - vertex_uvs[vi].Y);
		cw[vi] = vertex_color;
	}

	PackedInt32Array indices;
	indices.resize(icount);
	int32_t *iw = indices.ptrw();
	for (Csm::csmInt32 ii = 0; ii < icount; ++ii) {
		iw[ii] = vertex_indices[ii];
	}

	const RID ci = user_model->get_base_rid();
	ERR_FAIL_COND(!ci.is_valid());

	const Csm::csmInt32 texture_index = model->GetDrawableTextureIndex(drawable_index);
	ERR_FAIL_COND(texture_index < 0);

	const RID tex = user_model->get_texture_rid(texture_index);
	ERR_FAIL_COND(!tex.is_valid());

	RenderingServer::get_singleton()->canvas_item_add_triangle_array(ci, indices, pts, cols, uvs, PackedInt32Array(), PackedFloat32Array(), tex);
}

void Live2DRenderer::_render_offscreen(int32_t /*p_offscreen_index*/) {
	WARN_PRINT_ONCE_ED("[GDLive2D] The offscreen feature is not implemented right now.");
}

void Live2DRenderer::_draw_sorted() {
	ERR_FAIL_COND(!user_model);
	Csm::CubismModel *model = user_model->get_model();
	ERR_FAIL_COND(!model);

	const Csm::csmInt32 drawable_count = model->GetDrawableCount();
	const Csm::csmInt32 offscreen_count = model->GetOffscreenCount();
	const int32_t total_count = drawable_count + offscreen_count;
	const Csm::csmInt32 *render_order = model->GetRenderOrders();

	ERR_FAIL_COND(!render_order || drawable_count <= 0 || total_count <= 0);

	sorted_objects_type_list.resize(total_count);
	sorted_objects_index_list.resize(total_count);
	DrawableObjectType *types_w = sorted_objects_type_list.ptrw();
	int32_t *indices_w = sorted_objects_index_list.ptrw();
	for (int32_t i = 0; i < total_count; ++i) {
		types_w[i] = DrawableObjectType::Drawable;
		indices_w[i] = 0;
	}

	for (int32_t i = 0; i < total_count; ++i) {
		const Csm::csmInt32 order = render_order[i];
		if (i < drawable_count) {
			types_w[order] = DrawableObjectType::Drawable;
			indices_w[order] = i;
		} else {
			types_w[order] = DrawableObjectType::Offscreen;
			indices_w[order] = i - drawable_count;
		}
	}

	for (int32_t i = 0; i < total_count; ++i) {
		const DrawableObjectType object_type = sorted_objects_type_list[i];
		const int32_t object_index = sorted_objects_index_list[i];

		switch (object_type) {
			case DrawableObjectType::Drawable: {
				_render_drawable(object_index);
			} break;
			case DrawableObjectType::Offscreen: {
				_render_offscreen(object_index);
			} break;
			default: {
				CubismLogError("Unknown drawable type: %d", (int32_t)object_type);
			} break;
		}
	}
}
