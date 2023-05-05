/*
 * XTags - Simple multitag implementation for X11
 * Copyright (C) 2008  Claudio M. Alessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h> /* For layout() */

#include "xtags.h"

unsigned int numlockmask = 0;

/* Clean the key mask (no NumLock, CapsLock, etc.) */
#define CLEANMASK(mask) (mask & ~(numlockmask|LockMask))

int
main(int argc, char *argv[])
{
   XEvent ev;
   int i;

   /*
    * Show version informations (only '-v' implemented ATM)
   */
   if( argc == 2 ) {
      if( argv[1][0] == '-' && argv[1][1] == 'v' ) {
         printf(
	    "XTags-" VERSION " - Copyright (C) 2008  Claudio M. Alessi\n"
	 );
	 exit(EXIT_SUCCESS);
      }
   }
   if( argc > 2 )
      errx(EXIT_FAILURE, "only '-v' implemented.");

   startup(); /* Initialize XTags */

   XSync(info.dpy, False);

   while(1) {

      XNextEvent(info.dpy, &ev);

      /*
       * XXX: the following events are totally ignored: KeyRelease,
       *      ConfigureNotify, CreateNotify, DestroyNotify. Some of
       *      these cause problems with applications like xkill(1).
       *      A possible fix is to NOT select the SubstructureNotify
       *      event-mask and found an alternative way to handle the
       *      clients (by getting the Map/Unmap Notify events in a
       *      different way).
      */

      /* Handle events */
      switch(ev.type) {

         case KeyPress:

            for(i = 0; i < KEYS; i++) {
               if( CLEANMASK(keys[i].mod) == CLEANMASK(ev.xkey.state) &&
                   keys[i].key == XKeycodeToKeysym(info.dpy, ev.xkey.keycode, 0) ) {
                  if(keys[i].f)
                     keys[i].f(keys[i].arg);
               }
            }

	    break;

	 case MapNotify:
	    if( ! ev.xmap.override_redirect ) {

	       if( info.selclient )
	          setbordercolor(BORDER_COLNORM);

               client_add(ev.xmap.window);
               client_filter();

               arrange();
	    }
	    break;

         case UnmapNotify:
            client_del(ev.xunmap.window);
	    arrange();
	    break;
      }
   }

   XFlush(info.dpy);
   XCloseDisplay(info.dpy);

   return EXIT_SUCCESS;
} /* eof main() */

/*
 * Initialization routine
*/
void
startup(void)
{
   int i;

   /* Open the display */
   if( ! (info.dpy = XOpenDisplay(NULL)) )
      errx(EXIT_FAILURE, "Cannot open the display");

   if( ! (info.layouts = malloc( sizeof(Client) )) )
      err(EXIT_FAILURE, "client_add (malloc)");

   info.screen = DefaultScreen(info.dpy);
   info.root = RootWindow(info.dpy, info.screen);
   info.width = DisplayWidth(info.dpy, info.screen);
   info.height = DisplayHeight(info.dpy, info.screen);
   info.curtag = info.prevtag = 0;
   info.clients = info.selclient = NULL;

   /* Set the default layout for all tags */
   for(i = 0; i < TAGS; i++)
      info.layouts[i] = LAYOUT;

   signal(SIGTSTP, SIG_IGN);
   signal(SIGINT, SIG_IGN);

   XSelectInput(info.dpy, info.root, SubstructureNotifyMask);

   grabkeys(info.root);	/* Set the keys shortcuts */
   clients_scan();	/* Attach already opened clients */
   arrange();		/* Arrange the clients */

} /* eof startup() */

/*
 * X error handler (ignore) - Just ignore the events
*/
int
xerror_handler_ignore(Display *d, XErrorEvent *e)
{
   (void)d, (void)e; /* Avoid some C compiler warning */
   return 0;
} /* xerror_handler_ignore() */

/*
 * Load the user-configurable key bindings
*/
void
grabkeys(Window win)
{
   KeyCode code;
   XModifierKeymap *modmap;
   int i, j;

   modmap = XGetModifierMapping(info.dpy);
   for(i = 0; i < 8; i++) {
      for(j = 0; j < modmap->max_keypermod; j++) {
         if(modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(info.dpy, XK_Num_Lock))
            numlockmask = (1 << i);
      }
   }
   XFreeModifiermap(modmap);

   for(i = 0; i < KEYS; i++) {
      code = XKeysymToKeycode(info.dpy, keys[i].key);
      XGrabKey(info.dpy, code, keys[i].mod,
	 win, True, GrabModeAsync, GrabModeAsync);

      /* Handle NumLock, CapsLock, BlocScorr, ... */
      XGrabKey(info.dpy, code, keys[i].mod|LockMask,
	 win, True, GrabModeAsync, GrabModeAsync);
      XGrabKey(info.dpy, code, keys[i].mod|numlockmask,
	 win, True, GrabModeAsync, GrabModeAsync);
      XGrabKey(info.dpy, code, keys[i].mod|numlockmask|LockMask,
	 win, True, GrabModeAsync, GrabModeAsync);
   }

} /* eof grabkeys() */

/*
 * Change the view to the specified tag
*/
void
tagview(const char *tagstr)
{
   unsigned int tag;

   if( ! tagstr )
      tag = info.prevtag;

   else if( ! strcmp(tagstr, "next") )
      tag = (info.curtag + 1 < TAGS ? info.curtag + 1 : 0);

   else if( ! strcmp(tagstr, "prev") )
      tag = ((int)info.curtag - 1 >= 0 ? info.curtag - 1 : TAGS - 1);

   else
      tag = (unsigned int)atoi(tagstr);

   if( info.curtag != tag ) {
      info.prevtag = info.curtag;
      info.curtag = tag;
      arrange();
   }

} /* eof viewtag() */

/*
 * Run an external program
*/
void
spawn(const char *cmd)
{
   /*
    * Fork twice to avoid zombies
   */
   if(fork() == 0) {
      if(fork() == 0) {
         close(ConnectionNumber(info.dpy));
         setsid(); /* Start a new session */
         execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
	 warn("execl");
      }
      exit(0);
   }
   wait(0);

} /* eof spawn() */

/*
 * Add all opened clients into the list
*/
void
clients_scan(void)
{
   XWindowAttributes wa;
   Window *siblings, root, parent;
   unsigned int i, num;

   if(XQueryTree(info.dpy, info.root, &root, &parent, &siblings, &num)) {

      for(i = 0; i < num; i++) {
         if(XGetWindowAttributes(info.dpy, siblings[i], &wa) &&
	    wa.map_state == IsViewable )
	    client_add(siblings[i]);
      }

      if(siblings)
         XFree(siblings);
   }

} /* eof clients_scan() */

/*
 * Arrange the clients
*/
void
arrange(void)
{
   Client *c;

   info.selclient = NULL;

   /* Move the untagged clients out of the screen area */
   for(c = info.clients; c; c = c->next) {
      if( ! c->tags[info.curtag] ) {
         if( c->isvisible ) {
#ifdef SLIDE_OUT
            SLIDE_OUT(c)
#endif
            XMoveWindow(info.dpy, c->win, info.width, c->y);
	    c->isvisible = !c->isvisible;
         }
      }
      else if( ! c->isvisible )
         c->isvisible = !c->isvisible;
   }

   info.layouts[info.curtag](); /* Arrange the layout */

   /* Focus to the current client */
   if( info.selclient ) {
      XSetInputFocus(info.dpy, info.selclient->win,
         RevertToParent, CurrentTime);
#if BORDER_SIZE
      setbordercolor(BORDER_COLSEL);
#endif
   }

   /* Empty tag: focus to the root window */
   else
      XSetInputFocus(info.dpy, info.root, RevertToNone, CurrentTime);

} /* eof arrange() */

/*
 * Set the border color for the current client to the value
 * of the string _colstr_, which should be represented as an
 * hexadecimal value.
*/
void
setbordercolor(const char *colstr)
{
   Colormap cmap = DefaultColormap(info.dpy, info.screen);
   XColor color;

   if( ! info.selclient )
      return;

   if( ! XAllocNamedColor(info.dpy, cmap, colstr, &color, &color))
      errx(EXIT_FAILURE, "Cannot allocate named color.");

   XSetWindowBorder(info.dpy, info.selclient->win, color.pixel);

} /* eof setbordercolor() */

/*
 * Return the client associated to the given window, if any,
 * else return NULL.
*/
Client *
getclient(Window win)
{
   Client *c;

   for(c = info.clients; c && c->win != win; c = c->next)
      ;
   
   return c;
} /* eof getclient() */

/*
 * Make all clients visible, do some cleanup and exit.
*/
void
quit(const char *arg)
{
   (void)arg; /* Avoid a C compiler warning */

   /* Place all clients into the same tag */
   clients_totag(NULL);

   XUngrabKey(info.dpy, AnyKey, AnyModifier, info.root);
   XSync(info.dpy, False);

   XFlush(info.dpy);
   XCloseDisplay(info.dpy);

   exit(EXIT_SUCCESS);

} /* eof quit() */

/*
 * Move all clients into the specified tag. If the _tagstr_
 * argument is null, then the current tag is used instead.
*/
void
clients_totag(const char *tagstr)
{
   Client *c;
   unsigned int tag = (tagstr ? (unsigned int)atoi(tagstr) : info.curtag);

   /* Move the clients */
   for(c = info.clients; c; c = c->next) {
      if( ! c->tags[tag] ) {
         c->tags[info.curtag] = False;
         c->tags[tag] = True;
      }
   }
   
   arrange();

} /* eof clients_totag() */

/*
 * Close all clients into the current tag.
*/
void
clients_kill(const char *arg)
{
   Client *c;

   for(c = info.clients; c; c = c->next) {
      if( c->tags[info.curtag] ) {
         info.selclient = c;
	 client_kill(NULL);
      }
   }   

   arrange();

} /* eof clients_kill() */

/*
 * Check if the given window is raised between the
 * clients of the current tag and return a boolean
 * value.
*/
Bool
israised(Window win)
{
   Client *c;
   XWindowAttributes wa;
   Window *siblings, root, parent;
   unsigned int num;
   int i, (*xerror_handler_default)(Display *d, XErrorEvent *e);

   /* Ignore all errors */
   xerror_handler_default = XSetErrorHandler(xerror_handler_ignore);

   if(XQueryTree(info.dpy, info.root, &root, &parent, &siblings, &num)) {
      
      /* Check the window in reverse stacking order */
      for(i = num - 1; i >= 0; i--) {

         /* Skip unviewable windows */
         if(XGetWindowAttributes(info.dpy, siblings[i], &wa) &&
	    wa.map_state == IsViewable ) {

	    /* Found the coresponding client */
	    for(c = info.clients; c; c = c->next) {
               if( c->tags[info.curtag] && siblings[i] == c->win ) {

	          XFree(siblings);

	          if( c->win == win )
		     return True;

		  return False;
	       }
	    }
	 }
      }

      if(siblings)
         XFree(siblings);
   }

   /* Restore the default X error handler */
   XSetErrorHandler(xerror_handler_default);

   return False;
} /* eof israised() */

/*
 * Move and/or resize the given client. If _save_ is true
 * the new size and position is stored else is just ignored.
*/
void
resize(Client *c, int x, int y, int w, int h, Bool save)
{
   XWindowChanges xwc;
   Bool border = True;

   /* Remove the borders from full screen clients */
   if( (w == info.width - 2 * BORDER_SIZE && h == info.height - 2 * BORDER_SIZE) ||
       (w == info.width && h == info.height) )
      border = False;

   /* Remove the base size */
   w -= c->basew;
   h -= c->baseh;

   /* Toggle the exceeded increment size, if any */
   if(c->incw)
      w -= w % c->incw;
   if(c->inch)
      h -= h % c->inch;

   /* Restore the base size */
   w += c->basew;
   h += c->baseh;

   /* Store the new informations */
   if( save ) {
      if( c->x != x )
         c->x = x;

      if( c->y != y )
         c->y = y;

      if( c->w != w )
         c->w = w;

      if( c->h != h )
         c->h = h;
   }

   xwc.x = x;
   xwc.y = y;
   xwc.width = w;
   xwc.height = h;

   /* Set the border width */
   xwc.border_width = ( border ? BORDER_SIZE : 0);

   XConfigureWindow(info.dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &xwc);

} /* eof resize() */

/*
 * Get a character from the keyboard.
*/
int
XGetc(void)
{
   XEvent ev;
   KeySym keysym;
   XComposeStatus status;
   char c[2] = { 0 };

   XGrabKeyboard(info.dpy, info.root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
   XMaskEvent(info.dpy, KeyPressMask, &ev);
   XLookupString((XKeyEvent *)&ev, c, 1, &keysym, &status);
   XUngrabKeyboard(info.dpy, CurrentTime);

   return *c;
} /* eof XGetc() */

