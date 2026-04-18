#pragma once

#ifdef GDEXTENSION
#include <godot_cpp/godot.hpp>
using namespace godot;
#elif defined(GODOT_MODULE)
#include "modules/register_module_types.h"
#endif

void initialize_live2d_module(ModuleInitializationLevel p_level);
void uninitialize_live2d_module(ModuleInitializationLevel p_level);
