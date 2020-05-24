#include <cairo.h>
#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>

#include "nast.h"
#include "writable.h"

static gboolean tty_io(GIOChannel *src, GIOCondition cond, gpointer user_data);

void die_on_ttywrite(const char *buf, size_t len){
    (void)buf; (void)len;
    die("we don't support tty writes from the terminal yet\n");
}

#define MODE_ECHO 1 << 0;
#define MODE_CLRF 1 << 1;

// TODO: not this.
#define IS_SET(arg) TRUE

typedef struct {
    GtkIMContext *im_ctx;
    int ttyfd;
    struct writable writable;
    gboolean write_pending;
    GtkWidget *window;
    GtkWidget *darea;
    GIOChannel *wr_ttychan;
} globals_t;

void ttywrite(globals_t *g, const char *s, size_t n, int may_echo){
    if(!g->write_pending){
        g->write_pending = TRUE;
        g_io_add_watch(g->wr_ttychan, G_IO_OUT, tty_io, g);
    }

    if(may_echo && IS_SET(MODE_ECHO)) {
        twrite(s, n, 1);
    }

    if(!IS_SET(MODE_CRLF)){
        writable_add_bytes(&g->writable, s, n);
    }else{
        // do an extra copy with the \r -> \r\n conversion
        char buffer[16384];
        size_t len = 0;
        for(size_t i = 0; i < n; i++){
            char c = s[i];
            if(c == '\r'){
                buffer[len++] = '\r';
                buffer[len++] = '\n';
            }else{
                buffer[len++] = c;
            }
            // need to flush the buffer?
            if(len + 2 >= sizeof(buffer)){
                writable_add_bytes(&g->writable, buffer, len);
                len = 0;
            }
        }
        // anything to flush at the end?
        if(len > 0){
            writable_add_bytes(&g->writable, buffer, len);
        }
    }
}

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
    globals_t *g = user_data;
    (void)g;

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
    globals_t *g = user_data;

    if(gtk_im_context_filter_keypress(g->im_ctx, event_key)){
        return TRUE;
    }

    if(event_key->type == GDK_KEY_PRESS){
        switch(event_key->keyval){
            case GDK_KEY_BackSpace:
                // printf("bs\n");
                ttywrite(g, "\b", 1, 0);
                // twrite("\b", 1, 0);
                gtk_widget_queue_draw(g->darea);
                break;
            case GDK_KEY_Return:
                // printf("return\n");
                ttywrite(g, "\n", 1, 0);
                // twrite("\n", 1, 0);
                gtk_widget_queue_draw(g->darea);
                break;
            default: printf("unhandled key! (%c)\n", event_key->keyval); return FALSE;
        }
    }
    return TRUE;
}

static void im_commit(GtkIMContext *im_ctx, gchar *str, gpointer user_data){
    // printf("commit! (%s)\n", str);
    globals_t *g = user_data;
    ttywrite(g, str, strlen(str), 0);
    // twrite(str, strlen(str), 0);
    gtk_widget_queue_draw(g->darea);
}

static void im_preedit_start(GtkIMContext *im_ctx, gpointer user_data){
    // printf("preedit start!\n");
}

static void im_preedit_end(GtkIMContext *im_ctx, gpointer user_data){
    // printf("preedit end!\n");
}

static void im_preedit_changed(GtkIMContext *im_ctx, gpointer user_data){
    // printf("preedit changed!\n");
}

static gboolean tty_read(GIOChannel *src, globals_t *g){
    gchar buf[16384];
    GError *err = NULL;
    gsize bytes_read = 0;
    GIOStatus status = g_io_channel_read_chars(src, buf, sizeof(buf), &bytes_read, &err);
    switch(status){
        case G_IO_STATUS_ERROR: die("G_IO_STATUS_ERROR during read\n"); break;
        case G_IO_STATUS_EOF: die("G_IO_STATUS_EOF during read\n"); break;
        case G_IO_STATUS_AGAIN: die("G_IO_STATUS_AGAIN during read\n"); break;
        case G_IO_STATUS_NORMAL: break;
    }
    // printf("read: %s\n", buf);
    // for(size_t i = 0; i < sizeof(buf) / sizeof(*buf); i++){
    //     if(buf[i] < 128 && buf[i] > 31) continue;
    //     if(buf[i] == '\n') continue;
    //     buf[i] = 'X';
    // }
    twrite(buf, bytes_read, 0);
    // redraw
    gtk_widget_queue_draw(g->darea);
    // always be ready to read again
    return TRUE;
}

static gboolean tty_write(GIOChannel *src, globals_t *g){
    size_t n;
    const char *s;

    // write until the buffer is full
    while((s = writable_get_string(&g->writable, &n))){
        // printf("writing %.*s\n", (int)n, s);

        GError *err = NULL;
        gsize n_written = 0;

        GIOStatus status = g_io_channel_write_chars(src, s, n, &n_written, &err);
        switch(status){
            case G_IO_STATUS_ERROR:
                die("G_IO_STATUS_ERROR during write\n");
            case G_IO_STATUS_EOF:
                die("G_IO_STATUS_EOF during write\n");
            case G_IO_STATUS_AGAIN:
                break;
            case G_IO_STATUS_NORMAL:
                break;
        }

        if(n_written < n){
            // return any leftover bytes
            // there's more to write, but the buffer is full
            writable_return_bytes(&g->writable, n - n_written);
            return TRUE;
        }

    }

    // we wrote everything we needed to
    g->write_pending = FALSE;
    return FALSE;
}

static gboolean tty_io(GIOChannel *src, GIOCondition cond, gpointer user_data){
    globals_t *g = user_data;

    switch(cond){
        case G_IO_OUT:
            return tty_write(src, g);
        case G_IO_IN:
        case G_IO_PRI:
            return tty_read(src, g);
            break;
        case G_IO_ERR:
            die("got G_IO_ERR from tty io callback\n");
            break;
        case G_IO_HUP:
            die("got G_IO_HUP from tty io callback\n");
            break;
        case G_IO_NVAL:
            die("got G_IO_NVAL from tty io callback\n");
            break;
    }

    // this event source should not be removed.
    return TRUE;
}

void prep_channel(GIOChannel *chan){
    GError *err = NULL;
    GIOStatus status = g_io_channel_set_encoding(chan, NULL, &err);
    switch(status){
        case G_IO_STATUS_ERROR:
            die("G_IO_STATUS_ERROR during set_encoding\n");
            break;
        case G_IO_STATUS_EOF:
            die("G_IO_STATUS_EOF during set_encoding\n");
            break;
        case G_IO_STATUS_AGAIN:
            die("G_IO_STATUS_AGAIN during set_encoding\n");
            break;
        case G_IO_STATUS_NORMAL:
            break;
    }

    g_io_channel_set_buffered(chan, FALSE);

    GIOFlags old_flags = g_io_channel_get_flags(chan);
    GIOFlags new_flags = old_flags | G_IO_FLAG_NONBLOCK;
    status = g_io_channel_set_flags(chan, new_flags, &err);
    switch(status){
        case G_IO_STATUS_ERROR:
            die("G_IO_STATUS_ERROR during set_encoding\n");
            break;
        case G_IO_STATUS_EOF:
            die("G_IO_STATUS_EOF during set_encoding\n");
            break;
        case G_IO_STATUS_AGAIN:
            die("G_IO_STATUS_AGAIN during set_encoding\n");
            break;
        case G_IO_STATUS_NORMAL:
            break;
    }
}

int main(int argc, char *argv[]){
    globals_t g = {0};

    gtk_init(&argc, &argv);

    g.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    g.darea = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(g.window), g.darea);

    // GTK input handling: developer.gnome.org/gtk3/stable/chap-input-handling.html
    g_signal_connect(G_OBJECT(g.darea), "draw", G_CALLBACK(on_draw_event), &g);
    g_signal_connect(G_OBJECT(g.window), "destroy", G_CALLBACK(gtk_main_quit), &g);


    // configure the input method
    g.im_ctx = gtk_im_context_simple_new();
    if(!g.im_ctx) die("gtk_im_context_simple_new()\n");

    g_signal_connect(G_OBJECT(g.im_ctx), "commit", G_CALLBACK(im_commit), &g);
    g_signal_connect(G_OBJECT(g.im_ctx), "preedit-end", G_CALLBACK(im_preedit_end), &g);
    g_signal_connect(G_OBJECT(g.im_ctx), "preedit-start", G_CALLBACK(im_preedit_start), &g);
    g_signal_connect(G_OBJECT(g.im_ctx), "preedit-changed", G_CALLBACK(im_preedit_changed), &g);

    //// get keypresses from the drawing area (does not work)
    // gtk_widget_add_events(GTK_WIDGET(darea), GDK_KEY_PRESS_MASK);
    // g_signal_connect(G_OBJECT(darea), "key-press-event", G_CALLBACK(on_key_press), NULL);

    // get keypresses from the window (does work)
    g_signal_connect(G_OBJECT(g.window), "key-press-event", G_CALLBACK(on_key_event), &g);
    g_signal_connect(G_OBJECT(g.window), "key-release-event", G_CALLBACK(on_key_event), &g);

    gtk_window_set_position(GTK_WINDOW(g.window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(g.window), 400, 400);
    gtk_window_set_title(GTK_WINDOW(g.window), "nast");

    gtk_widget_show_all(g.window);

    // create the terminal
    tnew(80, 40, "monospace 10", die_on_ttywrite);

    // g.ttyfd = ttynew(NULL, "/bin/sh", NULL, (char*[]){"cat", NULL});
    g.ttyfd = ttynew(NULL, "/bin/sh", NULL, NULL);

    // add the ttyfd to the main loop
    GIOChannel *ttychan = g_io_channel_unix_new(g.ttyfd);
    if(!ttychan) die("g_io_channel_unix_new()\n");
    prep_channel(ttychan);

    // write channel must be a different channel to have independent watches
    g.wr_ttychan = g_io_channel_unix_new(g.ttyfd);
    if(!g.wr_ttychan) die("g_io_channel_unix_new()\n");
    prep_channel(g.wr_ttychan);

    // await bytes on the ttyfd
    GIOCondition cond = G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
    guint rd_event_src_id = g_io_add_watch(ttychan, cond, tty_io, &g);
    (void)rd_event_src_id;


    // write some shit
    char buf[] = "\x1b[35mhello\x1b[m \x1b[45mworld\x1b[m!\r\nthis \x1b[45mis a\r\n\x1b[30mtest\x1b[m\r\n";
    twrite(buf, sizeof(buf) - 1,  0);

    gtk_main();

    return 0;
}