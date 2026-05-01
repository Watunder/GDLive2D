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
	ReleaseConfigs();

	if (_settingJson) {
		memdelete(_settingJson);
		_settingJson = nullptr;
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

void CubismUserModelExtend::SetupConfigs(const Csm::csmChar *baseDir) {
	ERR_FAIL_COND(!_settingJson);
	ERR_FAIL_COND(!_model);
	ERR_FAIL_COND(!_modelMatrix);

	_updating = true;
	_initialized = false;

	ReleaseConfigs();

	const String dir = String::utf8(baseDir);

	{
		const String poseFile = String::utf8(_settingJson->GetPoseFileName());
		if (!poseFile.is_empty()) {
			PackedByteArray jsonBuffer = FileAccess::get_file_as_bytes(dir.path_join(poseFile));
			if (!jsonBuffer.is_empty()) {
				LoadPose(jsonBuffer.ptr(), jsonBuffer.size());
			}
		}
	}

	{
		const String physicsFile = String::utf8(_settingJson->GetPhysicsFileName());
		if (!physicsFile.is_empty()) {
			PackedByteArray jsonBuffer = FileAccess::get_file_as_bytes(dir.path_join(physicsFile));
			if (!jsonBuffer.is_empty()) {
				LoadPhysics(jsonBuffer.ptr(), jsonBuffer.size());
			}
		}
	}

	{
		const String userDataFile = String::utf8(_settingJson->GetUserDataFile());
		if (!userDataFile.is_empty()) {
			PackedByteArray jsonBuffer = FileAccess::get_file_as_bytes(dir.path_join(userDataFile));
			if (!jsonBuffer.is_empty()) {
				LoadUserData(jsonBuffer.ptr(), jsonBuffer.size());
			}
		}
	}

	Csm::csmMap<Csm::csmString, Csm::csmFloat32> layout;
	_settingJson->GetLayoutMap(layout);
	_modelMatrix->SetupFromLayout(layout);

	_updating = false;
	_initialized = true;
}

void CubismUserModelExtend::ReleaseConfigs() {
	if (_pose) {
		Csm::CubismPose::Delete(_pose);
		_pose = nullptr;
	}

	if (_physics) {
		Csm::CubismPhysics::Delete(_physics);
		_physics = nullptr;
	}

	if (_modelUserData) {
		Csm::CubismModelUserData::Delete(_modelUserData);
		_modelUserData = nullptr;
	}
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

	_opacity = _model->GetModelOpacity();

	_dragManager->Update(deltaTime);

	_dragX = _dragManager->GetX();
	_dragY = _dragManager->GetY();

	_model->AddParameterValue(_idParamAngleX, _dragX * static_cast<Csm::csmFloat32>(30));
	_model->AddParameterValue(_idParamAngleY, _dragY * static_cast<Csm::csmFloat32>(30));
	_model->AddParameterValue(_idParamAngleZ, _dragX * _dragY * static_cast<Csm::csmFloat32>(-30));
	_model->AddParameterValue(_idParamBodyAngleX, _dragX * static_cast<Csm::csmFloat32>(10));
	_model->AddParameterValue(_idParamEyeBallX, _dragX);
	_model->AddParameterValue(_idParamEyeBallY, _dragY);

	if (_breath != nullptr) {
		_breath->UpdateParameters(_model, deltaTime);
	}

	if (_physics != nullptr) {
		_physics->Evaluate(_model, deltaTime);
	}

	if (_lipSync) {
		WARN_PRINT_ONCE("[GDLive2D] The lipSync feature is not implemented right now.");
	}

	if (_pose != nullptr) {
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

void CubismUserModelExtend::SetModelParameterValue(const Csm::csmChar *parameterId, Csm::csmFloat32 value) {
	if (!_model || !parameterId || parameterId[0] == '\0') {
		return;
	}

	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(parameterId);
	if (!id) {
		return;
	}

	_model->SetParameterValue(id, value);
}

void CubismUserModelExtend::GetModelParameterIds(Csm::csmVector<Csm::csmString> &ids) const {
	ids.Clear();
	if (!_model) {
		return;
	}

	const Csm::csmInt32 count = _model->GetParameterCount();
	for (Csm::csmInt32 i = 0; i < count; ++i) {
		const Csm::CubismId *id = _model->GetParameterId(i);
		if (!id) {
			continue;
		}
		ids.PushBack(id->GetString().GetRawString());
	}
}

Csm::csmFloat32 CubismUserModelExtend::GetModelParameterValue(const Csm::csmChar *parameterId) const {
	if (!_model || !parameterId || parameterId[0] == '\0') {
		return 0.0f;
	}
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(parameterId);
	if (!id) {
		return 0.0f;
	}
	return _model->GetParameterValue(id);
}

bool CubismUserModelExtend::GetModelParameterRange(const Csm::csmChar *parameterId, Csm::csmFloat32 &min, Csm::csmFloat32 &max) const {
	if (!_model || !parameterId || parameterId[0] == '\0') {
		return false;
	}

	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(parameterId);
	if (!id) {
		return false;
	}

	const Csm::csmInt32 count = _model->GetParameterCount();
	for (Csm::csmInt32 i = 0; i < count; ++i) {
		const Csm::CubismId *pi = _model->GetParameterId(i);
		if (pi == id) {
			min = _model->GetParameterMinimumValue(i);
			max = _model->GetParameterMaximumValue(i);
			return true;
		}
	}

	return false;
}

Csm::csmFloat32 CubismUserModelExtend::GetModelParameterDefaultValue(const Csm::csmChar *parameterId) const {
	if (!_model || !parameterId || parameterId[0] == '\0') {
		return 0.0f;
	}

	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(parameterId);
	if (!id) {
		return 0.0f;
	}

	const Csm::csmInt32 count = _model->GetParameterCount();
	for (Csm::csmInt32 i = 0; i < count; ++i) {
		const Csm::CubismId *pi = _model->GetParameterId(i);
		if (pi == id) {
			return _model->GetParameterDefaultValue(i);
		}
	}

	return 0.0f;
}

void CubismUserModelExtend::SetModelPartVisible(const Csm::csmChar *partId, Csm::csmBool visible) {
	if (!_model || !partId || partId[0] == '\0') {
		return;
	}

	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(partId);
	if (!id) {
		return;
	}

	Csm::csmInt32 parameterIndex = _model->GetParameterIndex(id);
	if (parameterIndex == -1) {
		return;
	}
	_model->SetParameterValue(parameterIndex, visible ? 1.0f : 0.0f);
}
