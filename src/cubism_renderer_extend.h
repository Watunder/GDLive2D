#pragma once

#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <Rendering/CubismRenderer.hpp>

class CubismRendererExtend : public Csm::Rendering::CubismRenderer {
	friend class Csm::Rendering::CubismRenderer;

private:
	Csm::CubismUserModel *_userModel = nullptr;

	struct MaskTriangle {
		Csm::csmFloat32 ax = 0.0f;
		Csm::csmFloat32 ay = 0.0f;
		Csm::csmFloat32 bx = 0.0f;
		Csm::csmFloat32 by = 0.0f;
		Csm::csmFloat32 cx = 0.0f;
		Csm::csmFloat32 cy = 0.0f;
	};

	struct DrawableMaskPass {
		Csm::csmBool has_mask = false;
		Csm::csmBool inverted = false;
		Csm::csmVector<MaskTriangle> triangles;
	};

	Csm::csmVector<DrawableMaskPass> _drawableMaskPassList;

	void BuildDrawableMaskPass(Csm::CubismModel *model);
	Csm::csmBool IsMaskedVertexVisible(Csm::csmInt32 drawableIndex, Csm::csmFloat32 x, Csm::csmFloat32 y) const;

	Csm::csmVector<Csm::csmInt32> _sortedObjectsIndexList;
	Csm::csmVector<Csm::csmInt32> _sortedObjectsTypeList;

public:
	virtual void Initialize(Csm::CubismModel *model) override;
	virtual void Initialize(Csm::CubismModel *model, Csm::csmInt32 maskBufferCount) override;

	void SetupUserModel(Csm::CubismUserModel *userModel);
	void RenderDrawable(Csm::csmInt32 drawableIndex);
	void RenderOffscreen(Csm::csmInt32 offscreenIndex);

protected:
	virtual void DoDrawModel() override;

	virtual void SaveProfile() override {}
	virtual void RestoreProfile() override {}
	virtual void BeforeDrawModelRenderTarget() override {}
	virtual void AfterDrawModelRenderTarget() override {}

	CubismRendererExtend(Csm::csmUint32 width, Csm::csmUint32 height);
	virtual ~CubismRendererExtend() override;
};
