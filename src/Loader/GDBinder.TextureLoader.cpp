#include "GDBinder.TextureLoader.h"

namespace GDBinder
{
	godot::ImageTexture* TextureLoader::load(godot::String path)
	{
		// Load by Godot
		auto loader = godot::ResourceLoader::get_singleton();
		auto resource = loader->load(path);
		if (!resource.is_valid())
		{
			return nullptr;
		}

		auto texture = (godot::ImageTexture*)resource.ptr();
		texture->set_flags(godot::Texture::FLAG_MIPMAPS);

		return texture;
	}
}
