#include "register_types.h"

#ifdef GDEXTENSION
#include <gdextension_interface.h>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "core/io/resource_loader.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/variant/variant.h"
#endif

#include "src/cubism_allocator.h"
#include "src/live2d_moc_file.h"
#include "src/live2d_model_instance.h"

#ifdef TOOLS_ENABLED
#include "src/editor/live2d_editor_plugin.h"
#include "src/editor/live2d_model_importer.h"
#ifdef GDEXTENSION
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#elif defined(GODOT_MODULE)
#include "editor/plugins/editor_plugin.h"
#endif
#endif

#include <CubismFramework.hpp>

static CubismAllocator _cubism_allocator;
static Csm::CubismFramework::Option _cubism_option;

static Ref<ResourceFormatLoaderLive2DMocFile> resource_format_loader_live2d_moc_file;

void initialize_live2d_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
#ifdef DEBUG_ENABLED
		_cubism_option.LoggingLevel = _cubism_option.LogLevel_Verbose;
#else
		_cubism_option.LoggingLevel = _cubism_option.LogLevel_Error;
#endif
		_cubism_option.LogFunction = [](const char *p_message) {
			String message = p_message;
			if (message.begins_with("[CSM]")) {
				int64_t pos = message.find("][");
				String level = String::chr(message[pos + 2]);
				String color = "white";
				if (level == "V") {
					color = "white";
				} else if (level == "D") {
					color = "blue";
				} else if (level == "I") {
					color = "green";
				} else if (level == "W") {
					color = "yellow";
				} else if (level == "E") {
					color = "red";
				}
				print_line_rich(vformat("[color=%s]%s[/color]", color, message));
			} else {
				print_line(message);
			}
		};

		Csm::CubismFramework::StartUp(&_cubism_allocator, &_cubism_option);
		Csm::CubismFramework::Initialize();

		GDREGISTER_CLASS(Live2DMocFile);
		GDREGISTER_CLASS(ResourceFormatLoaderLive2DMocFile);

		GDREGISTER_CLASS(Live2DModelInstance);

		resource_format_loader_live2d_moc_file.instantiate();
#ifdef GDEXTENSION
		ResourceLoader::get_singleton()->add_resource_format_loader(resource_format_loader_live2d_moc_file);
#elif defined(GODOT_MODULE)
		ResourceLoader::add_resource_format_loader(resource_format_loader_live2d_moc_file);
#endif
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_CLASS(Live2DEditorPlugin);
		GDREGISTER_CLASS(Live2DModelImporter);
		EditorPlugins::add_by_type<Live2DEditorPlugin>();
	}
#endif
}

void uninitialize_live2d_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		Csm::Rendering::CubismRenderer::StaticRelease();
		Csm::CubismFramework::Dispose();

#ifdef GDEXTENSION
		ResourceLoader::get_singleton()->remove_resource_format_loader(resource_format_loader_live2d_moc_file);
#elif defined(GODOT_MODULE)
		ResourceLoader::remove_resource_format_loader(resource_format_loader_live2d_moc_file);
#endif
		resource_format_loader_live2d_moc_file.unref();
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
#ifdef GDEXTENSION
		EditorPlugins::remove_by_type<Live2DEditorPlugin>();
#endif
	}
#endif
}

#ifdef GDEXTENSION
extern "C" {
GDExtensionBool GDE_EXPORT gdlive2d_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(&initialize_live2d_module);
	init_obj.register_terminator(&uninitialize_live2d_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
#endif
