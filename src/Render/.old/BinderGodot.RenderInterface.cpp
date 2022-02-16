#include "BinderGodot.RenderInterface.h"

namespace _godot
{

RenderCommand3D::RenderCommand3D()
{
	auto vs = godot::VisualServer::get_singleton();
	m_immediate = vs->immediate_create();
	m_instance = vs->instance_create();
	m_material = vs->material_create();
	vs->instance_geometry_set_material_override(m_instance, m_material);
}

RenderCommand3D::~RenderCommand3D()
{
	auto vs = godot::VisualServer::get_singleton();
	vs->free_rid(m_instance);
	vs->free_rid(m_immediate);
	vs->free_rid(m_material);
}

void RenderCommand3D::Reset()
{
	auto vs = godot::VisualServer::get_singleton();
	vs->immediate_clear(m_immediate);
	vs->instance_set_base(m_instance, godot::RID());
}

void RenderCommand3D::DrawSprites(godot::World* world, int32_t priority)
{
	auto vs = godot::VisualServer::get_singleton();

	vs->instance_set_base(m_instance, m_immediate);
	vs->instance_set_scenario(m_instance, world->get_scenario());
	vs->material_set_render_priority(m_material, priority);
}

void RenderCommand3D::DrawModel(godot::World* world, godot::RID mesh, int32_t priority)
{
	auto vs = godot::VisualServer::get_singleton();

	vs->instance_set_base(m_instance, mesh);
	vs->instance_set_scenario(m_instance, world->get_scenario());
	vs->material_set_render_priority(m_material, priority);
}

//-----------------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------------

IndexBufferBase::IndexBufferBase(int maxCount, bool isDynamic)
	: m_indexMaxCount(maxCount)
	, m_indexCount(0)
	, m_isDynamic(false)
	, m_isLock(false)
	, m_resource(NULL)
{
}

IndexBufferBase::~IndexBufferBase()
{
}

void IndexBufferBase::Push(const void* buffer, int count)
{
	assert(m_isLock);
	memcpy(GetBufferDirect(count), buffer, count * stride_);
}

int IndexBufferBase::GetCount() const
{
	return m_indexCount;
}

int IndexBufferBase::GetMaxCount() const
{
	return m_indexMaxCount;
}

void* IndexBufferBase::GetBufferDirect(int count)
{
	assert(m_isLock);
	assert(m_indexMaxCount >= m_indexCount + count);

	uint8_t* pBuffer = NULL;

	pBuffer = (uint8_t*)m_resource + (m_indexCount * stride_);
	m_indexCount += count;

	return pBuffer;
}

//-----------------------------------------------------------------------------------

VertexBufferBase::VertexBufferBase(int size, bool isDynamic)
	: m_isDynamic(isDynamic)
	, m_size(size)
	, m_offset(0)
	, m_resource(NULL)
	, m_isLock(false)
{
}

VertexBufferBase::~VertexBufferBase()
{
}

void VertexBufferBase::Push(const void* buffer, int size)
{
	assert(m_isLock);
	memcpy(GetBufferDirect(size), buffer, size);
}

int VertexBufferBase::GetMaxSize() const
{
	return m_size;
}

int VertexBufferBase::GetSize() const
{
	return m_offset;
}

void* VertexBufferBase::GetBufferDirect(int size)
{
	assert(m_isLock);
	if (m_offset + size > m_size)
		return NULL;

	void* pBuffer = NULL;

	// ¥Ð¥Ã¥Õ¥¡¤Ë×·Ó›
	pBuffer = m_resource + m_offset;
	m_offset += size;

	return pBuffer;
}

//-----------------------------------------------------------------------------------

RenderStateBase::State::State()
{
	Reset();
}

void RenderStateBase::State::Reset()
{
	DepthTest = false;
	DepthWrite = false;
	AlphaBlend = AlphaBlendType::Blend;
	CullingType = CullingType::Double;
	TextureFilterTypes.fill(TextureFilterType::Nearest);
	TextureWrapTypes.fill(TextureWrapType::Clamp);
	TextureIDs.fill(0);
}

void RenderStateBase::State::CopyTo(State& state)
{
	state.DepthTest = DepthTest;
	state.DepthWrite = DepthWrite;
	state.AlphaBlend = AlphaBlend;
	state.CullingType = CullingType;
	state.TextureFilterTypes = TextureFilterTypes;
	state.TextureWrapTypes = TextureWrapTypes;
	state.TextureIDs = TextureIDs;
}

RenderStateBase::RenderStateBase()
{
}

RenderStateBase::~RenderStateBase()
{
}

void RenderStateBase::Pop()
{
	assert(!m_stateStack.empty());

	State top = m_stateStack.top();
	m_stateStack.pop();

	m_next = top;
}

//-----------------------------------------------------------------------------------

RenderStateBase::State& RenderStateBase::GetActiveState()
{
	return m_next;
}

RendererInterface::RendererInterface(int32_t squareMaxCount)
	:m_squareMaxCount(squareMaxCount)
{
}

RendererInterface::~RendererInterface()
{

}

bool RendererInterface::Initialize(int32_t drawMaxCount)
{
	m_renderCommand3Ds.resize((size_t)drawMaxCount);
	m_renderCommand2Ds.resize((size_t)drawMaxCount);

	m_customTexture1.Init(DEFAULT_TEXTURE_WIDTH, DEFAULT_TEXTURE_HEIGHT);
	m_customTexture2.Init(DEFAULT_TEXTURE_WIDTH, DEFAULT_TEXTURE_HEIGHT);
	m_uvTangentTexture.Init(DEFAULT_TEXTURE_WIDTH, DEFAULT_TEXTURE_HEIGHT);

	return true;
}

void RendererInterface::Destroy()
{
	m_renderCommand3Ds.clear();
	m_renderCommand2Ds.clear();
}

void RendererInterface::ResetState()
{
	for (size_t i = 0; i < m_renderCount3D; i++)
	{
		m_renderCommand3Ds[i].Reset();
	}
	m_renderCount3D = 0;

	for (size_t i = 0; i < m_renderCount2D; i++)
	{
		m_renderCommand2Ds[i].Reset();
	}
	m_renderCount2D = 0;

	m_vertexTextureOffset = 0;
}

}