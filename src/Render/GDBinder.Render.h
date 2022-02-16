#pragma once

#include <stdint.h>
#include <array>
#include <stack>
#include <vector>
#include <assert.h>

#include <Godot.hpp>
#include <World.hpp>
#include <Image.hpp>
#include <Node2D.hpp>
#include <VisualServer.hpp>

namespace GDBinder
{
	struct Triangle2D
	{
		godot::PoolIntArray indices;
		godot::PoolVector2Array position;
		godot::PoolColorArray color;
		godot::PoolVector2Array uv;

		Triangle2D(int num_vertices, int num_indices)
		{
			indices.resize(num_indices);
			position.resize(num_vertices);
			color.resize(num_vertices);
			uv.resize(num_vertices);
		}
	};

	class RenderCommand
	{
	public:
		RenderCommand();
		~RenderCommand();
		void Reset();
		void DrawSprites(godot::World* world, int32_t priority);
		void DrawModel(godot::World* world, godot::RID mesh, int32_t priority);

		godot::RID GetImmediate() { return m_immediate; }
		godot::RID GetInstance() { return m_instance; }
		godot::RID GetMaterial() { return m_material; }

	private:
		godot::RID m_immediate;
		godot::RID m_instance;
		godot::RID m_material;
	};

	class RenderCommand2D
	{
	public:
		RenderCommand2D();
		~RenderCommand2D();

		void Reset();
		void DrawSprites(godot::Node2D* parent);
		void DrawModel(godot::Node2D* parent, godot::RID mesh);

		godot::RID GetCanvasItem() { return m_canvasItem; }
		godot::RID GetMaterial() { return m_material; }

	private:
		godot::RID m_canvasItem;
		godot::RID m_material;
	};

	class DynamicTexture
	{
	public:
		struct LockedRect
		{
			float* ptr;
			size_t pitch;
			int32_t x;
			int32_t y;
			int32_t width;
			int32_t height;
		};

		DynamicTexture();
		~DynamicTexture();
		void Init(int32_t width, int32_t height);
		const LockedRect* Lock(int32_t x, int32_t y, int32_t width, int32_t height);
		void Unlock();

		godot::RID GetRID() { return m_imageTexture; }

	private:
		godot::RID m_imageTexture;
		godot::PoolByteArray m_rectData;
		LockedRect m_lockedRect{};
	};
}
