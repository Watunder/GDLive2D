#include "live2d_user_model.h"

#ifdef GDEXTENSION
#include <godot_cpp/classes/file_access.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/file_access.h"
#endif

#include "live2d_renderer.h"

#include <CubismDefaultParameterId.hpp>
#include <CubismModelSettingJson.hpp>
#include <Id/CubismIdManager.hpp>

Live2DUserModel::Live2DUserModel() {
	id_param_angle_x = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamAngleX);
	id_param_angle_y = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamAngleY);
	id_param_angle_z = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamAngleZ);
	id_param_body_angle_x = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamBodyAngleX);
	id_param_eye_ball_x = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamEyeBallX);
	id_param_eye_ball_y = Csm::CubismFramework::GetIdManager()->GetId(Csm::DefaultParameterId::ParamEyeBallY);

	drag_manager = CSM_NEW Csm::CubismTargetPoint();
}

Live2DUserModel::~Live2DUserModel() {
	if (setting_json) {
		memdelete(setting_json);
		setting_json = nullptr;
	}

	release_configs();

	if (moc) {
		moc->DeleteModel(model);
	}
	Csm::CubismMoc::Delete(moc);
	moc = nullptr;
	model = nullptr;

	CSM_DELETE(model_matrix);
	model_matrix = nullptr;

	Csm::CubismEyeBlink::Delete(eye_blink);
	eye_blink = nullptr;
	Csm::CubismBreath::Delete(breath);
	breath = nullptr;

	CSM_DELETE(drag_manager);
	drag_manager = nullptr;

	_delete_renderer();
}

void Live2DUserModel::_load_model(const Csm::csmByte *buffer, Csm::csmSizeInt size) {
	moc = Csm::CubismMoc::Create(buffer, size, true);

	if (moc == nullptr) {
		CubismLogError("Failed to CubismMoc::Create().");
		return;
	}

	model = moc->CreateModel();

	if (model == nullptr) {
		CubismLogError("Failed to CreateModel().");
		return;
	}

	model->SaveParameters();

	if (model_matrix) {
		CSM_DELETE(model_matrix);
		model_matrix = nullptr;
	}

	model_matrix = CSM_NEW Csm::CubismModelMatrix(model->GetCanvasWidth(), model->GetCanvasHeight());
}

void Live2DUserModel::_load_pose(const Csm::csmByte *buffer, Csm::csmSizeInt size) {
	pose = Csm::CubismPose::Create(buffer, size);
	if (!pose) {
		CubismLogError("Failed to LoadPose().");
	}
}

void Live2DUserModel::_load_physics(const Csm::csmByte *buffer, Csm::csmSizeInt size) {
	physics = Csm::CubismPhysics::Create(buffer, size);
	if (!physics) {
		CubismLogError("Failed to LoadPhysics().");
	}
}

void Live2DUserModel::_load_user_data(const Csm::csmByte *buffer, Csm::csmSizeInt size) {
	if (!buffer) {
		CubismLogError("Failed to LoadUserData().");
		return;
	}

	model_user_data = Csm::CubismModelUserData::Create(buffer, size);
}

void Live2DUserModel::_delete_renderer() {
	if (renderer) {
		CSM_DELETE(renderer);
		renderer = nullptr;
	}
}

void Live2DUserModel::create_renderer() {
	ERR_FAIL_COND(!model);

	create_renderer(static_cast<uint32_t>(model->GetCanvasWidth()), static_cast<uint32_t>(model->GetCanvasHeight()), 1);
}

void Live2DUserModel::create_renderer(uint32_t width, uint32_t height, int32_t mask_buffer_count) {
	if (renderer) {
		_delete_renderer();
	}

	renderer = CSM_NEW Live2DRenderer(width, height);
	ERR_FAIL_COND(!renderer);

	renderer->initialize(this, mask_buffer_count);
}

Csm::CubismModelSettingJson *Live2DUserModel::get_setting_json() const {
	return setting_json;
}

void Live2DUserModel::load_setting_json(const String &json_path) {
	const PackedByteArray json_buffer = FileAccess::get_file_as_bytes(json_path);
	if (json_buffer.is_empty()) {
		return;
	}

	if (setting_json) {
		memdelete(setting_json);
		setting_json = nullptr;
	}

	setting_json = memnew(Csm::CubismModelSettingJson(json_buffer.ptr(), json_buffer.size()));
}

void Live2DUserModel::load_model_from_moc3(const PackedByteArray &moc_data) {
	if (moc) {
		moc->DeleteModel(model);
		Csm::CubismMoc::Delete(moc);
		moc = nullptr;
		model = nullptr;
	}

	if (model_matrix) {
		CSM_DELETE(model_matrix);
		model_matrix = nullptr;
	}

	_load_model(moc_data.ptr(), moc_data.size());
	ERR_FAIL_COND(!model);

	Live2D::Cubism::Core::csmVector2 tmp_size_in_pixels{ 0.0f, 0.0f };
	Live2D::Cubism::Core::csmVector2 tmp_origin_in_pixels{ 0.0f, 0.0f };
	Csm::csmFloat32 tmp_pixels_per_unit = 0.0f;

	Live2D::Cubism::Core::csmReadCanvasInfo(model->GetModel(), &tmp_size_in_pixels, &tmp_origin_in_pixels, &tmp_pixels_per_unit);

	canvas_info.size_in_pixels = Vector2(tmp_size_in_pixels.X, tmp_size_in_pixels.Y);
	canvas_info.origin_in_pixels = Vector2(tmp_origin_in_pixels.X, tmp_origin_in_pixels.Y);
	canvas_info.pixels_per_unit = tmp_pixels_per_unit;
}

Live2DUserModel::CanvasInfo Live2DUserModel::get_model_canvas_info() const {
	return canvas_info;
}

void Live2DUserModel::setup_configs(const String &base_dir) {
	ERR_FAIL_COND(!setting_json);
	ERR_FAIL_COND(!model);
	ERR_FAIL_COND(!model_matrix);

	updating = true;
	initialized = false;

	release_configs();

	{
		const String pose_file = String::utf8(setting_json->GetPoseFileName());
		if (!pose_file.is_empty()) {
			const PackedByteArray json_buffer = FileAccess::get_file_as_bytes(base_dir.path_join(pose_file));
			if (!json_buffer.is_empty()) {
				_load_pose(json_buffer.ptr(), json_buffer.size());
			}
		}
	}

	{
		const String physics_file = String::utf8(setting_json->GetPhysicsFileName());
		if (!physics_file.is_empty()) {
			const PackedByteArray json_buffer = FileAccess::get_file_as_bytes(base_dir.path_join(physics_file));
			if (!json_buffer.is_empty()) {
				_load_physics(json_buffer.ptr(), json_buffer.size());
			}
		}
	}

	{
		const String user_data_file = String::utf8(setting_json->GetUserDataFile());
		if (!user_data_file.is_empty()) {
			const PackedByteArray json_buffer = FileAccess::get_file_as_bytes(base_dir.path_join(user_data_file));
			if (!json_buffer.is_empty()) {
				_load_user_data(json_buffer.ptr(), json_buffer.size());
			}
		}
	}

	Csm::csmMap<Csm::csmString, Csm::csmFloat32> layout;
	setting_json->GetLayoutMap(layout);
	model_matrix->SetupFromLayout(layout);

	updating = false;
	initialized = true;
}

void Live2DUserModel::release_configs() {
	if (pose) {
		Csm::CubismPose::Delete(pose);
		pose = nullptr;
	}

	if (physics) {
		Csm::CubismPhysics::Delete(physics);
		physics = nullptr;
	}

	if (model_user_data) {
		Csm::CubismModelUserData::Delete(model_user_data);
		model_user_data = nullptr;
	}
}

RID Live2DUserModel::get_base_rid() const {
	return base_rid;
}

void Live2DUserModel::bind_base_rid(const RID &rid) {
	base_rid = rid;
}

RID Live2DUserModel::get_texture_rid(int32_t index) const {
	if (index < 0 || index >= texture_rids.size()) {
		return RID();
	}
	return texture_rids.get(index);
}

void Live2DUserModel::bind_texture_rid(int32_t index, const RID &rid) {
	if (index < 0) {
		return;
	}

	const Vector<RID>::Size idx = index;
	while (texture_rids.size() <= idx) {
		texture_rids.push_back(RID());
	}

	texture_rids.set(idx, rid);
}

void Live2DUserModel::update(float delta_time) {
	ERR_FAIL_COND(!model);

	const Csm::csmFloat32 dt = delta_time;

	opacity = model->GetModelOpacity();

	drag_manager->Update(dt);

	drag_x = drag_manager->GetX();
	drag_y = drag_manager->GetY();

	model->AddParameterValue(id_param_angle_x, drag_x * 30.0f);
	model->AddParameterValue(id_param_angle_y, drag_y * 30.0f);
	model->AddParameterValue(id_param_angle_z, drag_x * drag_y * -30.0f);
	model->AddParameterValue(id_param_body_angle_x, drag_x * 10.0f);
	model->AddParameterValue(id_param_eye_ball_x, drag_x);
	model->AddParameterValue(id_param_eye_ball_y, drag_y);

	if (breath != nullptr) {
		breath->UpdateParameters(model, dt);
	}

	if (physics != nullptr) {
		physics->Evaluate(model, dt);
	}

	if (lip_sync) {
		WARN_PRINT_ONCE("[GDLive2D] The lipSync feature is not implemented right now.");
	}

	if (pose != nullptr) {
		pose->UpdateParameters(model, dt);
	}

	model->Update();
}

void Live2DUserModel::draw(Csm::CubismMatrix44 &matrix) {
	ERR_FAIL_COND(!model);
	ERR_FAIL_COND(!renderer);

	matrix.MultiplyByMatrix(model_matrix);

	renderer->set_mvp_matrix(&matrix);
	renderer->draw_model();
}

void Live2DUserModel::get_model_parameter_ids(PackedStringArray &out_ids) const {
	out_ids.clear();
	if (!model) {
		return;
	}

	const Csm::csmInt32 count = model->GetParameterCount();
	for (Csm::csmInt32 i = 0; i < count; ++i) {
		const Csm::CubismId *id = model->GetParameterId(i);
		if (!id) {
			continue;
		}
		out_ids.append(String::utf8(id->GetString().GetRawString()));
	}
}

void Live2DUserModel::set_model_parameter_value(const String &parameter_id, float value) {
	if (!model || parameter_id.is_empty()) {
		return;
	}

	const CharString utf8 = parameter_id.utf8();
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(utf8.get_data());
	if (!id) {
		return;
	}

	model->SetParameterValue(id, value);
}

float Live2DUserModel::get_model_parameter_value(const String &parameter_id) const {
	if (!model || parameter_id.is_empty()) {
		return 0.0f;
	}

	const CharString utf8 = parameter_id.utf8();
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(utf8.get_data());
	if (!id) {
		return 0.0f;
	}

	return model->GetParameterValue(id);
}

bool Live2DUserModel::get_model_parameter_range(const String &parameter_id, float &min_out, float &max_out) const {
	if (!model || parameter_id.is_empty()) {
		return false;
	}

	const CharString utf8 = parameter_id.utf8();
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(utf8.get_data());
	if (!id) {
		return false;
	}

	const Csm::csmInt32 count = model->GetParameterCount();
	for (Csm::csmInt32 i = 0; i < count; ++i) {
		const Csm::CubismId *pi = model->GetParameterId(i);
		if (pi == id) {
			min_out = model->GetParameterMinimumValue(i);
			max_out = model->GetParameterMaximumValue(i);
			return true;
		}
	}

	return false;
}

float Live2DUserModel::get_model_parameter_default_value(const String &parameter_id) const {
	if (!model || parameter_id.is_empty()) {
		return 0.0f;
	}

	const CharString utf8 = parameter_id.utf8();
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(utf8.get_data());
	if (!id) {
		return 0.0f;
	}

	const Csm::csmInt32 count = model->GetParameterCount();
	for (Csm::csmInt32 i = 0; i < count; ++i) {
		const Csm::CubismId *pi = model->GetParameterId(i);
		if (pi == id) {
			return model->GetParameterDefaultValue(i);
		}
	}

	return 0.0f;
}

void Live2DUserModel::get_model_part_ids(PackedStringArray &out_ids) const {
	out_ids.clear();
	if (!model) {
		return;
	}

	const Csm::csmInt32 count = model->GetPartCount();
	for (Csm::csmInt32 i = 0; i < count; ++i) {
		const Csm::CubismId *id = model->GetPartId(static_cast<Csm::csmUint32>(i));
		if (!id) {
			continue;
		}
		out_ids.append(String::utf8(id->GetString().GetRawString()));
	}
}

void Live2DUserModel::set_model_part_visible(const String &part_id, bool visible) {
	if (!model || part_id.is_empty()) {
		return;
	}

	const CharString utf8 = part_id.utf8();
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(utf8.get_data());
	if (!id) {
		return;
	}

	const Csm::csmInt32 parameter_index = model->GetParameterIndex(id);
	if (parameter_index == -1) {
		return;
	}
	model->SetParameterValue(parameter_index, visible ? 1.0f : 0.0f);
}

void Live2DUserModel::set_model_part_opacity(const String &part_id, float value) {
	if (!model || part_id.is_empty()) {
		return;
	}

	const CharString utf8 = part_id.utf8();
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(utf8.get_data());
	if (!id) {
		return;
	}

	return model->SetPartOpacity(id, value);
}

float Live2DUserModel::get_model_part_opacity(const String &part_id) const {
	if (!model || part_id.is_empty()) {
		return 0.0f;
	}

	const CharString utf8 = part_id.utf8();
	const Csm::CubismId *id = Csm::CubismFramework::GetIdManager()->GetId(utf8.get_data());
	if (!id) {
		return 0.0f;
	}

	const Csm::csmInt32 parameter_index = model->GetParameterIndex(id);
	if (parameter_index == -1) {
		return 0.0f;
	}

	return model->GetPartOpacity(id);
}
