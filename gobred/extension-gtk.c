#include <glib.h>
#include <glib-object.h>
#include <webkit2/webkit-web-extension.h>
#include <JavaScriptCore/JavaScript.h>
#include <libgobred/gobred-bridge.h>

//void initialize_brige(JSContextRef context);

static void
window_object_cleared_callback (WebKitScriptWorld *world,
				WebKitWebPage *page,
				WebKitFrame *frame,
				gpointer user_data)
{
  JSGlobalContextRef ctx =
      webkit_frame_get_javascript_context_for_script_world (frame, world);
  gobred_bridge_setup (ctx);
}

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
  g_print ("debug name %s\n", G_LOG_DOMAIN);
  gobred_bridge_init ();
  g_signal_connect(webkit_script_world_get_default (), "window-object-cleared",
		   G_CALLBACK (window_object_cleared_callback), NULL);
}

G_MODULE_EXPORT void
g_module_unload (GModule *module)
{
  gobred_bridge_end ();
}
