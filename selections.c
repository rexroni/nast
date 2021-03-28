
#define ISDELIM(u)        (u && wcschr(worddelimiters, u))

void
selinit(void)
{
    sel.mode = SEL_IDLE;
    sel.snap = 0;
    sel.ob.x = -1;
}

void
selstart(Term *t, int col, int row, int snap)
{
    selclear(t);
    sel.mode = SEL_EMPTY;
    sel.type = SEL_REGULAR;
    sel.alt = IS_SET(t, MODE_ALTSCREEN);
    sel.snap = snap;
    sel.oe.x = sel.ob.x = col;
    sel.oe.y = sel.ob.y = row;
    selnormalize(t);

    if (sel.snap != 0)
        sel.mode = SEL_READY;
    tsetdirt(t, sel.nb.y, sel.ne.y);
}

void
selextend(Term *t, int col, int row, int type, int done)
{
    int oldey, oldex, oldsby, oldsey, oldtype;

    if (sel.mode == SEL_IDLE)
        return;
    if (done && sel.mode == SEL_EMPTY) {
        selclear(t);
        return;
    }

    oldey = sel.oe.y;
    oldex = sel.oe.x;
    oldsby = sel.nb.y;
    oldsey = sel.ne.y;
    oldtype = sel.type;

    sel.oe.x = col;
    sel.oe.y = row;
    selnormalize(t);
    sel.type = type;

    if (oldey != sel.oe.y || oldex != sel.oe.x || oldtype != sel.type || sel.mode == SEL_EMPTY)
        tsetdirt(t, MIN(sel.nb.y, oldsby), MAX(sel.ne.y, oldsey));

    sel.mode = done ? SEL_IDLE : SEL_READY;
}

void
selnormalize(Term *t)
{
    int i;

    if (sel.type == SEL_REGULAR && sel.ob.y != sel.oe.y) {
        sel.nb.x = sel.ob.y < sel.oe.y ? sel.ob.x : sel.oe.x;
        sel.ne.x = sel.ob.y < sel.oe.y ? sel.oe.x : sel.ob.x;
    } else {
        sel.nb.x = MIN(sel.ob.x, sel.oe.x);
        sel.ne.x = MAX(sel.ob.x, sel.oe.x);
    }
    sel.nb.y = MIN(sel.ob.y, sel.oe.y);
    sel.ne.y = MAX(sel.ob.y, sel.oe.y);

    selsnap(t, &sel.nb.x, &sel.nb.y, -1);
    selsnap(t, &sel.ne.x, &sel.ne.y, +1);

    /* expand selection over line breaks */
    if (sel.type == SEL_RECTANGULAR)
        return;
    i = tlinelen(t, sel.nb.y);
    if (i < sel.nb.x)
        sel.nb.x = i;
    if (tlinelen(t, sel.ne.y) <= sel.ne.x)
        sel.ne.x = t->col - 1;
}

int
selected(Term *t, int x, int y)
{
    if (sel.mode == SEL_EMPTY || sel.ob.x == -1 ||
            sel.alt != IS_SET(t, MODE_ALTSCREEN))
        return 0;

    if (sel.type == SEL_RECTANGULAR)
        return BETWEEN(y, sel.nb.y, sel.ne.y)
            && BETWEEN(x, sel.nb.x, sel.ne.x);

    return BETWEEN(y, sel.nb.y, sel.ne.y)
        && (y != sel.nb.y || x >= sel.nb.x)
        && (y != sel.ne.y || x <= sel.ne.x);
}

void
selsnap(Term *t, int *x, int *y, int direction)
{
    int newx, newy, xt, yt;
    int delim, prevdelim;
    Glyph *gp, *prevgp;

    switch (sel.snap) {
    case SNAP_WORD:
        /*
         * Snap around if the word wraps around at the end or
         * beginning of a line.
         */
        prevgp = &t->line[*y][*x];
        prevdelim = ISDELIM(prevgp->u);
        for (;;) {
            newx = *x + direction;
            newy = *y;
            if (!BETWEEN(newx, 0, t->col - 1)) {
                newy += direction;
                newx = (newx + t->col) % t->col;
                if (!BETWEEN(newy, 0, t->row - 1))
                    break;

                if (direction > 0)
                    yt = *y, xt = *x;
                else
                    yt = newy, xt = newx;
                if (!(t->line[yt][xt].mode & ATTR_WRAP))
                    break;
            }

            if (newx >= tlinelen(t, newy))
                break;

            gp = &t->line[newy][newx];
            delim = ISDELIM(gp->u);
            if (!(gp->mode & ATTR_WDUMMY) && (delim != prevdelim
                    || (delim && gp->u != prevgp->u)))
                break;

            *x = newx;
            *y = newy;
            prevgp = gp;
            prevdelim = delim;
        }
        break;
    case SNAP_LINE:
        /*
         * Snap around if the the previous line or the current one
         * has set ATTR_WRAP at its end. Then the whole next or
         * previous line will be selected.
         */
        *x = (direction < 0) ? 0 : t->col - 1;
        if (direction < 0) {
            for (; *y > 0; *y += direction) {
                if (!(t->line[*y-1][t->col-1].mode
                        & ATTR_WRAP)) {
                    break;
                }
            }
        } else if (direction > 0) {
            for (; *y < t->row-1; *y += direction) {
                if (!(t->line[*y][t->col-1].mode
                        & ATTR_WRAP)) {
                    break;
                }
            }
        }
        break;
    }
}

char *
getsel(Term *t)
{
    char *str, *ptr;
    int y, bufsize, lastx, linelen;
    Glyph *gp, *last;

    if (sel.ob.x == -1)
        return NULL;

    bufsize = (t->col+1) * (sel.ne.y-sel.nb.y+1) * UTF_SIZ;
    ptr = str = xmalloc(bufsize);

    /* append every set & selected glyph to the selection */
    for (y = sel.nb.y; y <= sel.ne.y; y++) {
        if ((linelen = tlinelen(t, y)) == 0) {
            *ptr++ = '\n';
            continue;
        }

        if (sel.type == SEL_RECTANGULAR) {
            gp = &t->line[y][sel.nb.x];
            lastx = sel.ne.x;
        } else {
            gp = &t->line[y][sel.nb.y == y ? sel.nb.x : 0];
            lastx = (sel.ne.y == y) ? sel.ne.x : t->col-1;
        }
        last = &t->line[y][MIN(lastx, linelen-1)];
        while (last >= gp && last->u == ' ')
            --last;

        for ( ; gp <= last; ++gp) {
            if (gp->mode & ATTR_WDUMMY)
                continue;

            ptr += utf8encode(gp->u, ptr);
        }

        /*
         * Copy and pasting of line endings is inconsistent
         * in the inconsistent terminal and GUI world.
         * The best solution seems like to produce '\n' when
         * something is copied from st and convert '\n' to
         * '\r', when something to be pasted is received by
         * st.
         * FIXME: Fix the computer world.
         */
        if ((y < sel.ne.y || lastx >= linelen) && !(last->mode & ATTR_WRAP))
            *ptr++ = '\n';
    }
    *ptr = 0;
    return str;
}

void
selclear(Term *t)
{
    if (sel.ob.x == -1)
        return;
    sel.mode = SEL_IDLE;
    sel.ob.x = -1;
    tsetdirt(t, sel.nb.y, sel.ne.y);
}

void
selscroll(Term *t, int orig, int n)
{
    if (sel.ob.x == -1)
        return;

    if (BETWEEN(sel.ob.y, orig, t->bot) || BETWEEN(sel.oe.y, orig, t->bot)) {
        if ((sel.ob.y += n) > t->bot || (sel.oe.y += n) < t->top) {
            selclear(t);
            return;
        }
        if (sel.type == SEL_RECTANGULAR) {
            if (sel.ob.y < t->top)
                sel.ob.y = t->top;
            if (sel.oe.y > t->bot)
                sel.oe.y = t->bot;
        } else {
            if (sel.ob.y < t->top) {
                sel.ob.y = t->top;
                sel.ob.x = 0;
            }
            if (sel.oe.y > t->bot) {
                sel.oe.y = t->bot;
                sel.oe.x = t->col;
            }
        }
        selnormalize(t);
    }
}
void
printsel(Term *t, const Arg *arg)
{
    tdumpsel(t);
}

// these are die()-hooks in nast.c, but this is what they looked like:
void
tdumpsel(Term *t)
{
    char *ptr;

    if ((ptr = getsel(t))) {
        tprinter(t, ptr, strlen(ptr));
        free(ptr);
    }
}
void
tscrollup(Term *t, int orig, int n)
{
    int i;
    Line temp;

    LIMIT(n, 0, t->bot-orig+1);

    tclearregion_term(t, 0, orig, t->col-1, orig+n-1);
    tsetdirt(t, orig+n, t->bot);

    for (i = orig; i <= t->bot-n; i++) {
        temp = t->line[i];
        t->line[i] = t->line[i+n];
        t->line[i+n] = temp;
    }

    selscroll(t, orig, -n);
}
void
tscrolldown(Term *t, int orig, int n)
{
    int i;
    Line temp;

    LIMIT(n, 0, t->bot-orig+1);

    tsetdirt(t, orig, t->bot-n);
    tclearregion_term(t, 0, t->bot-n+1, t->col-1, t->bot);

    for (i = t->bot; i >= orig+n; i--) {
        temp = t->line[i];
        t->line[i] = t->line[i-n];
        t->line[i-n] = temp;
    }

    selscroll(t, orig, n);
}
