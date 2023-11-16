#include <fcntl.h>
#include <frida-gum.h>
#include <unistd.h>

typedef struct _ExampleListener ExampleListener;
typedef enum _ExampleHookId ExampleHookId;

struct _ExampleListener {
	GObject parent;

	guint num_calls;
	guint num_exits;
};

enum _ExampleHookId { EXAMPLE_HOOK_OPEN, EXAMPLE_HOOK_CLOSE };

static void example_listener_iface_init(gpointer g_iface, gpointer iface_data);

#define EXAMPLE_TYPE_LISTENER (example_listener_get_type())
G_DECLARE_FINAL_TYPE(ExampleListener, example_listener, EXAMPLE, LISTENER,
		     GObject)
G_DEFINE_TYPE_EXTENDED(ExampleListener, example_listener, G_TYPE_OBJECT, 0,
		       G_IMPLEMENT_INTERFACE(GUM_TYPE_INVOCATION_LISTENER,
					     example_listener_iface_init))

void example_agent_main(const gchar *data, gboolean *stay_resident)
{
	GumInvocationListener *listener;

	GumInterceptor *interceptor;

	/* We don't want to our library to be unloaded after we return. */
	*stay_resident = TRUE;

	gum_init_embedded();

	g_printerr("example_agent_main()\n");

	interceptor = gum_interceptor_obtain();

	listener = g_object_new(EXAMPLE_TYPE_LISTENER, NULL);

	g_printerr("find my_test_func func %p, %p, %p\n",
		   gum_find_function("my_test_func"),
		   gum_find_functions_named("my_test_func"),
		   gum_module_find_export_by_name(NULL, "my_test_func"));

	g_printerr("find my_static_test_func func %p, %p, %p\n",
		   gum_find_function("my_static_test_func"),
		   gum_find_functions_named("my_static_test_func"),
		   gum_module_find_export_by_name(NULL, "my_static_test_func"));

	gum_interceptor_begin_transaction(interceptor);
	gum_interceptor_attach(interceptor, gum_find_function("my_test_func"),
			       listener, GSIZE_TO_POINTER(EXAMPLE_HOOK_OPEN));
	// gum_interceptor_attach(
	// 	interceptor,
	// 	GSIZE_TO_POINTER(gum_module_find_export_by_name(NULL, "open")),
	// 	listener, GSIZE_TO_POINTER(EXAMPLE_HOOK_OPEN));
	// gum_interceptor_attach(
	// 	interceptor,
	// 	GSIZE_TO_POINTER(gum_module_find_export_by_name(NULL, "close")),
	// 	listener, GSIZE_TO_POINTER(EXAMPLE_HOOK_CLOSE));
	gum_interceptor_end_transaction(interceptor);

	return;
}

static void example_listener_on_enter(GumInvocationListener *listener,
				      GumInvocationContext *ic)
{
	ExampleListener *self = EXAMPLE_LISTENER(listener);
	// ExampleHookId hook_id = GUM_IC_GET_FUNC_DATA(ic, ExampleHookId);

	// switch (hook_id) {
	// case EXAMPLE_HOOK_OPEN:
	// 	g_print("[*] open(\"%s\")\n",
	// 		(const gchar *)gum_invocation_context_get_nth_argument(
	// 			ic, 0));
	// 	break;
	// case EXAMPLE_HOOK_CLOSE:
	// 	g_print("[*] close(%d)\n",
	// 		GPOINTER_TO_INT(gum_invocation_context_get_nth_argument(
	// 			ic, 0)));
	// 	break;
	// }

	self->num_calls++;
}

static void example_listener_on_leave(GumInvocationListener *listener,
				      GumInvocationContext *ic)
{
	// ExampleHookId hook_id = GUM_IC_GET_FUNC_DATA(ic, ExampleHookId);
	ExampleListener *self = EXAMPLE_LISTENER(listener);
	self->num_exits++;
	// 	switch (hook_id) {
	// 	case EXAMPLE_HOOK_OPEN:
	// 		g_print("[*] open(\"\") return %d\n",
	// 			(int)gum_invocation_context_get_return_value(ic));
	// 		break;
	//   case EXAMPLE_HOOK_CLOSE:
	//     g_print("[*] close() return %d\n",
	//       (int)gum_invocation_context_get_return_value(ic));
	//     break;
	// 	}
}

static void example_listener_class_init(ExampleListenerClass *klass)
{
	(void)EXAMPLE_IS_LISTENER;
	(void)glib_autoptr_cleanup_ExampleListener;
}

static void example_listener_iface_init(gpointer g_iface, gpointer iface_data)
{
	GumInvocationListenerInterface *iface = g_iface;

	iface->on_enter = example_listener_on_enter;
	iface->on_leave = example_listener_on_leave;
}

static void example_listener_init(ExampleListener *self)
{
}
