/*
 *  wmfs2 by Martin Duquesnoy <xorg62@gmail.com> { for(i = 2011; i < 2111; ++i) ©(i); }
 *  File created by David Delassus.
 *  For license, see COPYING.
 */

#include "wmfs.h"
#include "fifo.h"
#include "config.h"

void
fifo_init(void)
{
     xasprintf(&(W->fifo.path), "%s/wmfs-fifo.%d", P_tmpdir, getpid());

     if(mkfifo(W->fifo.path, 0644) < 0
               || !(W->fifo.fd = open(W->fifo.path, O_NONBLOCK, 0)))
     {
          warnx("Can't create FIFO: %s\n", strerror(errno));
          return;
     }
}

static void
fifo_parse(char *cmd)
{
     void (*func)(Uicb);
     char *uicb, *arg, *p;

     /* remove trailing newline */
     if((p = strchr(cmd, '\n')))
          *p = 0;

     /* Check if an argument is present */
     if((p = strchr(cmd, ' ')))
     {
          *p = 0;
          arg = xstrdup(p + 1);
     }
     uicb = xstrdup(cmd);

     func = uicb_name_func(uicb);
     if(func)
          func(arg);

     if(arg)
          free(arg);
     free(uicb);

     XSync(W->dpy, false);
}

void
fifo_read(void)
{
     char buf[256] = {0};

     /* Don't read it if not open */
     if(!(W->fifo.fd))
          return;

     read(W->fifo.fd, buf, sizeof(buf) - 1);

     if(buf[0])
          fifo_parse(buf);
}

