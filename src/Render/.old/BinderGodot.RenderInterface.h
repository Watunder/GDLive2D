#ifndef BINDERGODOT_RENDERINTERFACE_H
#define BINDERGODOT_RENDERINTERFACE_H

namespace _godot
{

//-----------------------------------------------------------------------------------

class RendererInterface
{
public:
	RendererInterface();
	virtual ~RendererInterface();

protected:
	size_t m_renderCount3D = 0;
	size_t m_renderCount2D = 0;
	std::vector<RenderCommand3D> m_renderCommand3Ds;
	std::vector<RenderCommand2D> m_renderCommand2Ds;
};

// 3D drawing

class RenderCommand3D
{
public:
	RenderCommand3D();
	~RenderCommand3D();
	void Reset();
	void DrawSprites(godot::World* world, int32_t priority);
	void DrawModel(godot::World* world, godot::RID mesh, int32_t priority);

	godot::RID GetImmediate() { return m_immediate; }
	godot::RID GetInstance() { return m_instance; }
	godot::RID GetMaterial() { return m_material; }

protected:
	godot::RID m_immediate;
	godot::RID m_instance;
	godot::RID m_material;
};

// 2D drawing

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

// dynamic texture

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

//-----------------------------------------------------------------------------------

}

#endif