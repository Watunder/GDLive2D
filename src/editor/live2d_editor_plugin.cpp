#include "live2d_editor_plugin.h"

void Live2DEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			model_importer.instantiate();
			motion_importer.instantiate();
			expression_importer.instantiate();
			add_import_plugin(model_importer, true);
			add_import_plugin(motion_importer, true);
			add_import_plugin(expression_importer, true);
		} break;

		case NOTIFICATION_EXIT_TREE: {
			remove_import_plugin(model_importer);
			remove_import_plugin(motion_importer);
			remove_import_plugin(expression_importer);
			model_importer.unref();
			motion_importer.unref();
			expression_importer.unref();
		} break;
	}
}
