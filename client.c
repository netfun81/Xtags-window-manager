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

#include "xtags.h"
#include <string.h> /* For client_filter() */

#define INC_DEF 25 /* Default height/width increment */

/*
 * Add the current client into the given tag
*/
void
client_tag(const char *tagstr)
{
   unsigned int tag;

   if( ! info.selclient )
      return;

   /* All tags */
   if( ! tagstr ) {
      for(tag = 0; tag < TAGS; tag++) {
         if( ! info.selclient->tags[tag] )
	    info.selclient->tags[tag] = True;
      }
   }
   else {
      if( (tag = (unsigned int)atoi(tagstr)) != info.curtag )
         info.selclient->tags[tag] = ! info.selclient->tags[tag];
   }

} /* eof client_tag() */

/*
 * Add a client into the clients list
*/
void
client_add(Window win)
{
   XWindowAttributes xwa;
   Client **c, *newc;
   int i;

   if( ! XGetWindowAttributes(info.dpy, win, &xwa))
      return; /* No window attributes */

   if( ! (newc = malloc( sizeof(Client) )) )
      err(EXIT_FAILURE, "client_add (malloc)");
   
   newc->x = xwa.x;
   newc->y = xwa.y;
   newc->w = xwa.width;
   newc->h = xwa.height;
   newc->win = win;
   newc->isvisible = True;
   newc->isfloating = False;
   newc->label = 0;
  
   if( ! (newc->tags = malloc( sizeof(Bool) * (TAGS))) )
      err(EXIT_FAILURE, "client_add (malloc)");

   /* Initialize the tags */
   for(i = 0; i < TAGS; i++)
      newc->tags[i] = False;
   newc->tags[info.curtag] = True;

   if( ! info.clients ) {
      info.clients = newc;
      newc->prev = NULL;
   }
   else {

      /* Found the client and select the previous one */
      for(c = &info.clients; *c; newc->prev = *c, c = &(*c)->next)
         ;

      *c = newc; /* Link! */
   }
   newc->next = NULL;

   client_sethints(newc);	/* Set the client size hints */

   XMoveResizeWindow(info.dpy, win, newc->x, newc->y, newc->w, newc->h);
   grabkeys(newc->win);

   info.selclient = newc;	/* Current client */

} /* eof client_add() */

/*
 * Delete a client from the clients list
*/
void
client_del(Window win)
{
   Client *c, *oldc = NULL;

   for(c = info.clients; c; c = c->next)
      if( c->win == win )
         break;

   /* The window is not in list */
   if( ! c )
      return;

   oldc = c;

   if(c->prev)
      c->prev->next = c->next;

   if(c->next)
      c->next->prev = c->prev;

   if(c == info.clients)
      info.clients = c->next;

   c->next = c->prev = NULL;

   free(oldc->tags);
   free(oldc);

} /* eof client_del() */

/*
 * Move a client into the specified tag (and untag it
 * from the others tags).
*/
void
client_totag(const char *tagstr)
{
   unsigned int tag = (tagstr ? (unsigned int)atoi(tagstr) : info.prevtag);
   int t;

   if( ! info.selclient )
      return;

   /* Set the new tag for the client */
   if( tag != info.curtag && ! info.selclient->tags[tag] )
      info.selclient->tags[tag] = True;

   /* Untag the client from all other tags */
   for(t = 0; t < TAGS; t++) {
      if( t != tag && info.selclient->tags[t] )
         info.selclient->tags[t] = False;
   }

   arrange();

} /* eof client_totag() */

/*
 * Switch to the next client in the current tag
*/
void
client_next(const char *arg)
{
   Client *c;

   (void)arg; /* Avoid a C compiler warning */

   if( ! info.selclient )
      return;

   for(c = info.selclient->next; c; c = c->next) {
      if( c->tags[info.curtag] )
         break;
   }

   if( ! c ) {
      for(c = info.clients; c != info.selclient; c = c->next) {
         if( c->tags[info.curtag] )
	    break;
      }
   }

   /* Raise the active client */
   if( c ) {
      setbordercolor(BORDER_COLNORM);

      info.selclient = c;
      XRaiseWindow(info.dpy, c->win);
      XSetInputFocus(info.dpy, c->win, RevertToParent, CurrentTime);

      setbordercolor(BORDER_COLSEL);
   }

} /* eof client_next() */

/*
 * Switch to the previous client in the current tag
*/
void
client_prev(const char *arg)
{
   Client *c;

   (void)arg; /* Avoid a C compiler warning */

   if( ! info.selclient )
      return;
 
   /*
    * Search backward the first client into the current tag
    * starting from the current client.
   */
   for(c = info.selclient->prev; c && ! c->tags[info.curtag]; c = c->prev) 
      ;

   if( ! c ) {

      /* Switch to the last client */
      if( (c = info.selclient->next) ) {
         while( c->next )
	    c = c->next;

         /*
          * Search backward, again. This time start from the last
          * client to the current one.
         */
         while( ! c->tags[info.curtag] && c != info.selclient )
            c = c->prev;
      }
   }

   /* Raise the active client */
   if( c ) {
      setbordercolor(BORDER_COLNORM);

      info.selclient = c;
      XRaiseWindow(info.dpy, c->win);
      XSetInputFocus(info.dpy, c->win, RevertToParent, CurrentTime);

      setbordercolor(BORDER_COLSEL);
   }

} /* eof client_prev() */

/*
 * Kill the current client
*/
void
client_kill(const char *arg)
{
   XEvent ev;
   Atom adel, aprots, *protocols;
   Bool isprotodel;
   int i, n;

   if( ! info.selclient )
      return;

   /* Set the atoms */
   adel = XInternAtom(info.dpy, "WM_DELETE_WINDOW", False);
   aprots = XInternAtom(info.dpy, "WM_PROTOCOLS", False);

   /* Check if it's a protocol delete window */
   isprotodel = False;
   if(XGetWMProtocols(info.dpy, info.selclient->win, &protocols, &n)) {
      for(i = 0; !isprotodel && i < n; i++)
         if(protocols[i] == adel)
	    isprotodel = True;

      XFree(protocols);
   }

   /* Send an event (killing softly) */
   if( isprotodel ) {
      ev.type = ClientMessage;
      ev.xclient.window = info.selclient->win;
      ev.xclient.message_type = aprots;
      ev.xclient.format = 32;
      ev.xclient.data.l[0] = adel;
      ev.xclient.data.l[1] = CurrentTime;
      XSendEvent(info.dpy, info.selclient->win, False, NoEventMask, &ev);
   }
   else /* Forcely close the connection to the X server */
      XKillClient(info.dpy, info.selclient->win);

} /* eof client_next() */

/*
 * Switch to the selected client in the current tag
*/
void
client_select(const char *arg)
{
   Client *c;
   int i, id;

   if( ! (arg && info.selclient) )
      return;

   /* Set the client id, if any */
   if( (id = atoi(arg)) < 0 )
      return;

   /*
    * Look for the given client through the clients into the
    * current tag.
   */
   for(i = 0, c = info.clients; c; c = c->next) {
      if( c->tags[info.curtag] ) {
         if( i == id )
            break;

	 ++i;
      }
   }

   /* Check if the given ID match a client other than the current */
   if( c && c != info.selclient ) {
      if( ! israised(c->win) ) {
         XRaiseWindow(info.dpy, c->win);
	 XSetInputFocus(info.dpy, c->win, RevertToParent, CurrentTime);
      }
      info.selclient = c;
   }

} /* eof client_select() */

/*
 * Move the client to the direction specified by the
 * _route_ argument ([2,4,6,8] as in the keypad). 
*/
void
client_move(const char *route)
{
   Bool update = True;
   int incw, inch;

   /* Work only in floating mode */
   if( ! info.selclient || (info.layouts[info.curtag] == maximize && ! info.selclient->isfloating) )
      return;

   incw = (info.selclient->incw ? info.selclient->incw : INC_DEF) * MRFACT;
   inch = (info.selclient->inch ? info.selclient->inch : INC_DEF) * MRFACT;

   switch(atoi(route)) {
      case 8:	/* up */
         info.selclient->y = (info.selclient->y + info.selclient->h - inch > 0 ?
	    info.selclient->y - inch : -info.selclient->h) - 2 * BORDER_SIZE;
         break;
      case 2:	/* down */
         info.selclient->y = (info.selclient->y + inch < info.height ?
	    info.selclient->y + inch : info.height);
         break;
      case 4: /* left */
         info.selclient->x = (info.selclient->x + info.selclient->w - incw > 0 ?
	    info.selclient->x - incw : -info.selclient->w) - 2 * BORDER_SIZE;
         break;
      case 6: /* right */
         info.selclient->x = (info.selclient->x + incw <= info.width ?
	    info.selclient->x + incw : info.width);
         break;
      default:
         update = False;
	 break;
   }

   if( update )
      XMoveWindow(info.dpy, info.selclient->win,
         info.selclient->x, info.selclient->y);

} /* eof client_move() */

/*
 * Set the size hints (min and max width and height) for
 * the given client.
*/
void
client_sethints(Client *c)
{
   XSizeHints *xsh;
   long int msize;

   if( ! c )
      return;

   /* Allocate and initialize the hints */
   if( ! (xsh = XAllocSizeHints()) )
      errx(EXIT_FAILURE, "Cannot allocate XSizeHints structure.");

   /* Get the hints, hopefully */
   (void)XGetWMNormalHints(info.dpy, c->win, xsh, &msize);

   /*
    * Minimum width/height must always be respected in order
    * to prevent some client to do wrong things (e.g. xterm).
    * Also the incremental width and height offset should be
    * honored in order to avoid weirdness between clients. To
    * resize properly the base width and height have to be
    * used into the resize() function.
   */

   /* Set the minimum size */
   if( xsh->flags & PMinSize ) {
      c->minw = xsh->min_width;
      c->minh = xsh->min_height;
   }
   else
      c->minw = c->minh = 1; /* Minimum size */

   if( xsh->flags & PResizeInc ) {
      c->incw = xsh->width_inc;
      c->inch = xsh->height_inc;
   }
   else
      c->incw = c->inch = 0;

   /* Set the base size */
   if( xsh->flags & PBaseSize ) {
      c->basew = xsh->base_width;
      c->baseh = xsh->base_height;
   }
   else if( xsh->flags & PMinSize ) {
      c->basew = xsh->min_width;
      c->baseh = xsh->min_height;
   }
   else
      c->basew = c->baseh = 0;

   /* Set the maximum size */
   if( xsh->flags & PMaxSize ) {
      c->maxw = xsh->max_width;
      c->maxh = xsh->max_height;
   }
   else
      c->maxw = c->maxh = 0;

   XFree(xsh);

} /* client_sethints() */

/*
 * As client_move() but the client is resized, instead.
 * If the _route_ is a negative integer, then the client
 * size is decremented from the opposite side of the client.
 * If it's positive then it's enlarged from the specified
 * side. A zero value do nothing, ATM. Maybe will be handled
 * in a different way in further versions.
*/
void
client_resize(const char *route)
{
   Bool update = True;
   long int msize;
   int size = atoi(route), w, h, incw, inch;

   /* Work only in floating mode */
   if( ! info.selclient || (info.layouts[info.curtag] == maximize && ! info.selclient->isfloating) )
      return;

   w = info.selclient->w;
   h = info.selclient->h;
   incw = (info.selclient->incw ? info.selclient->incw : INC_DEF) * MRFACT;
   inch = (info.selclient->inch ? info.selclient->inch : INC_DEF) * MRFACT;

   switch( (size < 0 ? -size : size) ) {
      case 8:	/* up */
         if( size > 0 ) { 
	    if( ! info.selclient->maxh || info.selclient->h + inch <= info.selclient->maxh ) {
	       h += inch;
	       client_move("8");
	    }
	 }
	 else if( info.selclient->h - inch >= info.selclient->minh )
	    h -= inch;
	 break;
      case 2:	/* down */
	 if( size > 0 ) {
	    if( ! info.selclient->maxh || info.selclient->h + inch <= info.selclient->maxh )
	       h += inch;
	 }
	 else if( info.selclient->h - inch >= info.selclient->minh ) {
	    h -= inch;
	    client_move("2");
	 }
         break;
      case 4:	/* left */
	 if( size > 0 ) {
	    if( ! info.selclient->maxw || info.selclient->w + incw <= info.selclient->maxw ) {
	       w += incw;
	       client_move("4");
	    }
	 }
	 else if( info.selclient->w - incw >= info.selclient->minw )
	    w -= incw;
         break;
      case 6:	/* right */
	 if( size > 0 ) {
	    if( ! info.selclient->maxw || info.selclient->w + incw <= info.selclient->maxw )
	       w += incw;
	 }
	 else if( info.selclient->w - incw >= info.selclient->minw ) {
	    w -= incw;
	    client_move("6");
	 }
         break;
      default:
         update = False;
	 break;
   }

   if( update )
      resize(info.selclient, info.selclient->x, info.selclient->y, w, h, True);

} /* eof client_resize() */

/*
 * Apply the client's filter.
*/
void
client_filter(void)
{
   XClassHint ch = { 0 };
   int i;

   if(!XGetClassHint(info.dpy, info.selclient->win, &ch)) {
      warn("unable to filter the client");
      return;
   }

   /* Look for matching filters */
   for(i = 0; i < FILTERS; i++) {

      if( (ch.res_name && strstr(filters[i].name, ch.res_name)) ||
          (ch.res_class && strstr(filters[i].name, ch.res_class)) ) {

         /* Snap! */
	 if( filters[i].isfloating )
	    info.selclient->isfloating = filters[i].isfloating;

         /* To tag */
         if( filters[i].tag )
	    client_totag(filters[i].tag);

	 break; /* No need to apply other filters, ATM */
      }
   }

   XFree(ch.res_name);
   XFree(ch.res_class);

} /* eof client_filter() */

/*
 * Set a label for the current client.
*/
void
client_label(const char *arg)
{

   (void)arg; /* Avoid a C compiler warning */

   if( ! info.selclient )
      return;

   info.selclient->label = XGetc();

} /* eof clietn_label() */

/*
 * Focus to the clients matching the given label.
*/
void
client_focus(const char *arg)
{
   Client *c;
   char label;

   (void)arg; /* Avoid a C compiler warning */

   label = XGetc();

   /* Look for the labeled client in the current tag */
   for(c = info.clients; c; c = c->next)
      if( c->tags[info.curtag] && c->label == label )
         break;

   /* Change the focus */
   if( c && c != info.selclient ) {
      if( ! israised(c->win) ) {
         XRaiseWindow(info.dpy, c->win);
	 XSetInputFocus(info.dpy, c->win, RevertToParent, CurrentTime);
      }
      info.selclient = c;
   }

} /* eof client_focus() */

