#pragma once

#include "GDBinder.Render.h"

namespace GDBinder
{
	class RendererImplemented
	{
	public:
		void TransferVertexToImmediate3D(godot::RID immediate, const void* vertexData, int32_t spriteCount);
		void TransferVertexToCanvasItem2D(godot::RID canvas_item, const void* vertexData, int32_t spriteCount);
		void DrawSprites(const void* vertexData, int32_t spriteCount);
	private:
		std::vector<RenderCommand> m_renderCommands;
		size_t m_renderCount = 0;
		std::vector<RenderCommand2D> m_renderCommand2Ds;
		size_t m_renderCount2D = 0;

		DynamicTexture m_customData1Texture;
		DynamicTexture m_customData2Texture;
		DynamicTexture m_customData3Texture;
		DynamicTexture m_customData4Texture;
		int32_t m_vertexTextureOffset = 0;
	};
}
