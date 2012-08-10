/*
 * aim.c By Martin Duquesnoy <xorg62@gmail.com>
 * AIM training tool
 * cc -Wall -lX11 aim.c -o aim
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/queue.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#define INAREA(i, j, a) ((i) >= (a).x && (i) <= (a).x + (a).w && (j) >= (a).y && (j) <= (a).y + (a).h)
#define RAND(min, max)  ((rand() % (max - min + 1)) + min)

#define WIDTH      800
#define HEIGHT     600
#define XHAIR_X    (WIDTH >> 1)
#define XHAIR_Y    (HEIGHT >> 1)
#define XHAIR_SIZE 5

#define RUN_FLAG          0x01
#define FIRST_TIME_FLAG   0x02
#define MOUSE_MOTION_FLAG 0x04
#define GRAB_FLAG         0x08
static unsigned int flags = 0;

static Display *dpy;
static Window win, root;
static Drawable dr;
static GC gc;
static Cursor no_ptr;
static int score = 0;

static const unsigned int lifecols[] = { 0x000000, 0xFF0000, 0xBB4444, 0x996666, 0xFFAAAA, 0xFFFFFF };

struct pos
{
     int x, y;
     int w, h;
     int incx, incy;
     int life;
     SLIST_ENTRY(pos) next;
};

SLIST_HEAD(pos_head, pos) phead;

static inline void
swap_int(int *x, int *y)
{
     *y = *x ^ *y;
     *x = *y ^ *x;
     *y = *x ^ *y;
}

static struct pos*
pos_new(int x, int y, int w, int h, int incx, int incy)
{
     struct pos *p = calloc(1, sizeof(struct pos));

     p->x = x;
     p->y = y;
     p->w = w;
     p->h = h;
     p->incx = incx;
     p->incy = incy;
     p->life = 5;

     SLIST_INSERT_HEAD(&phead, p, next);

     return p;
}

static void
pos_remove(struct pos *p)
{
     SLIST_REMOVE(&phead, p, pos, next);
     free(p);
}

static void
pos_flush(void)
{
     struct pos *p;

     while(!SLIST_EMPTY(&phead))
     {
          p = SLIST_FIRST(&phead);
          SLIST_REMOVE_HEAD(&phead, next);
          free(p);
     }
}

static void
pos_draw(void)
{
     struct pos *p;

     if(SLIST_EMPTY(&phead))
     {
          printf("[aimtrain] No more target\n[aimtrain] Score: %d\n", score);
          flags ^= RUN_FLAG;
          return;
     }

     SLIST_FOREACH(p, &phead, next)
     {
          p->x += p->incx;
          p->y += p->incy;

          if(p->x < 0)
               p->x = WIDTH;
          if(p->y < 0)
               p->y = HEIGHT;
          if(p->x > WIDTH)
               p->x = 0;
          if(p->y > HEIGHT)
               p->y = 0;


          XSetForeground(dpy, gc, lifecols[p->life]);

          XFillRectangle(dpy, dr, gc, p->x, p->y, p->w, p->h);
     }
}

static void
pos_inc_all(int incx, int incy)
{
     struct pos *p;

     SLIST_FOREACH(p, &phead, next)
     {
          p->x += incx;
          p->y += incy;
     }
}

static void
pos_check_hit(void)
{
     struct pos *p;

     SLIST_FOREACH(p, &phead, next)
     {
          if(INAREA(XHAIR_X, XHAIR_Y, *p))
          {
               printf("[aimtrain] Hit!\n");
               swap_int(&p->incx, &p->incy);

               if(--p->life <= 0)
               {
                    pos_remove(p);
                    printf("[aimtrain] Frag! (score: %d)\n", ++score);
               }
          }
     }
}

static void
pos_init(void)
{
     int i = 0;

     for(; i < RAND(10, 50); ++i)
     {
          pos_new(RAND(0, WIDTH),
                  RAND(0, HEIGHT),
                  RAND(5, 25),
                  RAND(5, 25),
                  RAND(-5, 5),
                  RAND(-5, 5));
     }
}

void
xhair(void)
{
     XSetForeground(dpy, gc, 0xffffff);
     XDrawLine(dpy, dr, gc,
               XHAIR_X - XHAIR_SIZE,
               XHAIR_Y,
               XHAIR_X + XHAIR_SIZE,
               XHAIR_Y);
     XDrawLine(dpy, dr, gc,
               XHAIR_X,
               XHAIR_Y - XHAIR_SIZE,
               XHAIR_X,
               XHAIR_Y + XHAIR_SIZE);
}

static void
cursorinit(void)
{
     char bm_no_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
     XColor black, dummy;
     Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));
     Pixmap bm_no = XCreateBitmapFromData(dpy, root, bm_no_data, 8, 8);

     no_ptr = XCreatePixmapCursor(dpy, bm_no, bm_no, &black, &black, 0, 0);

     XAllocNamedColor(dpy, cmap, "black", &black, &dummy);

     if(bm_no != None)
          XFreePixmap(dpy, bm_no);

     XFreeColors(dpy, cmap, &black.pixel, 1, 0);
}

int
main(int argc, char **argv)
{
     struct timespec ts;
     int mx, my, mincx, mincy;

     SLIST_INIT(&phead);

     dpy = XOpenDisplay(NULL);
     root = RootWindow(dpy, 0);
     gc = XCreateGC(dpy, root, 0, 0);
     cursorinit();

     win = XCreateSimpleWindow(dpy, root, 0, 0, HEIGHT, WIDTH, 0, 0, BlackPixel(dpy, 0));
     dr = XCreatePixmap(dpy, root, WIDTH, HEIGHT, 24);

     XSelectInput(dpy, win, (KeyPressMask
                             | PointerMotionMask
                             | ButtonPressMask
                             | ButtonReleaseMask
                             | ButtonMotionMask));

     XSetForeground(dpy, gc, WhitePixel(dpy, 0));
     XAllowEvents(dpy, AsyncBoth, CurrentTime);

     flags |= RUN_FLAG;

     XMapWindow(dpy, win);

     ts.tv_sec = 0;
     ts.tv_nsec = 20000000;

     pos_init();

     while(flags & RUN_FLAG)
     {
          XSetForeground(dpy, gc, 0x000000);
          XFillRectangle(dpy, dr, gc, 0, 0, WIDTH, HEIGHT);
          XSetForeground(dpy, gc, 0xffffff);
          XDrawRectangle(dpy, dr, gc, 0, 0, WIDTH - 1, HEIGHT - 1);

          pos_draw();

          while(XPending(dpy))
          {
               XEvent ev;
               KeySym keysym;
               XNextEvent(dpy, &ev);

               switch(ev.type)
               {
               case KeyPress:
                    keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev.xkey.keycode, 0, 0);
                    if(keysym == XK_q || keysym == XK_Q)
                         flags ^= RUN_FLAG;
                    break;
               case ButtonPress:
                    if(ev.xbutton.window == win)
                    {
                         if(ev.xbutton.button == 1)
                         {
                              printf("[aimtrain] Fire!\n");
                              pos_check_hit();
                         }
                         else if(ev.xbutton.button == 3)
                         {
                              if(flags & GRAB_FLAG)
                                   XUngrabPointer(dpy, CurrentTime);
                              else
                                   XGrabPointer(dpy, win, 1,
                                                PointerMotionMask
                                                | ButtonPressMask
                                                | ButtonReleaseMask
                                                | ButtonMotionMask,
                                                GrabModeAsync, GrabModeAsync, None, no_ptr, CurrentTime);

                              flags ^= GRAB_FLAG;
                         }
                    }
                    break;

               case MotionNotify:
                    if(ev.xmotion.window == win)
                    {
                         mincx = mx - ev.xmotion.x;
                         mincy = my - ev.xmotion.y;
                         mx = ev.xmotion.x;
                         my = ev.xmotion.y;

                         pos_inc_all(mincx, mincy);
                    }

                    break;
               }
          }

          xhair();

          XCopyArea(dpy, dr, win, gc, 0, 0, WIDTH, HEIGHT, 0, 0);
          nanosleep(&ts, NULL);
     }

     pos_flush();

     XFreePixmap(dpy, dr);
     XFreeCursor(dpy, no_ptr);
     XDestroyWindow(dpy, win);
     XCloseDisplay(dpy);

     return 0;
}
