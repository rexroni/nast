#include <cairo.h>
#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>

#include "nast.h"

// return -1 on failure, 0 on success
static int cairo_get_lims(cairo_t *cr, int *w, int *h){
    cairo_surface_t *cr_srfc = cairo_get_target(cr);

    cairo_surface_type_t type = cairo_surface_get_type(cr_srfc);
    switch(type){
        case CAIRO_SURFACE_TYPE_IMAGE:
            *w = cairo_image_surface_get_width(cr_srfc);
            *h = cairo_image_surface_get_height(cr_srfc);
            *w *= PANGO_SCALE;
            *h *= PANGO_SCALE;
            break;

        case CAIRO_SURFACE_TYPE_XLIB:
            *w = cairo_xlib_surface_get_width(cr_srfc);
            *h = cairo_xlib_surface_get_height(cr_srfc);
            break;

        case CAIRO_SURFACE_TYPE_PDF:            printf("PDF\n");            return -1;
        case CAIRO_SURFACE_TYPE_PS:             printf("PS\n");             return -1;
        case CAIRO_SURFACE_TYPE_XCB:            printf("XCB\n");            return -1;
        case CAIRO_SURFACE_TYPE_GLITZ:          printf("GLITZ\n");          return -1;
        case CAIRO_SURFACE_TYPE_QUARTZ:         printf("QUARTZ\n");         return -1;
        case CAIRO_SURFACE_TYPE_WIN32:          printf("WIN32\n");          return -1;
        case CAIRO_SURFACE_TYPE_BEOS:           printf("BEOS\n");           return -1;
        case CAIRO_SURFACE_TYPE_DIRECTFB:       printf("DIRECTFB\n");       return -1;
        case CAIRO_SURFACE_TYPE_SVG:            printf("SVG\n");            return -1;
        case CAIRO_SURFACE_TYPE_OS2:            printf("OS2\n");            return -1;
        case CAIRO_SURFACE_TYPE_WIN32_PRINTING: printf("WIN32_PRINTING\n"); return -1;
        case CAIRO_SURFACE_TYPE_QUARTZ_IMAGE:   printf("QUARTZ_IMAGE\n");   return -1;
        case CAIRO_SURFACE_TYPE_SCRIPT:         printf("SCRIPT\n");         return -1;
        case CAIRO_SURFACE_TYPE_QT:             printf("QT\n");             return -1;
        case CAIRO_SURFACE_TYPE_RECORDING:      printf("RECORDING\n");      return -1;
        case CAIRO_SURFACE_TYPE_VG:             printf("VG\n");             return -1;
        case CAIRO_SURFACE_TYPE_GL:             printf("GL\n");             return -1;
        case CAIRO_SURFACE_TYPE_DRM:            printf("DRM\n");            return -1;
        case CAIRO_SURFACE_TYPE_TEE:            printf("TEE\n");            return -1;
        case CAIRO_SURFACE_TYPE_XML:            printf("XML\n");            return -1;
        case CAIRO_SURFACE_TYPE_SKIA:           printf("SKIA\n");           return -1;
        case CAIRO_SURFACE_TYPE_SUBSURFACE:     printf("SUBSURFACE\n");     return -1;
        case CAIRO_SURFACE_TYPE_COGL:           printf("COGL\n");           return -1;
            // not supported
            return -1;
    }

    return 0;
}

// developer.gnome.org/gtk3/3.24/GtkWidget.html#GtkWidget-draw
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr,
        gpointer user_data){
    int w, h;
    if(cairo_get_lims(cr, &w, &h) < 0){
        die("unable to detect cairo surface dimensions\n");
    }
    trender(cr, w, h);

    // allow other handlers to process the event
    return FALSE;
}

// developer.gnome.org/gtk3/3.24/GtkWidget.html#GtkWidget-key-press-event
static gboolean on_key_event(GtkWidget *widget, GdkEventKey *event_key,
        gpointer user_data){

    GtkIMContext *im_ctx = user_data;
    if(gtk_im_context_filter_keypress(im_ctx, event_key)){
        return TRUE;
    }
    switch(event_key->keyval){
        case GDK_KEY_BackSpace: twrite("\x07", 1, 0); break;
        default: printf("unhandled key!\n"); return FALSE;
    }
    return TRUE;
}

static void im_commit(GtkIMContext *im_ctx, gchar *str, gpointer user_data){
    printf("commit!\n");
    twrite(str, strlen(str), 0);
    gtk_widget_queue_draw(user_data);
}

static void im_preedit_start(GtkIMContext *im_ctx, gpointer user_data){
    printf("preedit start!\n");
}

static void im_preedit_end(GtkIMContext *im_ctx, gpointer user_data){
    printf("preedit end!\n");
}

static void im_preedit_changed(GtkIMContext *im_ctx, gpointer user_data){
    printf("preedit changed!\n");
}

int main(int argc, char *argv[]){
    GtkWidget *window;
    GtkWidget *darea;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    darea = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), darea);

    // GTK input handling: developer.gnome.org/gtk3/stable/chap-input-handling.html
    g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), NULL);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);


    // configure the input method
    GtkIMContext *im_ctx = gtk_im_context_simple_new();
    if(!im_ctx) die("gtk_im_context_simple_new()\n");

    g_signal_connect(G_OBJECT(im_ctx), "commit", G_CALLBACK(im_commit), darea);
    g_signal_connect(G_OBJECT(im_ctx), "preedit-end", G_CALLBACK(im_preedit_end), NULL);
    g_signal_connect(G_OBJECT(im_ctx), "preedit-start", G_CALLBACK(im_preedit_start), NULL);
    g_signal_connect(G_OBJECT(im_ctx), "preedit-changed", G_CALLBACK(im_preedit_changed), NULL);

    //// get keypresses from the drawing area (does not work)
    // gtk_widget_add_events(GTK_WIDGET(darea), GDK_KEY_PRESS_MASK);
    // g_signal_connect(G_OBJECT(darea), "key-press-event", G_CALLBACK(on_key_press), NULL);

    // get keypresses from the window (does work)
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(on_key_event), im_ctx);
    g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(on_key_event), im_ctx);

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_window_set_title(GTK_WINDOW(window), "nast");

    gtk_widget_show_all(window);

    // create the terminal
    tnew(80, 40, "monospace 10");

    // write some shit
    const char *buf = "A) This is a test This is a test This is a test This is a test This is a test th"
    "A2) is is a test This is a test\r\n"
    "B) This is a test\r\n"
    "C) This is a test\r\n"
    "D) This is a test\r\n";
    twrite(buf, strlen(buf), 0);

    gtk_main();

    return 0;
}
