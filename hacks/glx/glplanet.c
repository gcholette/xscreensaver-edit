/* -*- Mode: C; tab-width: 4 -*- */
/* glplanet --- 3D rotating planet, e.g., Earth.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Revision History:
 *
 * 16-Jan-02: jwz@jwz.org   gdk_pixbuf support.
 * 21-Mar-01: jwz@jwz.org   Broke sphere routine out into its own file.
 *
 * 9-Oct-98:  dek@cgl.ucsf.edu  Added stars.
 *
 * 8-Oct-98:  jwz@jwz.org   Made the 512x512x1 xearth image be built in.
 *                          Made it possible to load XPM or XBM files.
 *                          Made the planet bounce and roll around.
 *
 * 8-Oct-98: Released initial version of "glplanet"
 * (David Konerding, dek@cgl.ucsf.edu)
 *
 * BUGS:
 * -bounce is broken
 * 
 *   For even more spectacular results, grab the images from the "SSystem"
 *   package (http://www.msu.edu/user/kamelkev/) and use its JPEGs!
 */


/*-
 * due to a Bug/feature in VMS X11/Intrinsic.h has to be placed before xlock.
 * otherwise caddr_t is not defined correctly
 */

#include <X11/Intrinsic.h>

#ifdef STANDALONE
# define PROGCLASS						"Planet"
# define HACK_INIT						init_planet
# define HACK_DRAW						draw_planet
# define HACK_RESHAPE					reshape_planet
# define HACK_HANDLE_EVENT				planet_handle_event
# define EVENT_MASK						PointerMotionMask
# define planet_opts					xlockmore_opts
#define DEFAULTS	"*delay:			15000   \n"	\
					"*showFPS:			False   \n" \
                    "*rotate:           True    \n" \
                    "*roll:             True    \n" \
                    "*wander:           True    \n" \
					"*wireframe:		False	\n"	\
					"*light:			True	\n"	\
					"*texture:			True	\n" \
					"*stars:			True	\n" \
					"*image:			BUILTIN	\n" \
					"*imageForeground:	Green	\n" \
					"*imageBackground:	Blue	\n"

# include "xlockmore.h"				/* from the xscreensaver distribution */
#else  /* !STANDALONE */
# include "xlock.h"					/* from the xlockmore distribution */
#endif /* !STANDALONE */

#ifdef USE_GL /* whole file */

#include "sphere.h"

#ifdef HAVE_XMU
# ifndef VMS
#  include <X11/Xmu/Drawing.h>
#else  /* VMS */
#  include <Xmu/Drawing.h>
# endif /* VMS */
#endif


#include <GL/glu.h>

#define DEF_ROTATE  "True"
#define DEF_ROLL    "True"
#define DEF_WANDER  "True"
#define DEF_TEXTURE "True"
#define DEF_STARS   "True"
#define DEF_LIGHT   "True"
#define DEF_RESOLUTION "64"
#define DEF_IMAGE   "BUILTIN"

#undef countof
#define countof(x) (sizeof((x))/sizeof((*x)))

static int do_rotate;
static int do_roll;
static int do_wander;
static int do_texture;
static int do_stars;
static int do_light;
static char *which_image;
static int resolution;

static XrmOptionDescRec opts[] = {
  {"-rotate",  ".glplanet.rotate",  XrmoptionNoArg, (caddr_t) "true" },
  {"+rotate",  ".glplanet.rotate",  XrmoptionNoArg, (caddr_t) "false" },
  {"-roll",    ".glplanet.roll",    XrmoptionNoArg, (caddr_t) "true" },
  {"+roll",    ".glplanet.roll",    XrmoptionNoArg, (caddr_t) "false" },
  {"-wander",  ".glplanet.wander",  XrmoptionNoArg, (caddr_t) "true" },
  {"+wander",  ".glplanet.wander",  XrmoptionNoArg, (caddr_t) "false" },
  {"-texture", ".glplanet.texture", XrmoptionNoArg, (caddr_t) "true" },
  {"+texture", ".glplanet.texture", XrmoptionNoArg, (caddr_t) "false" },
  {"-stars",   ".glplanet.stars",   XrmoptionNoArg, (caddr_t) "true" },
  {"+stars",   ".glplanet.stars",   XrmoptionNoArg, (caddr_t) "false" },
  {"-light",   ".glplanet.light",   XrmoptionNoArg, (caddr_t) "true" },
  {"+light",   ".glplanet.light",   XrmoptionNoArg, (caddr_t) "false" },
  {"-image",   ".glplanet.image",  XrmoptionSepArg, (caddr_t) 0 },
  {"-resolution", ".glplanet.resolution", XrmoptionSepArg, (caddr_t) 0 },
};

static argtype vars[] = {
  {(caddr_t *) &do_rotate,   "rotate",  "Rotate",  DEF_ROTATE,  t_Bool},
  {(caddr_t *) &do_roll,     "roll",    "Roll",    DEF_ROLL,    t_Bool},
  {(caddr_t *) &do_wander,   "wander",  "Wander",  DEF_WANDER,  t_Bool},
  {(caddr_t *) &do_texture,  "texture", "Texture", DEF_TEXTURE, t_Bool},
  {(caddr_t *) &do_stars,  "stars", "Stars", DEF_STARS, t_Bool},
  {(caddr_t *) &do_light,    "light",   "Light",   DEF_LIGHT,   t_Bool},
  {(caddr_t *) &which_image, "image",   "Image",   DEF_IMAGE,   t_String},
  {(caddr_t *) &resolution,  "resolution","Resolution", DEF_RESOLUTION, t_Int},
};

ModeSpecOpt planet_opts = {countof(opts), opts, countof(vars), vars, NULL};

#ifdef USE_MODULES
ModStruct   planet_description =
{"planet", "init_planet", "draw_planet", "release_planet",
 "draw_planet", "init_planet", NULL, &planet_opts,
 1000, 1, 2, 1, 4, 1.0, "",
 "Animates texture mapped sphere (planet)", 0, NULL};
#endif

#include "../images/earth.xpm"
#include "xpm-ximage.h"
#include "rotator.h"
#include "gltrackball.h"


/*-
 * slices and stacks are used in the sphere parameterization routine.
 * more slices and stacks will increase the quality of the sphere,
 * at the expense of rendering speed
 */

#define NUM_STARS 1000

/* radius of the sphere- fairly arbitrary */
#define RADIUS 4

/* distance away from the sphere model */
#define DIST 40



/* structure for holding the planet data */
typedef struct {
  GLuint platelist;
  GLuint latlonglist;
  GLuint starlist;
  int screen_width, screen_height;
  GLXContext *glx_context;
  Window window;
  XColor fg, bg;
  GLfloat sunpos[4];
  double z;
  rotator *rot;
  trackball_state *trackball;
  Bool button_down_p;
} planetstruct;


static planetstruct *planets = NULL;


static inline void
normalize(GLfloat v[3])
{
  GLfloat d = (GLfloat) sqrt((double) (v[0] * v[0] +
                                       v[1] * v[1] +
                                       v[2] * v[2]));
  if (d != 0)
    {
      v[0] /= d;
      v[1] /= d;
      v[2] /= d;
	}
  else
    {
      v[0] = v[1] = v[2] = 0;
	}
}


/* Set up and enable texturing on our object */
static void
setup_xpm_texture (ModeInfo *mi, char **xpm_data)
{
  XImage *image = xpm_to_ximage (MI_DISPLAY (mi), MI_VISUAL (mi),
                                  MI_COLORMAP (mi), xpm_data);
  char buf[1024];
  clear_gl_error();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               image->width, image->height, 0,
               GL_RGBA,
               /* GL_UNSIGNED_BYTE, */
               GL_UNSIGNED_INT_8_8_8_8_REV,
               image->data);
  sprintf (buf, "builtin texture (%dx%d)", image->width, image->height);
  check_gl_error(buf);

  /* setup parameters for texturing */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}


static void
setup_file_texture (ModeInfo *mi, char *filename)
{
  Display *dpy = mi->dpy;
  Visual *visual = mi->xgwa.visual;
  char buf[1024];

  Colormap cmap = mi->xgwa.colormap;
  XImage *image = xpm_file_to_ximage (dpy, visual, cmap, filename);

  clear_gl_error();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               image->width, image->height, 0,
               GL_RGBA,
               /* GL_UNSIGNED_BYTE, */
               GL_UNSIGNED_INT_8_8_8_8_REV,
               image->data);
  sprintf (buf, "texture: %.100s (%dx%d)",
           filename, image->width, image->height);
  check_gl_error(buf);

  /* setup parameters for texturing */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}


static void
setup_texture(ModeInfo * mi)
{
/*  planetstruct *gp = &planets[MI_SCREEN(mi)];*/

  glEnable(GL_TEXTURE_2D);

  if (!which_image ||
	  !*which_image ||
	  !strcmp(which_image, "BUILTIN"))
	setup_xpm_texture (mi, earth_xpm);
  else
	setup_file_texture (mi, which_image);

  check_gl_error("texture initialization");

  /* Need to flip the texture top for bottom for some reason. */
  glMatrixMode (GL_TEXTURE);
  glScalef (1, -1, 1);
  glMatrixMode (GL_MODELVIEW);
}


void
init_stars (ModeInfo *mi)
{
  int i, j;
  GLfloat max_size = 3;
  GLfloat inc = 0.5;
  int steps = max_size / inc;
  int width  = MI_WIDTH(mi);
  int height = MI_HEIGHT(mi);

  planetstruct *gp = &planets[MI_SCREEN(mi)];
  Bool wire = MI_IS_WIREFRAME(mi);
  
  if (!wire)
    glEnable (GL_POINT_SMOOTH);

  gp->starlist = glGenLists(1);
  glNewList(gp->starlist, GL_COMPILE);

  glScalef (1.0/width, 1.0/height, 1);

  for (j = 1; j <= steps; j++)
    {
      glPointSize(inc * j);
      glBegin(GL_POINTS);
      for (i = 0 ; i < NUM_STARS / steps; i++)
        {
          glColor3f (0.6 + frand(0.3),
                     0.6 + frand(0.3),
                     0.6 + frand(0.3));
          glVertex2f ((GLfloat) (random() % width),
                      (GLfloat) (random() % height));
        }
      glEnd();
    }
  glEndList();

  check_gl_error("stars initialization");
}


void
draw_stars (ModeInfo * mi)
{
  int width  = MI_WIDTH(mi);
  int height = MI_HEIGHT(mi);

  planetstruct *gp = &planets[MI_SCREEN(mi)];
  
  /* Sadly, this causes a stall of the graphics pipeline (as would the
     equivalent calls to glGet*.)  But there's no way around this, short
     of having each caller set up the specific display matrix we need
     here, which would kind of defeat the purpose of centralizing this
     code in one file.
   */
  glPushAttrib(GL_TRANSFORM_BIT |  /* for matrix contents */
               GL_ENABLE_BIT |     /* for various glDisable calls */
               GL_CURRENT_BIT |    /* for glColor3f() */
               GL_LIST_BIT);       /* for glListBase() */
  {
    check_gl_error ("glPushAttrib");

    /* disable lighting and texturing when drawing stars!
       (glPopAttrib() restores these.)
     */
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    /* glPopAttrib() does not restore matrix changes, so we must
       push/pop the matrix stacks to be non-intrusive there.
     */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    {
      check_gl_error ("glPushMatrix");
      glLoadIdentity();

      /* Each matrix mode has its own stack, so we need to push/pop
         them separately. */
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      {
        check_gl_error ("glPushMatrix");
        glLoadIdentity();

        gluOrtho2D (0, width, 0, height);
        check_gl_error ("gluOrtho2D");

        /* Draw the stars */
        glScalef (width, height, 1);
        glCallList(gp->starlist);
        check_gl_error ("drawing stars");
      }
      glPopMatrix();
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

  }
  /* clean up after our state changes */
  glPopAttrib();
  check_gl_error ("glPopAttrib");
}



/* Set up lighting */
static void
init_sun (ModeInfo * mi)
{
  planetstruct *gp = &planets[MI_SCREEN(mi)];

  GLfloat lamb[4] = { 0.1, 0.1, 0.1, 1.0 };
  GLfloat ldif[4] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat spec[4] = { 1.0, 1.0, 1.0, 1.0 };

  GLfloat mamb[4] = { 0.5, 0.5, 0.5, 1.0 };
  GLfloat mdif[4] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mpec[4] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat shiny = .4;

  {
    double h =  0.1 + frand(0.8);   /* east-west position - screen-side. */
    double v = -0.3 + frand(0.6);   /* north-south position */

    if (h > 0.3 && h < 0.8)         /* avoid having the sun at the camera */
      h += (h > 0.5 ? 0.2 : -0.2);

    gp->sunpos[0] = cos(h * M_PI);
    gp->sunpos[1] = sin(h * M_PI);
    gp->sunpos[2] = sin(v * M_PI);
    gp->sunpos[3] =  0.00;
  }

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  glLightfv (GL_LIGHT0, GL_POSITION, gp->sunpos);
  glLightfv (GL_LIGHT0, GL_AMBIENT,  lamb);
  glLightfv (GL_LIGHT0, GL_DIFFUSE,  ldif);
  glLightfv (GL_LIGHT0, GL_SPECULAR, spec);

  glMaterialfv (GL_FRONT, GL_AMBIENT,  mamb);
  glMaterialfv (GL_FRONT, GL_DIFFUSE,  mdif);
  glMaterialfv (GL_FRONT, GL_SPECULAR, mpec);
  glMaterialf  (GL_FRONT, GL_SHININESS, shiny);


  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glShadeModel(GL_SMOOTH);

  check_gl_error("lighting");
}


#define RANDSIGN() ((random() & 1) ? 1 : -1)

void
reshape_planet (ModeInfo *mi, int width, int height)
{
  GLfloat h = (GLfloat) height / (GLfloat) width;

  glViewport(0, 0, (GLint) width, (GLint) height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -h, h, 5.0, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -DIST);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


Bool
planet_handle_event (ModeInfo *mi, XEvent *event)
{
  planetstruct *gp = &planets[MI_SCREEN(mi)];

  if (event->xany.type == ButtonPress &&
      event->xbutton.button & Button1)
    {
      gp->button_down_p = True;
      gltrackball_start (gp->trackball,
                         event->xbutton.x, event->xbutton.y,
                         MI_WIDTH (mi), MI_HEIGHT (mi));
      return True;
    }
  else if (event->xany.type == ButtonRelease &&
           event->xbutton.button & Button1)
    {
      gp->button_down_p = False;
      return True;
    }
  else if (event->xany.type == MotionNotify &&
           gp->button_down_p)
    {
      gltrackball_track (gp->trackball,
                         event->xmotion.x, event->xmotion.y,
                         MI_WIDTH (mi), MI_HEIGHT (mi));
      return True;
    }

  return False;
}


void
init_planet (ModeInfo * mi)
{
  planetstruct *gp;
  int screen = MI_SCREEN(mi);
  Bool wire = MI_IS_WIREFRAME(mi);

  if (planets == NULL) {
	if ((planets = (planetstruct *) calloc(MI_NUM_SCREENS(mi),
										  sizeof (planetstruct))) == NULL)
	  return;
  }
  gp = &planets[screen];

  if ((gp->glx_context = init_GL(mi)) != NULL) {
	reshape_planet(mi, MI_WIDTH(mi), MI_HEIGHT(mi));
  }

  {
	char *f = get_string_resource("imageForeground", "Foreground");
	char *b = get_string_resource("imageBackground", "Background");
	char *s;
	if (!f) f = strdup("white");
	if (!b) b = strdup("black");
	
	for (s = f + strlen(f)-1; s > f; s--)
	  if (*s == ' ' || *s == '\t')
		*s = 0;
	for (s = b + strlen(b)-1; s > b; s--)
	  if (*s == ' ' || *s == '\t')
		*s = 0;

    if (!XParseColor(mi->dpy, mi->xgwa.colormap, f, &gp->fg))
      {
		fprintf(stderr, "%s: unparsable color: \"%s\"\n", progname, f);
		exit(1);
      }
    if (!XParseColor(mi->dpy, mi->xgwa.colormap, b, &gp->bg))
      {
		fprintf(stderr, "%s: unparsable color: \"%s\"\n", progname, f);
		exit(1);
      }

	free (f);
	free (b);
  }

  {
    double spin_speed   = 1.0;
    double wander_speed = 0.05;
    gp->rot = make_rotator (do_roll ? spin_speed : 0,
                            do_roll ? spin_speed : 0,
                            0, 1,
                            do_wander ? wander_speed : 0,
                            True);
    gp->z = frand (1.0);
    gp->trackball = gltrackball_init ();
  }

  if (wire)
    {
      do_texture = False;
      do_light = False;
      glEnable (GL_LINE_SMOOTH);
    }

  if (do_texture)
    setup_texture (mi);

  if (do_light)
	init_sun (mi);

  if (do_stars)
    init_stars (mi);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK); 

  /* construct the polygons of the planet
   */
  gp->platelist = glGenLists(1);
  glNewList (gp->platelist, GL_COMPILE);
  glColor3f (1,1,1);
  glPushMatrix ();
  glScalef (RADIUS, RADIUS, RADIUS);
  glRotatef (90, 1, 0, 0);
  unit_sphere (resolution, resolution, wire);
  mi->polygon_count += resolution*resolution;
  glPopMatrix ();
  glEndList();

  /* construct the polygons of the latitude/longitude/axis lines.
   */
  gp->latlonglist = glGenLists(1);
  glNewList (gp->latlonglist, GL_COMPILE);
  glPushMatrix ();
  if (do_texture) glDisable (GL_TEXTURE_2D);
  if (do_light)   glDisable (GL_LIGHTING);
  glColor3f (0.1, 0.3, 0.1);
  glScalef (RADIUS, RADIUS, RADIUS);
  glScalef (1.01, 1.01, 1.01);
  glRotatef (90, 1, 0, 0);
  unit_sphere (12, 24, 1);
  glBegin(GL_LINES);
  glVertex3f(0, -2, 0);
  glVertex3f(0,  2, 0);
  glEnd();
  if (do_light)   glEnable(GL_LIGHTING);
  if (do_texture) glEnable(GL_TEXTURE_2D);
  glPopMatrix ();
  glEndList();
}

void
draw_planet (ModeInfo * mi)
{
  planetstruct *gp = &planets[MI_SCREEN(mi)];
  Display    *display = MI_DISPLAY(mi);
  Window      window = MI_WINDOW(mi);
  double x, y, z;

  if (!gp->glx_context)
	return;

  glDrawBuffer(GL_BACK);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glXMakeCurrent (display, window, *(gp->glx_context));

  if (do_stars)
    draw_stars (mi);

  glPushMatrix();

  get_position (gp->rot, &x, &y, &z, !gp->button_down_p);
  glTranslatef((x - 0.5) * 15,
               (y - 0.5) * 15,
               (z - 0.5) * 8);

  gltrackball_rotate (gp->trackball);

  glRotatef (90,1,0,0);

  if (do_roll)
    {
      get_rotation (gp->rot, &x, &y, 0, !gp->button_down_p);
      glRotatef (x * 360, 1.0, 0.0, 0.0);
      glRotatef (y * 360, 0.0, 1.0, 0.0);
    }

  glLightfv (GL_LIGHT0, GL_POSITION, gp->sunpos);

  glRotatef (gp->z * 360, 0.0, 0.0, 1.0);
  if (do_rotate && !gp->button_down_p)
    {
      gp->z -= 0.01;     /* the sun sets in the west */
      if (gp->z < 0) gp->z += 1;
    }

  glCallList (gp->platelist);
  if (gp->button_down_p)
    glCallList (gp->latlonglist);
  glPopMatrix();

  if (mi->fps_p) do_fps (mi);
  glFinish();
  glXSwapBuffers(display, window);
}


void
release_planet (ModeInfo * mi)
{
  if (planets != NULL) {
	int screen;

	for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
	  planetstruct *gp = &planets[screen];

	  if (gp->glx_context) {
		/* Display lists MUST be freed while their glXContext is current. */
		glXMakeCurrent(MI_DISPLAY(mi), gp->window, *(gp->glx_context));

		if (glIsList(gp->platelist))
		  glDeleteLists(gp->platelist, 1);
		if (glIsList(gp->starlist))
		  glDeleteLists(gp->starlist, 1);
	  }
	}
	(void) free((void *) planets);
	planets = NULL;
  }
  FreeAllGL(mi);
}


#endif
