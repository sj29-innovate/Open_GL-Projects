#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <set>
#include <map>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <bits/stdc++.h>
#include <ao/ao.h>
#define BUF_SIZE 10
using namespace std;
void playsound(int rates)
{
ao_device *device;
	ao_sample_format format;
	int default_driver;
	char *buffer;
	int buf_size;
	int sample;
	float freq = 440.0;
	int i;

	/* -- Initialize -- */
	ao_initialize();
	/* -- Setup for default driver -- */
	default_driver = ao_default_driver_id();
    memset(&format, 0, sizeof(format));
	format.bits = 16;
	format.channels = 2;
	format.rate = rates;
	format.byte_format = AO_FMT_LITTLE;

	/* -- Open driver -- */
	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		fprintf(stderr, "Error opening device.\n");
	}

	/* -- Play some stuff -- */
	buf_size = format.bits/8 * format.channels * format.rate;
	buffer = (char *)calloc(buf_size,sizeof(char));
	for (i = 0; i < format.rate; i++) {
		sample = (int)(0.75 * 32768.0 *
			sin(2 * M_PI * freq * ((float) i/format.rate)));
		/* Put the same stuff in left and right channel */
		buffer[4*i] = buffer[4*i+2] = sample & 0xff;
		buffer[4*i+1] = buffer[4*i+3] = (sample >> 8) & 0xff;
	}
	ao_play(device, buffer, buf_size/10);
	/* -- Close and shutdown -- */
	ao_close(device);
	ao_shutdown();
	free(buffer);
}
double brickspeed=-0.005;
struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;
struct bucketnode{
	struct VAO* bucket;
	glm::mat4 tranlastionvector;
	glm::mat4 rotationvector;
};
struct brickdata{
	struct VAO* brick;
	glm::mat4 tranlastionvector;
	int type;
	int index;
};
int laseractivity=1,totaltime=0;
map<int,struct brickdata> bricks;
int totalbricks=0;
int redscore=0;
int bluescore=0;
int gameover=0;
struct canondata{
	struct VAO* canon_object;
	glm::mat4 tranlastionvector;
	glm::mat4 rotationvector;
	glm::mat4 canontransform;
	double canon_rotation;
};
struct canondata canon;
glm::mat4 loadingtranslationvector=glm::mat4(1.0f);
struct canondata canon1;
struct canondata canon2;
struct canondata canon3;
struct mirrordata{
	struct VAO* mirror_object;
	glm::mat4 tranlastionvector;
	glm::mat4 rotationvector;
	double mirror_rotation;
	double a,b,c;
};
struct mirrordata mirrors[3];
struct laserdata{
	struct VAO* laser_object;
	glm::mat4 tranlastionvector;
	glm::mat4 rotationvector;
	glm::mat4 lasertransform;
	int direction;
	double laser_rotation;
};
vector<struct laserdata> lasers;
struct bucketnode buckets[2];
struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Executed when a regular key is pressed */
void draw();
struct VAO* createlaser();
bool checkoverlap(double a1x,double a1y,double a2x,double a2y,double b1x,double b1y,double b2x,double b2y,int index)
{
	double x1,y1,x2,y2,x3,y3,x4,y4,angle;
	x1=a1x;y1=a1y;x3=a2x;y3=a2y;
	angle=((lasers[index].laser_rotation)*M_PI)/(180.0f);
	x2=x1+((0.3f)*(cos(angle)));
	y2=y1+((0.3f)*(sin(angle)));
	x4=(x1+x3-x2);y4=(y1+y3-y2);
	double check1,check2;
	check1=(y1-b1y)*(y1-b2y);
	check2=(x1-b1x)*(x1-b2x);
	if((check1<=0.05 && check1>=-0.05) && (check2<=0.05 && check2>=-0.05))return true;
	check1=(y2-b1y)*(y2-b2y);
	check2=(x2-b1x)*(x2-b2x);
	if((check1<=0.05 && check1>=-0.05) && (check2<=0.05 && check2>=-0.05))return true;
	check1=(y3-b1y)*(y3-b2y);
	check2=(x3-b1x)*(x3-b2x);
	if((check1<=0.05 && check1>=-0.05) && (check2<=0.05 && check2>=-0.05))return true;
	check1=(y4-b1y)*(y4-b2y);
	check2=(x4-b1x)*(x4-b2x);
	if((check1<=0.05 && check1>=-0.05) && (check2<=0.05 && check2>=-0.05))return true;
	return false;
}
int checklaserbrickcollision(int index)
{
	double lx1,ly1,lx2,ly2,bx1,by1,bx2,by2,angle,c,s,cx,cy,lxx1,lyy1,lxx2,lyy2;
	lx1=-4.1+(lasers[index].tranlastionvector[3][0]);ly1=-0.02+(lasers[index].tranlastionvector[3][1]);
	lx2=-3.8+(lasers[index].tranlastionvector[3][0]);ly2=0.02+(lasers[index].tranlastionvector[3][1]);
	angle=((lasers[index].laser_rotation)*M_PI)/(180.0f);
	cx=((lx1+lx2)/(2.0f));cy=((ly1+ly2)/(2.0f));	
	c=cos(angle);s=sin(angle);
	lxx1=((lx1-cx)*c)-((ly1-cy)*s);lyy1=((lx1-cx)*s)+((ly1-cy)*c);lxx2=((lx2-cx)*c)-((ly2-cy)*s);lyy2=((lx2-cx)*s)+((ly2-cy)*c);
	lx1=lxx1+cx;lx2=lxx2+cx;ly2=lyy2+cy;ly1=lyy1+cy;
	map<int,struct brickdata>:: iterator it=bricks.begin();
	while(it!=bricks.end())
	{
	struct brickdata curr=it->second;
	bx1=curr.tranlastionvector[3][0];by1=4+curr.tranlastionvector[3][1];bx2=curr.tranlastionvector[3][0]+0.2;by2=curr.tranlastionvector[3][1]+4.2;
	if(checkoverlap(lx1,ly1,lx2,ly2,bx1,by1,bx2,by2,index)){playsound(90000);bricks.erase(it->first);}
	it++;
 	}
    return 0;
  } 
void keyboardDown (unsigned char key, int x, int y)
{
	glm::mat4 t1,t2,t3,t,l,l1,l2,l3;
    if(key=='s')
    {
    	if(canon.canon_rotation<=65.0f)
    	{canon.canon_rotation++;canon1.canon_rotation++;}
    	t1=glm::translate (glm::vec3(-3.55f,0.0f, 0.0f)); 
    	t2=glm::translate (glm::vec3(3.55f,0.0f,0.0f));
    	t3=glm::rotate((float)((canon.canon_rotation)*M_PI/180.0f), glm::vec3(0,0,1));
    	t=(t1*t3*t2);
    	canon.canontransform = (t1*t3*t2);
    	t1=glm::translate (glm::vec3(-3.55f,0.0f, 0.0f)); 
    	t2=glm::translate (glm::vec3(3.55f,0.0f,0.0f));
    	t3=glm::rotate((float)((canon1.canon_rotation)*M_PI/180.0f), glm::vec3(0,0,1));
    	t=(t1*t3*t2);
    	canon1.canontransform = (t1*t3*t2);

	}
	if(key=='f')
    {
    	if(canon.canon_rotation>=-65)	
    	{canon.canon_rotation--;canon1.canon_rotation--;}
    	t1=glm::translate (glm::vec3(-3.55f,0.0f, 0.0f)); 
    	t2=glm::translate (glm::vec3(3.55f,0.0f,0.0f));
    	t3=glm::rotate((float)((canon.canon_rotation)*M_PI/180.0f), glm::vec3(0,0,1));
    	t=(t1*t3*t2);
    	canon.canontransform = (t1*t3*t2);
    	t1=glm::translate (glm::vec3(-3.55f,0.0f, 0.0f)); 
    	t2=glm::translate (glm::vec3(3.55f,0.0f,0.0f));
    	t3=glm::rotate((float)((canon1.canon_rotation)*M_PI/180.0f), glm::vec3(0,0,1));
    	t=(t1*t3*t2);
    	canon1.canontransform = (t1*t3*t2);
	}
	if(key=='a')
	{
	glm::mat4 temp=canon.tranlastionvector*glm::translate(glm::vec3(0.0f,0.1f,0.0f));
	if(temp[3][1]+0.15f<3.0f)
	{
	canon.tranlastionvector*=glm::translate(glm::vec3(0.0f,0.1f,0.0f));
	canon1.tranlastionvector*=glm::translate(glm::vec3(0.0f,0.1f,0.0f));
	canon2.tranlastionvector*=glm::translate(glm::vec3(0.0f,0.1f,0.0f));
	canon3.tranlastionvector*=glm::translate(glm::vec3(0.0f,0.1f,0.0f));
	}
	}
	if(key=='d')
	{
		glm::mat4 temp=canon.tranlastionvector*glm::translate(glm::vec3(0.0f,0.1f,0.0f));
		if(temp[3][1]-0.65f>-2.8f)
		{
		canon.tranlastionvector*=glm::translate(glm::vec3(0.0f,-0.1f,0.0f));
		canon1.tranlastionvector*=glm::translate(glm::vec3(0.0f,-0.1f,0.0f));
		canon2.tranlastionvector*=glm::translate(glm::vec3(0.0f,-0.1f,0.0f));
		canon3.tranlastionvector*=glm::translate(glm::vec3(0.0f,-0.1f,0.0f));
		}
	}
	if(key==32 && laseractivity)
	{
		struct laserdata templaser;
  		loadingtranslationvector=glm::mat4(1.0f);
		laseractivity=0;
		totaltime=0;
		templaser.laser_object=createlaser();
		templaser.direction=1;
		templaser.tranlastionvector=canon.tranlastionvector;templaser.lasertransform=canon.canontransform;
		templaser.laser_rotation=canon.canon_rotation;
		lasers.push_back(templaser);
		playsound(100);
	}
	if(key=='g' && brickspeed>=-0.02)brickspeed-=0.001;
	if(key=='h' && brickspeed<=-0.001)brickspeed+=0.001;
}	
/* Executed when a regular key is released */
void keyboardUp (unsigned char key, int x, int y)
{
    switch (key) {
        case 'c':
        case 'C':
            rectangle_rot_status = !rectangle_rot_status;
            break;
        case 'p':
        case 'P':
            triangle_rot_status = !triangle_rot_status;
            break;
        default:
            break;
    }
}
/* Executed when a special key is pressed */
void keyboardSpecialDown (int key, int x, int y)
{
	if(key==GLUT_KEY_LEFT && (glutGetModifiers() == GLUT_ACTIVE_CTRL))
		{
		glm::mat4 temp=buckets[1].tranlastionvector*glm::translate (glm::vec3(-0.1,0,0));
		if(temp[3][0]-0.3 > -3.5)
		{
		buckets[1].tranlastionvector*=glm::translate (glm::vec3(-0.1,0,0));
    	draw();
    	}
    }
    if(key==GLUT_KEY_RIGHT && (glutGetModifiers() == GLUT_ACTIVE_CTRL))
		{
		glm::mat4 temp=buckets[1].tranlastionvector*glm::translate (glm::vec3(0.1,0,0));
		if(temp[3][0]+0.3 < 2.5)
		{
		buckets[1].tranlastionvector*=glm::translate (glm::vec3(0.1,0,0));
		draw();
		}
    	}
    if(key==GLUT_KEY_LEFT && (glutGetModifiers() == GLUT_ACTIVE_ALT))
		{
		glm::mat4 temp=buckets[0].tranlastionvector*glm::translate (glm::vec3(-0.1,0,0));
		if(temp[3][0]-0.3 > -3.5){
		buckets[0].tranlastionvector*=glm::translate (glm::vec3(-0.1,0,0));
    	draw();}
    	}
    if(key==GLUT_KEY_RIGHT && (glutGetModifiers() == GLUT_ACTIVE_ALT))
		{
		glm::mat4 temp=buckets[0].tranlastionvector*glm::translate (glm::vec3(0.1,0,0));
		if(temp[3][0]+0.3 < 2.5){
		buckets[0].tranlastionvector*=glm::translate (glm::vec3(0.1,0,0));
    	draw();}
    	}
}

/* Executed when a special key is released */
void keyboardSpecialUp (int key, int x, int y)
{
}

/* Executed when a mouse button 'button' is put into state 'state'
 at screen position ('x', 'y')
 */
int canonselect=0,canony,bucketredselect=0,bucketblueselect=0,bucketredxx,bucketbluexx;
void mouseClick (int button, int state, int x, int y)
{
    if(button==GLUT_LEFT_BUTTON && state == GLUT_DOWN)
     {
     			double xx=(-250+double(x))/(50),yy=((250-double(y))/50);
         		if(xx>=-7.0f && xx<=-1.3f && yy<=(0.25f+canon.tranlastionvector[3][1]+0.5f) && yy>=(-0.25f+canon.tranlastionvector[3][1])-0.5f)
     			{canonselect=1;canony=yy;}
     			if(yy>=-5.0f && yy<=-3.0f)
     			{
     				if(xx>=-1+buckets[0].tranlastionvector[3][0] && xx<=1+buckets[0].tranlastionvector[3][0])
     				{bucketblueselect=1;bucketbluexx=xx;}
     				if(xx>=-1+buckets[1].tranlastionvector[3][0] && xx<=1+buckets[1].tranlastionvector[3][0])
     				{bucketredselect=1;bucketredxx=xx;}	
     			}
     }
     if(button==GLUT_LEFT_BUTTON && state == GLUT_UP)
     {
     		double xx=(-200+double(x))/(50),yy=((250-double(y))/50);
     		if(canonselect)
     		{
     		glm::mat4 temp=canon.tranlastionvector*glm::translate(glm::vec3(0,yy-canony,0));
     		if((temp[3][1]+0.15f<3.0f) && (temp[3][1]-0.65f>-3.4f))
     		{
     		canon.tranlastionvector*=glm::translate(glm::vec3(0,yy-canony,0));
     		canon1.tranlastionvector*=glm::translate(glm::vec3(0,yy-canony,0));
     		canon2.tranlastionvector*=glm::translate(glm::vec3(0,yy-canony,0));
     		canon3.tranlastionvector*=glm::translate(glm::vec3(0,yy-canony,0));
     		}
     		canonselect=0;
     		}
     		else if(bucketblueselect)
     		{
     			glm::mat4 temp=buckets[0].tranlastionvector*glm::translate(glm::vec3(xx-bucketbluexx,0,0));
     			if((temp[3][0]-0.3 > -3.5) && (temp[3][0]+0.3 < 2.5))
     			{
     			buckets[0].tranlastionvector*=glm::translate(glm::vec3(xx-bucketbluexx,0,0));
     			bucketbluexx=xx;
     			}
     			bucketblueselect=0;
     		}
     		else if(bucketredselect)
     		{
     			glm::mat4 temp=buckets[1].tranlastionvector*glm::translate(glm::vec3(xx-bucketredxx,0,0));
     			if((temp[3][0]-0.3 > -3.5) && (temp[3][0]+0.3 < 2.5))
     			{
     			buckets[1].tranlastionvector*=glm::translate(glm::vec3(xx-bucketredxx,0,0));
     			bucketredxx=xx;
     			}
     			bucketredselect=0;
     		}
     }
}

/* Executed when the mouse moves to position ('x', 'y') */
void mouseMotion (int x, int y)
{
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (int width, int height)
{
	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) width, (GLsizei) height);

	// set the projection matrix as perspective/ortho
	// Store the projection matrix in a variable for future use

    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) width / (GLfloat) height, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *triangle, *rectangle;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
struct VAO* createbrick(int type);
void createbricks()
{
	int count=(rand()%4)+1;
	for(int i=1;i<=count;i++)
	{
	totalbricks++;
	struct brickdata temp;
	double x=(rand()%3);
	int y=rand()%2;if(!y)x*=(-1);
	double z=rand()%3;
	temp.brick=createbrick(z);
	temp.tranlastionvector=glm::translate(glm::vec3(x,0,0));
	temp.index=totalbricks;
	temp.type=z;
	bricks[totalbricks]=temp;
	}
}
struct VAO* createbrick(int type)
{
	const GLfloat vertex_buffer_data [] = {
    0,4,0, // vertex 1
    0.2,4,0, // vertex 2
    0.2,4.2,0, // vertex 3

    0.2,4.2,0, // vertex 3
    0,4.2,0, // vertex 4
    0,4,0  // vertex 1
  };
  if(type==0)
  {
   static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);}
  if(type==1)
  {
   static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3

    1,0,0, // color 3
    1,0,0, // color 4
    1,0,0  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);}
  if(type==2)
  {
   static const GLfloat color_buffer_data [] = {
    0,0,1, // color 1
    0,0,1, // color 2
    0,0,1, // color 3

    0,0,1, // color 3
    0,0,1, // color 4
    0,0,1  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);}
}
struct VAO* ScoreSheet()
{
	const GLfloat vertex_buffer_data [] = {
    2.5,-3.97,0, // vertex 1
    5,-3.97,0, // vertex 2
    5,10,0, // vertex 3

    5,10,0, // vertex 3
    2.5,10,0, // vertex 4
    2.5,-3.97,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
};
struct VAO* LoadingBar()
{
	const GLfloat vertex_buffer_data [] = {
    0.5,3.2,0, // vertex 1
    2.5,3.2,0, // vertex 2
    2.5,3.5,0, // vertex 3

    2.5,3.5,0, // vertex 3
    0.5,3.5,0, // vertex 4
    0.5,3.2,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.4,0.4,0.4, // color 1
    0.4,0.4,0.4, // color 2
    0.4,0.4,0.4, // color 3

    0.4,0.4,0.4, // color 3
    0.4,0.4,0.4, // color 4
    0.4,0.4,0.4  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
};
struct VAO* createmirror()
{
	const GLfloat vertex_buffer_data [] = {
    -0.4,-0.02,0, // vertex 1
    0.4,-0.02,0, // vertex 2
    0.4,0.02,0, // vertex 3

    0.4,0.02,0, // vertex 3
    -0.4,0.02,0, // vertex 4
    -0.4,-0.02,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
};
struct VAO* createcanon()
{
	const GLfloat vertex_buffer_data [] = {
    -4,-0.25,0, // vertex 1
    -3.3,-0.15,0, // vertex 2
    -3.3,0.15,0, // vertex 3

    -3.3,0.15,0, // vertex 3
    -4,0.25,0, // vertex 4
    -4,-0.25,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.5,0,1, // color 1
    0.5,0,1, // color 2
    0.5,0,1, // color 3

    0.5,0,1, // color 3
    0.5,0,1, // color 4
    0.5,0,1  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
struct VAO* createcanon1()
{
	const GLfloat vertex_buffer_data [] = {
    -3.3,-0.05,0, // vertex 1
    -3,-0.05,0, // vertex 2
    -3,0.05,0, // vertex 3

    -3,0.05,0, // vertex 3
    -3.3,0.05,0, // vertex 4
    -3.3,-0.05,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.5,0,1, // color 1
    0.5,0,1, // color 2
    0.5,0,1, // color 3

    0.5,0,1, // color 3
    0.5,0,1, // color 4
    0.5,0,1  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}struct VAO* createcanon2()
{
	const GLfloat vertex_buffer_data [] = {
    -3.75,-0.55,0, // vertex 1
    -3.65,-0.55,0, // vertex 2
    -3.65,-0.05,0, // vertex 3

    -3.65,-0.05,0, // vertex 3
    -3.75,-0.05,0, // vertex 4
    -3.75,-0.55,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.5,0,1, // color 1
    0.5,0,1, // color 2
    0.5,0,1, // color 3

    0.5,0,1, // color 3
    0.5,0,1, // color 4
    0.5,0,1  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}struct VAO* createcanon3()
{
	const GLfloat vertex_buffer_data [] = {
   	-3.85,-0.6,0, // vertex 1
    -3.55,-0.6,0, // vertex 2
    -3.55,-0.55,0, // vertex 3

    -3.55,-0.55,0, // vertex 3
    -3.85,-0.55,0, // vertex 4
    -3.85,-0.6,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.5,0,1, // color 1
    0.5,0,1, // color 2
    0.5,0,1, // color 3

    0.5,0,1, // color 3
    0.5,0,1, // color 4
    0.5,0,1  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void drawmirrors()
{
	mirrors[0].mirror_object=createmirror();mirrors[1].mirror_object=createmirror();mirrors[2].mirror_object=createmirror();
	mirrors[0].rotationvector = glm::rotate((float)((-M_PI)/4.0f), glm::vec3(0,0,1));
	mirrors[0].a=1.0f;mirrors[0].b=1.0f;mirrors[0].c=(((0.4f*(sin(M_PI/4.0f)))-(0.4f*(M_PI/4.0f))))-0.53f;
	mirrors[1].rotationvector = glm::rotate((float)(M_PI/4.0f), glm::vec3(0,0,1));
	mirrors[1].a=1.0f;mirrors[1].b=-1.0f;mirrors[1].c=(0.4f*(sin(M_PI/4.0f)-cos(M_PI/4.0f)))-0.57f;
	mirrors[2].rotationvector = glm::rotate((float)(M_PI/2.0f), glm::vec3(0,0,1));
	mirrors[2].a=1.0f;mirrors[2].b=0.0f;mirrors[2].c=-2.28f;
	mirrors[0].tranlastionvector=glm::translate(glm::vec3(-1.45f,2.0f,0));
	mirrors[1].tranlastionvector=glm::translate(glm::vec3(-1.45f,-2.0f,0));
	mirrors[2].tranlastionvector=glm::translate(glm::vec3(2.3f,0.0f,0));
}
struct VAO* createlaser()
{
	const GLfloat vertex_buffer_data [] = {
    -4.1,-0.02,0, // vertex 1
    -3.8,-0.02,0, // vertex 2
    -3.8,0.02,0, // vertex 3

    -3.8,0.02,0, // vertex 3
    -4.1,0.02,0, // vertex 4
    -4.1,-0.02,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.5,0.2,0.2, // color 1
    0.5,0.2,0.2, // color 2
    0.5,0.2,0.2, // color 3

    0.5,0.2,0.2, // color 3
    0.5,0.2,0.2, // color 4
    0.5,0.2,0.2  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
struct VAO* CanonArena()
{
	const GLfloat vertex_buffer_data [] = {
    -4,-3,0, // vertex 1
    -3.5,-3,0, // vertex 2
    -3.5,3,0, // vertex 3

    -3.5,3,0, // vertex 3
    -4,3,0, // vertex 4
    -4,-3,0  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
};
struct VAO* createBucket (int i)
{
  // GL3 accepts only Triangles. Quads are not supported static
  const GLfloat vertex_buffer_data [] = {
    -0.2,-3.97,0, // vertex 1
    0.2,-3.97,0, // vertex 2
    0.3, -3.27,0, // vertex 3

    0.3, -3.27,0, // vertex 3
    -0.3, -3.27,0, // vertex 4
    -0.2,-3.97,0  // vertex 1
  };
  if(i==0)
  {
  static const GLfloat color_buffer_data [] = {
    0,0,1, // color 1
    0,0,1, // color 2
    0,0,1, // color 3

    0,0,1, // color 3
    0,0,1, // color 4
    0,0,1  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
  else
  {
  	static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3

    1,0,0, // color 3
    1,0,0, // color 4	
    1,0,0  // color 1
  };
  return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  }

  // create3DObject creates and returns a handle to a VAO that can be used later
  
}


/* Render the scene with openGL */
/* Edit this function according to your assignment */
int counter=0,red=0,blue=0;
bool checkbrickcollision(struct brickdata BRICK)
{
	double bucketredx=buckets[1].tranlastionvector[3][0],bucketbluex=buckets[0].tranlastionvector[3][0],brickx=BRICK.tranlastionvector[3][0],bricky=BRICK.tranlastionvector[3][1];
	if(BRICK.type==2 && bucketbluex-0.3<=brickx && bucketbluex+0.3>=brickx && bricky<=-7.3f){bluescore++;cout << "RED: " << redscore << "\nBLUE: " << bluescore << endl;return true;}
	if(BRICK.type==1 && bucketredx-0.3<=brickx && bucketredx+0.3>=brickx && bricky<=-7.3f){redscore++;cout << "RED: " << redscore << "\nBLUE: " << bluescore << endl;return true;}
	if(BRICK.type==0 && ((bucketredx-0.3<=brickx && bucketredx+0.3>=brickx)||(bucketbluex-0.3<=brickx && bucketbluex+0.3>=brickx)) && bricky<=-7.3f)
	{cout<<"GAMEOVER\n";gameover=1;return true;}
	if(bricky<=-7.3f)return true;
	return false;
}
int yspeed=1;
bool checklaserboundary(int index)
{
	double lx,ly,lxx,lyy,angle,cx,cy,c,s;
	lx=-3.8f+(lasers[index].tranlastionvector[3][0]);ly=0.02f+(lasers[index].tranlastionvector[3][1]);
	angle=((lasers[index].laser_rotation)*M_PI)/(180.0f);
	cx=-3.95f+(lasers[index].tranlastionvector[3][0]);cy=(lasers[index].tranlastionvector[3][1]);	
	c=cos(angle);s=sin(angle);
	lxx=((lx-cx)*c)-((ly-cy)*s);lyy=((lx-cx)*s)+((ly-cy)*c);
	lx=lxx+cx;ly=lyy+cy;
	if(lx<=-4 || lx>=4)return true;
	if(ly>=8 || ly<=-8)return true;
	return false;
}
bool checklasermirrorcollision(int index)
{
	double lx,ly,lxx,lyy,angle,cx,cy,c,s,y1,y2,y3,y4,rx,ry,rxx,ryy;
	lx=-3.8f+(lasers[index].tranlastionvector[3][0]);ly=0.02f+(lasers[index].tranlastionvector[3][1]);
	angle=((lasers[index].laser_rotation)*M_PI)/(180.0f);
	cx=-3.95f+(lasers[index].tranlastionvector[3][0]);cy=(lasers[index].tranlastionvector[3][1]);	
	c=cos(angle);s=sin(angle);
	lxx=((lx-cx)*c)-((ly-cy)*s);lyy=((lx-cx)*s)+((ly-cy)*c);
	lx=lxx+cx;ly=lyy+cy;
	y1=-0.02f+2.0f+(0.4f*(sin(M_PI/4.0f)));y2=-0.02+2.0f-(0.4f*(sin(M_PI/4.0f)));
	y3=-0.02f-2.0f+(0.4f*(sin(M_PI/4.0f)));y4=-0.02-2.0f-(0.4f*(sin(M_PI/4.0f)));
	//checking collision by first mirror//
	glm::mat4 t1,t2,t3,t;
	if((abs((mirrors[0].a*lx)+(mirrors[0].b*ly)+mirrors[0].c) <= 0.1f) && ly<=y1+0.3 && ly>=y2)
	{
			playsound(400);
			double temp=lasers[index].laser_rotation;
			lasers[index].laser_rotation+=((((-2.0f)*(temp))+(270.0f)));
			t1=glm::translate (glm::vec3(-3.95f,0.0f, 0.0f)); 
    		t2=glm::translate (glm::vec3(3.95f,0.0f,0.0f));
    		t3=glm::rotate((float)((((2.0f*(temp))-90.0f))*(-M_PI)/180.f),glm::vec3(0,0,1));
    		t=(t1*t3*t2);
    		lasers[index].lasertransform*=(t);
	}
	//checking collision by second mirror
	else if((abs((mirrors[1].a*lx)+(mirrors[1].b*ly)+mirrors[1].c) <= 0.1f) && ly>=y4-0.3 && ly<=y3+0.3)
	{
			playsound(400);
			double temp=lasers[index].laser_rotation;
			lasers[index].laser_rotation+=((90)-(2*temp));
			t1=glm::translate (glm::vec3(-3.95f,0.0f, 0.0f)); 
    		t2=glm::translate (glm::vec3(3.95f,0.0f,0.0f));
    		t3=glm::rotate((float)((((2*(temp))+90))*(-M_PI)/180.f),glm::vec3(0,0,1));
    		t=(t1*t3*t2);
    		lasers[index].lasertransform*=(t);
	}
	//checking collision by third mirror
	else if((abs((mirrors[2].a *lx)+(mirrors[2].b*ly)+mirrors[2].c) <= 0.1f) && ly<=0.6 && ly>=-0.6)
	{
			playsound(400);
			double temp=lasers[index].laser_rotation;
			lasers[index].laser_rotation+=(180-2*(temp));
			t1=glm::translate (glm::vec3(-3.95f,0.0f, 0.0f)); 
    		t2=glm::translate (glm::vec3(3.95f,0.0f,0.0f));
    		t3=glm::rotate((float)(-(2*angle)),glm::vec3(0,0,1));
    		t=(t1*t3*t2);
    		lasers[index].lasertransform*=(t);
	}
}
struct VAO* loadingbar;
void draw ()
{
  // clear the color and depth in the frame buffer
	if(gameover)return;
	counter++;
	totaltime++;
	if(counter%250==0)
  	{createbricks();counter=0;}
  	if(totaltime%100==0)laseractivity=1;
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);
  if(!laseractivity)
  loadingtranslationvector*=glm::translate(glm::vec3(0.02f,0.0f,0.0f));
  
  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);

  /* Render your scene */

  glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  Matrices.model *= triangleTransform; 
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
 // draw3DObject(triangle);
   drawmirrors();
   
  for(int i=0;i<3;i++)
  {
  	Matrices.model = glm::mat4(1.0f);
  	Matrices.model*=(mirrors[i].tranlastionvector*mirrors[i].rotationvector);
  	MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  	draw3DObject(mirrors[i].mirror_object);
  }
  for(int it=0;it<lasers.size();it++)
  {
  	checklaserbrickcollision(it);
    checklasermirrorcollision(it);
    if(checklaserboundary(it))
    {
    	lasers.erase(lasers.begin()+it);
    }
    else
    {
  	Matrices.model = glm::mat4(1.0f);
  	double laserangle=(((lasers[it].laser_rotation)*M_PI)/180.0f);
  	lasers[it].tranlastionvector*=glm::translate(glm::vec3((0.1f)*(cos(laserangle)),(0.1f)*(sin(laserangle)),0));
  	Matrices.model*=lasers[it].tranlastionvector;
  	Matrices.model*=lasers[it].lasertransform;
  	MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  	draw3DObject(lasers[it].laser_object);
  }
  }
  Matrices.model = glm::mat4(1.0f);MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  struct VAO* scoresheet=ScoreSheet();
  draw3DObject(scoresheet);
  Matrices.model = glm::mat4(1.0f); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  struct VAO* canonarena=CanonArena();	
  draw3DObject(canonarena);
  Matrices.model = glm::mat4(1.0f);Matrices.model*=canon.tranlastionvector;
  Matrices.model *= (canon.canontransform);
  MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  canon.canon_object=createcanon();	
  draw3DObject(canon.canon_object);
  Matrices.model = glm::mat4(1.0f);
  Matrices.model*=canon1.tranlastionvector;
  Matrices.model *= (canon1.canontransform);
  MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  canon1.canon_object=createcanon1();	
  draw3DObject(canon1.canon_object);
   Matrices.model = glm::mat4(1.0f);
   Matrices.model*=canon2.tranlastionvector;
  MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  canon2.canon_object=createcanon2();	
  draw3DObject(canon2.canon_object);
  Matrices.model = glm::mat4(1.0f);
  Matrices.model*=canon3.tranlastionvector;
  MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  canon3.canon_object=createcanon3();	
  draw3DObject(canon3.canon_object);
   Matrices.model = glm::mat4(1.0f);
  //buckets[0].tranlastionvector= glm::translate (glm::vec3(2, -3.5, 0));        // glTranslatef
  buckets[0].rotationvector = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));
   // rotate about vector (-1,1,1)
  Matrices.model *= (buckets[0].tranlastionvector * buckets[0].rotationvector);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix	
  draw3DObject(buckets[0].bucket);
  Matrices.model = glm::mat4(1.0f);
  buckets[1].rotationvector = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));
   // rotate about vector (-1,1,1)
  Matrices.model *= (buckets[1].tranlastionvector * buckets[1].rotationvector);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(buckets[1].bucket);
  // Swap the frame buffers
 
 Matrices.model = glm::mat4(1.0f);
  Matrices.model *= (loadingtranslationvector);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(loadingbar);
  map<int,struct brickdata>::iterator it=bricks.begin();
  while(it!=bricks.end())
  {
  	struct brickdata currbrick=it->second;
    currbrick.tranlastionvector *= glm::translate(glm::vec3(0.0f,brickspeed,0.0f));
    if(!checkbrickcollision(currbrick))
    {
  	Matrices.model = glm::mat4(1.0f);Matrices.model*=currbrick.tranlastionvector;
  	MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);	
   	draw3DObject(currbrick.brick);
   	bricks[it->first]=currbrick;
   }
   else bricks.erase(it->first);
  	it++;
  }
  glutSwapBuffers ();

  // Increment angles
  float increments = 1;
	
 // camera_rotation_angle++; // Simulating camera rotation
 //	 triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
 // rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;

}

/* Executed when the program is idle (no I/O activity) */
void idle () {
    // OpenGL should never stop drawing
    // can draw the same scene or a modified scene
    draw (); // drawing same scene
}


/* Initialise glut window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
void initGLUT (int& argc, char** argv, int width, int height)
{
    // Init glut
    glutInit (&argc, argv);
      buckets[0].tranlastionvector= glm::translate (glm::vec3(2,0, 0));        // glTranslatef
  	  buckets[1].tranlastionvector= glm::translate (glm::vec3(-3,0, 0));        // glTranslatef
    
    
    // Init glut window
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitContextVersion (3, 3); // Init GL 3.3
    glutInitContextFlags (GLUT_CORE_PROFILE); // Use Core profile - older functions are deprecated
    glutInitWindowSize (width, height);
    glutCreateWindow ("Sample OpenGL3.3 Application");
    // Initialize GLEW, Needed in Core profile
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cout << "Error: Failed to initialise GLEW : "<< glewGetErrorString(err) << endl;
        exit (1);
    }

    // register glut callbacks
    glutKeyboardFunc (keyboardDown);
    glutKeyboardUpFunc (keyboardUp);

    glutSpecialFunc (keyboardSpecialDown);
    glutSpecialUpFunc (keyboardSpecialUp);

    glutMouseFunc (mouseClick);
    glutMotionFunc (mouseMotion);

    glutReshapeFunc (reshapeWindow);

    glutDisplayFunc (draw); // function to draw when active
    glutIdleFunc (idle); // function to draw when idle (no I/O activity)
    
 //   glutIgnoreKeyRepeat (true); // Ignore keys held down
}

/* Process menu option 'op' */
void menu(int op)
{
    switch(op)
    {
        case 'Q':
        case 'q':
            exit(0);
    }
}

void addGLUTMenus ()
{
    // create sub menus
    int subMenu = glutCreateMenu (menu);
    glutAddMenuEntry ("Do Nothing", 0);
    glutAddMenuEntry ("Really Quit", 'q');

    // create main "middle click" menu
    glutCreateMenu (menu);
    glutAddSubMenu ("Sub Menu", subMenu);
    glutAddMenuEntry ("Quit", 'q');
    glutAttachMenu (GLUT_MIDDLE_BUTTON);
}


/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (int width, int height)
{
	// Create the models
	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer 
	loadingbar=LoadingBar();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (width, height);

	// Background color of the scene
	glClearColor (0.4f,0.4f,0.4f,1.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	buckets[0].bucket=createBucket (0);
	buckets[1].bucket=createBucket (1);
	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 500;
	int height = 500;
    initGLUT (argc, argv, width, height);
    addGLUTMenus ();

	initGL (width, height);

    glutMainLoop ();

    return 0;
}
