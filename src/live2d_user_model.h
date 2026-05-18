#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/templates/vector.h"
#include "core/typedefs.h"
#include "core/variant/variant.h"
#endif

#include <CubismFramework.hpp>
#include <CubismModelSettingJson.hpp>
#include <Effect/CubismBreath.hpp>
#include <Effect/CubismEyeBlink.hpp>
#include <Effect/CubismPose.hpp>
#include <Id/CubismId.hpp>
#include <Math/CubismModelMatrix.hpp>
#include <Math/CubismTargetPoint.hpp>
#include <Model/CubismMoc.hpp>
#include <Model/CubismModel.hpp>
#include <Model/CubismModelUserData.hpp>
#include <Physics/CubismPhysics.hpp>

class Live2DRenderer;

class Live2DUserModel {
public:
	struct CanvasInfo {
		Vector2 size_in_pixels;
		Vector2 origin_in_pixels;
		float pixels_per_unit = 0.0f;
	};

private:
	Csm::CubismModelSettingJson *setting_json = nullptr;

	const Csm::CubismId *id_param_angle_x = nullptr;
	const Csm::CubismId *id_param_angle_y = nullptr;
	const Csm::CubismId *id_param_angle_z = nullptr;
	const Csm::CubismId *id_param_body_angle_x = nullptr;
	const Csm::CubismId *id_param_eye_ball_x = nullptr;
	const Csm::CubismId *id_param_eye_ball_y = nullptr;

	RID base_rid;
	Vector<RID> texture_rids;

	CanvasInfo canvas_info;

	Csm::CubismMoc *moc = nullptr;
	Csm::CubismModel *model = nullptr;

	Csm::CubismEyeBlink *eye_blink = nullptr;
	Csm::CubismBreath *breath = nullptr;
	Csm::CubismModelMatrix *model_matrix = nullptr;
	Csm::CubismPose *pose = nullptr;
	Csm::CubismTargetPoint *drag_manager = nullptr;
	Csm::CubismPhysics *physics = nullptr;
	Csm::CubismModelUserData *model_user_data = nullptr;

	Live2DRenderer *renderer = nullptr;

	bool initialized = false;
	bool updating = false;
	bool lip_sync = true;

	float opacity = 1.0f;
	float drag_x = 0.0f;
	float drag_y = 0.0f;

	void _load_model(const uint8_t *p_buffer, int64_t p_size);
	void _load_pose(const uint8_t *p_buffer, int64_t p_size);
	void _load_physics(const uint8_t *p_buffer, int64_t p_size);
	void _load_user_data(const uint8_t *p_buffer, int64_t p_size);
	void _delete_renderer();

public:
	Live2DUserModel();
	virtual ~Live2DUserModel();

	Csm::CubismModel *get_model() const { return model; }
	bool is_initialized() const { return initialized; }

	void create_renderer();
	void create_renderer(uint32_t p_width, uint32_t p_height, int32_t p_mask_buffer_count = 1);

	Csm::CubismModelSettingJson *get_setting_json() const;
	void load_setting_json(const String &p_json_path);

	void load_model_from_moc3(const PackedByteArray &p_moc_data);
	CanvasInfo get_model_canvas_info() const;

	void setup_configs(const String &p_base_dir);
	void release_configs();

	RID get_base_rid() const;
	void bind_base_rid(const RID &p_rid);

	RID get_texture_rid(int32_t p_index) const;
	void bind_texture_rid(int32_t p_index, const RID &p_rid);

	void update(float p_delta_time);
	void draw(Csm::CubismMatrix44 &p_matrix);

	void get_model_parameter_ids(PackedStringArray &r_parameter_ids) const;
	void set_model_parameter_value(const String &p_parameter_id, float p_value);
	float get_model_parameter_value(const String &p_parameter_id) const;
	bool get_model_parameter_range(const String &p_parameter_id, float &r_min, float &r_max) const;
	float get_model_parameter_default_value(const String &p_parameter_id) const;

	void get_model_part_ids(PackedStringArray &r_out_ids) const;
	void set_model_part_visible(const String &p_part_id, bool p_visible);
	void set_model_part_opacity(const String &p_part_id, float p_value);
	float get_model_part_opacity(const String &p_part_id) const;
};
