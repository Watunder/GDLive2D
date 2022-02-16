#pragma once

#include <stdint.h>
#include <Resource.hpp>

#include "GDBinder.TextureLoader.h"

namespace GDBinder
{
	class Texture
	{
	public:
		godot::RID GetRID() const { return textureRid_; }

	private:
		friend class TextureLoader;

		godot::Ref<godot::Resource> godotTexture_;
		godot::RID textureRid_;
	};

	class Model
	{
	public:
		Model(const void* data, int32_t size);
		~Model();
		godot::RID GetRID() const { return meshRid_; }

	private:
		friend class ModelLoader;

		godot::RID meshRid_;
	};
}
