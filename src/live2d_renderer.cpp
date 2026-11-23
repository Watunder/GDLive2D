#include "live2d_renderer.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/marshalls.h"
#include "scene/resources/mesh.h"
#include "servers/rendering_server.h"
#endif

#include "live2d_shader_library.h"

#include <Rendering/CubismClippingManager.hpp>

#include <Rendering/CubismClippingManager.tpp>

// Stub implementation
void Csm::Rendering::CubismRenderer::StaticRelease() {}

using DrawableObjectType = Csm::Rendering::CubismRenderer::DrawableObjectType;

using Live2DClippingManagerBase = Csm::Rendering::CubismClippingManager<Live2DClippingContext, void>;

class Live2DClippingContext : public Csm::Rendering::CubismClippingContext {
public:
	Live2DClippingContext(Live2DClippingManagerBase *p_manager, Csm::CubismModel &p_model,
			const Csm::csmInt32 *p_clipping_drawable_indices, Csm::csmInt32 p_clip_count) :
			Csm::Rendering::CubismClippingContext(p_clipping_drawable_indices, p_clip_count) {
		(void)p_manager;
		(void)p_model;
	}
};

class Live2DClippingManager : public Live2DClippingManagerBase {
public:
	int32_t get_mask_context_count() const {
		return static_cast<int32_t>(_clippingContextListForMask.GetSize());
	}

	Live2DClippingContext *get_mask_context(int32_t p_index) const {
		return _clippingContextListForMask[p_index];
	}

	Live2DClippingContext *get_context_for_drawable(int32_t p_drawable_index) const {
		if (p_drawable_index < 0 || p_drawable_index >= static_cast<int32_t>(_clippingContextListForDraw.GetSize())) {
			return nullptr;
		}
		return _clippingContextListForDraw[p_drawable_index];
	}

	void setup_matrix_for_drawable(Live2DClippingContext *p_context) {
		Csm::csmRectF *all_clipped_draw_rect = p_context->_allClippedDrawRect;
		Csm::csmRectF *layout_bounds = p_context->_layoutBounds;
		const Csm::csmFloat32 MARGIN = 0.05f;

		_tmpBoundsOnModel.SetRect(all_clipped_draw_rect);
		_tmpBoundsOnModel.Expand(all_clipped_draw_rect->Width * MARGIN, all_clipped_draw_rect->Height * MARGIN);

		const Csm::csmFloat32 scale_x = layout_bounds->Width / _tmpBoundsOnModel.Width;
		const Csm::csmFloat32 scale_y = layout_bounds->Height / _tmpBoundsOnModel.Height;

		CreateMatrixForMask(false, layout_bounds, scale_x, scale_y);
		p_context->_matrixForMask.SetMatrix(_tmpMatrixForMask.GetArray());
		p_context->_matrixForDraw.SetMatrix(_tmpMatrixForDraw.GetArray());
	}

	const Csm::csmRectF &get_tmp_bounds_on_model() const {
		return _tmpBoundsOnModel;
	}
};

static void _encode_position(PackedByteArray &r_buffer, int32_t p_vertex_index, float p_x, float p_y) {
	const int32_t offset = p_vertex_index * 12;
#ifdef GDEXTENSION
	r_buffer.encode_float(offset, p_x);
	r_buffer.encode_float(offset + 4, p_y);
	r_buffer.encode_float(offset + 8, 0.0f);
#elif defined(GODOT_MODULE)
	uint8_t *w = r_buffer.ptrw();
	encode_float(p_x, &w[offset]);
	encode_float(p_y, &w[offset + 4]);
	encode_float(0.0f, &w[offset + 8]);
#endif
}

static int32_t _get_draw_index(const Csm::CubismModel &p_model, int32_t p_drawable_index) {
	const Csm::csmInt32 *render_order = p_model.GetRenderOrders();
	const int32_t drawable_count = p_model.GetDrawableCount();
	int32_t order = p_drawable_index;
	if (render_order && p_drawable_index >= 0 && p_drawable_index < drawable_count) {
		order = render_order[p_drawable_index];
	}
	if (order < 0) {
		order = 0;
	}
	return order * 4 + 3;
}

static Array _create_drawable_mesh_arrays(const Csm::CubismModel &p_model, int32_t p_drawable_index) {
	const Csm::csmInt32 vcount = p_model.GetDrawableVertexCount(p_drawable_index);
	const Csm::csmInt32 icount = p_model.GetDrawableVertexIndexCount(p_drawable_index);
	const Csm::csmUint16 *vertex_indices = p_model.GetDrawableVertexIndices(p_drawable_index);
	const Live2D::Cubism::Core::csmVector2 *vertex_uvs = p_model.GetDrawableVertexUvs(p_drawable_index);
	if (vcount <= 0 || icount <= 0 || !vertex_indices || !vertex_uvs) {
		return Array();
	}

	PackedVector3Array vertices;
	vertices.resize(vcount);
	Vector3 *vtx_w = vertices.ptrw();
	for (Csm::csmInt32 vi = 0; vi < vcount; ++vi) {
		vtx_w[vi] = Vector3(0, 0, 0);
	}

	PackedVector2Array uvs;
	uvs.resize(vcount);
	Vector2 *uv_w = uvs.ptrw();
	for (Csm::csmInt32 vi = 0; vi < vcount; ++vi) {
		uv_w[vi] = Vector2(vertex_uvs[vi].X, 1.0f - vertex_uvs[vi].Y);
	}

	PackedColorArray colors;
	colors.resize(vcount);
	Color *col_w = colors.ptrw();
	for (Csm::csmInt32 vi = 0; vi < vcount; ++vi) {
		col_w[vi] = Color(1, 1, 1, 1);
	}

	PackedInt32Array indices;
	indices.resize(icount);
	int32_t *idx_w = indices.ptrw();
	for (Csm::csmInt32 ii = 0; ii < icount; ++ii) {
		idx_w[ii] = vertex_indices[ii];
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_COLOR] = colors;
	arrays[Mesh::ARRAY_INDEX] = indices;

	return arrays;
}

static void _free_rid(const RID &p_rid) {
#ifdef GDEXTENSION
	RenderingServer::get_singleton()->free_rid(p_rid);
#elif defined(GODOT_MODULE)
	RenderingServer::get_singleton()->free(p_rid);
#endif
}

Ref<ShaderMaterial> Live2DRenderer::_get_material(const Ref<Shader> &p_shader, bool p_masked) {
	for (int32_t i = 0; i < shared_materials.size(); ++i) {
		const SharedShaderMaterial &sm = shared_materials.get(i);
		if (sm.material->get_shader() == p_shader) {
			return sm.material;
		}
	}

	SharedShaderMaterial new_sm;
	new_sm.material.instantiate();
	new_sm.material->set_shader(p_shader);
	new_sm.masked = p_masked;

	shared_materials.push_back(new_sm);

	return new_sm.material;
}

bool Live2DRenderer::_has_mask_target() const {
	ERR_FAIL_COND_V(!render_context.mask_viewport_rid.is_valid(), false);
	ERR_FAIL_COND_V(!render_context.mask_container_rid.is_valid(), false);
	return true;
}

void Live2DRenderer::_release_masks() {
	_release_mask_target();
	mask_texture_rid = RID();
}

void Live2DRenderer::_build_masks(Csm::CubismModel &p_model) {
	_release_masks();

	ERR_FAIL_COND(!clipping_manager);

	if (!_has_mask_target()) {
		return;
	}

	_build_mask_target(p_model);
}

void Live2DRenderer::_update_mask_texture() {
	if (!_has_mask_target()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	const RID texture_rid = rs->viewport_get_texture(render_context.mask_viewport_rid);
	if (!texture_rid.is_valid() || texture_rid == mask_texture_rid) {
		return;
	}

	mask_texture_rid = texture_rid;
	for (int32_t i = 0; i < shared_materials.size(); ++i) {
		const SharedShaderMaterial &sm = shared_materials.get(i);
		if (sm.masked) {
			sm.material->set_shader_parameter("mask_texture", mask_texture_rid);
		}
	}
}

void Live2DRenderer::_release_mask_target() {
	for (int32_t i = 0; i < mask_sources.size(); ++i) {
		MaskSourceRenderData &source = mask_sources.write[i];
		if (source.canvas_item_rid.is_valid()) {
			_free_rid(source.canvas_item_rid);
			source.canvas_item_rid = RID();
		}
		if (source.mesh_rid.is_valid()) {
			_free_rid(source.mesh_rid);
			source.mesh_rid = RID();
		}
	}
	mask_sources.clear();

	if (mask_background_rid.is_valid()) {
		_free_rid(mask_background_rid);
		mask_background_rid = RID();
	}
}

void Live2DRenderer::_build_mask_target(Csm::CubismModel &p_model) {
	ERR_FAIL_COND(!clipping_manager);

	if (!_has_mask_target()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();

	const Ref<ShaderMaterial> mask_material = _get_material(Live2DShaderLibrary::get_mask_shader(), false);

	mask_background_rid = rs->canvas_item_create();
	rs->canvas_item_set_parent(mask_background_rid, render_context.mask_container_rid);
	rs->canvas_item_set_draw_index(mask_background_rid, 0);
	rs->canvas_item_add_rect(mask_background_rid, Rect2(0, 0, MASK_TARGET_SIZE, MASK_TARGET_SIZE), Color(1, 1, 1, 1));

	const int32_t context_count = clipping_manager->get_mask_context_count();
	int32_t draw_index = 1;
	for (int32_t ci = 0; ci < context_count; ++ci) {
		Live2DClippingContext *context = clipping_manager->get_mask_context(ci);
		for (Csm::csmInt32 k = 0; k < context->_clippingIdCount; ++k) {
			const int32_t source_index = context->_clippingIdList[k];
			if (source_index < 0 || source_index >= p_model.GetDrawableCount()) {
				continue;
			}

			Array mesh_arrays = _create_drawable_mesh_arrays(p_model, source_index);
			if (mesh_arrays.is_empty()) {
				continue;
			}

			RID mesh_rid = rs->mesh_create();
			rs->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, mesh_arrays);

			RID canvas_item_rid = rs->canvas_item_create();
			rs->canvas_item_set_parent(canvas_item_rid, render_context.mask_container_rid);
			rs->canvas_item_set_draw_index(canvas_item_rid, draw_index++);
			rs->canvas_item_set_material(canvas_item_rid, mask_material->get_rid());

			MaskSourceRenderData data;
			data.context_index = ci;
			data.drawable_index = source_index;
			data.canvas_item_rid = canvas_item_rid;
			data.texture_rid = render_context.get_texture_rid(p_model.GetDrawableTextureIndex(source_index));
			data.mesh_rid = mesh_rid;
			mask_sources.push_back(data);
		}
	}
}

void Live2DRenderer::_update_mask_target(Csm::CubismModel &p_model) {
	ERR_FAIL_COND(!clipping_manager);

	RenderingServer *rs = RenderingServer::get_singleton();

	const int32_t context_count = clipping_manager->get_mask_context_count();
	int32_t using_count = 0;
	for (int32_t ci = 0; ci < context_count; ++ci) {
		Live2DClippingContext *context = clipping_manager->get_mask_context(ci);
		clipping_manager->CalcClippedTotalBounds(p_model, context, DrawableObjectType::DrawableObjectType_Drawable);
		if (context->_isUsing) {
			++using_count;
		}
	}

	if (using_count <= 0) {
		for (int32_t i = 0; i < mask_sources.size(); ++i) {
			rs->canvas_item_set_visible(mask_sources[i].canvas_item_rid, false);
		}
		return;
	}

	clipping_manager->SetupLayoutBounds(using_count);

	int32_t prepared_context = -1;
	for (int32_t i = 0; i < mask_sources.size(); ++i) {
		MaskSourceRenderData &source = mask_sources.write[i];
		Live2DClippingContext *context = clipping_manager->get_mask_context(source.context_index);

		if (!context->_isUsing) {
			rs->canvas_item_set_visible(source.canvas_item_rid, false);
			continue;
		}

		if (source.context_index != prepared_context) {
			clipping_manager->setup_matrix_for_drawable(context);
			prepared_context = source.context_index;
		}

		const Csm::csmRectF &bounds = clipping_manager->get_tmp_bounds_on_model();
		const Csm::csmRectF *layout = context->_layoutBounds;
		const Csm::Rendering::CubismRenderer::CubismTextureColor *channel = clipping_manager->GetChannelFlagAsColor(context->_layoutChannelIndex);
		const Csm::csmFloat32 scale_x = layout->Width / bounds.Width;
		const Csm::csmFloat32 scale_y = layout->Height / bounds.Height;

		const int32_t di = source.drawable_index;
		const Csm::csmInt32 vcount = p_model.GetDrawableVertexCount(di);
		const Csm::csmFloat32 *vertices = p_model.GetDrawableVertices(di);
		if (vcount <= 0 || !vertices) {
			rs->canvas_item_set_visible(source.canvas_item_rid, false);
			continue;
		}

		PackedByteArray update_buffer;
		update_buffer.resize(vcount * 12);
		for (Csm::csmInt32 vi = 0; vi < vcount; ++vi) {
			const Csm::csmFloat32 vx = vertices[Csm::Constant::VertexOffset + vi * Csm::Constant::VertexStep];
			const Csm::csmFloat32 vy = vertices[Csm::Constant::VertexOffset + vi * Csm::Constant::VertexStep + 1];

			_encode_position(update_buffer, vi,
					((vx - bounds.X) * scale_x + layout->X) * MASK_TARGET_SIZE,
					((vy - bounds.Y) * scale_y + layout->Y) * MASK_TARGET_SIZE);
		}
		rs->mesh_surface_update_vertex_region(source.mesh_rid, 0, 0, update_buffer);
		rs->mesh_set_custom_aabb(source.mesh_rid, AABB(Vector3(0, 0, 0), Vector3(MASK_TARGET_SIZE, MASK_TARGET_SIZE, 0)));

		rs->canvas_item_set_instance_shader_parameter(source.canvas_item_rid, "channel_flag", Vector4(channel->R, channel->G, channel->B, channel->A));
		rs->canvas_item_set_instance_shader_parameter(source.canvas_item_rid, "layout_bounds", Vector4(layout->X, layout->Y, layout->X + layout->Width, layout->Y + layout->Height));
		rs->canvas_item_set_instance_shader_parameter(source.canvas_item_rid, "mask_size", Vector2(MASK_TARGET_SIZE, MASK_TARGET_SIZE));

		rs->canvas_item_set_visible(source.canvas_item_rid, true);
		rs->canvas_item_clear(source.canvas_item_rid);
		rs->canvas_item_add_mesh(source.canvas_item_rid, source.mesh_rid, Transform2D(), Color(1, 1, 1, 1), source.texture_rid);
	}
}

void Live2DRenderer::_release_drawables() {
	for (int32_t i = 0; i < drawables.size(); ++i) {
		DrawableRenderData &d = drawables.write[i];
		if (d.canvas_item_rid.is_valid()) {
			_free_rid(d.canvas_item_rid);
			d.canvas_item_rid = RID();
		}
		if (d.backbuffer_copy_rid.is_valid()) {
			_free_rid(d.backbuffer_copy_rid);
			d.backbuffer_copy_rid = RID();
		}
		if (d.mesh_rid.is_valid()) {
			_free_rid(d.mesh_rid);
			d.mesh_rid = RID();
		}
	}
	drawables.clear();
}

void Live2DRenderer::_build_drawables(Csm::CubismModel &p_model) {
	_release_drawables();

	ERR_FAIL_COND(!render_context.base_rid.is_valid());

	const Csm::csmInt32 drawable_count = p_model.GetDrawableCount();
	if (drawable_count <= 0) {
		return;
	}

	drawables.resize(drawable_count);

	const Csm::csmInt32 *mask_counts = p_model.GetDrawableMaskCounts();

	RenderingServer *rs = RenderingServer::get_singleton();

	for (int32_t di = 0; di < drawable_count; ++di) {
		Array mesh_arrays = _create_drawable_mesh_arrays(p_model, di);
		if (mesh_arrays.is_empty()) {
			continue;
		}

		RID mesh_rid = rs->mesh_create();
		rs->mesh_add_surface_from_arrays(mesh_rid, RenderingServer::PRIMITIVE_TRIANGLES, mesh_arrays);

		RID canvas_item_rid = rs->canvas_item_create();
		rs->canvas_item_set_parent(canvas_item_rid, render_context.base_rid);

		const int32_t draw_index = _get_draw_index(p_model, di);
		rs->canvas_item_set_draw_index(canvas_item_rid, draw_index);

		const bool is_using_masking = mask_counts && mask_counts[di] > 0 && _has_mask_target();

		const Csm::csmBlendMode blend_mode = p_model.GetDrawableBlendModeType(di);
		const bool masked = is_using_masking && clipping_manager->get_context_for_drawable(di) != nullptr;

		Ref<ShaderMaterial> material = _get_material(Live2DShaderLibrary::get_drawable_shader(blend_mode, masked), masked);
		rs->canvas_item_set_material(canvas_item_rid, material->get_rid());

		RID backbuffer_copy_rid;
		if (Live2DShaderLibrary::requires_backbuffer(blend_mode)) {
			backbuffer_copy_rid = rs->canvas_item_create();
			rs->canvas_item_set_parent(backbuffer_copy_rid, render_context.base_rid);
			rs->canvas_item_set_draw_index(backbuffer_copy_rid, draw_index - 1);
			rs->canvas_item_set_copy_to_backbuffer(backbuffer_copy_rid, true, Rect2());
		}

		DrawableRenderData data;
		data.drawable_index = di;
		data.masked = masked;
		data.canvas_item_rid = canvas_item_rid;
		data.backbuffer_copy_rid = backbuffer_copy_rid;
		data.texture_rid = render_context.get_texture_rid(p_model.GetDrawableTextureIndex(di));
		data.mesh_rid = mesh_rid;
		drawables.write[di] = data;
	}
}

void Live2DRenderer::_render_drawable(Csm::CubismModel &p_model, int32_t drawable_index) {
	if (drawable_index < 0 || drawable_index >= drawables.size()) {
		return;
	}

	const DrawableRenderData &d = drawables.get(drawable_index);
	const RID ci = d.canvas_item_rid;
	if (!d.mesh_rid.is_valid() || !ci.is_valid()) {
		return;
	}

	const int32_t di = d.drawable_index;
	const Csm::csmInt32 vcount = p_model.GetDrawableVertexCount(di);
	if (vcount <= 0) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();

	const bool visible = p_model.GetDrawableDynamicFlagIsVisible(di);
	rs->canvas_item_set_visible(ci, visible);
	if (d.backbuffer_copy_rid.is_valid()) {
		rs->canvas_item_set_visible(d.backbuffer_copy_rid, visible);
	}
	if (!visible) {
		return;
	}

	const Csm::csmFloat32 *cubism_vertices = p_model.GetDrawableVertices(di);
	if (!cubism_vertices) {
		return;
	}

	ERR_FAIL_COND_MSG(mvp_matrix.size() != 16, "[GDLive2D] MVP matrix was not set before drawing.");
	const float *mvp = mvp_matrix.ptr();

	PackedByteArray update_buffer;
	update_buffer.resize(vcount * 12);

	float min_x = 0.0f, min_y = 0.0f, max_x = 0.0f, max_y = 0.0f;
	for (Csm::csmInt32 vi = 0; vi < vcount; ++vi) {
		const Csm::csmFloat32 vx = cubism_vertices[Csm::Constant::VertexOffset + vi * Csm::Constant::VertexStep];
		const Csm::csmFloat32 vy = cubism_vertices[Csm::Constant::VertexOffset + vi * Csm::Constant::VertexStep + 1];

		const Csm::csmFloat32 x = mvp[0] * vx + mvp[4] * vy + mvp[12];
		const Csm::csmFloat32 y = mvp[1] * vx + mvp[5] * vy + mvp[13];
		const Csm::csmFloat32 w = mvp[3] * vx + mvp[7] * vy + mvp[15];

		Vector2 pos;
		if (w != 0.0f && w != 1.0f) {
			pos = Vector2(x / w, -(y / w));
		} else {
			pos = Vector2(x, -y);
		}

		_encode_position(update_buffer, vi, pos.x, pos.y);

		if (pos.x < min_x) {
			min_x = pos.x;
		}
		if (pos.x > max_x) {
			max_x = pos.x;
		}
		if (pos.y < min_y) {
			min_y = pos.y;
		}
		if (pos.y > max_y) {
			max_y = pos.y;
		}
	}
	rs->mesh_surface_update_vertex_region(d.mesh_rid, 0, 0, update_buffer);

	if (max_x >= min_x && max_y >= min_y) {
		rs->mesh_set_custom_aabb(d.mesh_rid, AABB(Vector3(min_x, min_y, 0.0f), Vector3(max_x - min_x, max_y - min_y, 0.0f)));
	}

	rs->canvas_item_clear(ci);
	rs->canvas_item_add_mesh(ci, d.mesh_rid, Transform2D(), Color(1, 1, 1, 1), d.texture_rid);

	const Csm::CubismModelMultiplyAndScreenColor &multiply_and_screen_color = p_model.GetOverrideMultiplyAndScreenColor();
	const Csm::Rendering::CubismRenderer::CubismTextureColor multiply_color = multiply_and_screen_color.GetDrawableMultiplyColor(di);
	const Csm::Rendering::CubismRenderer::CubismTextureColor screen_color = multiply_and_screen_color.GetDrawableScreenColor(di);

	rs->canvas_item_set_instance_shader_parameter(ci, "multiply_color", Vector4(multiply_color.R, multiply_color.G, multiply_color.B, multiply_color.A));
	rs->canvas_item_set_instance_shader_parameter(ci, "screen_color", Vector4(screen_color.R, screen_color.G, screen_color.B, screen_color.A));

	if (d.masked) {
		Csm::CubismMatrix44 mvp;
		mvp.SetMatrix(mvp_matrix.ptrw());
		Csm::CubismMatrix44 inverse = mvp.GetInvert();

		Live2DClippingContext *mask_context = clipping_manager->get_context_for_drawable(d.drawable_index);

		Csm::CubismMatrix44 clip_matrix(mask_context->_matrixForDraw);
		clip_matrix.MultiplyByMatrix(&inverse);
		Csm::csmFloat32 *clip = clip_matrix.GetArray();
		for (int32_t i = 4; i < 8; ++i) {
			clip[i] = -clip[i];
		}

		const Csm::Rendering::CubismRenderer::CubismTextureColor *channel = clipping_manager->GetChannelFlagAsColor(mask_context->_layoutChannelIndex);
		rs->canvas_item_set_instance_shader_parameter(ci, "mask_channel", Vector4(channel->R, channel->G, channel->B, channel->A));
		rs->canvas_item_set_instance_shader_parameter(ci, "mask_inverted", p_model.GetDrawableInvertedMask(di));
		rs->canvas_item_set_instance_shader_parameter(ci, "clip_row0", Vector4(clip[0], clip[4], clip[8], clip[12]));
		rs->canvas_item_set_instance_shader_parameter(ci, "clip_row1", Vector4(clip[1], clip[5], clip[9], clip[13]));
	}

	const float opacity = p_model.GetDrawableOpacity(di) * p_model.GetModelOpacity();
	rs->canvas_item_set_modulate(ci, Color(1, 1, 1, CLAMP(opacity, 0.0f, 1.0f)));
}

Live2DRenderer::Live2DRenderer(const Live2DRenderContext &p_context) :
		render_context(p_context) {
}

Live2DRenderer::~Live2DRenderer() {
	_release_drawables();
	_release_masks();
	shared_materials.clear();

	CSM_DELETE_SELF(Live2DClippingManager, clipping_manager);
}

void Live2DRenderer::initialize(Csm::CubismModel &p_model, int32_t p_mask_buffer_count) {
	clipping_manager = CSM_NEW(Live2DClippingManager);
	ERR_FAIL_COND(!clipping_manager);

	clipping_manager->Initialize(p_model, p_mask_buffer_count, DrawableObjectType::DrawableObjectType_Drawable);

	if (p_model.GetOffscreenCount() > 0) {
		WARN_PRINT_ED("[GDLive2D] Offscreen drawing is not implemented.");
	}
}

void Live2DRenderer::update_masks(Csm::CubismModel &p_model) {
	_update_mask_target(p_model);
	_update_mask_texture();
}

void Live2DRenderer::update_model(Csm::CubismModel &p_model) {
	// Masks must be built before drawables for material selection
	_build_masks(p_model);
	_build_drawables(p_model);
}

void Live2DRenderer::set_mvp_matrix(Csm::CubismMatrix44 *p_matrix) {
	ERR_FAIL_COND(!p_matrix);

	const Csm::csmFloat32 *values = p_matrix->GetArray();
	mvp_matrix.resize(16);
	float *dst = mvp_matrix.ptrw();
	for (int32_t i = 0; i < 16; ++i) {
		dst[i] = values[i];
	}
}

void Live2DRenderer::draw_model(Csm::CubismModel &p_model) {
	for (int32_t i = 0; i < drawables.size(); ++i) {
		_render_drawable(p_model, i);
	}
}
