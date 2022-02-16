#include "GDBinder.Render.h"

namespace GDBinder
{
	RenderCommand::RenderCommand()
	{
		auto vs = godot::VisualServer::get_singleton();
		m_immediate = vs->immediate_create();
		m_instance = vs->instance_create();
		m_material = vs->material_create();
		vs->instance_geometry_set_material_override(m_instance, m_material);
	}

	RenderCommand::~RenderCommand()
	{
		auto vs = godot::VisualServer::get_singleton();
		vs->free_rid(m_instance);
		vs->free_rid(m_immediate);
		vs->free_rid(m_material);
	}

	void RenderCommand::Reset()
	{
		auto vs = godot::VisualServer::get_singleton();
		vs->immediate_clear(m_immediate);
		vs->instance_set_base(m_instance, godot::RID());
	}

	void RenderCommand::DrawSprites(godot::World* world, int32_t priority)
	{
		auto vs = godot::VisualServer::get_singleton();

		vs->instance_set_base(m_instance, m_immediate);
		vs->instance_set_scenario(m_instance, world->get_scenario());
		vs->material_set_render_priority(m_material, priority);
	}

	void RenderCommand::DrawModel(godot::World* world, godot::RID mesh, int32_t priority)
	{
		auto vs = godot::VisualServer::get_singleton();

		vs->instance_set_base(m_instance, mesh);
		vs->instance_set_scenario(m_instance, world->get_scenario());
		vs->material_set_render_priority(m_material, priority);
	}

	RenderCommand2D::RenderCommand2D()
	{
		auto vs = godot::VisualServer::get_singleton();
		m_canvasItem = vs->canvas_item_create();
		m_material = vs->material_create();
	}

	RenderCommand2D::~RenderCommand2D()
	{
		auto vs = godot::VisualServer::get_singleton();
		vs->free_rid(m_canvasItem);
		vs->free_rid(m_material);
	}

	void RenderCommand2D::Reset()
	{
		auto vs = godot::VisualServer::get_singleton();
		vs->canvas_item_clear(m_canvasItem);
		vs->canvas_item_set_parent(m_canvasItem, godot::RID());
	}

	void RenderCommand2D::DrawSprites(godot::Node2D* parent)
	{
		auto vs = godot::VisualServer::get_singleton();

		vs->canvas_item_set_parent(m_canvasItem, parent->get_canvas_item());
		vs->canvas_item_set_transform(m_canvasItem, parent->get_global_transform().affine_inverse());
		vs->canvas_item_set_material(m_canvasItem, m_material);
	}

	void RenderCommand2D::DrawModel(godot::Node2D* parent, godot::RID mesh)
	{
		auto vs = godot::VisualServer::get_singleton();

		vs->canvas_item_set_parent(m_canvasItem, parent->get_canvas_item());
		vs->canvas_item_set_transform(m_canvasItem, parent->get_global_transform().affine_inverse());
		vs->canvas_item_add_mesh(m_canvasItem, mesh);
		vs->canvas_item_set_material(m_canvasItem, m_material);
	}

	static constexpr int32_t CUSTOM_DATA_TEXTURE_WIDTH = 256;
	static constexpr int32_t CUSTOM_DATA_TEXTURE_HEIGHT = 256;

	DynamicTexture::DynamicTexture()
	{
	}

	DynamicTexture::~DynamicTexture()
	{
		auto vs = godot::VisualServer::get_singleton();
		vs->free_rid(m_imageTexture);
	}

	void DynamicTexture::Init(int32_t width, int32_t height)
	{
		auto vs = godot::VisualServer::get_singleton();
		godot::Ref<godot::Image> image;
		image.instance();
		image->create(width, height, false, godot::Image::FORMAT_RGBAF);
		m_imageTexture = vs->texture_create_from_image(image, 0);
	}

	const DynamicTexture::LockedRect* DynamicTexture::Lock(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		assert(m_lockedRect.ptr == nullptr);
		assert(m_lockedRect.width == 0 && m_lockedRect.height == 0);

		m_rectData.resize(width * height * sizeof(godot::Color));
		m_lockedRect.ptr = (float*)m_rectData.write().ptr();
		m_lockedRect.pitch = width * sizeof(godot::Color);
		m_lockedRect.x = x;
		m_lockedRect.y = y;
		m_lockedRect.width = width;
		m_lockedRect.height = height;
		return &m_lockedRect;
	}

	void DynamicTexture::Unlock()
	{
		assert(m_lockedRect.ptr != nullptr);
		assert(m_lockedRect.width > 0 && m_lockedRect.height > 0);

		godot::Ref<godot::Image> image;
		image.instance();
		image->create_from_data(m_lockedRect.width, m_lockedRect.height,
			false, godot::Image::FORMAT_RGBAF, m_rectData);

		auto vs = godot::VisualServer::get_singleton();
		vs->texture_set_data_partial(m_imageTexture, image,
			0, 0, m_lockedRect.width, m_lockedRect.height,
			m_lockedRect.x, m_lockedRect.y, 0, 0);

		m_rectData.resize(0);
		m_lockedRect = {};
	}
}
