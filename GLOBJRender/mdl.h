


#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <vector>
#include <string>


#ifndef M_PI
#define M_PI 3.14159265f
#endif

/* x << y is x * 2^y.*/

#define MDL_NONE      (0)            /* 0 render with only vertices */
#define MDL_COLOR     (1 << 0)       /* 1 render with colors */
#define MDL_MATERIAL  (1 << 1)       /* 10 render with materials */
#define MDL_POINTS    (1 << 2)       /* 100 render points */
#define MDL_WIREFRAME (1 << 3)       /* 1000 render wireframe */
#define MDL_TEXTURE   (1 << 4)       /* 10000 render textures */

using namespace std;

/* MDLtriangle: Structure that defines a triangle in a model.
 */
typedef struct _MDLtexture 
{
  string name;			   /* name of the texture*/
  GLubyte* image;          /* the bitmap */
  GLuint dimensions[2];    /* number of pixels vertically and horizontally */
} MDLtexture;

/* MDLmaterial: Structure that defines a material in a model. 
 */
typedef struct _MDLmaterial
{
  string   name;                /* name of material */
  GLfloat diffuse[4];           /* diffuse component */
  GLfloat ambient[4];           /* ambient component */
  GLfloat specular[4];          /* specular component */
  GLfloat emmissive[4];         /* emmissive component */
  GLfloat shininess;            /* specular exponent */
  GLint tindex;                 /* texture used for this material */
} MDLmaterial;


/* MDLtriangle: Structure that defines a triangle in a model.
 */
typedef struct _MDLtriangle 
{
  GLuint vindices[3];      /* array of triangle vertex indices */
  GLuint nindices[3];      /* array of triangle normal indices */
  GLuint tindices[3];      /* array of triangle texcoord indices */
} MDLtriangle;

/* MDLgroup: Structure that defines a section of a group
 * A section is a bunch of triangles in a group that use the same material
 */
typedef struct _MDLsection 
{
  GLuint            numtriangles;   /* number of triangles in this section */
  vector<MDLtriangle>    triangles; /* array of triangles */
  GLuint            material;       /* index to material for section */
} MDLsection;

/* MDLgroup: Structure that defines a group in a model.
 */
typedef struct _MDLgroup 
{
  string            name;           /* name of this group */
  GLuint			numsections;    /* number of section in this group */
  vector<MDLsection*>    sections;  /* array of sections */
  GLuint			numtrsections;  /* number of transparent sections in this group */
  vector<MDLsection*>    trsections;/* array of transparent sections */
} MDLgroup;

/* MDLmodel: Structure that defines a model.
 */
typedef struct _MDLmodel 
{

  string    pathname;               /* path to this model */
  string    mtllibname;             /* name of the material library */

  GLuint   numvertices;             /* number of vertices in model */
  vector<GLfloat> vertices;         /* array of vertices  */

  GLuint   numnormals;              /* number of normals in model */
  vector<GLfloat> normals;          /* array of normals */

  GLuint   numtexcoords;            /* number of texcoords in model */
  vector<GLfloat> texcoords;        /* array of texture coordinates */

  GLuint       nummaterials;        /* number of materials in model */
  vector<MDLmaterial*> materials;   /* array of materials */

  GLuint       numgroups;           /* number of groups in model */
  vector<MDLgroup*>    groups;      /* array of groups */

  GLuint       numtextures;         /* number of textures in model */
  vector<MDLtexture*>    textures;  /* array of textures */
  GLuint*      gltexs;               /* array of GLgen textures */

  GLuint       numtriangles;        /* number of tiangles in the model */

  GLfloat position[3];              /* position of the model */

  GLfloat max[3];				    /* max x,y,z of the bounding box */
  GLfloat min[3];				    /* max x,y,z of the bounding box */

} MDLmodel;


/* mdlReadOBJ: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * mdlDelete().
 *
 * filename - name of the file containing the Wavefront .OBJ format data.  
 */
MDLmodel* mdlReadOBJ(string filename);

/* mdlPrintStats: Prints the number of verticies, normals, texture coordinates,
 * groups and triangles of the passed model.
 * 
 *
 * model - the model to be printed about.
 */
GLvoid mdlPrintStats(MDLmodel* model);


/* mdlDraw: Renders the model to the current OpenGL context using the
 * mode specified.
 *
 * model    - initialized MDLmodel structure
 * mode     - a bitwise OR of values describing what is to be rendered.
 *            MDL_MATERIAL -  Applies the specified material property to each group
 *            MDL_COLOR -  Applies a flat color to each group, the color is determined 
 *                         by the diffuse property of the group
 * groupDisp- determines which groups should be shown
 */
GLvoid mdlDraw(MDLmodel* model, GLuint mode, int groupDisp);

/* mdlLoadModel: Puts model compnents back in video memory
 *
 * model    - model  to be loaded
 */
GLvoid mdlLoadModel(MDLmodel* model);


// Simple function to implement a pause before exiting
GLvoid exitError(int error);
