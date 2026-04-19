#include "cubism_renderer_extend.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/variant/variant.h"
#include "servers/rendering_server.h"
#endif

#include "cubism_user_model_extend.h"

#include <CubismFramework.hpp>
#include <Model/CubismModel.hpp>

static RID GetRIDFromId(Csm::csmUint64 id) {
#ifdef GDEXTENSION
	return UtilityFunctions::rid_from_int64(static_cast<int64_t>(id));
#elif defined(GODOT_MODULE)
	return RID::from_uint64(id);
#endif
}

/**************************************************************************/

static Csm::csmFloat32 ApplyScreenColor(Csm::csmFloat32 baseValue, Csm::csmFloat32 screenValue) {
	const Csm::csmFloat32 b = CLAMP(baseValue, 0.0f, 1.0f);
	const Csm::csmFloat32 s = CLAMP(screenValue, 0.0f, 1.0f);
	return 1.0f - (1.0f - b) * (1.0f - s);
}

static bool IsPointInTriangle(
		Csm::csmFloat32 px,
		Csm::csmFloat32 py,
		Csm::csmFloat32 ax,
		Csm::csmFloat32 ay,
		Csm::csmFloat32 bx,
		Csm::csmFloat32 by,
		Csm::csmFloat32 cx,
		Csm::csmFloat32 cy) {
	const Csm::csmFloat32 d1 = (px - bx) * (ay - by) - (ax - bx) * (py - by);
	const Csm::csmFloat32 d2 = (px - cx) * (by - cy) - (bx - cx) * (py - cy);
	const Csm::csmFloat32 d3 = (px - ax) * (cy - ay) - (cx - ax) * (py - ay);
	const bool has_neg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
	const bool has_pos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
	return !(has_neg && has_pos);
}

Csm::Rendering::CubismRenderer *Csm::Rendering::CubismRenderer::Create(Csm::csmUint32 width, Csm::csmUint32 height) {
	return CSM_NEW CubismRendererExtend(width, height);
}

void Csm::Rendering::CubismRenderer::StaticRelease() {
}

/**************************************************************************/

void CubismRendererExtend::BuildDrawableMaskPass(Csm::CubismModel *model) {
	ERR_FAIL_COND(!model);

	const Csm::csmInt32 drawableCount = model->GetDrawableCount();
	const Csm::csmInt32 *maskCounts = model->GetDrawableMaskCounts();
	const Csm::csmInt32 **masks = model->GetDrawableMasks();

	_drawableMaskPassList.Resize(drawableCount);

	for (Csm::csmInt32 drawableIndex = 0; drawableIndex < drawableCount; ++drawableIndex) {
		DrawableMaskPass &maskPass = _drawableMaskPassList[drawableIndex];
		maskPass.has_mask = false;
		maskPass.inverted = false;
		maskPass.triangles.Clear();

		if (!maskCounts || !masks) {
			continue;
		}

		const Csm::csmInt32 maskCount = maskCounts[drawableIndex];
		if (maskCount <= 0) {
			continue;
		}

		maskPass.has_mask = true;
		maskPass.inverted = model->GetDrawableInvertedMask(drawableIndex);

		for (Csm::csmInt32 mi = 0; mi < maskCount; ++mi) {
			const Csm::csmInt32 maskDrawableIndex = masks[drawableIndex][mi];
			if (maskDrawableIndex < 0 || maskDrawableIndex >= drawableCount) {
				continue;
			}

			const Csm::csmInt32 icount = model->GetDrawableVertexIndexCount(maskDrawableIndex);
			if (icount <= 0 || (icount % 3) != 0) {
				continue;
			}

			const Csm::csmFloat32 *maskVertices = model->GetDrawableVertices(maskDrawableIndex);
			const Csm::csmUint16 *maskIndices = model->GetDrawableVertexIndices(maskDrawableIndex);
			if (!maskVertices || !maskIndices) {
				continue;
			}

			for (Csm::csmInt32 ii = 0; ii < icount; ii += 3) {
				const Csm::csmInt32 ia = static_cast<Csm::csmInt32>(maskIndices[ii]);
				const Csm::csmInt32 ib = static_cast<Csm::csmInt32>(maskIndices[ii + 1]);
				const Csm::csmInt32 ic = static_cast<Csm::csmInt32>(maskIndices[ii + 2]);

				MaskTriangle tri;
				tri.ax = maskVertices[Csm::Constant::VertexOffset + ia * Csm::Constant::VertexStep];
				tri.ay = maskVertices[Csm::Constant::VertexOffset + ia * Csm::Constant::VertexStep + 1];
				tri.bx = maskVertices[Csm::Constant::VertexOffset + ib * Csm::Constant::VertexStep];
				tri.by = maskVertices[Csm::Constant::VertexOffset + ib * Csm::Constant::VertexStep + 1];
				tri.cx = maskVertices[Csm::Constant::VertexOffset + ic * Csm::Constant::VertexStep];
				tri.cy = maskVertices[Csm::Constant::VertexOffset + ic * Csm::Constant::VertexStep + 1];
				maskPass.triangles.PushBack(tri);
			}
		}
	}
}

Csm::csmBool CubismRendererExtend::IsMaskedVertexVisible(Csm::csmInt32 drawableIndex, Csm::csmFloat32 x, Csm::csmFloat32 y) const {
	if (drawableIndex < 0 || drawableIndex >= static_cast<Csm::csmInt32>(_drawableMaskPassList.GetSize())) {
		return true;
	}

	const DrawableMaskPass &maskPass = _drawableMaskPassList[drawableIndex];
	if (!maskPass.has_mask) {
		return true;
	}

	bool insideMask = false;
	for (Csm::csmUint32 i = 0; i < maskPass.triangles.GetSize(); ++i) {
		const MaskTriangle &tri = maskPass.triangles[i];
		if (IsPointInTriangle(x, y, tri.ax, tri.ay, tri.bx, tri.by, tri.cx, tri.cy)) {
			insideMask = true;
			break;
		}
	}

	if (maskPass.inverted) {
		return !insideMask;
	}
	return insideMask;
}

void CubismRendererExtend::Initialize(Csm::CubismModel *model) {
	Csm::Rendering::CubismRenderer::Initialize(model, 1);
}

void CubismRendererExtend::Initialize(Csm::CubismModel *model, Csm::csmInt32 maskBufferCount) {
	Csm::Rendering::CubismRenderer::Initialize(model, maskBufferCount);
}

void CubismRendererExtend::SetupUserModel(Csm::CubismUserModel *userModel) {
	_userModel = userModel;
}

void CubismRendererExtend::RenderDrawable(Csm::csmInt32 drawableIndex) {
	Csm::CubismModel *model = GetModel();
	ERR_FAIL_COND(!model);

	if (!model->GetDrawableDynamicFlagIsVisible(drawableIndex)) {
		return;
	}

	const Csm::csmInt32 vcount = model->GetDrawableVertexCount(drawableIndex);
	const Csm::csmInt32 icount = model->GetDrawableVertexIndexCount(drawableIndex);
	if (vcount <= 0 || icount <= 0) {
		return;
	}

	const Csm::csmBlendMode blendMode = model->GetDrawableBlendModeType(drawableIndex);
	if (blendMode.GetColorBlendType() != Live2D::Cubism::Core::csmColorBlendType_Normal ||
			blendMode.GetAlphaBlendType() != Live2D::Cubism::Core::csmAlphaBlendType_Over) {
		WARN_PRINT_ONCE_ED("[GDLive2D] The blend mode of drawable is not implemented right now.");
		return;
	}

	const Csm::csmFloat32 *vertices = model->GetDrawableVertices(drawableIndex);
	const Live2D::Cubism::Core::csmVector2 *vertexUvs = model->GetDrawableVertexUvs(drawableIndex);
	const Csm::csmUint16 *vertexIndices = model->GetDrawableVertexIndices(drawableIndex);
	ERR_FAIL_COND(!vertices || !vertexUvs || !vertexIndices);

	PackedVector2Array pts;
	PackedVector2Array uvs;
	PackedColorArray cols;
	pts.resize(vcount);
	uvs.resize(vcount);
	cols.resize(vcount);

	const CubismTextureColor tint = GetModelColorWithOpacity(model->GetDrawableOpacity(drawableIndex) * model->GetModelOpacity());
	const Live2D::Cubism::Core::csmVector4 mult = model->GetDrawableMultiplyColor(drawableIndex);
	const Live2D::Cubism::Core::csmVector4 screen = model->GetDrawableScreenColor(drawableIndex);
	const Csm::csmFloat32 cr = ApplyScreenColor(tint.R * mult.X, screen.X);
	const Csm::csmFloat32 cg = ApplyScreenColor(tint.G * mult.Y, screen.Y);
	const Csm::csmFloat32 cb = ApplyScreenColor(tint.B * mult.Z, screen.Z);
	const Csm::csmFloat32 ca = CLAMP(tint.A * mult.W, 0.0f, 1.0f);
	const Color vertexColor(cr, cg, cb, ca);

	Vector2 *pw = pts.ptrw();
	Vector2 *uw = uvs.ptrw();
	Color *cw = cols.ptrw();

	const Csm::csmFloat32 *mvp = GetMvpMatrix().GetArray();

	for (Csm::csmInt32 vi = 0; vi < vcount; ++vi) {
		Csm::csmFloat32 lx = 0.0f, ly = 0.0f;

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
		uw[vi] = Vector2(vertexUvs[vi].X, 1.0f - vertexUvs[vi].Y);
		cw[vi] = IsMaskedVertexVisible(drawableIndex, vx, vy) ? vertexColor : Color(cr, cg, cb, 0.0f);
	}

	PackedInt32Array indices;
	indices.resize(icount);
	int32_t *iw = indices.ptrw();
	for (Csm::csmInt32 ii = 0; ii < icount; ++ii) {
		iw[ii] = static_cast<int32_t>(vertexIndices[ii]);
	}

	CubismUserModelExtend *userModelExtend = static_cast<CubismUserModelExtend *>(_userModel);
	ERR_FAIL_COND(!userModelExtend);

	RID ci = GetRIDFromId(userModelExtend->GetBase());
	ERR_FAIL_COND(!ci.is_valid());

	const Csm::csmInt32 textureIndex = model->GetDrawableTextureIndex(drawableIndex);
	ERR_FAIL_COND(textureIndex < 0);

	RID tex = GetRIDFromId(userModelExtend->GetTexture(textureIndex));
	ERR_FAIL_COND(!tex.is_valid());

	RenderingServer::get_singleton()->canvas_item_add_triangle_array(ci, indices, pts, cols, uvs, PackedInt32Array(), PackedFloat32Array(), tex);
}

void CubismRendererExtend::RenderOffscreen(Csm::csmInt32 offscreenIndex) {
	WARN_PRINT_ONCE_ED("[GDLive2D] The offscreen feature is not implemented right now.");
}

void CubismRendererExtend::DoDrawModel() {
	Csm::CubismModel *model = GetModel();
	ERR_FAIL_COND(!model);

	CubismUserModelExtend *userModelExtend = static_cast<CubismUserModelExtend *>(_userModel);
	ERR_FAIL_COND(!userModelExtend);

	BuildDrawableMaskPass(model);

	const Csm::csmInt32 drawableCount = model->GetDrawableCount();
	const Csm::csmInt32 offscreenCount = model->GetOffscreenCount();
	const Csm::csmInt32 totalCount = drawableCount + offscreenCount;
	const Csm::csmInt32 *renderOrder = model->GetRenderOrders();

	ERR_FAIL_COND(!renderOrder || drawableCount <= 0 || totalCount <= 0);

	_sortedObjectsTypeList.Resize(totalCount, DrawableObjectType_Drawable);
	_sortedObjectsIndexList.Resize(totalCount, 0);

	for (Csm::csmInt32 i = 0; i < totalCount; ++i) {
		const Csm::csmInt32 order = renderOrder[i];
		if (i < drawableCount) {
			_sortedObjectsTypeList[order] = DrawableObjectType_Drawable;
			_sortedObjectsIndexList[order] = i;
		} else {
			_sortedObjectsTypeList[order] = DrawableObjectType_Offscreen;
			_sortedObjectsIndexList[order] = i - drawableCount;
		}
	}

	for (Csm::csmInt32 i = 0; i < totalCount; ++i) {
		const Csm::csmInt32 objectType = _sortedObjectsTypeList[i];
		const Csm::csmInt32 objectIndex = _sortedObjectsIndexList[i];

		switch (objectType) {
			case DrawableObjectType_Drawable: {
				RenderDrawable(objectIndex);
			} break;
			case DrawableObjectType_Offscreen: {
				RenderOffscreen(objectIndex);
			} break;
			default: {
				CubismLogError("Unknown drawable type: %d", objectType);
			} break;
		}
	}
}

CubismRendererExtend::CubismRendererExtend(Csm::csmUint32 width, Csm::csmUint32 height) :
		Csm::Rendering::CubismRenderer(width, height) {
}

CubismRendererExtend::~CubismRendererExtend() {
}
