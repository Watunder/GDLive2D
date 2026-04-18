#include "cubism_user_model_extend.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/file_access.h"
#include "core/variant/variant.h"
#endif

#include "cubism_renderer_extend.h"

#include <CubismDefaultParameterId.hpp>
#include <CubismModelSettingJson.hpp>
#include <Id/CubismIdManager.hpp>

CubismUserModelExtend::CubismUserModelExtend() :
		Csm::CubismUserModel() {
	_idParamAngleX = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamAngleX);
	_idParamAngleY = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamAngleY);
	_idParamAngleZ = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamAngleZ);
	_idParamBodyAngleX = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamBodyAngleX);
	_idParamEyeBallX = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamEyeBallX);
	_idParamEyeBallY = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamEyeBallY);
}

CubismUserModelExtend::~CubismUserModelExtend() {
	if (_settingJson) {
		memdelete(_settingJson);
	}
}

void CubismUserModelExtend::CreateRenderer() {
	ERR_FAIL_COND(!_model);

	CreateRenderer(_model->GetCanvasWidth(), _model->GetCanvasHeight(), 1);
}

void CubismUserModelExtend::CreateRenderer(Csm::csmUint32 width, Csm::csmUint32 height, Csm::csmInt32 maskBufferCount) {
	Csm::CubismUserModel::CreateRenderer(width, height, maskBufferCount);

	CubismRendererExtend *renderer = GetRenderer<CubismRendererExtend>();
	ERR_FAIL_COND(!renderer);

	renderer->SetupUserModel(this);
}

Csm::CubismModelSettingJson *CubismUserModelExtend::GetSettingJson() const {
	return _settingJson;
}

void CubismUserModelExtend::LoadSettingJson(const Csm::csmChar *jsonPath) {
	PackedByteArray jsonBuffer = FileAccess::get_file_as_bytes(String::utf8(jsonPath));
	if (jsonBuffer.is_empty()) {
		return;
	}

	if (_settingJson) {
		memdelete(_settingJson);
		_settingJson = nullptr;
	}

	_settingJson = memnew(Csm::CubismModelSettingJson(jsonBuffer.ptr(), jsonBuffer.size()));
}

void CubismUserModelExtend::LoadModelFromMoc3(const Csm::csmByte *buffer, Csm::csmSizeInt size) {
	if (_moc) {
		_moc->DeleteModel(_model);
		Csm::CubismMoc::Delete(_moc);
	}

	if (_modelMatrix) {
		CSM_DELETE(_modelMatrix);
	}

	LoadModel(buffer, size);

	Live2D::Cubism::Core::csmVector2 tmpSizeInPixels{ 0.0f, 0.0f };
	Live2D::Cubism::Core::csmVector2 tmpOriginInPixels{ 0.0f, 0.0f };
	Csm::csmFloat32 tmpPixelsPerUnit = 0.0f;

	Live2D::Cubism::Core::csmReadCanvasInfo(_model->GetModel(), &tmpSizeInPixels, &tmpOriginInPixels, &tmpPixelsPerUnit);

	_canvasInfo = CanvasInfo{ tmpSizeInPixels, tmpOriginInPixels, tmpPixelsPerUnit };
}

CubismUserModelExtend::CanvasInfo CubismUserModelExtend::GetModelCanvasInfo() const {
	return _canvasInfo;
}

void CubismUserModelExtend::LoadPoseJson(const Csm::csmChar *jsonPath) {
	PackedByteArray jsonBuffer = FileAccess::get_file_as_bytes(String::utf8(jsonPath));
	if (jsonBuffer.is_empty()) {
		return;
	}

	if (_pose) {
		Csm::CubismPose::Delete(_pose);
	}

	LoadPose(jsonBuffer.ptr(), jsonBuffer.size());
}

void CubismUserModelExtend::LoadPhysicsJson(const Csm::csmChar *jsonPath) {
	PackedByteArray jsonBuffer = FileAccess::get_file_as_bytes(String::utf8(jsonPath));
	if (jsonBuffer.is_empty()) {
		return;
	}

	if (_physics) {
		Csm::CubismPhysics::Delete(_physics);
	}

	LoadPhysics(jsonBuffer.ptr(), jsonBuffer.size());
}

void CubismUserModelExtend::LoadUserDataJson(const Csm::csmChar *jsonPath) {
	PackedByteArray jsonBuffer = FileAccess::get_file_as_bytes(String::utf8(jsonPath));
	if (jsonBuffer.is_empty()) {
		return;
	}

	if (_modelUserData) {
		Csm::CubismModelUserData::Delete(_modelUserData);
	}

	LoadUserData(jsonBuffer.ptr(), jsonBuffer.size());
}

Csm::csmUint64 CubismUserModelExtend::GetBase() const {
	return _baseId;
}

void CubismUserModelExtend::BindBase(Csm::csmUint64 id) {
	_baseId = id;
}

Csm::csmUint64 CubismUserModelExtend::GetTexture(Csm::csmInt32 index) const {
	if (index < 0 || index >= static_cast<Csm::csmInt32>(_textureIds.GetSize())) {
		return 0ULL;
	}
	return _textureIds[index];
}

void CubismUserModelExtend::BindTexture(Csm::csmInt32 index, Csm::csmUint64 id) {
	if (index < 0) {
		return;
	}

	const Csm::csmInt32 need = index + 1;
	if (need > static_cast<Csm::csmInt32>(_textureIds.GetSize())) {
		_textureIds.UpdateSize(need, 0ULL, false);
	}

	_textureIds[index] = id;
}

void CubismUserModelExtend::Update(const Csm::csmFloat32 deltaTime) {
	ERR_FAIL_COND(!_model);

	_dragManager->Update(deltaTime);

	_dragX = _dragManager->GetX();
	_dragY = _dragManager->GetY();

	Csm::csmBool motionUpdated = false;

	_model->LoadParameters();

	if (_motionManager->IsFinished()) {
	} else {
		motionUpdated = _motionManager->UpdateMotion(_model, deltaTime);
	}

	_model->SaveParameters();

	_opacity = _model->GetModelOpacity();

	if (!motionUpdated) {
		if (_eyeBlink != NULL) {
			_eyeBlink->UpdateParameters(_model, deltaTime);
		}
	}

	if (_expressionManager != NULL) {
		_expressionManager->UpdateMotion(_model, deltaTime);
	}

	_model->AddParameterValue(_idParamAngleX, _dragX * static_cast<Csm::csmFloat32>(30));
	_model->AddParameterValue(_idParamAngleY, _dragY * static_cast<Csm::csmFloat32>(30));
	_model->AddParameterValue(_idParamAngleZ, _dragX * _dragY * static_cast<Csm::csmFloat32>(-30));
	_model->AddParameterValue(_idParamBodyAngleX, _dragX * static_cast<Csm::csmFloat32>(10));
	_model->AddParameterValue(_idParamEyeBallX, _dragX);
	_model->AddParameterValue(_idParamEyeBallY, _dragY);

	if (_breath != NULL) {
		_breath->UpdateParameters(_model, deltaTime);
	}

	if (_physics != NULL) {
		_physics->Evaluate(_model, deltaTime);
	}

	if (_lipSync) {
		WARN_PRINT_ONCE("[GDLive2D] The lipSync feature is not implemented right now.");
	}

	if (_pose != NULL) {
		_pose->UpdateParameters(_model, deltaTime);
	}

	_model->Update();
}

void CubismUserModelExtend::Draw(Csm::CubismMatrix44 &matrix) {
	ERR_FAIL_COND(!_model);

	CubismRendererExtend *renderer = GetRenderer<CubismRendererExtend>();
	ERR_FAIL_COND(!renderer);

	matrix.MultiplyByMatrix(_modelMatrix);

	renderer->SetMvpMatrix(&matrix);

	renderer->DrawModel();
}
