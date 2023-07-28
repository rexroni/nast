#include <stdbool.h>
#include <cairo.h>
#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>

#include "nast.h"
#include "writable.h"
#include "strs.h"

#include "keymap.h"

#include "config.h"

typedef struct {
    // hooks pointer, must be the first element
    THooks hooks;
    Term *term;
    pid_t pid;

    char *font_name;
    int font_size;

    // rendering and io state
    bool want_focus;

    int ttyfd;
    struct writable writable;
    gboolean write_pending;
    GtkWidget *window;
    GtkWidget *darea;
    GIOChannel *wr_ttychan;
} globals_t;

// sigchld needs to know the globals I guess?
globals_t *G;

// forward declarations
static gboolean tty_io(GIOChannel *src, GIOCondition cond, gpointer user_data);
void ttywrite(globals_t *g, const char *s, size_t n, int may_echo);

static void ttywrite_hook(THooks *thooks, const char *buf, size_t len){
    globals_t *g = (globals_t*)thooks;
    ttywrite(g, buf, len, 0);
}

static void ttyresize_hook(THooks *thooks, int row, int col){
    globals_t *g = (globals_t*)thooks;

    // TIOCSWINSZ = "Tty IO Ctl Set WINdow SiZe" (man 4 ioctl_tty)
    struct winsize w = {
        .ws_row = row,
        .ws_col = col,
    };

    if (ioctl(g->ttyfd, TIOCSWINSZ, &w) < 0)
        fprintf(stderr, "Couldn't set window size: %s\n", strerror(errno));
}

static void ttyhangup_hook(THooks *thooks){
    globals_t *g = (globals_t*)thooks;
    /* Send SIGHUP to shell */
    kill(g->pid, SIGHUP);
}

static void bell_hook(THooks *thooks){
    (void)thooks;
}

static void sendbreak_hook(THooks *thooks){
    globals_t *g = (globals_t*)thooks;
    if (tcsendbreak(g->ttyfd, 0)){
        perror("Error sending break");
    }
}

void set_title(THooks *thooks, const char *title){
    (void)thooks;
    (void)title;
    // we will always just ignore this and leave ourselves called "nast"
}

void set_clipboard(THooks *thooks, char *buf, size_t len){
    (void)thooks;
    free(buf);
    (void)len;
    die("set_clipboard() not handled\n");
}

void ttywrite(globals_t *g, const char *s, size_t n, int may_echo){
    if(!g->write_pending){
        g->write_pending = TRUE;
        g_io_add_watch(g->wr_ttychan, G_IO_OUT, tty_io, g);
    }

    if(may_echo && t_isset_echo(g->term)) {
        twrite(g->term, s, n, 1);
    }

    if(!t_isset_crlf(g->term)){
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

// // return -1 on failure, 0 on success
// static int cairo_get_lims(cairo_t *cr, int *w, int *h){
//     cairo_surface_t *srfc = cairo_get_target(cr);
//
//     cairo_surface_type_t type = cairo_surface_get_type(srfc);
//     switch(type){
//         case CAIRO_SURFACE_TYPE_IMAGE:
//             *w = cairo_image_surface_get_width(srfc);
//             *h = cairo_image_surface_get_height(srfc);
//             *w *= PANGO_SCALE;
//             *h *= PANGO_SCALE;
//             break;
//
//         case CAIRO_SURFACE_TYPE_XLIB:
//             *w = cairo_xlib_surface_get_width(srfc);
//             *h = cairo_xlib_surface_get_height(srfc);
//             break;
//
//         case CAIRO_SURFACE_TYPE_PDF:            printf("PDF\n");            return -1;
//         case CAIRO_SURFACE_TYPE_PS:             printf("PS\n");             return -1;
//         case CAIRO_SURFACE_TYPE_XCB:            printf("XCB\n");            return -1;
//         case CAIRO_SURFACE_TYPE_GLITZ:          printf("GLITZ\n");          return -1;
//         case CAIRO_SURFACE_TYPE_QUARTZ:         printf("QUARTZ\n");         return -1;
//         case CAIRO_SURFACE_TYPE_WIN32:          printf("WIN32\n");          return -1;
//         case CAIRO_SURFACE_TYPE_BEOS:           printf("BEOS\n");           return -1;
//         case CAIRO_SURFACE_TYPE_DIRECTFB:       printf("DIRECTFB\n");       return -1;
//         case CAIRO_SURFACE_TYPE_SVG:            printf("SVG\n");            return -1;
//         case CAIRO_SURFACE_TYPE_OS2:            printf("OS2\n");            return -1;
//         case CAIRO_SURFACE_TYPE_WIN32_PRINTING: printf("WIN32_PRINTING\n"); return -1;
//         case CAIRO_SURFACE_TYPE_QUARTZ_IMAGE:   printf("QUARTZ_IMAGE\n");   return -1;
//         case CAIRO_SURFACE_TYPE_SCRIPT:         printf("SCRIPT\n");         return -1;
//         case CAIRO_SURFACE_TYPE_QT:             printf("QT\n");             return -1;
//         case CAIRO_SURFACE_TYPE_RECORDING:      printf("RECORDING\n");      return -1;
//         case CAIRO_SURFACE_TYPE_VG:             printf("VG\n");             return -1;
//         case CAIRO_SURFACE_TYPE_GL:             printf("GL\n");             return -1;
//         case CAIRO_SURFACE_TYPE_DRM:            printf("DRM\n");            return -1;
//         case CAIRO_SURFACE_TYPE_TEE:            printf("TEE\n");            return -1;
//         case CAIRO_SURFACE_TYPE_XML:            printf("XML\n");            return -1;
//         case CAIRO_SURFACE_TYPE_SKIA:           printf("SKIA\n");           return -1;
//         case CAIRO_SURFACE_TYPE_SUBSURFACE:     printf("SUBSURFACE\n");     return -1;
//         case CAIRO_SURFACE_TYPE_COGL:           printf("COGL\n");           return -1;
//             // not supported
//             return -1;
//     }
//
//     return 0;
// }

// developer.gnome.org/gtk3/3.24/GtkWidget.html#GtkWidget-draw
static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr,
        gpointer user_data){
    globals_t *g = user_data;
    (void)g;

    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);

    double x1, y1, x2, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    trender(g->term, cr, w, h, x1, y1, x2, y2);

    // allow other handlers to process the event
    return FALSE;
}

// developer.gnome.org/gtk3/3.24/GtkWidget.html#GtkWidget-key-press-event
static gboolean on_key_event(
    GtkWidget *widget, GdkEventKey *event_key, gpointer user_data
){
    globals_t *g = user_data;

    // ignore modifier keys themselves, let GTK track their state
    if(event_key->is_modifier) return TRUE;

    // ignore releases
    if(event_key->type != GDK_KEY_PRESS) return TRUE;

    int key = -1;
    if(event_key->keyval < 128){
        // ascii keys are 1:1 with key
        key = event_key->keyval;
    }else{
        // certain other keys have explicit mappings to a key
        switch(event_key->keyval){
            // see gtk-3.0/gdk/gdkkeysyms.h
            case GDK_KEY_Home:      key = NAST_KEY_HOME; break;
            case GDK_KEY_End:       key = NAST_KEY_END; break;
            case GDK_KEY_Insert:    key = NAST_KEY_INSERT; break;
            case GDK_KEY_Delete:    key = NAST_KEY_DELETE; break;
            case GDK_KEY_Page_Up:   key = NAST_KEY_PGUP; break;
            case GDK_KEY_Page_Down: key = NAST_KEY_PGDN; break;
            case GDK_KEY_BackSpace: key = NAST_KEY_BKSP; break;
            case GDK_KEY_Return:    key = NAST_KEY_ENTER; break;
            case GDK_KEY_ISO_Left_Tab:
            case GDK_KEY_KP_Tab:
            case GDK_KEY_Tab:       key = NAST_KEY_TAB; break;
            case GDK_KEY_Escape:    key = NAST_KEY_ESC; break;

            case GDK_KEY_Up:        key = NAST_KEY_UP; break;
            case GDK_KEY_Down:      key = NAST_KEY_DN; break;
            case GDK_KEY_Right:     key = NAST_KEY_RIGHT; break;
            case GDK_KEY_Left:      key = NAST_KEY_LEFT; break;

            case GDK_KEY_KP_0:      key = NAST_KEY_KP0; break;
            case GDK_KEY_KP_1:      key = NAST_KEY_KP1; break;
            case GDK_KEY_KP_2:      key = NAST_KEY_KP2; break;
            case GDK_KEY_KP_3:      key = NAST_KEY_KP3; break;
            case GDK_KEY_KP_4:      key = NAST_KEY_KP4; break;
            case GDK_KEY_KP_5:      key = NAST_KEY_KP5; break;
            case GDK_KEY_KP_6:      key = NAST_KEY_KP6; break;
            case GDK_KEY_KP_7:      key = NAST_KEY_KP7; break;
            case GDK_KEY_KP_8:      key = NAST_KEY_KP8; break;
            case GDK_KEY_KP_9:      key = NAST_KEY_KP9; break;

            case GDK_KEY_KP_Multiply: key = NAST_KEY_KPASTERISK; break;
            case GDK_KEY_KP_Subtract: key = NAST_KEY_KPMINUS; break;
            case GDK_KEY_KP_Add:      key = NAST_KEY_KPPLUS; break;
            case GDK_KEY_KP_Decimal:  key = NAST_KEY_KPCOMMA; break;
            case GDK_KEY_KP_Divide:   key = NAST_KEY_KPSLASH; break;
            case GDK_KEY_KP_Enter:    key = NAST_KEY_KPENTER; break;

            case GDK_KEY_KP_Insert:    key = NAST_KEY_KP0u; break;
            case GDK_KEY_KP_End:       key = NAST_KEY_KP1u; break;
            case GDK_KEY_KP_Down:      key = NAST_KEY_KP2u; break;
            case GDK_KEY_KP_Page_Down: key = NAST_KEY_KP3u; break;
            case GDK_KEY_KP_Left:      key = NAST_KEY_KP4u; break;
            case GDK_KEY_KP_Begin:     key = NAST_KEY_KP5u; break;
            case GDK_KEY_KP_Right:     key = NAST_KEY_KP6u; break;
            case GDK_KEY_KP_Home:      key = NAST_KEY_KP7u; break;
            case GDK_KEY_KP_Up:        key = NAST_KEY_KP8u; break;
            case GDK_KEY_KP_Page_Up:   key = NAST_KEY_KP9u; break;

            case GDK_KEY_KP_Delete:    key = NAST_KEY_KPCOMMAu; break;

            case GDK_KEY_KP_F1:
            case GDK_KEY_F1:        key = NAST_KEY_F1; break;
            case GDK_KEY_KP_F2:
            case GDK_KEY_F2:        key = NAST_KEY_F2; break;
            case GDK_KEY_KP_F3:
            case GDK_KEY_F3:        key = NAST_KEY_F3; break;
            case GDK_KEY_KP_F4:
            case GDK_KEY_F4:        key = NAST_KEY_F4; break;

            case GDK_KEY_F5:        key = NAST_KEY_F5; break;
            case GDK_KEY_F6:        key = NAST_KEY_F6; break;
            case GDK_KEY_F7:        key = NAST_KEY_F7; break;
            case GDK_KEY_F8:        key = NAST_KEY_F8; break;
            case GDK_KEY_F9:        key = NAST_KEY_F9; break;
            case GDK_KEY_F10:       key = NAST_KEY_F10; break;
            case GDK_KEY_F11:       key = NAST_KEY_F11; break;
            case GDK_KEY_F12:       key = NAST_KEY_F12; break;
            case GDK_KEY_F13:       key = NAST_KEY_F13; break;
            case GDK_KEY_F14:       key = NAST_KEY_F14; break;
            case GDK_KEY_F15:       key = NAST_KEY_F15; break;
            case GDK_KEY_F16:       key = NAST_KEY_F16; break;
            case GDK_KEY_F17:       key = NAST_KEY_F17; break;
            case GDK_KEY_F18:       key = NAST_KEY_F18; break;
            case GDK_KEY_F19:       key = NAST_KEY_F19; break;
            case GDK_KEY_F20:       key = NAST_KEY_F20; break;
            case GDK_KEY_F21:       key = NAST_KEY_F21; break;
            case GDK_KEY_F22:       key = NAST_KEY_F22; break;
            case GDK_KEY_F23:       key = NAST_KEY_F23; break;
            case GDK_KEY_F24:       key = NAST_KEY_F24; break;
            case GDK_KEY_F25:       key = NAST_KEY_F25; break;
            case GDK_KEY_F26:       key = NAST_KEY_F26; break;
            case GDK_KEY_F27:       key = NAST_KEY_F27; break;
            case GDK_KEY_F28:       key = NAST_KEY_F28; break;
            case GDK_KEY_F29:       key = NAST_KEY_F29; break;
            case GDK_KEY_F30:       key = NAST_KEY_F30; break;
            case GDK_KEY_F31:       key = NAST_KEY_F31; break;
            case GDK_KEY_F32:       key = NAST_KEY_F32; break;
            case GDK_KEY_F33:       key = NAST_KEY_F33; break;
            case GDK_KEY_F34:       key = NAST_KEY_F34; break;
            case GDK_KEY_F35:       key = NAST_KEY_F35; break;
        }
    }

    if(key == -1){
        if(event_key->type == GDK_KEY_PRESS){
            fprintf(stderr, "unhandled keypress! (0x%x)\n", event_key->keyval);
        }
        return TRUE;
    }

    unsigned int state = event_key->state;
    unsigned int mods = (CTRL_MASK * !!(state & GDK_CONTROL_MASK))
                      | (SHIFT_MASK * !!(state & GDK_SHIFT_MASK))
                      | (ALT_MASK * !!(state & GDK_MOD1_MASK))
                      | (META_MASK * !!(state & GDK_META_MASK));

    key_ev_t ev = { key, mods };
    int redraw = tkeyev(g->term, ev);
    if(redraw) gtk_widget_queue_draw(g->darea);
    return FALSE;
}

// developer.gnome.org/gtk3/3.24/GtkWidget.html#GtkWidget-focus-in-event
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-FocusIn_FocusOut
static gboolean on_focus_in(
    GtkWidget *widget, GdkEvent *event, gpointer user_data
){
    (void)widget;
    (void)event;
    globals_t *g = user_data;
    tfocusev(g->term, true);
    return FALSE;
}

// developer.gnome.org/gtk3/3.24/GtkWidget.html#GtkWidget-focus-out-event
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-FocusIn_FocusOut
static gboolean on_focus_out(
    GtkWidget *widget, GdkEvent *event, gpointer user_data
){
    (void)widget;
    (void)event;
    globals_t *g = user_data;
    tfocusev(g->term, false);
    return FALSE;
}

// https://docs.gtk.org/gtk3/signal.Widget.button-press-event.html
// https://docs.gtk.org/gdk3/struct.EventButton.html
static gboolean on_button_event(
    GtkWidget *widget, GdkEventButton *event, gpointer user_data
){
    (void)widget;
    unsigned int modstate = event->state & GDK_MODIFIER_MASK;
    globals_t *g = user_data;
    /* Discard doubleclick and tripleclick events, we'll just recalculate them.
       GTK's logic is weird anyway, since the second click of a doubleclick
       sends both a BUTTON_PRESS and a 2BUTTON_PRESS */
    if(event->type == GDK_2BUTTON_PRESS) return TRUE;
    if(event->type == GDK_3BUTTON_PRESS) return FALSE;

    // remaining events are passed straight to the terminal
    mouse_ev_e type;
    if(event->type == GDK_BUTTON_PRESS){
        type = MOUSE_EV_PRESS;
    }else{
        type = MOUSE_EV_RELEASE;
    }
    mouse_ev_t ev = {
        .type = type,
        .mods = (CTRL_MASK * !!(modstate & GDK_CONTROL_MASK))
              | (SHIFT_MASK * !!(modstate & GDK_SHIFT_MASK))
              | (ALT_MASK * !!(modstate & GDK_MOD1_MASK))
              | (META_MASK * !!(modstate & GDK_META_MASK)),
        .ms = event->time,
        .n = event->button,
        .x = (int)event->x,
        .y = (int)event->y,
        .pix_coords = true,
    };
    bool redraw = tmouseev(g->term, ev);
    if(redraw) gtk_widget_queue_draw(g->darea);
    return FALSE;
}

// https://docs.gtk.org/gtk3/signal.Widget.scroll-event.html
// https://docs.gtk.org/gdk3/struct.EventScroll.html
static gboolean on_scroll_event(
    GtkWidget* widget, GdkEventScroll *event, gpointer user_data
){
    globals_t *g = user_data;
    // gdk tracks other modifiers we don't care about, so this isn't stable:
    // unsigned int modstate = event->state & GDK_MODIFIER_MASK;
    unsigned int modstate = event->state & (
        GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK | GDK_META_MASK
    );
    int n = 0;
    if(event->direction == GDK_SCROLL_UP){
        n = +1;
    }else if(event->direction == GDK_SCROLL_DOWN){
        n = -1;
    }else{
        // not scroll up or scroll down
        return TRUE;
    }

    // intercept ctrl+scroll for zoom
    if(modstate == GDK_CONTROL_MASK){
        // ctrl+scroll
        int new_size = g->font_size + n;
        // don't let font_size drop to zero
        if(new_size == 0) return FALSE;
        int ret = tsetfont(g->term, g->font_name, new_size);
        if(ret < 0) return FALSE;
        // found new font successfully
        g->font_size = new_size;
        gtk_widget_queue_draw(g->darea);
        return FALSE;
    }

    // anything else is passed straight to terminal
    mouse_ev_t ev = {
        .type = MOUSE_EV_SCROLL,
        .mods = (CTRL_MASK * !!(modstate & GDK_CONTROL_MASK))
              | (SHIFT_MASK * !!(modstate & GDK_SHIFT_MASK))
              | (ALT_MASK * !!(modstate & GDK_MOD1_MASK))
              | (META_MASK * !!(modstate & GDK_META_MASK)),
        .ms = event->time,
        .n = n,
        .x = (int)event->x,
        .y = (int)event->y,
        .pix_coords = true,
    };
    bool redraw = tmouseev(g->term, ev);
    if(redraw) gtk_widget_queue_draw(g->darea);
    return FALSE;
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
    // printf("tty_read: "); dumpstr(stdout, buf, bytes_read); printf("\n");
    twrite(g->term, buf, bytes_read, 0);
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

void sigchld(int a){
    int stat;
    pid_t p;

    /* TODO: in the case of multiple children, react differently based on which
             one die died */
    if ((p = waitpid(-1, &stat, WNOHANG)) < 0)
        die("wait() after SIGCHLD failed: %s\n", strerror(errno));

    if (p != G->pid)
        return;

    if (WIFEXITED(stat) && WEXITSTATUS(stat))
        die("child exited with status %d\n", WEXITSTATUS(stat));
    else if (WIFSIGNALED(stat))
        die("child terminated due to signal %d\n", WTERMSIG(stat));
    exit(0);
}

int main(int argc, char *argv[]){
    globals_t g = {
        .hooks = {
            .ttywrite = ttywrite_hook,
            .ttyresize = ttyresize_hook,
            .ttyhangup = ttyhangup_hook,
            .bell = bell_hook,
            .sendbreak = sendbreak_hook,
            .set_title = set_title,
            .set_clipboard = set_clipboard,
        },
        .font_name = "monospace",
        .font_size = 20,
    };
    G = &g;

    gtk_init(&argc, &argv);

    g.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    g.darea = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(g.window), g.darea);

    // GTK input handling: developer.gnome.org/gtk3/stable/chap-input-handling.html
    g_signal_connect(G_OBJECT(g.darea), "draw", G_CALLBACK(on_draw_event), &g);
    g_signal_connect(G_OBJECT(g.window), "destroy", G_CALLBACK(gtk_main_quit), &g);

    // // get keypresses from the drawing area (does not work)
    // gtk_widget_add_events(GTK_WIDGET(g.darea), GDK_KEY_PRESS_MASK);
    // gtk_widget_add_events(GTK_WIDGET(g.darea), GDK_KEY_RELEASE_MASK);
    // g_signal_connect(G_OBJECT(g.darea), "key-press-event", G_CALLBACK(on_key_event), NULL);
    // g_signal_connect(G_OBJECT(g.darea), "key-release-event", G_CALLBACK(on_key_event), NULL);

    // get keypresses from the window (does work)
    g_signal_connect(G_OBJECT(g.window), "key-press-event", G_CALLBACK(on_key_event), &g);
    g_signal_connect(G_OBJECT(g.window), "key-release-event", G_CALLBACK(on_key_event), &g);

    // mouse buttons
    // strangely, if I connect button press to the g.window I get duplicate events
    gtk_widget_add_events(g.darea, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(g.darea, GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(g.darea), "button-press-event", G_CALLBACK(on_button_event), &g);
    g_signal_connect(G_OBJECT(g.darea), "button-release-event", G_CALLBACK(on_button_event), &g);

    // mouse scroll
    gtk_widget_add_events(g.darea, GDK_SCROLL_MASK);
    g_signal_connect(G_OBJECT(g.darea), "scroll-event", G_CALLBACK(on_scroll_event), &g);

    // get focus events from the window
    g_signal_connect(G_OBJECT(g.window), "focus-in-event", G_CALLBACK(on_focus_in), &g);
    g_signal_connect(G_OBJECT(g.window), "focus-out-event", G_CALLBACK(on_focus_out), &g);

    gtk_window_set_position(GTK_WINDOW(g.window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(g.window), 400, 400);
    gtk_window_set_title(GTK_WINDOW(g.window), "nast");

    gtk_widget_show_all(g.window);

    // create the terminal
    tnew(&g.term, 80, 40, g.font_name, g.font_size, (THooks*)&g);

    char **cmd = argc > 1 ? argv+1 : NULL;
    g.ttyfd = ttynew(g.term, &g.pid, cmd);
    (void)shell;
    // g.ttyfd = ttynew(g.term, &g.pid, NULL, NULL, NULL, NULL);
    signal(SIGCHLD, sigchld);

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

    gtk_main();

    return 0;
}
