#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/classes/editor_plugin.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "editor/plugins/editor_plugin.h"
#endif

#include "live2d_model_importer.h"

class Live2DEditorPlugin : public EditorPlugin {
	GDCLASS(Live2DEditorPlugin, EditorPlugin);

	Ref<Live2DModelImporter> model_importer;

protected:
	void _notification(int p_what);
	static void _bind_methods() {}
};
