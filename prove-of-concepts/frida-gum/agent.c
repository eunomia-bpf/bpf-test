#include <fcntl.h>
#include <frida-gum.h>

static int replacement_open(const char *path, int oflag, ...);

void example_agent_main(const gchar *data, gboolean *stay_resident)
{
	GumInterceptor *interceptor;

	/* We don't want to our library to be unloaded after we return. */
	*stay_resident = TRUE;

	gum_init_embedded();

	g_printerr("example_agent_main(): %s\n", data);

	interceptor = gum_interceptor_obtain();

	/* Transactions are optional but improve performance with multiple
	 * hooks. */
	gum_interceptor_begin_transaction(interceptor);

	gum_interceptor_replace(
		interceptor,
		(gpointer)gum_module_find_export_by_name(NULL, "open"),
		replacement_open, 0x1234, NULL);
	/*
	 * ^
	 * |
	 * This is using replace(), but there's also attach() which can be used
	 * to hook functions without any knowledge of argument types, calling
	 * convention, etc. It can even be used to put a probe in the middle of
	 * a function.
	 */

	gum_interceptor_end_transaction(interceptor);
}

static int replacement_open(const char *path, int oflag, ...)
{
	g_printerr("open(\"%s\", 0x%x)\n", path, oflag);
	GumInvocationContext *ctx;
	const char *data;

	ctx = gum_interceptor_get_current_invocation();
	g_assert_nonnull(ctx);

	data = (const char *)gum_invocation_context_get_replacement_data(ctx);
	g_printerr("data: %p\n", data);

	return open(path, oflag);
}
