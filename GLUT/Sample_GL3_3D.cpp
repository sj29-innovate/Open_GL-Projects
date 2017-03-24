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
void draw();
int haligned,valigned,zaligned,fx,fz,fy,bx,by,bz,bridge1,bridge2;
double elevation=0,azimuthal=0;
	GLfloat fov = 90.0f;
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
struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;
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
int level=0;

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
struct CUBEstructure{
	VAO* top;VAO* bottom;VAO* right;VAO* left;VAO* near;VAO* far;
	glm::mat4 toptranslate,bottomtranslate,toprotate,bottomrotate,righttranslate,lefttranslate,leftrotate,rightrotate,neartranslate,fartranslate,nearrotate,totaltransform,
	totaltranslate,totalrotate;
};
struct CUBEstructure block[2];
struct Boardcellstructure{
	VAO* cell;
	glm::mat4 celltranslate;
	glm::mat4 cellrotate;
};
struct Boardcellstructure board[12][12][12];
int tiles[12][12];
/**************************
 * Customizable functions *
 **************************/
double cx=-3,cy=4,cz=9,lx=-0.5,ly=0,lz=0;
void createlevel3()
{
	for(int i=5;i<=6;i++)
	{
		for(int j=1;j<=10;j++)tiles[j][i]=1;
	}
	for(int i=1;i<=10;i++)
	{
		for(int j=8;j<=10;j++)
			tiles[j][i]=1;
	}
   tiles[6][4]=2;tiles[3][4]=2;tiles[4][4]=1;tiles[5][4]=1;
   tiles[4][1]=tiles[5][1]=tiles[6][1]=1;tiles[1][7]=tiles[1][8]=tiles[1][9]=tiles[1][10]=1;tiles[2][7]=tiles[2][8]=tiles[2][9]=tiles[2][10]=1;
   tiles[5][2]=tiles[6][2]=1;tiles[5][3]=tiles[6][3]=tiles[4][3]=1;tiles[4][9]=3;
}
void createlevel2()
{
	for(int i=1;i<=10;i++)
	for(int j=1;j<=10;j++)
	{
	if(!(i==1 && (j==2 || j==3 || j==4)))
	tiles[i][j]=1;
	}
	for(int i=1;i<=5;i++)
		for(int j=1;j<=4;j++)if(!(i==1 && (j==2 || j==3 || j==4)))tiles[i][j]=0;
	for(int i=4;i<=7;i++)
		for(int j=7;j<=10;j++)if(!(i==1 && (j==2 || j==3 || j==4)))tiles[i][j]=0;
	tiles[10][1]=tiles[9][1]=tiles[9][2]=tiles[5][1]=tiles[5][2]=tiles[4][2]=1;
	tiles[5][5]=tiles[5][6]=tiles[5][7]=0;
	tiles[6][9]=3;
	
}
void createlevel1()
{
	for(int i=1;i<=10;i++)for(int j=1;j<=10;j++)tiles[j][i]=1;
	for(int i=1;i<=5;i++)
	for(int j=1;j<=6;j++)tiles[j][i]=0;
	tiles[4][3]=3;
}
void resetblock()
{
	system("aplay -q  Emerge1.wav &");
	haligned=1;fz=bz=1;fx=1;bx=2;fy=0;by=0;
	double yy=2.0f;
	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,yy,5-fz));
	block[1].totaltranslate=glm::translate(glm::vec3(bx-6,yy,5-bz));
	while(yy>0)
	{
	yy-=0.1f;
	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,yy,5-fz));
	block[1].totaltranslate=glm::translate(glm::vec3(bx-6,yy,5-bz));
	draw();
	}
	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
	if(level==2)
	{
	bridge1=0;bridge2=0;
	tiles[3][10]=tiles[5][9]=tiles[7][9]=tiles[7][10]=tiles[4][2]=4;
	}
	if(level==1)
		tiles[1][2]=tiles[1][3]=tiles[1][4]=4;
}
int view=0,height,width,follow=0,blockview=0,helicam=0,blockcam=0;
void changelevel()
{
	level++;
	if(level==1){
			haligned=1;fz=bz=1;fx=1;bx=2;fy=0;by=0;
			block[0].totaltranslate=glm::translate(glm::vec3(fx-5,0.0f,5-fz));
			block[1].totaltranslate=glm::translate(glm::vec3(bx-6,0.0f,5-bz));
			tiles[1][2]=tiles[1][3]=tiles[1][4]=4;
			}
	if(level==2){
				haligned=1;fz=bz=1;fx=1;bx=2;fy=0;by=0;
	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,0.0f,5-fz));
	block[1].totaltranslate=glm::translate(glm::vec3(bx-6,0.0f,5-bz));
				memset(tiles,0,sizeof(tiles));
				cx=-3;cy=4;cz=9;tiles[3][10]=tiles[5][9]=tiles[7][9]=tiles[7][10]=tiles[4][2]=4;bridge2=0;bridge1=0;lx=-0.15;ly=0;lz=0;
			   }

}
void keyboardDown (unsigned char key, int x, int y)
{
 if(key=='d'){
 	if(haligned)
 	{
 	system("aplay -q  Explosion2.wav &");
 	double x=0;
 	while(x<90){
 	x+=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(1.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.5f,0.5f,0.0f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(1.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.5f,0.5f,0.0f))));
 	draw();
 	}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	fx+=2;bx++;fy++;haligned=0;valigned=1;
 	block[0].totaltranslate=glm::translate(glm::vec3(min(fx,bx)-5,fy,5-fz));
 	block[1].totaltranslate=glm::translate(glm::vec3(bx-6,by,5-bz));
 	if(tiles[fx][fz]==1 && tiles[bx][bz]==1)
 	{
 		x=0;double ffy=fy,bby=by;
 		int c=0;
 		 		system("aplay -q Broken_Robot3.wav &");

 		while(c<100)
 		{
 			x+=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-1.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-0.5f,1.5f,0.0f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(1.5f,-0.5,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.5f,0.5f,0.0f))));		
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		}
 		resetblock();
 	}
 	if(tiles[fx][fz]==4 || tiles[bx][bz]==4 || tiles[fx][fz]==3 || tiles[bx][bz]==3)
 	{
 		double ffy=fy,bby=by;int c=0;
 		system("aplay -q  Broken_Robot3.wav &");
 		tiles[bx][bz]=1;
 		while(c<150)
 		{c++;
 		ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 		draw();
 		}
 		 		if(tiles[bx][bz]==3)
 		 		{
 		 			if(level==2)exit(1);
 		 			changelevel();draw();
 		 		}
 		resetblock();
 	}
 	}
 	else if(valigned)
 	{
 	 	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x<90){
 	x+=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-1.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-0.5f,1.5f,0.0f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(1.5f,-0.5,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.5f,0.5f,0.0f))));
 	draw();}
 	fx+=2;bx++;fy--;haligned=1;valigned=0;
 	block[0].totaltranslate=glm::translate(glm::vec3(min(fx,bx)-5,fy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(max(fx,bx)-6,by,5-bz));
 	if(bx<fx){int temp=bx;bx=fx;fx=temp;}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if(tiles[fx][fz]==1 && tiles[bx][bz]==1)
 	{
 		x=0;double ffy=fy,bby=by;
 		int c=0;
 		 		system("aplay -q Broken_Robot3.wav &");

 		while(c<100)
 		{
 			x+=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(1.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.5f,0.5f,0.0f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(1.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.5f,0.5f,0.0f))));		
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
	}
	else if(zaligned)
 	{
 	 	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x<90){
 	x+=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.0f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(1.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.5f,0.5f,0.0f))));
 	draw();}
 	fx++;bx++;
 	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,fy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,by,5-bz));
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if(tiles[fx][fz]==1 && tiles[bx][bz]==1)
 	{
 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			 		system("aplay -q Broken_Robot3.wav &");

 			x+=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.0f,0.5f,0.0f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(1.0f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-1.0f,0.5f,0.0f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
	}
}
if(key=='a'){
 	if(haligned)
 	{
 	 	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x>-90){
 	x-=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(-0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,0.5f,0.0f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(-0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,0.5f,0.0f))));
 	draw();
 	}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	fx--;bx-=2;by++;haligned=0;valigned=1;
 	block[0].totaltranslate=glm::translate(glm::vec3(min(fx,bx)-5,max(fy,by),5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,min(fy,by),5-bz));
 	if(by>fy){int temp=by;by=fy;fy=temp;}
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fx<1||bx<1)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x-=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(-0.5f,-1.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,1.5f,0.0f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.0f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
 	if(tiles[fx][fz]==4 || tiles[bx][bz]==4 || tiles[fx][fz]==3 || tiles[bx][bz]==3)
 	{
 		double ffy=fy,bby=by;int c=0;tiles[bx][bz]=1;
 		 		system("aplay -q  Broken_Robot3.wav &");

 		while(c<150)
 		{
 		c++;
 		ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 		draw();
 		}
 		if(tiles[bx][bz]==3){if(level==2)exit(1);changelevel();draw();}
 		resetblock();
 	}
 	}
 	else if(valigned)
 	{
  	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x>-90){
 	x-=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(-0.5f,-1.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,1.5f,0.0f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.0f))));
 	draw();}
 	fx-=2;bx--;fy--;haligned=1;valigned=0;
 	block[0].totaltranslate=glm::translate(glm::vec3(min(fx,bx)-5,fy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(max(fx,bx)-6,by,5-bz));
 	if(bx<fx){int temp=bx;bx=fx;fx=temp;}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1) || fx<1 || bx<1)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x-=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(-0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,0.5f,0.0f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(-0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,0.5f,0.0f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} resetblock();

 	}
	}
	else if(zaligned)
 	{
  	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x>-90){
 	x-=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(-0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,0.5f,0.0f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.0f))));
 	draw();}
 	fx--;bx--;
 	block[0].totaltranslate=glm::translate(glm::vec3(min(fx,bx)-5,fy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(max(fx,bx)-6,by,5-bz));
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fx<1||bx<1)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x-=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(-0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(0.5f,0.5f,0.0f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.0f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(0,0,-1)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.0f))));		
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
	}	
}
if(key=='w'){
 	if(haligned)
 	{
  	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x<90){
 	x+=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.5f))));
 	draw();
 	}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	haligned=1;fz++;bz++;
 	block[0].totaltranslate=glm::translate(glm::vec3(min(fx,bx)-5,max(fy,by),5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,min(fy,by),5-bz));
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fz>10||bz>10)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x+=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.5f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.5f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
 	}
 	else if(valigned)
 	{
  	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x<90){
 	x+=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-1.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,1.5f,0.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.5f))));
 	draw();}
 	fz+=2;bz++;fy--;zaligned=1;valigned=0;
 	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,fy,5-min(fz,bz)));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,by,5-max(fz,bz)));
 	if(bz<fz){int temp=bz;bz=fz;fz=temp;}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fz>10||bz>10)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x+=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,-1.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,1.5f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,0.5f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
	}
	else if(zaligned)
 	{
  	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x<90){
 	x+=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,-1.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,1.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,0.5f))));
 	draw();}
 	fz+=2;bz++;fy++;zaligned=0;valigned=1;
 	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,fy,5-min(fz,bz)));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,by,5-max(bz,fz)));
 	if(bz<fz){int temp=bz;bz=fz;fz=temp;}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fz>10||bz>10)
 	{
 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		system("aplay -q  Broken_Robot3.wav &");
 		while(c<100)
 		{
 			x+=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-1.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,1.5f,0.5f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,-0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,0.5f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
 	if(tiles[fx][fz]==4 || tiles[bx][bz]==4 || tiles[fx][fz]==3 || tiles[bx][bz]==3)
 	{
 		system("aplay -q  Broken_Robot3.wav &");
 		double ffy=fy,bby=by;int c=0;tiles[bx][bz]=1;
 		while(c<150)
 		{c++;
 		ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 		draw();
 		}
 		if(tiles[bx][bz]==3){if(level==2)exit(1);changelevel();draw();}
 		resetblock();
 	}
	}
}
if(key=='s'){
 	if(haligned)
 	{
 	 	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x>-90){
 	x-=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,-0.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,-0.5f))));
 	draw();
 	}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	haligned=1;fz--;bz--;
 	block[0].totaltranslate=glm::translate(glm::vec3(min(fx,bx)-5,max(fy,by),5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,min(fy,by),5-bz));
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fz<1||bz<1)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x-=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,-0.5f))));
 			block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,-0.5f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
 	}
 	else if(valigned)
 	{
 	 	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x>-90){
 	x-=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-1.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,1.5f,-0.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,-0.5f))));
 	draw();}
 	fz-=2;bz--;fy--;zaligned=1;valigned=0;
 	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,fy,5-min(fz,bz)));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,by,5-max(fz,bz)));
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fz<1||bz<1)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x-=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,-0.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,1.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,-1.5f))));
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
	}
	else if(zaligned)
 	{
 	 	system("aplay -q  Explosion2.wav &");

 	double x=0;
 	while(x>-90){
 	x-=4.0;
 	block[0].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,-0.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.0f,-0.5f,1.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(0.0f,0.5f,-1.5f))));
 	draw();}
 	fz--;bz-=2;fy++;zaligned=0;valigned=1;
 	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,fy,5-min(fz,bz)));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,by,5-max(bz,fz)));
 	if(bz<fz){int temp=bz;bz=fz;fz=temp;}
 	block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 	if((tiles[fx][fz]==1 && tiles[bx][bz]==1)||fz<1||bz<1)
 	{
 		 		system("aplay -q  Broken_Robot3.wav &");

 		x=0;double ffy=fy,bby=by,bbx=bx,ffx=fx;int c=0;
 		while(c<100)
 		{
 			x-=4.0;c++;
 			block[0].totalrotate=((glm::translate(glm::vec3(0.5f,-1.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,1.5f,-0.5f))));
 	block[1].totalrotate=((glm::translate(glm::vec3(0.5f,-0.5f,0.5f)))*(glm::rotate((float)(x*M_PI/180.0f),glm::vec3(-1,0,0)))*(glm::translate(glm::vec3(-0.5f,0.5f,-0.5f))));;
 			draw();		
 			ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 			block[0].totalrotate=glm::mat4(1.0f);block[1].totalrotate=glm::mat4(1.0f);
 		} 		resetblock();

 	}
 	if(tiles[fx][fz]==4 || tiles[bx][bz]==4 || tiles[fx][fz]==3 || tiles[bx][bz]==3)
 	{
 		double ffy=fy,bby=by;int c=0;tiles[bx][bz]=1;
 		system("aplay -q  Broken_Robot3.wav &");
 		while(c<150)
 		{c++;
 		ffy-=0.1f;bby-=0.1f;block[0].totaltranslate=glm::translate(glm::vec3(fx-5,ffy,5-fz));block[1].totaltranslate=glm::translate(glm::vec3(bx-6,bby,5-bz));
 		draw();
 		}
 		if(tiles[bx][bz]==3){if(level==2)exit(1);changelevel();draw();}
 		resetblock();
 	}
	}
}
if(key=='v')
{
	view++;
	view=view%5;
	if(view!=2)follow=0;
	if(view!=3)helicam=0;
	if(view!=4 && viewsss!=3){Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);blockcam=0;}
	if(view==1)//top view
	{
			cx=0;cy=4;cz=4;
			lx=0;ly=0;lz=0;
	}
	else if(view==0)//tower view
	{
		cx=-3;cy=4;cz=9;
		lx=-0.15;ly=0;lz=0;
	}
	else if(view==2)//follow-cam view
	follow=1;
	else if(view==3){helicam=1;cx=0;cy=5;cz=5;lx=-1;ly=0;lz=0;azimuthal=45*(M_PI/180);elevation=45*(M_PI/180);Matrices.projection = glm::perspective (fov, (GLfloat) width / (GLfloat) height, 0.1f, 500.0f);}//helicam
	else if(view==4){cx=fx-5;cy=fy+2;cz=5-fz;lx=0;ly=-2;lz=0;blockcam=1;Matrices.projection = glm::perspective (fov, (GLfloat) width / (GLfloat) height, 0.1f, 500.0f);}

}
if(key=='b' && blockcam)
{
	blockview++;
	blockview=blockview%4;
}
if(key=='l')
{
	changelevel();
	system("aplay -q  Emerge1.wav &");
}
}
/* Executed when a regular key is released */
void keyboardUp (unsigned char key, int x, int y)
{
}
/* Executed when a special key is pressed */
void keyboardSpecialDown (int key, int x, int y)
{
	if(key==GLUT_KEY_LEFT && (glutGetModifiers() == GLUT_ACTIVE_CTRL))
		{
		
    }
    if(key==GLUT_KEY_RIGHT && (glutGetModifiers() == GLUT_ACTIVE_CTRL))
		{
		
    	}
    if(key==GLUT_KEY_LEFT && (glutGetModifiers() == GLUT_ACTIVE_ALT))
		{
		}
    if(key==GLUT_KEY_RIGHT && (glutGetModifiers() == GLUT_ACTIVE_ALT))
		{
		
    	}
}

/* Executed when a special key is released */
void keyboardSpecialUp (int key, int x, int y)
{
}
/* Executed when a mouse button 'button' is put into state 'state'
 at screen position ('x', 'y')
 */
double px=315,py=250,tx,ty,k=0.5,r=5*sqrt(2);
void mouseClick (int button, int state, int x, int y)
{
    if(button==3 && state == GLUT_DOWN)
     {
     		r--;
     		if(helicam)
     		{
     			cx=(cos(elevation)*sin(azimuthal))*r;
		cz=(cos(elevation)*cos(azimuthal))*r;
		cy=(sin(elevation))*r;
     		}
     }
     if(button==4 && state == GLUT_DOWN)
     {
     		r++;
     		if(helicam)
     		{
     			cx=(cos(elevation)*sin(azimuthal))*r;
		cz=(cos(elevation)*cos(azimuthal))*r;
		cy=(sin(elevation))*r;
     		}
     }
}
/* Executed when the mouse moves to position ('x', 'y') */
void mouseMotion (int x, int y)
{
	double xx=x,yy=y;
	if(helicam)
	{
		tx=(xx-px)*(M_PI/180)*k;
		ty=((yy-py)*(M_PI/180))*k;
		azimuthal-=tx;elevation-=ty;
		cx=(cos(elevation)*sin(azimuthal))*r;
		cz=(cos(elevation)*cos(azimuthal))*r;
		cy=(sin(elevation))*r;
		lx=0;ly=0;lz=0;
		px=xx;py=yy;
	}
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (int width, int height)
{

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) width, (GLsizei) height);

	// set the projection matrix as perspective/ortho
	// Store the projection matrix in a variable for future use

    // Perspective projection for 3D views
  //   Matrices.projection = glm::perspective (fov, (GLfloat) width / (GLfloat) height, 0.1f, 500.0f);

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
struct VAO* createrectangle()
{
	const GLfloat vertex_buffer_data [] = {
    -0.5,0.5,0,// vertex 1
    -0.5,-0.5,0, // vertex 2
    0.5,-0.5,0, // vertex 3

    0.5,-0.5,0, // vertex 3
    0.5,0.5,0, // vertex 4
    -0.5,0.5,0,  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.2,1,0, // color 1
    0.2,1,0, // color 2
    0.2,1,0, // color 3

    0.2,1,0, // color 3
    0.2,1,0, // color 4
    0.2,1,0  // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
struct VAO* createrectangle1()
{
	const GLfloat vertex_buffer_data [] = {
    -0.5,-0.5,0.5,// vertex 1
    -0.5,-0.5,-0.5, // vertex 2
    0.5,-0.5,-0.5, // vertex 3

    0.5,-0.5,-0.5, // vertex 3
    0.5,-0.5,0.5, // vertex 4
    -0.5,-0.5,0.5,  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
    0.6,0.6,0.6, // color 1
     0.6,0.6,0.6,// color 2
     0.6,0.6,0.6, // color 3

     0.6,0.6,0.6, // color 3
     0.6,0.6,0.6, // color 4
     0.6,0.6,0.6, // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
struct VAO* createrectangle2()
{
	const GLfloat vertex_buffer_data [] = {
    -0.5,-0.5,0.5,// vertex 1
    -0.5,-0.5,-0.5, // vertex 2
    0.5,-0.5,-0.5, // vertex 3

    0.5,-0.5,-0.5, // vertex 3
    0.5,-0.5,0.5, // vertex 4
    -0.5,-0.5,0.5,  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
     0.2,0.6,0.9, // color 1
     0.2,0.6,0.9,// color 2
     0.2,0.6,0.9, // color 3

     0.2,0.6,0.9, // color 3
     0.2,0.6,0.9, // color 4
     0.2,0.6,0.9, // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
struct VAO* createrectangle3()
{
	const GLfloat vertex_buffer_data [] = {
    -0.5,-0.5,0.5,// vertex 1
    -0.5,-0.5,-0.5, // vertex 2
    0.5,-0.5,-0.5, // vertex 3

    0.5,-0.5,-0.5, // vertex 3
    0.5,-0.5,0.5, // vertex 4
    -0.5,-0.5,0.5,  // vertex 1
  };
   static const GLfloat color_buffer_data [] = {
     0.9,0.3,0.3, // color 1
     0.9,0.3,0.3,// color 2
     0.9,0.3,0.3, // color 3

     0.9,0.3,0.3, // color 3
     0.9,0.3,0.3, // color 4
     0.9,0.3,0.3, // color 1
  };return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createblock()
{
		//Creating upper cube of the block
		block[0].top=createrectangle();block[0].bottom=createrectangle();block[0].right=createrectangle();
		block[0].left=createrectangle();block[0].near=createrectangle();block[0].far=createrectangle();
		block[0].neartranslate=glm::translate(glm::vec3(0.0f, 0.0f,0.5f));
		block[0].fartranslate=glm::translate(glm::vec3(0.0f, 0.0f,-0.5f));
		block[0].toptranslate=glm::translate(glm::vec3(0.0f, 0.5f,0.0f));
		block[0].bottomtranslate=glm::translate(glm::vec3(0.0f,-0.5f,0.0f));
		block[0].righttranslate=glm::translate(glm::vec3(0.5f, 0.0f,0.0f));
		block[0].lefttranslate=glm::translate(glm::vec3(-0.5f, 0.0f,0.0f));
		block[0].toprotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(1,0,0));
		block[0].bottomrotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(1,0,0));
		block[0].rightrotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,1,0));
		block[0].leftrotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,1,0));

		//Creating lower cube of the block
		block[1].top=createrectangle();block[1].bottom=createrectangle();block[1].right=createrectangle();
		block[1].left=createrectangle();block[1].near=createrectangle();block[1].far=createrectangle();
		block[1].neartranslate=glm::translate(glm::vec3(1.0f, 0.0f,0.5f));
		block[1].fartranslate=glm::translate(glm::vec3(1.0f, 0.0f,-0.5f));
		block[1].toptranslate=glm::translate(glm::vec3(1.0f, 0.5f,0.0f));
		block[1].bottomtranslate=glm::translate(glm::vec3(1.0f,-0.5f,0.0f));
		block[1].righttranslate=glm::translate(glm::vec3(1.5f, 0.0f,0.0f));
		block[1].lefttranslate=glm::translate(glm::vec3(0.5f, 0.0f,0.0f));
		block[1].toprotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(1,0,0));
		block[1].bottomrotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(1,0,0));
		block[1].rightrotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,1,0));
		block[1].leftrotate=glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,1,0));
}
void moveright()
{

}
double camera_rotation_angle=0,triangle_rotation,rectangle_rotation,triangle_rot_dir,triangle_rot_status;
void draw ()
{
  if(follow){cz=(-fz+5)+14;cx=fx-5;cy=10;lx=fx-5;ly=fy;lz=5-fz;}
  if(blockcam)
  {
  	if(blockview==0)//front view
  	{cx=fx-5;cy=fy+0.5;cz=5-bz-0.5;lx=cx;ly=0;lz=cz-2;}
  	else if(blockview==1)//right view
  	{cx=bx-5+0.5;cy=fy+0.5;cz=5-bz;lx=cx+2;ly=0;lz=cz;}
  	else if(blockview==2)
  	{cx=fx-5;cy=fy+0.5;cz=5-fz+0.5;lx=cx;ly=0;lz=cz+2;}
  	else if(blockview==3)
  	{cx=fx-5-0.5;cy=fy+0.5;cz=5-bz;lx=cx-2;ly=0;lz=cz;}
  }
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);
  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye (cx, cy, cz);
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (lx, ly, lz);
  // Up - Up vector definees tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);
  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(cx,cy,cz), glm::vec3(lx,ly,lz), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
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
//  glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
 // glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
 // glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
 // Matrices.model *= triangleTransform; 
 // MVP = VP * Matrices.model; // MVP = p * V * M
  //  Don't change unless you are sure!!
 // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
 // draw3DObject(triangle);
  if(level==0)
  createlevel1();
  else if(level==2)
  createlevel3();
  else if(level==1)createlevel2();
  //Drawing the Block
  block[0].totaltransform=block[0].totaltranslate*block[0].totalrotate;
  block[1].totaltransform=block[1].totaltranslate*block[1].totalrotate;
  Matrices.model = glm::mat4(1.0f);Matrices.model *= (block[0].totaltransform*(block[0].neartranslate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[0].near);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[0].totaltransform*(block[0].righttranslate)*(block[0].rightrotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[0].right);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[0].totaltransform*(block[0].lefttranslate)*(block[0].leftrotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[0].left);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[0].totaltransform*(block[0].fartranslate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[0].far);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[0].totaltransform*(block[0].toptranslate)*(block[0].toprotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[0].top);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[0].totaltransform*(block[0].bottomtranslate)*(block[0].bottomrotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[0].bottom);
  Matrices.model = glm::mat4(1.0f);Matrices.model *= (block[1].totaltransform*(block[1].neartranslate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[1].near);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[1].totaltransform*(block[1].righttranslate)*(block[1].rightrotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[1].right);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[1].totaltransform*(block[1].lefttranslate)*(block[1].leftrotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[1].left);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[1].totaltransform*(block[1].fartranslate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[1].far);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[1].totaltransform*(block[1].toptranslate)*(block[1].toprotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[1].top);
  Matrices.model=glm::mat4(1.0f);Matrices.model*=(block[1].totaltransform*(block[1].bottomtranslate)*(block[1].bottomrotate)); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(block[1].bottom);
  //Finished Drawing the Block

  //Draw the board
  	double cc=0.0f;
  	if((fx==6 && fz==4) || (bx==6 && bz==4) || bridge1)
  	{
  		if(level==2)
  		{
  		tiles[6][5]=tiles[6][6]=0;bridge1=1;
  		}
  	}
  	if((fx==3 && fz==4) || (bx==3 && bz==4) || bridge2)
  	{
  		if(level==2)
 		{  		tiles[4][4]=tiles[5][4]=0;bridge2=1;
  		}
  	}
  	for(int i=1;i<=10;i++)
  	{
  		for(int j=1;j<=10;j++)
  		{
  			if(tiles[i][j]==0){
  			board[i][j][0].cell=createrectangle1();
  			board[i][j][0].celltranslate=glm::translate(glm::vec3(i-5,0.0f,5-j));
  			Matrices.model=glm::mat4(1.0f);Matrices.model*=(board[i][j][0].celltranslate); MVP = VP * Matrices.model;
  			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  			draw3DObject(board[i][j][0].cell);}
  			else if(tiles[i][j]==2)
  			{
  			board[i][j][0].cell=createrectangle2();
  			board[i][j][0].celltranslate=glm::translate(glm::vec3(i-5,0.0f,5-j));
  			Matrices.model=glm::mat4(1.0f);Matrices.model*=(board[i][j][0].celltranslate); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  			draw3DObject(board[i][j][0].cell);
  			}	
  			else if(tiles[i][j]==4)
  			{
  				board[i][j][0].cell=createrectangle3();
  			board[i][j][0].celltranslate=glm::translate(glm::vec3(i-5,0.0f,5-j));
  			Matrices.model=glm::mat4(1.0f);Matrices.model*=(board[i][j][0].celltranslate); MVP = VP * Matrices.model;glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  			draw3DObject(board[i][j][0].cell);
  			}
  		}
  	}
  	
  //Finished drawing board
  glutSwapBuffers ();

  // Increment angles
  float increments = 1;

 // camera_rotation_angle++; // Simulating camera rotation
//  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
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
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");
	reshapeWindow (width, height);
	// Background color of the scene
	glClearColor (0.0f,0.0f,0.0f,0.0f); // R, G, B, A
	glClearDepth (1.0f);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}
int main (int argc, char** argv)
{
	width = 630;
	height = 500;
	tiles[3][10]=tiles[5][9]=tiles[7][9]=tiles[7][10]=tiles[4][2]=4;
	haligned=1;fz=bz=1;fx=1;bx=2;fy=0;by=0;
	block[0].totaltranslate=glm::translate(glm::vec3(fx-5,0.0f,5-fz));
	block[1].totaltranslate=glm::translate(glm::vec3(bx-6,0.0f,5-bz));
    initGLUT (argc, argv, width, height);
    addGLUTMenus ();
    createblock();
	initGL (width, height);
    glutMainLoop ();
    	system("aplay -q  Emerge1.wav &");

    return 0;
}
