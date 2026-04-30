#pragma once

#include <CubismFramework.hpp>
#include <CubismModelSettingJson.hpp>
#include <Id/CubismId.hpp>
#include <Model/CubismUserModel.hpp>

class CubismUserModelExtend : public Csm::CubismUserModel {
public:
	struct CanvasInfo {
		Live2D::Cubism::Core::csmVector2 sizeInPixels;
		Live2D::Cubism::Core::csmVector2 originInPixels;
		Csm::csmFloat32 pixelsPerUnit;
	};

private:
	Csm::CubismModelSettingJson *_settingJson = nullptr;

	const Csm::CubismId *_idParamAngleX = nullptr;
	const Csm::CubismId *_idParamAngleY = nullptr;
	const Csm::CubismId *_idParamAngleZ = nullptr;
	const Csm::CubismId *_idParamBodyAngleX = nullptr;
	const Csm::CubismId *_idParamEyeBallX = nullptr;
	const Csm::CubismId *_idParamEyeBallY = nullptr;

	Csm::csmUint64 _baseId = 0ULL;
	Csm::csmVector<Csm::csmUint64> _textureIds;

	CanvasInfo _canvasInfo;

public:
	CubismUserModelExtend();
	virtual ~CubismUserModelExtend();

	void CreateRenderer();
	void CreateRenderer(Csm::csmUint32 width, Csm::csmUint32 height, Csm::csmInt32 maskBufferCount = 1);

	Csm::CubismModelSettingJson *GetSettingJson() const;
	void LoadSettingJson(const Csm::csmChar *jsonPath);

	void LoadModelFromMoc3(const Csm::csmByte *buffer, Csm::csmSizeInt size);
	CanvasInfo GetModelCanvasInfo() const;

	void SetupConfigs(const Csm::csmChar *baseDir);
	void ReleaseConfigs();

	Csm::csmUint64 GetBase() const;
	void BindBase(Csm::csmUint64 id);

	Csm::csmUint64 GetTexture(Csm::csmInt32 index) const;
	void BindTexture(Csm::csmInt32 index, Csm::csmUint64 id);

	void Update(const Csm::csmFloat32 deltaTime);
	void Draw(Csm::CubismMatrix44 &matrix);

	void SetModelParameterValue(const Csm::csmChar *parameterId, Csm::csmFloat32 value);
	void GetModelParameterIds(Csm::csmVector<Csm::csmString> &ids) const;
	Csm::csmFloat32 GetModelParameterValue(const Csm::csmChar *parameterId) const;
	bool GetModelParameterRange(const Csm::csmChar *parameterId, Csm::csmFloat32 &min, Csm::csmFloat32 &max) const;
	Csm::csmFloat32 GetModelParameterDefaultValue(const Csm::csmChar *parameterId) const;

	void SetModelPartVisible(const Csm::csmChar *partId, Csm::csmBool visible);
};
