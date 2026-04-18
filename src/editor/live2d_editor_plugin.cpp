#include "live2d_editor_plugin.h"

void Live2DEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			model_importer.instantiate();
			add_import_plugin(model_importer, true);
		} break;

		case NOTIFICATION_EXIT_TREE: {
			remove_import_plugin(model_importer);
			model_importer.unref();
		} break;
	}
}
