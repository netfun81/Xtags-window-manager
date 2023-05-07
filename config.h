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

/*
 * Graphical stuff
*/
#if 0

/* Directive to spawn using an OSD message 
 * #define OSD_TOOL	"osd_cat"
 * #define OSD_FONT	"-*-helvetica-*-*-normal-*-34-*-*-*-*-*-*-*"
 * #define OSD_OPTS	"-p bottom -o -90 -i 50 -f '" OSD_FONT "' -c purple -d1"
 * #define ORUN(cmd)	(cmd "& echo \"( Running " #cmd " )\" |" OSD_TOOL " " OSD_OPTS)
*/

/* Directives to spawn using a vocal synthesizer
 * #define SYNT_TOOL	"espeak"
 * #define SYNT_OPTS	"-p 30"
 * #define SRUN(cmd)	(cmd "& echo " #cmd " | " SYNT_TOOL " " SYNT_OPTS)
*/

/* Moving effects */
#define SLIDE_SPEED 10	/* Slide speed (max useful value is 100) */
#define SLIDE_IN(c)	/* The slide-in effect definition */		\
	int i, pct = (info.width - c->x) / (1000 / SLIDE_SPEED);	\
        XMoveWindow(info.dpy, c->win, c->x, c->y);			\
	for(i = info.width - 1; i > (int)c->x; i -= pct)		\
           XMoveWindow(info.dpy, c->win, i, c->y);
#define SLIDE_OUT(c)	/* The slide-out effect definition */		\
	int i, pct = (info.width - c->x) / (1000 / SLIDE_SPEED);	\
	for(i = c->x + 1; i < info.width - 1; i += pct)			\
           XMoveWindow(info.dpy, c->win, i, c->y);

#endif /* Graphical stuff */

#define LAYOUT		maximize	/* Default layout: maximize/floating */
#define TAGS		5		/* Number of tags used */
#define MRFACT		2		/* Incremental factor for move/resize */
#define BORDER_SIZE	3		/* Border size (in pixel) */
#define BORDER_COLSEL	"#333399"	/* Border color for the selected client */
#define BORDER_COLNORM	"#123456"	/* Border color for background clients */
#define CHESS_C4R	2		/* Clients for row in chess layout */

/* Convenient constants for common keys */
#define META		Mod4Mask
#define CTRL		ControlMask
#define SHIFT		ShiftMask

/*
 * Client's filters
*/
static const Filter filters[] = {
   /*
    * Name          Tag#        Floating
   */
   { "MPlayer",     NULL,       True   },
   { "Gimp",        NULL,       True   },
   { "xmessage",    NULL,       True   },
};

/*
 * Key bindings
*/
static const Key keys[] = {
   /*
    * Modifier      Key         Function      Argument
   */
   { META,          XK_f,       layout,       "maximize"          },
   { META,          XK_space,   layout,       "floating"          },
   { META,          XK_g,       layout,       "chess"             },
   
   { META,          XK_z,       tagview,      "0"                 },
   { META,          XK_x,       tagview,      "1"                 },
   { META,          XK_c,       tagview,      "2"                 },
   { META,          XK_v,       tagview,      "3"                 },
   { META,          XK_b,       tagview,      "4"                 },
   { META,          XK_Left,    tagview,      "prev"              },
   { META,          XK_Right,   tagview,      "next"              },
   
   { META|SHIFT,    XK_z,       client_totag,  "0"                },
   { META|SHIFT,    XK_x,       client_totag,  "1"                },
   { META|SHIFT,    XK_c,       client_totag,  "2"                },
   { META|SHIFT,    XK_v,       client_totag,  "3"                },
   { META|SHIFT,    XK_b,       client_totag,  "4"                },
   
   { META,          XK_Tab,     client_next,   NULL               },
   { META|SHIFT,    XK_Tab,     client_prev,   NULL               },

   { META,          XK_h,       client_move,   "4" /* Left */     },
   { META,          XK_l,       client_move,   "6" /* Right */    },
   { META,          XK_j,       client_move,   "2" /* Down */     },
   { META,          XK_k,       client_move,   "8" /* Up */       },
   { META|SHIFT,    XK_h,       client_resize, "4"                },
   { META|SHIFT,    XK_l,       client_resize, "6"                },
   { META|SHIFT,    XK_j,       client_resize, "2"                },
   { META|SHIFT,    XK_k,       client_resize, "8"                },
   { META|CTRL,     XK_h,       client_resize, "-4"               },
   { META|CTRL,     XK_l,       client_resize, "-6"               },
   { META|CTRL,     XK_j,       client_resize, "-2"               },
   { META|CTRL,     XK_k,       client_resize, "-8"               },

   { META,          XK_d,       client_kill,   NULL               },
   { META|SHIFT,    XK_q,       quit,          NULL               },

   { META,          XK_w,       spawn,         "firefox"          },
   { META,          XK_e,       spawn,         "thunar"           },
   { META,          XK_r,       spawn,         "dmenu_run"        },
   { META,          XK_t,       spawn,         "urxvt -rv"        },
   { META,          XK_a,       spawn,         "pavucontrol"      },
   { META,          XK_s,       spawn,         "slock"            },
   { META|SHIFT,    XK_s,       spawn,         "loginctl suspend" },

};

