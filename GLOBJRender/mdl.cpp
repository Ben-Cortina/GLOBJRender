/* Roughly follows the formula set by Nate Robin's mdl.c.
 * This is mainly due to initially using mdl.h. But, in the end, bar
 * some of the include's and the four glMaterial lines in mdlDraw, every
 * line is my own.
 * ~Ben Cortina
 */

#include <stdlib.h> 
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "mdl.h"

using namespace std;

// Simply adds a pause before the exit to allow for the user to read the error
// regardless of if the console stays open or closes on exit.
void exitError(int error)
{
    cout << "Press <ENTER> to continue . . .";

    cin.clear();

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
	exit(error);
}

//finds and returns the working directory path from the file path
string DirName(string path)
{
	char slash;
	#if defined(_WIN32) || defined (_WIN64)
	  slash = '\\';
    #else
      slash = '/';
	#endif

	return path.substr(0,path.find_last_of(slash)+1);

}

void CheckFile(string &name)
{
	char good, bad;
	#if defined(_WIN32) || defined (_WIN64)
	  good = '\\';
	  bad = '/';
    #else
      good = '/';
	  bad = '\\';
	#endif
	while (name.find(bad) != string::npos)
		name[name.find(bad)] = good;
}

//Creates a new MDLgroup set its default values
MDLgroup* mdlNewGroup(string name)
{
	MDLgroup* group = new MDLgroup();
        group->name = name;
		group->numsections = 0;
        group->numtrsections = 0;

	return group;
}

//Creates a new MDLsection set its default values
MDLsection* mdlNewSection(GLuint material)
{
	MDLsection* section = new MDLsection();
        section->material = material; //default material
        section->numtriangles = 0;
	return section;
}

//Creates a new MDLmaterial set its default values

MDLmaterial* mdlNewMaterial(string name)
{
	MDLmaterial* material = new MDLmaterial();
	//default material values  
	  material->name = name;
      material->shininess = 65.0;
      material->diffuse[0] = 0.8;
      material->diffuse[1] = 0.8;
      material->diffuse[2] = 0.8;
      material->diffuse[3] = 1.0;
      material->ambient[0] = 0.2;
      material->ambient[1] = 0.2;
      material->ambient[2] = 0.2;
      material->ambient[3] = 1.0;
      material->specular[0] = 0.0;
      material->specular[1] = 0.0;
      material->specular[2] = 0.0;
      material->specular[3] = 1.0;
	  material->tindex = -1;
	return material;

}

//generates the opengl textures
GLvoid mdlGenTextures(MDLmodel* model)
{
	model->gltexs = new GLuint[model->numtextures]; 
	//create gltexture
	glGenTextures(model->numtextures, model->gltexs);
	for (int i = 0; i < model->numtextures; i++)
	{
        glBindTexture(GL_TEXTURE_2D, model->gltexs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, model->textures[i]->dimensions[0],
                    model->textures[i]->dimensions[1], 0, GL_RGB, GL_UNSIGNED_BYTE,
                    model->textures[i]->image);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} 
}

//Loads Model components, could be adapted for VBO, display lists, etc.
GLvoid
mdlLoadModel(MDLmodel* model)
{
	mdlGenTextures(model);
}

// Reads the .ppm file and stores the texture in the 
// passed MDLmodel 
GLvoid mdlReadPPM(MDLmodel* model, string name) 
{
    MDLtexture *texture = new MDLtexture();
	FILE *fd;

	int n, m;
	int k, nm;
	char c;
	char b[255];

	float s;
	char red, green, blue;
	int x, y;

	CheckFile(name); // fix any path errors in the passed filename
	//get file path
	string dir = DirName(model->pathname);
	string filename = dir+name;

	//Open file
	fd = fopen(filename.c_str(), "rb");
	if(!fd)
	{
		fprintf(stderr, "Can't open .ppm file \"%s\".\n", filename);
        exitError(1);
	}
	
    
	fscanf(fd, "%s", b); //Check for magic number
	if((b[0] != 'P') || (b[1] != '6'))
	{
		printf("%s is not a supported PPM file type!\n", b);
		exitError(1);
	}
    
	fgets(b, sizeof(b), fd); //skp through th rest of the first line
    
	//run through comments
	fscanf(fd, "%c", &c);
	while(c == '#')
	{
		//read rest of line
		fgets(b, sizeof(b), fd);
		//get next lines first char
		fscanf(fd, "%c", &c);
	}
    
	ungetc(c,fd);
    
	fscanf(fd, "%d", &n);
	fscanf(fd, "%d", &m);
	fscanf(fd, "%d", &k);
    fgets(b, sizeof(b), fd);
	nm = n*m; 
    
	texture->image = (GLubyte *)malloc(3*sizeof(GLubyte)*nm); // allocates room for the number
													// of pixels
    
	s = 255.0/k; // scales the values down to 255

	//get and store each RGB
	if(fread ( texture->image, sizeof(char)*3, nm, fd ) != nm)
	{
		printf("PPM file %s could not be read\n", filename);
		exitError(1);
	}
	if (s != 1)
		for(int i = 0; i < nm*3; i++)
			texture->image[i] *= s;

	//Close file
	fclose(fd);

	//store dimensions
	texture->dimensions[0] = n;
	texture->dimensions[1] = m;

	//store name
	texture->name = name;

	//store texture
	model->textures.push_back(texture);
	model->numtextures++;
}

// Reads the .mtl file and stores the information in the 
// passed MDLmodel 

GLvoid mdlReadMTL(MDLmodel* model, string name)
{
    ifstream file;
    string dir;
    string filename;
      
    dir = DirName(model->pathname);
	filename = dir+name;
    
    file.open(filename.c_str(), ios::in);		
    if (!file) 
    {
        fprintf(stderr, "Can't open .mtl file \"%s\".\n", filename.c_str());
        exitError(1);
    }

	model->mtllibname = filename;

	//current material
    MDLmaterial* material = mdlNewMaterial("");
	model->materials.push_back(material);
	model->nummaterials++;


	string line;
	string front;

	//==============parse file

	while(getline(file, line))
    {
        if(line == "" || line[0] == '#')// ignore comments and blank lines
            continue;

        istringstream lineStream(line);
        lineStream >> front;

        //new mat
        if(front == "newmtl")
        {
			string name;
			bool found = false;
	        lineStream >> name;
			for(int i=0; i < model->nummaterials; i++)
            {
				material = model->materials[i];
                if(material->name == name)
				{
					found = true;
					break;
				}
            }
		    if (!found) 
            {
		          material = mdlNewMaterial(name);
	              model->materials.push_back(material);
	              model->nummaterials++;
			}
		}
		// diffuse
		if(front == "Kd")
        {
	        string name;
	        sscanf(line.c_str(), "%*s %f %f %f",  
				&material->diffuse[0],
				&material->diffuse[1],
			    &material->diffuse[2]);
		}
		//specular
		if(front == "Ks")
        {
	        sscanf(line.c_str(), "%*s %f %f %f",  
				&material->specular[0],
				&material->specular[1],
			    &material->specular[2]);
		}
		//specular weight
		if(front == "Ns")
        {
			float s;
	        sscanf(line.c_str(), "%*s %f", &s);
			//.mtl is 0, 1000, OpenGL is 0, 128
			material->shininess = s * (128.0/1000.0);
		}
		//ambient
		if(front == "Ka")
        {
	        sscanf(line.c_str(), "%*s %f %f %f",  
				&material->ambient[0],
				&material->ambient[1],
			    &material->ambient[2]);
		}
		if(front == "d" || front == "Tr" )
        {
	        sscanf(line.c_str(), "%*s %f",  
				&material->diffuse[3]);
		}
		if (front == "map_Kd") 
        {
			string name, two;
			lineStream >> name;
			//check to see if this texture has been read
			bool found = false;
			for(int i=0; i < model->numtextures; i++)
            {
                if(model->textures[i]->name == name)
				{
					material->tindex = i;
					found = true;
					break;
				}
            }
			if (!found) 
            {
				material->tindex = model->numtextures;
				mdlReadPPM(model, name);
			}
		}
	}
	file.close();
}

//Reads the .obj file and stores the information in a 
//MDLmodel which is then returned

MDLmodel* mdlReadOBJ(string filename) 
{
    MDLmodel* model;
    ifstream file;

    file.open(filename.c_str(), ios::in);
    if (!file) 
    {
        fprintf(stderr, "Can't open .obj file \"%s\".\n", filename.c_str());
        exitError(1);
    }

	// create a model and set initial values
    model = new MDLmodel();
      model->pathname = filename;
	  model->numgroups = model->nummaterials = model->numnormals =
		  model->numtexcoords = model->numtriangles = model->numvertices = 
		  model->numtextures = 0;
	  model->position[0] = 0;
      model->position[1] = 0;
      model->position[2] = 0;
	  model->max[0] = -10000;
      model->max[1] = -10000;
      model->max[2] = -10000;
	  model->min[0] = 10000;
      model->min[1] = 10000;
      model->min[2] = 10000;

	//===========================Parse File


	// shortcuts to save me time/space
	MDLgroup* group;
	MDLsection* section;        

	group = mdlNewGroup("");
	section = mdlNewSection(0);
	group->sections.push_back(section);
    group->trsections.push_back(section);
	group->numsections++;
    group->numtrsections++;
	model->numgroups++;
	model->groups.push_back(group);

	string line;
	string front;
	
	//find and load materials
    while(getline(file, line)) 
    {
		istringstream lineStream(line);
        lineStream >> front;

	    if (front == "mtllib") 
        {
			string name, two;
			lineStream >> name;
			mdlReadMTL(model, name);
			break;
		}
	}

	//reset get position
	file.seekg(0);

	//parse the rest
    while(getline(file, line)) 
    {
        if(line == "" || line[0] == '#')// ignore comments and blank lines
            continue;

        istringstream lineStream(line);
        lineStream >> front;
	
        //vertex
        if(front == "v") 
        {
	        float x,y,z;
	        sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
	          model->vertices.push_back(x);
	          model->vertices.push_back(y);
	          model->vertices.push_back(z);
	        model->numvertices++;

			//bounding box stuff
			if (x > model->max[0])
				model->max[0] = x;
			if (y > model->max[1])
				model->max[1] = y;
			if (z > model->max[2])
				model->max[2] = z;

			if (x < model->min[0])
				model->min[0] = x;
			if (y < model->min[1])
				model->min[1] = y;
			if (z < model->min[2])
				model->min[2] = z;
		}
        // Normal
        if(front == "vn") 
        {
	        float x,y,z;
	        sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
	          model->normals.push_back(x);
	          model->normals.push_back(y);
	          model->normals.push_back(z);
			model->numnormals++;
		}
        //texture
        if(front == "vt") 
        {
	        float u, v;
	        sscanf(line.c_str(), "%*s %f %f", &u, &v);
	          model->texcoords.push_back(u);
	          model->texcoords.push_back(v);
	        model->numtexcoords++;
		}

	    //face (Im stiking with nate's stuctures because, while a bit overcomlicated, they make the most sense
        if(front == "f") 
        {
			string face;
            MDLtriangle last;
			MDLtriangle triangle;
            int indexCount = 0;
	        int v, t, n;
			int format = 0;

			if (line.find("//") != -1)
				format = 1;
			else if (line.find("/") != -1) 
            {
				if ( sscanf(line.c_str(), "%*s %d/%d/%d", &v, &t, &n) == 3)
				    format = 2;
				else 
				    format = 3;
			}
			else
				format = 4;
			//keep going until all the indicies have been read
			while (!lineStream.eof()) 
            {
				lineStream >> face;
				v = t = n = 1;
	            if(format == 1)
					sscanf(face.c_str(), "%d//%d", &v, &n);
			    if(format == 2)
					sscanf(face.c_str(), "%d/%d/%d", &v, &t, &n);
				if(format == 3)
					sscanf(face.c_str(), "%d/%d", &v, &t);
				if(format == 4)
					sscanf(face.c_str(), "%d", &v);
                
                if (indexCount < 3)
                {
	                triangle.vindices[indexCount] = v-1;
				    triangle.tindices[indexCount] = t-1;
				    triangle.nindices[indexCount] = n-1;
				    indexCount++;
                }
                else 
                {
                    triangle.vindices[0] = last.vindices[0];
				    triangle.tindices[0] = last.tindices[0];
				    triangle.nindices[0] = last.nindices[0];
                    triangle.vindices[1] = last.vindices[2];
				    triangle.tindices[1] = last.tindices[2];
				    triangle.nindices[1] = last.nindices[2];
                    triangle.vindices[2] = v-1;
				    triangle.tindices[2] = t-1;
				    triangle.nindices[2] = n-1;
                }
			
                if (indexCount >= 3) 
                {
			        last = triangle;
                    section->triangles.push_back(triangle);
			        section->numtriangles++;
			        model->numtriangles++;
                }
            }
		}

		//group
		if (front == "g") 
        {
			string name;
			if (line.length() > 2 )
				name = line.substr(2);
			//check if group exists
			bool found = false;
			for( int i=0; i < model->numgroups; i++ ) 
            {
				group = model->groups[i];

				if (group->name == name)
				{
					found = true;
					break;
				}
			}
			if (!found) 
            {
			    group = mdlNewGroup(name);
			    model->numgroups++;
			    model->groups.push_back(group);
			}
		}

		//material
		if (front == "usemtl") 
        {
			int matIndex = 0;
			string mat;
			lineStream >> mat;
			for( int i=0; i < model->nummaterials; i++ )
				if (model->materials[i]->name == mat)
					matIndex = i;
			section = mdlNewSection(matIndex);
            // this will segregate the transparent and opaque materials to allow
            // for accurate transpoarency drawing
            if(model->materials[matIndex]->diffuse[3] != 1)
            {
                group->trsections.push_back(section);
			    group->numtrsections++;
            } else
            {
			    group->sections.push_back(section);
			    group->numsections++;
            }
		}
    }
	file.close();

    if(model->texcoords.empty())
        model->texcoords.push_back(0);

	return model;
}

//draws the model
GLvoid mdlDraw(MDLmodel* model, GLuint mode, int groupDisp) 
{
	if (mode & MDL_COLOR)
        glEnable(GL_COLOR_MATERIAL);
    else if (mode & MDL_MATERIAL)
        glDisable(GL_COLOR_MATERIAL);
    
	MDLmaterial* material;
	MDLtriangle triangle;
	MDLsection* section;
	MDLgroup* group;

	if (mode & MDL_POINTS) 
    {
		glBegin(GL_POINTS);
		glColor3f(1.0f,1.0f,1.0f);
		  for (int i = 0; i < model->numvertices; i++)
			  glVertex3fv(&model->vertices[3 * i]);
		glEnd();
	}
	if (mode & MDL_WIREFRAME) 
    {
		glBegin(GL_LINE_STRIP);
		glColor3f(1.0f, 1.0f, 1.0f);
		  for (int i = 0;  i < model->numvertices; i++)
			  glVertex3fv(&model->vertices[3 * i]);
		glEnd();
	}

	//draw all the triangles in each group, applying the 
	//appropriate material properties
    //draw opaque sections
    for(int i = 0; i < model->numgroups; i++) 
    {

		if (groupDisp != i && groupDisp!= -1)
			continue;

		group = model->groups[i];

		for(int j = 0; j < group->numsections; j++) 
        {
			section = group->sections[j];
			material = model->materials[section->material];  
			if (material->tindex != -1 && (mode & MDL_TEXTURE))
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, model->gltexs[material->tindex]);
			}
			else
				glDisable(GL_TEXTURE_2D);

		 //bitwise and (if the bit corrosponging to the setting is 1, then apply setting)
			if (mode & MDL_MATERIAL) 
            {            
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material->ambient);
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->specular);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->shininess);
			}

			//render each triangle with the color of the diffuse property
			if (mode & MDL_COLOR)
			    glColor4fv(material->diffuse);
			
			for (int k = 0; k < section->numtriangles; k++) 
            {
                triangle = section->triangles[k];

				glBegin(GL_TRIANGLES);
				//Set each vertex and its normal
                
                //Vertex 1
				  //Set the texture coordinates for each vertex
				  glTexCoord2fv(&model->texcoords[2 * triangle.tindices[0]]);

                  //Set the normal for each vertex
                  glNormal3fv(&model->normals[3 * triangle.nindices[0]]);

                  //define the location
                  glVertex3fv(&model->vertices[3 * triangle.vindices[0]]);
                
                //Vertex 2
				  //Set the texture coordinates for each vertex
				  glTexCoord2fv(&model->texcoords[2 * triangle.tindices[1]]);

                  //Set the normal for each vertex
                  glNormal3fv(&model->normals[3 * triangle.nindices[1]]);
                
                  //define the location
                  glVertex3fv(&model->vertices[3 * triangle.vindices[1]]);
                
                
                //Vertex 3
				  //Set the texture coordinates for each vertex
				  glTexCoord2fv(&model->texcoords[2 * triangle.tindices[2]]);

                  //Set the normal for each vertex
                  glNormal3fv(&model->normals[3 * triangle.nindices[2]]);
                
                  //define the location
                  glVertex3fv(&model->vertices[3 * triangle.vindices[2]]);
				glEnd();
			}
		}
    }
    //draw the transparent sections
    for(int i = 0; i < model->numgroups; i++) 
    {

		if (groupDisp != i && groupDisp!= -1)
			continue;

		group = model->groups[i];

		for(int j = 0; j < group->numtrsections; j++) 
        {
			section = group->trsections[j];
			material = model->materials[section->material];  
			if (material->tindex != -1 && (mode & MDL_TEXTURE))
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, model->gltexs[material->tindex]);
			}
			else
				glDisable(GL_TEXTURE_2D);

		 //bitwise and (if the bit corrosponging to the setting is 1, then apply setting)
			if (mode & MDL_MATERIAL) 
            {            
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material->ambient);
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->specular);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->shininess);
			}

			//render each triangle with the color of the diffuse property
			if (mode & MDL_COLOR)
			    glColor4fv(material->diffuse);
			
			for (int k = 0; k < section->numtriangles; k++) 
            {
                triangle = section->triangles[k];

				glBegin(GL_TRIANGLES);
				//Set each vertex and its normal
                
                //Vertex 1
				  //Set the texture coordinates for each vertex
				  glTexCoord2fv(&model->texcoords[2 * triangle.tindices[0]]);

                  //Set the normal for each vertex
                  glNormal3fv(&model->normals[3 * triangle.nindices[0]]);

                  //define the location
                  glVertex3fv(&model->vertices[3 * triangle.vindices[0]]);
                
                //Vertex 2
				  //Set the texture coordinates for each vertex
				  glTexCoord2fv(&model->texcoords[2 * triangle.tindices[1]]);

                  //Set the normal for each vertex
                  glNormal3fv(&model->normals[3 * triangle.nindices[1]]);
                
                  //define the location
                  glVertex3fv(&model->vertices[3 * triangle.vindices[1]]);
                
                
                //Vertex 3
				  //Set the texture coordinates for each vertex
				  glTexCoord2fv(&model->texcoords[2 * triangle.tindices[2]]);

                  //Set the normal for each vertex
                  glNormal3fv(&model->normals[3 * triangle.nindices[2]]);
                
                  //define the location
                  glVertex3fv(&model->vertices[3 * triangle.vindices[2]]);
				glEnd();
			}
		}
    }
}

//print out model stats
GLvoid 
mdlPrintStats(MDLmodel* model)
{
	printf("\n--------------------------------------------------------\n");
	printf("Model loaded from %s holds:\n",  model->pathname.c_str());
	printf("%d verticies\n", model->numvertices);
	printf("%d normals\n", model->numnormals);
	printf("%d texture coordinates\n", model->numtexcoords);
	printf("%d triangles\n", model->numtriangles);
	printf("%d groups\n", model->numgroups);
	printf("%d textures\n", model->numtextures);
	printf("%d materials loaded from %s\n", model->nummaterials, model->mtllibname.c_str());
	printf("\n--------------------------------------------------------\n");
}

