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

    // rendering and io state
    bool appcursor;
    bool appkeypad;
    bool want_focus;

    GtkIMContext *im_ctx;
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

static void set_mode(THooks *thooks, enum win_mode mode, int val){
    globals_t *g = (globals_t*)thooks;

    if(mode & MODE_APPCURSOR){
        g->appcursor = (bool)val;
    }

    if(mode & MODE_APPKEYPAD){
        g->appkeypad = (bool)val;
    }

    if(mode & MODE_VISIBLE) die("VISIBLE mode not handled\n");
    if(mode & MODE_FOCUSED) die("FOCUSED mode not handled\n");
    if(mode & MODE_MOUSEBTN) die("MOUSEBTN mode not handled\n");
    if(mode & MODE_MOUSEMOTION) die("MOUSEMOTION mode not handled\n");
    if(mode & MODE_REVERSE) die("REVERSE mode not handled\n");
    if(mode & MODE_KBDLOCK) die("KBDLOCK mode not handled\n");
    if(mode & MODE_HIDE){
        // TODO: start rendering a cursor so that we can start hiding it.
    }
    if(mode & MODE_MOUSESGR) die("MOUSESGR mode not handled\n");
    if(mode & MODE_8BIT){
        // TODO: does 8bit mode mean anything to us?  Or 7bit mode?
    }
    if(mode & MODE_BLINK) die("BLINK mode not handled\n");
    if(mode & MODE_FBLINK) die("FBLINK mode not handled\n");
    if(mode & MODE_FOCUS){
        g->want_focus = (bool)val;
    }
    if(mode & MODE_MOUSEX10) die("MOUSEX10 mode not handled\n");
    if(mode & MODE_MOUSEMANY) die("MOUSEMANY mode not handled\n");
    if(mode & MODE_NUMLOCK) die("NUMLOCK mode not handled\n");

    // this one is a conglomerate of other modes
    if(mode & MODE_MOUSE) die("MODE_MOUSE mode not handled\n");
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

// shift_pgup is a key action
void shift_pgup(void *globals, GdkEventKey *event_key){
    (void)event_key;
    globals_t *g = globals;
    (void)g;
    die("shift_pgup\n");
}

// shift_pgdn is a key action
void shift_pgdn(void *globals, GdkEventKey *event_key){
    (void)event_key;
    globals_t *g = globals;
    (void)g;
    die("shift_pgdn\n");
}

// shift_insert is a key action
void shift_insert(void *globals, GdkEventKey *event_key){
    (void)event_key;
    globals_t *g = globals;
    (void)g;
    die("shift_insert\n");
}

// developer.gnome.org/gtk3/3.24/GtkWidget.html#GtkWidget-key-press-event
static gboolean on_key_event(GtkWidget *widget, GdkEventKey *event_key,
        gpointer user_data){
    globals_t *g = user_data;

    // some things will wrongly be captured by the im_context, like keypad
    if(event_key->type == GDK_KEY_PRESS){
        bool x = g->appkeypad;
        switch(event_key->keyval){
            // https://vt100.net/docs/vt100-ug/chapter3.html#S3.3 table 3-8
            case GDK_KEY_KP_0:
                ttywrite(g, x ? "\x1bOp" : "0", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_1:
                ttywrite(g, x ? "\x1bOq" : "1", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_2:
                ttywrite(g, x ? "\x1bOr" : "2", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_3:
                ttywrite(g, x ? "\x1bOs" : "3", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_4:
                ttywrite(g, x ? "\x1bOt" : "4", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_5:
                ttywrite(g, x ? "\x1bOu" : "5", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_6:
                ttywrite(g, x ? "\x1bOv" : "6", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_7:
                ttywrite(g, x ? "\x1bOw" : "7", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_8:
                ttywrite(g, x ? "\x1bOx" : "8", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_9:
                ttywrite(g, x ? "\x1bOy" : "9", x ? 3 : 1, 0);
                return TRUE;

            // GDK_KEY_KP_Separator seems to never appear.
            // (KEY_KPCOMMA is coming through GTK as GDK_KEY_KP_Decimal...?)
            // Probably this is fine since modern numpads have no comma.
            case GDK_KEY_KP_Separator:
                ttywrite(g, x ? "\x1bOl" : ",", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_Subtract:
                ttywrite(g, x ? "\x1bOm" : "-", x ? 3 : 1, 0);
                return TRUE;
            case GDK_KEY_KP_Decimal:
                ttywrite(g, x ? "\x1bOn" : ".", x ? 3 : 1, 0);
                return TRUE;

            // return key is handled later for non-appkey mode
            case GDK_KEY_KP_Enter:
                if(x){
                    ttywrite(g, "\x1bOM", 3, 0);
                    return TRUE;
                }
                break;


            // apparently none of [*+/] are special
            // case GDK_KEY_KP_Multiply:
            // case GDK_KEY_KP_Add:
            // case GDK_KEY_KP_Divide:
        }
    }

    if(gtk_im_context_filter_keypress(g->im_ctx, event_key)){
        return TRUE;
    }

    // ignore modifier keys themselves, let GTK track their state
    if(event_key->is_modifier) return TRUE;

    // ignore releases
    if(event_key->type != GDK_KEY_PRESS) return TRUE;

    bool ctrl = event_key->state & GDK_CONTROL_MASK;
    bool shift = event_key->state & GDK_SHIFT_MASK;
    bool alt = event_key->state & GDK_MOD1_MASK;
    bool meta = event_key->state & GDK_META_MASK;

    size_t key_idx = (size_t)-1;
    if(event_key->keyval < 128){
        // ascii keys are 1:1 with key_idx
        key_idx = event_key->keyval;
    }else{
        // certain other keys have explicit mappings to a key_idx
        switch(event_key->keyval){
            // see gtk-3.0/gdk/gdkkeysyms.h
            case GDK_KEY_Home:      key_idx = NAST_KEY_HOME; break;
            case GDK_KEY_KP_End:
            case GDK_KEY_End:       key_idx = NAST_KEY_END; break;
            case GDK_KEY_Insert:    key_idx = NAST_KEY_INSERT; break;
            case GDK_KEY_Delete:    key_idx = NAST_KEY_DELETE; break;
            case GDK_KEY_KP_Page_Up:
            case GDK_KEY_Page_Up:   key_idx = NAST_KEY_PGUP; break;
            case GDK_KEY_KP_Page_Down:
            case GDK_KEY_Page_Down: key_idx = NAST_KEY_PGDN; break;
            case GDK_KEY_BackSpace: key_idx = NAST_KEY_BKSP; break;
            case GDK_KEY_KP_Enter:
            case GDK_KEY_Return:    key_idx = NAST_KEY_ENTER; break;
            case GDK_KEY_ISO_Left_Tab:
            case GDK_KEY_KP_Tab:
            case GDK_KEY_Tab:       key_idx = NAST_KEY_TAB; break;
            case GDK_KEY_Escape:    key_idx = NAST_KEY_ESC; break;

            case GDK_KEY_Up:        key_idx = NAST_KEY_UP; break;
            case GDK_KEY_Down:      key_idx = NAST_KEY_DN; break;
            case GDK_KEY_Left:      key_idx = NAST_KEY_LEFT; break;
            case GDK_KEY_Right:     key_idx = NAST_KEY_RIGHT; break;

            case GDK_KEY_KP_F1:
            case GDK_KEY_F1:        key_idx = NAST_KEY_F1; break;
            case GDK_KEY_KP_F2:
            case GDK_KEY_F2:        key_idx = NAST_KEY_F2; break;
            case GDK_KEY_KP_F3:
            case GDK_KEY_F3:        key_idx = NAST_KEY_F3; break;
            case GDK_KEY_KP_F4:
            case GDK_KEY_F4:        key_idx = NAST_KEY_F4; break;

            case GDK_KEY_F5:        key_idx = NAST_KEY_F5; break;
            case GDK_KEY_F6:        key_idx = NAST_KEY_F6; break;
            case GDK_KEY_F7:        key_idx = NAST_KEY_F7; break;
            case GDK_KEY_F8:        key_idx = NAST_KEY_F8; break;
            case GDK_KEY_F9:        key_idx = NAST_KEY_F9; break;
            case GDK_KEY_F10:       key_idx = NAST_KEY_F10; break;
            case GDK_KEY_F11:       key_idx = NAST_KEY_F11; break;
            case GDK_KEY_F12:       key_idx = NAST_KEY_F12; break;
            case GDK_KEY_F13:       key_idx = NAST_KEY_F13; break;
            case GDK_KEY_F14:       key_idx = NAST_KEY_F14; break;
            case GDK_KEY_F15:       key_idx = NAST_KEY_F15; break;
            case GDK_KEY_F16:       key_idx = NAST_KEY_F16; break;
            case GDK_KEY_F17:       key_idx = NAST_KEY_F17; break;
            case GDK_KEY_F18:       key_idx = NAST_KEY_F18; break;
            case GDK_KEY_F19:       key_idx = NAST_KEY_F19; break;
            case GDK_KEY_F20:       key_idx = NAST_KEY_F20; break;
            case GDK_KEY_F21:       key_idx = NAST_KEY_F21; break;
            case GDK_KEY_F22:       key_idx = NAST_KEY_F22; break;
            case GDK_KEY_F23:       key_idx = NAST_KEY_F23; break;
            case GDK_KEY_F24:       key_idx = NAST_KEY_F24; break;
            case GDK_KEY_F25:       key_idx = NAST_KEY_F25; break;
            case GDK_KEY_F26:       key_idx = NAST_KEY_F26; break;
            case GDK_KEY_F27:       key_idx = NAST_KEY_F27; break;
            case GDK_KEY_F28:       key_idx = NAST_KEY_F28; break;
            case GDK_KEY_F29:       key_idx = NAST_KEY_F29; break;
            case GDK_KEY_F30:       key_idx = NAST_KEY_F30; break;
            case GDK_KEY_F31:       key_idx = NAST_KEY_F31; break;
            case GDK_KEY_F32:       key_idx = NAST_KEY_F32; break;
            case GDK_KEY_F33:       key_idx = NAST_KEY_F33; break;
            case GDK_KEY_F34:       key_idx = NAST_KEY_F34; break;
            case GDK_KEY_F35:       key_idx = NAST_KEY_F35; break;
        }
    }
    if(key_idx != (size_t)-1){
        key_map_t *map = keymap[key_idx];

        /* the ALTIFY flag on the zeroth element dictates if we allow alt to
           add 128 to the output */
        bool altify = map[0].mask & ALTIFY;

        size_t i = 0;
        while(true){
            bool ok = true;
            unsigned int mask = map[i].mask;
            ok &= !(mask & MATCH_CTRL) || (ctrl == !!(mask & CTRL_MASK));
            ok &= !(mask & MATCH_SHIFT) || (shift == !!(mask & SHIFT_MASK));
            ok &= !(mask & MATCH_ALT) || (alt == !!(mask & ALT_MASK));
            ok &= !(mask & MATCH_META) || (meta == !!(mask & META_MASK));
            if(ok) break;
            i++;
        }

        key_action_t *act = map[i].action;
        char buf[128];
        size_t len;
        simple_key_t simple;
        // descend through any pointers to a terminal action
        while(true){
            switch(act->type){
                case KEY_ACTION_APPCURSOR:
                    act = act->val.appcursor[!g->appcursor];
                    // restart the loop with the new key action
                    continue;

                case KEY_ACTION_APPKEY:
                    act = act->val.appcursor[!g->appkeypad];
                    // restart the loop with the new key action
                    continue;

                case KEY_ACTION_FUNC:
                    // execute the action and end the entire function
                    act->val.func(g, event_key);
                    return TRUE;

                case KEY_ACTION_MODS:
                    /* MODS is a terminal action only when there is a modifier
                       key pressed */
                    if(!shift && !ctrl && !alt && !meta){
                        act = act->val.mods[1];
                        continue;
                    }
                    // the "on" action for mods is always a simple
                    simple = act->val.mods[0]->val.simple;
                    int mod_idx = 1 + shift + 2*alt + 4*ctrl + 8*meta;
                    int ilen = sprintf(buf, simple.text, mod_idx);
                    if(ilen < 1){
                        fprintf(stderr, "failed to sprintf(%.*s, %d)\n",
                                (int)simple.len, simple.text, mod_idx);
                    }else{
                        ttywrite(g, buf, (size_t)ilen, 0);
                    }
                    return TRUE;

                case KEY_ACTION_SIMPLE:
                    simple = act->val.simple;
                    if(altify && alt){
                        /* when altify is set, we take the value of char we
                           would emit, add 128 to it, then utf-8 encode it and
                           emit the result */
                        Rune r = simple.text[0];
                        len = utf8encode(r + 128, buf);
                        ttywrite(g, buf, len, 0);
                    }else{
                        ttywrite(g, simple.text, simple.len, 0);
                    }
                    return TRUE;
            }
        }
    }

    if(event_key->type == GDK_KEY_PRESS){
        printf("unhandled keypress! (0x%x)\n", event_key->keyval);
    }

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
    if(g->want_focus){
        ttywrite(g, "\x1b[I", 3, 0);
    }
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
    if(g->want_focus){
        ttywrite(g, "\x1b[O", 3, 0);
    }
    return FALSE;
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
            .set_mode = set_mode,
            .set_title = set_title,
            .set_clipboard = set_clipboard,
        },
    };
    G = &g;

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
    g_signal_connect(G_OBJECT(g.window), "focus-in-event", G_CALLBACK(on_focus_in), &g);
    g_signal_connect(G_OBJECT(g.window), "focus-out-event", G_CALLBACK(on_focus_out), &g);

    gtk_window_set_position(GTK_WINDOW(g.window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(g.window), 400, 400);
    gtk_window_set_title(GTK_WINDOW(g.window), "nast");

    gtk_widget_show_all(g.window);

    // create the terminal
    tnew(&g.term, 80, 40, "monospace 10", (THooks*)&g);

    // g.ttyfd = ttynew(g.term, NULL, "/bin/sh", NULL, (char*[]){"cat", NULL});
    g.ttyfd = ttynew(g.term, &g.pid, NULL, shell, NULL, NULL);
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

    // write some shit
    char buf[] = "\x1b[35mhello\x1b[m \x1b[45mworld\x1b[m!\r\nthis \x1b[45mis a\r\n\x1b[30mtest\x1b[m\r\n";
    twrite(g.term, buf, sizeof(buf) - 1,  0);

    gtk_main();

    return 0;
}
