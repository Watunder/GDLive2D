#pragma once

#include <Image.hpp>
#include <ImageTexture.hpp>
#include <ResourceLoader.hpp>

namespace GDBinder
{
	class TextureLoader
	{
	public:
		TextureLoader() = default;
		virtual ~TextureLoader() = default;
		godot::ImageTexture* load(godot::String path);
	};
}
