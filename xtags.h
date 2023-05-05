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

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <err.h>

/*
 * Clients list
*/
typedef struct client_t {
   Window win;			/* Xlib Window object */
   int x, y, w, h;		/* Window size and position */
   int maxw, maxh, minw, minh;	/* Min/Max width and height */
   int incw, inch;		/* Increment width and height */
   int basew, baseh;		/* Base width and height */
   int label;			/* Client label (for client_focus()) */
   Bool *tags;			/* Selected tags */
   Bool isvisible;		/* Toggle visibility */
   Bool isfloating;		/* Toggle floating */
   struct client_t *next;	/* Link to the next client */
   struct client_t *prev;	/* Link to the previous client */
} Client;

/*
 * Key binding
*/
typedef struct {
   unsigned int mod;		/* Modifier */
   unsigned int key;		/* Key code */
   void (*f)(const char *arg);	/* Handler */
   const char *arg;		/* Argument for the handler */
} Key;

/*
 * Layout informations
*/
typedef struct {
   const char *name;		/* Layout name */
   void (*arrange)(void);	/* Arrangement function */
} Layout;

/*
 * Client's filter
*/
typedef struct {
   const char *name;		/* Client name */
   const char *tag;		/* Referenced tag */
   Bool isfloating;		/* Snap! */
} Filter;

/*
 * XTags commons informations
*/
typedef struct {
   Display *dpy;		/* The display */
   Window root;			/* The root window */
   Client *clients;		/* Clients list */
   Client *selclient;		/* Current client */
   unsigned int curtag;		/* Current tag */
   unsigned int prevtag;	/* Previous tag */
   int screen;			/* The screen */
   int width;			/* Screen width */
   int height;			/* Screen height */
   void (**layouts)(void);	/* Per-tag layouts */
} XTagsInfo;

/*
 * Globals
*/
XTagsInfo info;

/*
 * Functions prototype
*/

/* General routines */
int xerror_handler_ignore(Display *d, XErrorEvent *e);
void startup(void);
void arrange(void);
void clients_scan(void);
void grabkeys(Window win);
Bool israised(Window win);
void setbordercolor(const char *colstr);
Client *getclient(Window win);
void resize(Client *c, int x, int y, int w, int h, Bool save);
void client_add(Window win);
void client_del(Window win);
void client_sethints(Client *c);
void client_filter(void);
int XGetc(void);

/*
 * User functions
*/
typedef void user_func(const char *);
user_func
   spawn,
   tagview,
   snap,
   quit,
   flyout,
   clients_totag,
   clients_kill,
   client_totag,
   client_next,
   client_prev,
   client_kill,
   client_select,
   client_tag,
   client_move,
   client_resize,
   client_label,
   client_focus
; /* user functions */

/* Layout */
void layout(const char *name);
void maximize(void);
void floating(void);
void chess(void);

#include "config.h" /* Include the configuration */

/* Number of keys */
#define KEYS	((int)(sizeof keys / sizeof keys[0]))
#define LAYOUTS ((int)(sizeof layouts / sizeof layouts[0]))
#define FILTERS	((int)(sizeof filters / sizeof filters[0]))

