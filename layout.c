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

#include <string.h> /* For layout() */

#include "xtags.h"

/*
 * Layouts list
*/
const Layout layouts[] = {
   "maximize",		maximize,
   "floating",		floating,
   "chess",		chess,
};

/*
 * The maximize layout.
*/
void
maximize(void)
{
   Client *c;

   for(c = info.clients; c; c = c->next) {
      if( c->tags[info.curtag] ) {
#ifdef SLIDE_IN
	 SLIDE_IN(c)
#endif

         if( c->isfloating )
	    resize(c, c->x, c->y, c->w, c->h, False);
	 else
	    resize(c, 0, 0, info.width - 2 * BORDER_SIZE,
	                    info.height - 2 * BORDER_SIZE,
	  		    False);
      }

      /* Set the current (raised) client */
      if( ! info.selclient && israised(c->win) )
         info.selclient = c;
   }

} /* eof maximize() */

/*
 * The floating layout.
*/
void
floating(void)
{
   Client *c;

   for(c = info.clients; c; c = c->next) {
      if( c->tags[info.curtag] ) {
#ifdef SLIDE_IN
	 SLIDE_IN(c)
#endif

	 resize(c, c->x, c->y, c->w, c->h, False);

         /* Set the current (raised) client */
         if( ! info.selclient && israised(c->win) )
            info.selclient = c;
      }
   }

} /* eof floating() */

/*
 * Change the layout.
*/
void
layout(const char *name)
{
   int i;

   for(i = 0; i < LAYOUTS; i++) {
      if( ! strncmp(layouts[i].name, name, strlen(layouts[i].name)) &&
          info.layouts[info.curtag] != layouts[i].arrange )
         info.layouts[info.curtag] = layouts[i].arrange;
   }

   info.layouts[info.curtag](); /* Rearrange */

} /* eof layout() */

/*
 * ``Snap'' the client from the current layout.
*/
void
snap(const char *arg)
{
   if( info.selclient )
      info.selclient->isfloating = !info.selclient->isfloating;
   
   arrange();

} /* eof snap() */

/*
 * Change a layout since a client is selected, then
 * restore the previous layout and raise the selected
 * client.
*/
void
flyout(const char *name)
{
   XEvent ev;
   void (*olayout)(void);
   char c[2] = { 0 };
   
   olayout = info.layouts[info.curtag];
   layout(name);

   /* Disable the auto-repeat */
   XAutoRepeatOff(info.dpy);

   /* Select a client, hopefully */
   if( ( (c[0] = XGetc()) - '0') > 0 ) {
      --c[0]; /* Fit the client_select() range (starting from 1) */
      client_select(c);
   }

   /* Restore the layout */
   info.layouts[info.curtag] = olayout;
   info.layouts[info.curtag](); /* Rearrange */

   /* Enable the auto-repeat */
   XAutoRepeatOn(info.dpy);

} /* eof flyout() */

/*
 * The chess layout.
*/
void
chess(void)
{
   Client *c;
   int i, x, y, w, h, n;
   int clients, fclients;

   for(c = info.clients, clients = 0; c; c = c->next) {
      if( c->tags[info.curtag] ) {
         if( c->isfloating )
            ++fclients;
	 else
	    ++clients;
      }
   }

   if( clients ) { /* Check for unfloating clients (and configure their size) */
      w = info.width / (clients > CHESS_C4R ? CHESS_C4R : clients);
      h = info.height / (clients / CHESS_C4R + (clients % CHESS_C4R ? 1 : 0));
   }
   else if( ! fclients )
      return; /* No unfloating clients nor floating ones */

   for(c = info.clients, i = 0; c; c = c->next) {
      if( c->tags[info.curtag] ) {

#ifdef SLIDE_IN
	 SLIDE_IN(c)
#endif

	 if( ! c->isfloating ) {

            if( ! i )
	       x = y = 0;
	    else if( (i % CHESS_C4R) )
	       x += w;
	    else {
	       x = 0;
	       y += h;
	    }

	    resize(c, x, y, w - 2 * BORDER_SIZE,
	                    h - 2 * BORDER_SIZE,
	                    False);

	    ++i; /* Next unfloating client */
         }
	 else
	    resize(c, c->x, c->y, c->w, c->h, False);

         /* Set the current (raised) client */
         if( ! info.selclient && israised(c->win) )
            info.selclient = c;
      }
   }

} /* eof chess() */

