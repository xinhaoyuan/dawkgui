#include <webkit/webkit.h>
#include <JavaScriptCore/JavaScript.h>

#include <gtk/gtk.h>

#define NATIVE_OBJECT_NAME "native"
#define START_FILE   "ui/main.html"
#define WINDOW_TITLE "Desktop Application with WebKit GTK+ UI"

static JSValueRef
native_hello_cb(JSContextRef     context,
				JSObjectRef      function,
				JSObjectRef      this,
				size_t           argc,
				const JSValueRef argv[],
				JSValueRef*      exception)
{
	g_printf("HELLO WORLD\n");
	return JSValueMakeNull(context);
}

struct
{
	const char *name;	
	JSObjectCallAsFunctionCallback func;
} native_calls_list[] = {
	{ "hello", native_hello_cb },
};

/* ======================================== */

static gboolean
on_delete_event(GtkWidget *widget,
				GdkEvent  *event,
				gpointer   data)
{
	return FALSE;
}

JSGlobalContextRef js_native_context;
JSObjectRef js_native_object;

static void
load_status_notify(WebKitWebFrame *frame,
				   gboolean   arg1,
				   gpointer   data)
{
	if (webkit_web_frame_get_load_status(frame) == WEBKIT_LOAD_FINISHED)
	{
	 	JSStringRef name;
		name = JSStringCreateWithUTF8CString(NATIVE_OBJECT_NAME);
		JSGlobalContextRef js_global_context =
			webkit_web_frame_get_global_context(frame);

		JSObjectSetProperty(js_global_context,
							JSContextGetGlobalObject(js_global_context),
							name,
							js_native_object,
							0, NULL);
		JSStringRelease(name);
	}
}

int
main(int argc, char *argv[])
{
	GtkWidget *scrolled;
	GtkWidget *window;
	WebKitWebView *view;

	if (!g_thread_supported())
		g_thread_init(NULL);
	gtk_init_check(&argc, &argv);
	
	view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	char *cwd = g_get_current_dir();
	char *path = g_build_filename(cwd, START_FILE, NULL);
	char *start = g_filename_to_uri(path, NULL, NULL);
	
	webkit_web_view_load_uri(view, start);
	
	g_free(cwd);
	g_free(path);
	g_free(start);

  	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(view));

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), WINDOW_TITLE);
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
	
	g_signal_connect (window, "delete-event", G_CALLBACK (on_delete_event), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_container_add(GTK_CONTAINER(window), scrolled);

	g_signal_connect(webkit_web_view_get_main_frame(view), "notify::load-status", G_CALLBACK(load_status_notify), NULL);

	/* Create the native object with local callback properties */
	JSGlobalContextRef js_global_context =
		webkit_web_frame_get_global_context(webkit_web_view_get_main_frame(view));

	JSStringRef name;

	js_native_context = JSGlobalContextCreateInGroup(
		JSContextGetGroup(webkit_web_frame_get_global_context(webkit_web_view_get_main_frame(view))),
		NULL);

	js_native_object = JSObjectMake(js_native_context, NULL, NULL);
	name = JSStringCreateWithUTF8CString (NATIVE_OBJECT_NAME);
	JSObjectSetProperty(js_native_context,
						JSContextGetGlobalObject(js_native_context),
						name,
						js_native_object,
						0, NULL);
	JSStringRelease(name);

	int i;
	int count = sizeof(native_calls_list) / sizeof(native_calls_list[0]);
	for (i = 0; i < count; ++ i)
	{
		name = JSStringCreateWithUTF8CString(native_calls_list[i].name);
		JSObjectSetProperty(js_native_context,
							js_native_object,
							name,
							JSObjectMakeFunctionWithCallback(
								js_native_context, NULL,
								native_calls_list[i].func),
							0, NULL);
		JSStringRelease(name);
	}
	
	gtk_widget_show_all (window);
	gtk_main();

	return 0;
}
