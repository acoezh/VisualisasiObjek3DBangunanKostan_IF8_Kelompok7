/* 
	Kelompok 7
	Ahmad Kosasih	10109349
	Rizki Fauzan	10109318
	Diandra Osakina	10109358
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#endif

static GLfloat spin, spin2 = 0.0;
float angle = 0;
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int viewx = -302;
static int viewy = 25; 
static int viewz = 79;
static int centerx = 305;
static int centery = 0;
static int centerz = -10;

float rot = 0;
char * Tekstur[] = {"teksturRumput.bmp", "teksturJalan.bmp", "teksturDaun.bmp"}; //file tekstur
GLuint texture[3]; //array untuk texture
int gerakBus = -250;
int gerakMobil = -250;
int gerakTaxi = -100;

static GLfloat warnaLampuMerah[] = {1.0, 0.0, 0.0};
static GLfloat warnaLampuHijau[] = {0.0, 1.0, 0.0};
static GLfloat warnaLampuKuning[] = {1.0, 1.0, 0.0};

struct Images {//tempat image
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};
typedef struct Images Images;

//class untuk terain 2D
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;//memperhalus
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//end class
//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
//load terain di procedure inisialisasi
Terrain* loadTerrain(const char* filename, float height) {//ngambil file
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
//buat tipe data terain
Terrain* _terrainAir;
Terrain* _terrain;//pariable terain
Terrain* _terrainTanah;
Terrain* _terrainJalan;


void cleanup() {//untuk mehilangin file image
    delete _terrainJalan;
    delete _terrainAir;
	delete _terrain;
	delete _terrainTanah;
}
//mengambil gambar BMP
int ImageLoad(char *filename, Images *image) {
	FILE *file;
	unsigned long size; // ukuran image dalam bytes
	unsigned long i; // standard counter.
	unsigned short int plane; // number of planes in image

	unsigned short int bpp; // jumlah bits per pixel
	char temp; // temporary color storage for var warna sementara untuk memastikan filenya ada


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}
	// mencari file header bmp
	fseek(file, 18, SEEK_CUR);
	// read the width
	if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {//lebar beda
		printf("Error reading width from %s.\n", filename);
		return 0;
	}
	//printf("Width of %s: %lu\n", filename, image->sizeX);
	// membaca nilai height
	if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {//tingginya beda
		printf("Error reading height from %s.\n", filename);
		return 0;
	}
	//printf("Height of %s: %lu\n", filename, image->sizeY);
	//menghitung ukuran image(asumsi 24 bits or 3 bytes per pixel).

	size = image->sizeX * image->sizeY * 3;
	// read the planes
	if ((fread(&plane, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);//bukan file bmp
		return 0;
	}
	if (plane != 1) {
		printf("Planes from %s is not 1: %u\n", filename, plane);//
		return 0;
	}
	// read the bitsperpixel
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);

		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);//bukan 24 pixel
		return 0;
	}
	// seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);
	// read the data.
	image->data = (char *) malloc(size);
	if (image->data == NULL) {
		printf("Error allocating memory for color-corrected image data");//gagal ambil memory
		return 0;
	}
	if ((i = fread(image->data, size, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
	}
	for (i = 0; i < size; i += 3) { // membalikan semuan nilai warna (gbr - > rgb)
		temp = image->data[i];
		image->data[i] = image->data[i + 2];
		image->data[i + 2] = temp;
	}
	// we're done.
	return 1;
}




//mengambil tekstur
Images * loadTexture(char* filename) {
	Images *image1;
	// alokasi memmory untuk tekstur
	image1 = (Images *) malloc(sizeof(Images));
	if (image1 == NULL) {
		printf("Error allocating space for image");//memory tidak cukup
		exit(0);
	}
	//pic.bmp is a 64x64 picture
	if (!ImageLoad(filename, image1)) {
		exit(1);
	}
	return image1;
}



void initRendering() {//inisialisasi
	glEnable(GL_DEPTH_TEST);//kedalaman
	glEnable(GL_COLOR_MATERIAL);//warna
	glEnable(GL_LIGHTING);//cahaya
	glEnable(GL_LIGHT0);//lampu
	glEnable(GL_NORMALIZE);//normalisasi
	glShadeModel(GL_SMOOTH);//smoth
}

void drawScene() {//buat terain
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float scale = 500.0f / max(_terrain->width() - 1, _terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (_terrain->width() - 1) / 2, 0.0f,
			-(float) (_terrain->length() - 1) / 2);

	glColor3f(0.3f, 0.9f, 0.0f);
	for (int z = 0; z < _terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < _terrain->width(); x++) {
			Vec3f normal = _terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z), z);
			normal = _terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}


//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {

	float scale = 500.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

GLuint loadtextures(const char *filename, int width, int height) {//buat ngambil dari file image untuk jadi texture
	GLuint texture;

	unsigned char *data;
	FILE *file;


	file = fopen(filename, "rb");
	if (file == NULL)
		return 0;

	data = (unsigned char *) malloc(width * height * 3);  //file pendukung texture
	fread(data, width * height * 3, 1, file);

	fclose(file);

	glGenTextures(1, &texture);//generet (dibentuk)
	glBindTexture(GL_TEXTURE_2D, texture);//binding (gabung)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_NEAREST);//untuk membaca gambar jadi text dan dapat dibaca dengan pixel
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameterf(  GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB,
			GL_UNSIGNED_BYTE, data);

	data = NULL;

	return texture;
}

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
//const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 0.1f, 0.1f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };





void rumah(void) {
 
   //atap
	/*
    glPushMatrix();
    glScaled(0.8, 1.0, 0.8);
    glTranslatef(0.0, 4.85, -1.9);
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
    glutSolidCone(4.2, 1.5, 4, 1);
    glPopMatrix();
	*/
  
   //atap
	/*
    glPushMatrix();
    glScaled(0.8, 1.0, 0.8);
    glTranslatef(0.0, 4.85, 2.1);
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
    glutSolidCone(4.2, 1.5, 4, 1);
    glPopMatrix();
	*/
  

   //lantai 1  
    glPushMatrix();
    glScaled(1.115, 0.03, 2.2);
    glTranslatef(0.0, 0, 0.0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    //lantai 2 depan
    glPushMatrix();
    glScaled(1.015, 0.03, 1.2);
    glTranslatef(0.0,80, 1.7);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    //lantai 2 belakang
    glPushMatrix();
    glScaled(0.5, 0.03, 0.8);
    glTranslatef(2.5,80, -2.8);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    //lantai 3 
    glPushMatrix();
    glScaled(1.015, 0.03, 1.8);
    glTranslatef(0.0,160, 0.3);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    
  //Dinding Kiri Bawah
    glPushMatrix();
    glScaled(0.035, 0.5, 1.6);
    glTranslatef(-70.0, 2.45, 0.0);   
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(5.0);
    glPopMatrix();
  
  //Dinding Kanan Bawah
    glPushMatrix();
    glScaled(0.035, 0.5, 1.6);
    glTranslatef(70.0, 2.45, 0.0);  
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);  
    glutSolidCube(5.0);
    glPopMatrix();
  
//Dinding Kiri Atas
    glPushMatrix();
    glScaled(0.035, 0.5, 1.8);
    glTranslatef(-70.0, 7.45, 0.3);   
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174); 
    glutSolidCube(5.0);
    glPopMatrix();
  
//Dinding Kanan Atas
    glPushMatrix();
    glScaled(0.035, 0.5, 1.8);
    glTranslatef(70.0, 7.45, 0.3); 
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);  
    glutSolidCube(5.0);
    glPopMatrix();  
   
  //Dinding Belakang bawah
    glPushMatrix();
    //glScaled(0.035, 0.5, 0.8);
    glScaled(1.015, 0.5, 0.07);
    glTranslatef(0, 2.45,-58);  
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);  
    glutSolidCube(5.0);
    glPopMatrix();
    
  //Dinding Belakang atas
    glPushMatrix();
    //glScaled(0.035, 0.5, 0.8);
    glScaled(1.015, 0.5, 0.07);
    glTranslatef(0, 7.45,-58);  
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);  
    glutSolidCube(5.0);
    glPopMatrix();
    
    
    

  //Dinding Depan bawah
    glPushMatrix();
    glScaled(1.015, 0.5, 0.035);
    glTranslatef(0, 2.45,116); 
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);   
    glutSolidCube(5.0);
    glPopMatrix();

  //Dinding Depan atas
    glPushMatrix();
    glScaled(1.015, 0.5, 0.035);
    glTranslatef(0, 7.45,116);  
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);  
    glutSolidCube(5.0);
    glPopMatrix();

  //list hitam atas
    glPushMatrix();
    glScaled(0.35, 0.5, 0.035);
   glTranslatef(1, 7.2,124); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.1412, 0.1389, 0.1356);
    glutSolidCube(5.0);
    glPopMatrix();

  //list hitam atas
    glPushMatrix();
    glScaled(0.35, 0.43, 0.035);
   glTranslatef(1, 3.5,124); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.1412, 0.1389, 0.1356);
    glutSolidCube(5.0);
    glPopMatrix();

    //pintu atas
    glPushMatrix();
    glScaled(0.18, 0.35, 0.035);
   glTranslatef(-8, 9.5,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.0980, 0.0608, 0.0077);
    glutSolidCube(5.0);
    glPopMatrix();   
    
  //pintu bawah
    glPushMatrix();
    glScaled(0.18, 0.35, 0.035);
   glTranslatef(-8, 2.5,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.0980, 0.0608, 0.0077);
    glutSolidCube(5.0);
    glPopMatrix();
    
   //alis
    glPushMatrix();
    glScaled(0.18, 0.017, 0.035);
   glTranslatef(-8, 110.5,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0, 0, 0);
    glutSolidCube(5.0);
    glPopMatrix();
    
   //alis atas kiri
    glPushMatrix();
    glScaled(0.18, 0.017, 0.035);
   glTranslatef(-8, 254,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     //glColor3f(0.3402, 0.3412, 0.3117);
     glColor3f(0, 0, 0);
    glutSolidCube(5.0);
    glPopMatrix();
    
   //alis atas kanan
    glPushMatrix();
    glScaled(0.10, 0.017, 0.035);
    glTranslatef(18, 254,118); 
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0, 0, 0);
    glutSolidCube(5.0);
    glPopMatrix();    
 
   //alis jedela atas
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(22.5, 245,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();      
  
    //alis jedela bawah
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(22.5, 165,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();     
  
    //alis jedela kiri
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(96.6, 12.5,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();   
   
    //alis jedela kanan
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(115.1, 12.5,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();
    
//jendela bawah (kanan Bawah)
   //alis atas kanan (kanan Bawah)
    glPushMatrix();
    glScaled(0.10, 0.017, 0.035);
   glTranslatef(18, 110.5,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
       glColor3f(0, 0, 0);
    glutSolidCube(5.0);
    glPopMatrix();  
    
//alis jedela atas (kanan Bawah)
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(22.5, 101.5,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();      
  
    //alis jedela bawah (kanan Bawah)
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(22.5, 22.0,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();     
  
    //alis jedela kiri (kanan Bawah)
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(96.6, 3.8,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();   
   
    //alis jedela kanan (kanan Bawah)
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(115.1, 3.8,118); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix();    
    
//alis jedela atas (tengah1)
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(0, 119.5,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();  
    
    //alis jedela bawah (tengah1)
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(0, 40.0,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();     
  
    //alis jedela kiri (tengah1)
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(-9.6, 4.8,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();   
   
    //alis jedela kanan (tengah1)
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(9.5, 4.8,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix(); 

//alis jedela atas (tengah2)
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(9, 119.5,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();  
    
    //alis jedela bawah (tengah2)
    glPushMatrix();
    glScaled(0.08, 0.017, 0.035);
   glTranslatef(9, 40.0,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();     
  
    //alis jedela kiri (tengah2)
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(33, 4.8,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();   
   
    //alis jedela kanan (tengah2)
    glPushMatrix();
    glScaled(0.017,0.28, 0.035);
   glTranslatef(51.7, 4.8,128); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();     
    
    
    
   //alis tiang kiri atas orange
    glPushMatrix();
    glScaled(0.06, 0.037, 0.095);
   glTranslatef(-41, 115,51.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();
        
   //alis tiang kiri bawah orange
    glPushMatrix();
    glScaled(0.06, 0.037, 0.095);
   glTranslatef(-41, 80,51.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();     

    //alis tiang kanan atas orange
    glPushMatrix();
    glScaled(0.06, 0.037, 0.095);
   glTranslatef(41, 115,51.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();
    
   //alis tiang kanan bawah orange
    glPushMatrix();
    glScaled(0.06, 0.037, 0.095);
   glTranslatef(41, 80,51.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();      

    //orange 3 di tengah
    glPushMatrix();
    glScaled(0.017,0.33, 0.035);
   glTranslatef(-16.6, 12,125); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();     

    //orange 3 di tengah
    glPushMatrix();
    glScaled(0.017,0.33, 0.035);
   glTranslatef(-6.6, 12,125); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();  
    
//orange 3 di tengah
    glPushMatrix();
    glScaled(0.017,0.33, 0.035);
   glTranslatef(3.6, 12,125); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();  

    //pagar atas 1
    glPushMatrix();
    glScaled(.88, 0.017, 0.017);
   glTranslatef(-.01, 149,290); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 

    //pagar atas 1
    glPushMatrix();
    glScaled(.88, 0.017, 0.017);
   glTranslatef(-.01, 159,290); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();   
    
    //pagar atas 1
    glPushMatrix();
    glScaled(.88, 0.017, 0.017);
   glTranslatef(-.01, 169,290); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
       
    //pagar atas 1
    glPushMatrix();
    glScaled(.88, 0.017, 0.017);
   glTranslatef(-.01, 179,290); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(0.3402, 0.3412, 0.3117);
    glutSolidCube(5.0);
    glPopMatrix(); 
       
       
    //lampu kanan atas
    glPushMatrix();
    glScaled(0.05, 0.05, 0.05);
    glTranslatef(34.5, 95.4, 96);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); //untuk memunculkan warna
    glColor3ub(252, 243, 169);
    glutSolidSphere(2.0,20,50);
    glPopMatrix();       
      
    //lampu kiri atas
    glPushMatrix();
    glScaled(0.05, 0.05, 0.05);
    glTranslatef(-32.5, 95.4, 96);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3ub(252, 243, 169);
    glutSolidSphere(2.0,20,50);
    glPopMatrix();       

    //lampu kanan atas
    glPushMatrix();
    glScaled(0.05, 0.05, 0.05);
    glTranslatef(34.5, 47, 96);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); //untuk memunculkan warna
    glColor3ub(252, 243, 169);
    glutSolidSphere(2.0,20,50);
    glPopMatrix();       
      
    //lampu kiri atas
    glPushMatrix();
    glScaled(0.05, 0.05, 0.05);
    glTranslatef(-32.5, 47, 96);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3ub(252, 243, 169);
    glutSolidSphere(2.0,20,50);
    glPopMatrix(); 
    
    //pagar bawah
    glPushMatrix();
    glScaled(.7, 0.017, 0.017);
   glTranslatef(1, 50,400); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    //pagar bawah
    glPushMatrix();
    glScaled(.7, 0.017, 0.017);
   glTranslatef(1, 40,400); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    //pagar bawah
    glPushMatrix();
    glScaled(.7, 0.017, 0.017);
   glTranslatef(1, 30,400); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    //pagar bawah
    glPushMatrix();
    glScaled(.7, 0.017, 0.017);
   glTranslatef(1, 20,400); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
        
   //pagar bawah
    glPushMatrix();
    glScaled(.7, 0.017, 0.017);
   glTranslatef(1, 10,400); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
            
    // Batang Tiang Kanan
    glPushMatrix();
    glScaled(0.06, 0.2,0.06);
   glTranslatef(43, 3,115.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();    
    
    // Batang Tiang Kiri 1
    glPushMatrix();
    glScaled(0.06, 0.2,0.06);
   glTranslatef(-42, 3,115.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();    
    
    // Batang Tiang Kiri 2
    glPushMatrix();
    glScaled(0.06, 0.2,0.06);
   glTranslatef(-20, 3,115.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();      
      
}
void pohon(void){
//batang
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);    

glPushMatrix();
glColor3ub(104,70,14);
glRotatef(270,1,0,0);
gluCylinder(pObj, 4, 0.7, 30, 25, 25);
glPopMatrix();
}

//ranting  
void ranting(void){
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH); 
glPushMatrix();
glColor3ub(104,70,14);
glTranslatef(0,27,0);
glRotatef(330,1,0,0);
gluCylinder(pObj, 0.6, 0.1, 15, 25, 25);
glPopMatrix();

//daun
glEnable(GL_TEXTURE_2D);
glPushMatrix();
glColor3ub(18,118,13);
glBindTexture(GL_TEXTURE_2D, texture[2]);
glScaled(5, 5, 5);
glTranslatef(0,7,3);
glutSolidDodecahedron();
glPopMatrix();
glDisable(GL_TEXTURE_2D);
}

void mobil()
{
	//bodi
	glColor3f(1.0, 1.0, 0.0);
    glPushMatrix();
    //glRotatef(sudutk, 0.0, 0.0, 1.0);
    glTranslatef(0.0, 3.8, 0.0);
    glScalef(4.0, 1.0, 1.5);
    glutSolidCube(6.0f);
    glPopMatrix();

	glColor3f(0.0, 0.0, 0.0);
    glPushMatrix();
    //glRotatef(sudutk, 0.0, 0.0, 1.0);
    glTranslatef(0.0, 6.8, 0.0);
    glScalef(3.0, 3.0, 2.5);
    glutSolidCube(3.0f);
    glPopMatrix();

	//Lampu Depan
    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(12.1, 2.0, -3.5);
    glScalef(0.05, 0.02, 0.1);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(12.1, 2.0, 3.5);
    glScalef(0.05, 0.02, 0.1);
    glutSolidCube(6.0f);
    glPopMatrix();

	//kaca depan
	glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(5.3, 8.8, 0.0);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    //flat nomor Depan
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(12.0, 5.0, 0.0);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();

	//flat nomor
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(-12.0, 5.0, 0.0);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();

	//Lampu Rem
    glColor3f(1.0, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(-12.1, 2.0, -3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

	glColor3f(1.0, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(-12.1, 2.0, 3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

	//Ban Belakang
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(25.0, 4.5, 19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();

    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(17.5, 2.5, 8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(25.0, 4.5, -19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-17.5, 2.5, 8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();


    //Ban Depan
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-25.0, 4.5, -19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(17.5, 2.5, -8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-25.0, 4.5, 19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-17.5, 2.5, -8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();


}

void bus()
{
    //Bodi
    glColor3f(0.0, 0.0, 0.0);
    glPushMatrix();
    //glRotatef(sudutk, 0.0, 0.0, 1.0);
    glTranslatef(0.0, 3.8, 0.0);
    glScalef(4.0, 1.0, 1.5);
    glutSolidCube(6.0f);
    glPopMatrix();

	//lampu
	glColor3f(0.0,0.0,1.0);
	glPushMatrix();
	glTranslatef(10.0, 7.9, -2.2);
	glScaled(2.0,2.0,2.0);
	glutSolidCube(1.0f);
	glPopMatrix();

	glColor3f(1.0,0.0,0.0);
	glPushMatrix();
	glTranslatef(10.0, 7.9, 3.2);
	glScaled(2.0,2.0,2.0);
	glutSolidCube(1.0f);
	glPopMatrix();

    //Kaca Belakang
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(-12.0, 5.0, 0.0);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Lampu Rem
    glColor3f(1.0, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(-12.1, 2.0, -3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1.0, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(-12.1, 2.0, 3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Lampu Depan
    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(12.1, 2.0, -3.5);
    glScalef(0.05, 0.02, 0.1);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(12.1, 2.0, 3.5);
    glScalef(0.05, 0.02, 0.1);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Kaca Depan
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(12.0, 5.0, 0.0);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Kaca pinggir
    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(-1.5, 5.0, 4.0);
    glScalef(3.3, 0.5, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(0.0, 0.0, 1.0);
    glPushMatrix();
    glTranslatef(-1.5, 5.0, -4.0);
    glScalef(3.3, 0.5, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Pintu Kanan
    glColor3f(0.5, 0.5, 0.5);
    glPushMatrix();
    glTranslatef(11.1, 3.9, 4.0);
    glScalef(0.15, 0.8, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(10.1, 5.1, 4.0);
    glScalef(0.25, 0.4, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Pintu Kiri
    glColor3f(0.5, 0.5, 0.5);
    glPushMatrix();
    glTranslatef(11.1, 3.9, -4.0);
    glScalef(0.15, 0.8, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(10.1, 5.1, -4.0);
    glScalef(0.25, 0.4, 0.2);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Ban Belakang
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(25.0, 4.5, 19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(17.5, 2.5, 8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(25.0, 4.5, -19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-17.5, 2.5, 8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();


    //Ban Depan
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-25.0, 4.5, -19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(17.5, 2.5, -8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-25.0, 4.5, 19.3);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-17.5, 2.5, -8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();
}


void kursi(void){
    // Batang Tiang Kanan
    glPushMatrix();
    glScaled(0.06, 0.2,0.06);
   glTranslatef(43, 0,380.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1,1,1); 
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    
    // Batang Tiang Kiri
    glPushMatrix();
    glScaled(0.06, 0.2,0.06);
   glTranslatef(3, 0,380.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1,1,1); 
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    // Batang depan knan
    glPushMatrix();
    glScaled(0.06, 0.2,0.06);
   glTranslatef(43, 0,390.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
    // Batang Depan Kiri
    glPushMatrix();
    glScaled(0.06, 0.2,0.06);
   glTranslatef(3, 0,390.5); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1,1,1); 
    glutSolidCube(5.0);
    glPopMatrix();     

    // atas kursi
    glPushMatrix();
    glScaled(0.6, 0.05,0.3);
   glTranslatef(2.4,8,77); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();  
   
}
void markajalan(void) {

    // marka jalan
    glPushMatrix();
    glScaled(1, 0.05,0.3);
   glTranslatef(2.4,2.5,67); 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
     glColor3f(1,1,1);
    glutSolidCube(5.0);
    glPopMatrix(); 
    
} 



void awan(void){
glPushMatrix(); 
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glColor3ub(153, 223, 255);
glutSolidSphere(10, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(10,0,1);
glutSolidSphere(5, 50, 50);
glPopMatrix();   
glPushMatrix();
glTranslatef(-2,6,-2);
glutSolidSphere(7, 50, 50);
glPopMatrix();   
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(7, 50, 50);
glPopMatrix();  
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(7, 50, 50);
glPopMatrix();      
}     
        
void truk()
{
    //Bodi
    glColor3f(1.0,1.0,1.0);
    glPushMatrix();
    //glRotatef(sudutk, 0.0, 0.0, 1.0);
    glTranslatef(0.3, 4.0, 0.0);
    glScalef(2.5, 1.0, 1.5);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1.0,0.0,0.0);
    glPushMatrix();
    //glRotatef(sudutk, 0.0, 0.0, 1.0);
    glTranslatef(4.0, 3.8, 0.0);
    glScalef(3.0, 0.6, 1.3);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1.0,0.0,0.0);
    glPushMatrix();
    //glRotatef(sudutk, 0.0, 0.0, 1.0);
    glTranslatef(8.5,2.0, 0.1);
    glScalef(1.5, 0.5, 1.3);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Lampu Rem
    glColor3f(1,0.5,0.0);
    glPushMatrix();
    glTranslatef(-7.5, 2.0, -3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

    glColor3f(1,0.5,0.0);
    glPushMatrix();
    glTranslatef(-7.5, 2.0, 3.5);
    glScalef(0.02, 0.19, 0.08);
    glutSolidCube(6.0f);
    glPopMatrix();

    //Lampu Depan
    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(13.0, 1.4, -2.5);
    glScalef(7.5, 7.5, 7.5);
    glutSolidSphere(0.1, 5,10);
    glPopMatrix();

    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(13.0, 1.4, 2.5);
    glScalef(7.5, 7.5, 7.5);
    glutSolidSphere(0.1, 10,20);
    glPopMatrix();

    //Kaca Depan
    glColor3f(0.90,0.91,0.98);
    glPushMatrix();
    glTranslatef(13.0, 3.8, -0.4);
    glScalef(0.05, 0.5, 1.2);
    glutSolidCube(6.0f);
    glPopMatrix();
    //Kaca pinggir
    glColor3f(0.90,0.91,0.98);
    glPushMatrix();
    glTranslatef(8.5, 3.8, 4.0);
    glScalef(1.5, 0.5, 0.1);
    glutSolidCube(4.0f);
    glPopMatrix();

    glColor3f(0.90,0.91,0.98);
    glPushMatrix();
    glTranslatef(8.5, 3.8, -4.0);
    glScalef(1.5, 0.5, 0.1);
    glutSolidCube(4.0f);
    glPopMatrix();
//Ban Belakang
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-15, 1.5, 17.5);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(1.0,1.0,1.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-10.0, 0.5, 6.0);
    glutSolidSphere(2, 10 ,20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0,0.0,0.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(28.0, 1.5, -15.5);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(1.0,1.0,1.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(-12.0, 1.5, -8.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    //Ban Depan
    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(-17.0, 2.5, -17.5);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(1.0,1.0,1.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(20.0, 1.5, -7.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.35, 0.35, 0.25);
    glTranslatef(25.5, 1.5, 17.0);
    glutSolidTorus(2, 3, 20, 30);
    glPopMatrix();
    //Velg
    glPushMatrix();
    glColor3f(1.0,1.0,1.0);
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(18.0, 1.5, 7.0);
    glutSolidSphere(2, 10, 20);
    glPopMatrix();

}

void lampu()
{
   //Tiang Tegak
	glPushMatrix();
	glColor3f(0.5, 0.5, 0.5);
	glScalef(0.04,1.7,0.05);
	glutSolidCube(7.0f);
	glPopMatrix();

    //Tiang Atas
	glPushMatrix();
	glColor3f(0.5f, 0.5f, 0.5f);
	glTranslatef(0.0,5.3,-2.0);
    glScaled(0.5, 1.0 , 7.5);
    glutSolidCube(0.5f);
	glPopMatrix();

	//Lampu
	glPushMatrix();
	glTranslatef(0.0, 4.7, -3.7);
	glColor3f(0, 0, 0);
	glScalef(0.8,0.9,2.0);
	glutSolidCube(1.0f);
	glPopMatrix();

	//bola lampu kuning
	glPushMatrix();
	glTranslatef(0.0, 4.7, -3.7);
	glRotatef(90, 0.0, 4.7, 0.0);
	glColor3fv(warnaLampuKuning);
	glScalef(0.6,0.6, 0.8);
	glutSolidSphere(0.7, 10, 10);
	glPopMatrix();

	//bola lampu hijau
	glPushMatrix();
	glTranslatef(0.0, 4.7, -3.1);
	glRotatef(90, 0.0, 4.7, 0.0);
	glColor3fv(warnaLampuHijau);
	glScalef(0.6,0.6, 0.8);
	glutSolidSphere(0.7, 10, 10);
	glPopMatrix();

	//bola lampu merah
	glPushMatrix();
	glTranslatef(0.0, 4.7, -4.3);
	glRotatef(90, 0.0, 4.7, 0.0);
	glColor3fv(warnaLampuMerah);
	glScalef(0.6,0.6, 0.8);
	glutSolidSphere(0.7, 10, 10);
	glPopMatrix();
}

void display(void){
//    glutSwapBuffers();
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);

	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();//reset posisi
	gluLookAt(viewx, viewy, viewz, centerx, centery, centerz, 0.0, 1.0, 0.0);

	if (warnaLampuHijau[1] == 1.0 && gerakBus % 20 == 0) { 
		warnaLampuMerah[0] = 1.0; //merah nyala
		warnaLampuKuning[0] = 0.0; //kuning nyala
		warnaLampuKuning[1] = 0.0;
		warnaLampuHijau[1] = 0.0; //hijau padam

	} else if (warnaLampuMerah[0] == 1.0 && gerakBus % 20  == 0) {
		warnaLampuMerah[0] = 0.0; //merah padam
		warnaLampuKuning[0] = 1.0; //kuning nyala
		warnaLampuKuning[1] = 1.0;
		warnaLampuHijau[1] = 0.0;//hijau padam
	} else if (warnaLampuKuning[1] == 1.0 && gerakBus % 20 == 0) {
		warnaLampuMerah[0] = 0.0; //merah padam
		warnaLampuKuning[0] = 0.0; //kuning padam
		warnaLampuKuning[1] = 0.0;
		warnaLampuHijau[1] = 1.0;//hijau nyala
	}

    glPushMatrix();
    drawScene();
    glPopMatrix();

	glEnable(GL_TEXTURE_2D);

    glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

    glPushMatrix();

	glBindTexture(GL_TEXTURE_2D, texture[0]); //untuk mmanggil texture
	//drawSceneTanah(_terrain, 0.804f, 0.53999999f, 0.296f);
	drawSceneTanah(_terrain, 0.3f, 0.53999999f, 0.0654f);
	glPopMatrix();

	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_S);
	
	glDisable(GL_TEXTURE_2D);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, texture[1]); //untuk mmanggil texture
    drawSceneTanah(_terrainJalan, 0.4902f, 0.4683f,0.4594f);
    glPopMatrix();
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();

	/*
	glPushMatrix();
//	glBindTexture(GL_TEXTURE_2D, texture[0]);
	drawSceneTanah(_terrainTanah, 0.7f, 0.2f, 0.1f);
	glPopMatrix();
	*/

for (int i=0; i< 4; i++)
{
	//rumah 
	glPushMatrix();
	glTranslatef(-70 * i, 5, -10); 
	glScalef(5, 5, 5);
	//glBindTexture(GL_TEXTURE_2D, texture[0]);
	rumah();
	glPopMatrix();
}

if(gerakBus>=-600)
{
	glPushMatrix();
	glTranslatef(gerakBus,2,10);
	glTranslatef(350,5,70);
	glScaled(2,2,2);
	glRotatef(180,0,1,0);
	truk();
	glPopMatrix();
	gerakBus-=2;
} else gerakBus = -250;
printf("%d ", gerakBus);
	
if(gerakMobil<=250)
{
	glPushMatrix();
	glTranslatef(gerakMobil,5,60);
	gerakMobil+=2;
	glScaled(2,2,2);
    bus();
    glPopMatrix();
} else gerakMobil = -250;


if(gerakTaxi<=250)
{
	glPushMatrix();
	glTranslatef(gerakTaxi,2,10);
	gerakTaxi+=2;
	glTranslatef(50,5,50);
	glScaled(2,2,2);
	mobil();
    glPopMatrix();
} else gerakTaxi = -250;

//lampu
glPushMatrix();
glTranslatef(60,10,90); 
//glRotatef(180, 0, 0, 0);
glScalef(5, 5, 5);
//glBindTexture(GL_TEXTURE_2D, texture[0]);
lampu();
glPopMatrix();
// end lampu

//pohon2

glPushMatrix();
glTranslatef(35,0.5,-10);    
glScalef(0.5, 0.5, 0.5);
glRotatef(90,0,1,0);
pohon();

//ranting1
ranting();

//ranting2
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);   
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

//ranting3
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);   
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();

glPopMatrix();

//pohon3
glPushMatrix();
glTranslatef(55,0.2,-40);    
glScalef(0.7, 0.8, 0.8);
glRotatef(90,0,1,0);
pohon();

//ranting1
ranting();

//ranting2
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);   
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

//ranting3
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);   
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();

glPopMatrix();

//pohon4
glPushMatrix();
glTranslatef(145,0.2,-17);    
glScalef(0.7, 0.8, 0.8);
glRotatef(90,0,1,0);
pohon();

//ranting1 4
ranting();

//ranting2 4
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);   
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

//ranting3 4
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);   
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();

glPopMatrix();

//pohon5
glPushMatrix();
glTranslatef(115,0.5,-60);    
glScalef(0.5, 0.5, 0.5);
glRotatef(90,0,1,0);
pohon();

//ranting1 5
ranting();

//ranting2 5
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);   
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

//ranting3 5
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);   
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();

glPopMatrix();

//pohon6
glPushMatrix();
glTranslatef(35,0.5,100);    
glScalef(0.5, 0.5, 0.5);
glRotatef(90,0,1,0);
pohon();

//ranting1 6
ranting();

//ranting2 6
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);   
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

//ranting3 6
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);   
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();

glPopMatrix();

//pohon7
glPushMatrix();
glTranslatef(-15,0.5,100);    
glScalef(0.5, 0.5, 0.5);
glRotatef(90,0,1,0);
pohon();

//ranting1 7
ranting();

//ranting2 7
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);   
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

//ranting3 7
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);   
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();

glPopMatrix();

//pohon8
glPushMatrix();
glTranslatef(-65,0.5,100);    
glScalef(0.5, 0.5, 0.5);
glRotatef(90,0,1,0);
pohon();

//ranting1 8
ranting();

//ranting2 8
glPushMatrix();
glScalef(1.5, 1.5, 1.5);
glTranslatef(0,25,25);   
glRotatef(250,1,0,0);
ranting();
glPopMatrix();

//ranting3 8
glPushMatrix();
glScalef(1.8, 1.8, 1.8);
glTranslatef(0,-6,21.5);   
glRotatef(-55,1,0,0);
ranting();
glPopMatrix();

glPopMatrix();




//kursi nyamping
glPushMatrix();
glTranslatef(8,5,-15); 
glScalef(5, 5, 5);
    glRotated(90, 0, 1, 0);
//glBindTexture(GL_TEXTURE_2D, texture[0]);
kursi();
glPopMatrix();

//kursi1
glPushMatrix();
glTranslatef(0,5,-15); 
glScalef(5, 5, 5);
//glBindTexture(GL_TEXTURE_2D, texture[0]);
kursi();
glPopMatrix();


//kursi2
glPushMatrix();
glTranslatef(-50,5,-15); 
glScalef(5, 5, 5);
//glBindTexture(GL_TEXTURE_2D, texture[0]);
kursi();
glPopMatrix();

//kursi3
glPushMatrix();
glTranslatef(-50,4,55); 
glScalef(5, 5, 5);
//glBindTexture(GL_TEXTURE_2D, texture[0]);
kursi();
glPopMatrix();

//kursi4
glPushMatrix();
glTranslatef(10,4,55); 
glScalef(5, 5, 5);
//glBindTexture(GL_TEXTURE_2D, texture[0]);
kursi();
glPopMatrix();

for (int i = -250; i < 250; i+=30)
{
	//markajalan
	glPushMatrix();
	glTranslatef(i,1,9); 
	glScalef(3, 3, 3);
	//glBindTexture(GL_TEXTURE_2D, texture[0]);
	markajalan();
	glPopMatrix();
}

//awan
glPushMatrix();
glTranslatef(-75, 110, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-45, 110, -115);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-50, 120, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-140, 90, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-155, 90, -115);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-130, 110, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-190, 110, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-175, 120, -115);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-200, 100, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-30, 110, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-35, 95, -115);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-20, 90, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(-80, 90, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();
  
glPushMatrix();
glTranslatef(220, 90, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(180, 90, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();


glPushMatrix();
glTranslatef(190, 110, -120);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();

glPushMatrix();
glTranslatef(125, 110, -115);
glScalef(1.8, 1.0, 1.0);  
awan();
glPopMatrix();
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //disable the color mask
	glDepthMask(GL_FALSE); //disable the depth mask
	glEnable(GL_STENCIL_TEST); //enable the stencil testing
	glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); //set the stencil buffer to replace our next lot of data
	//ground
	//tanah(); //set the data plane to be replaced
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); //enable the color mask
	glDepthMask(GL_TRUE); //enable the depth mask
	glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); //set the stencil buffer to keep our next lot of data
	glDisable(GL_DEPTH_TEST); //disable depth testing of the reflection
	// glPopMatrix();  
	glEnable(GL_DEPTH_TEST); //enable the depth testing
	glDisable(GL_STENCIL_TEST); //disable the stencil testing
	//end of ground
	glEnable(GL_BLEND); //enable alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //set the blending function
	glRotated(1, 0, 0, 0);
	glDisable(GL_BLEND);
    glutSwapBuffers();//buffeer ke memory
	glFlush();//memaksakan untuk menampilkan
	rot++;
	angle++;
//glDisable(GL_COLOR_MATERIAL);
}

void init(void){
/*GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
GLfloat mat_shininess[] = {50.0};
GLfloat light_position[] = {1.0, 1.0, 1.0, 1.0};
     
glClearColor(0.0,0.0,0.0,0.0);

//glShadeModel(GL_FLAT);

glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
*/
glLightfv(GL_LIGHT0, GL_POSITION, light_position);
glDepthFunc(GL_LESS);
//glEnable(GL_NORMALIZE);
//glEnable(GL_COLOR_MATERIAL);
glDepthFunc(GL_LEQUAL);
//glShadeModel(GL_SMOOTH);
glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
glEnable(GL_CULL_FACE);
//glEnable(GL_TEXTURE_2D);
//glEnable(GL_TEXTURE_GEN_S);
//glEnable(GL_TEXTURE_GEN_T);

    initRendering();
        _terrainJalan = loadTerrain("ketinggianJalan.bmp", 20); //ketinggian jalan
        _terrainAir = loadTerrain("ketinggianAir.bmp", 20); 
	_terrain = loadTerrain("heightmap.bmp", 20);
	_terrainTanah = loadTerrain("ketinggianTanah.bmp", 20);
        
	//binding texture
glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// Generate texture/ membuat texture
	glGenTextures(1, texture);
	
for (int i = 0; i < 3; i++)
{
	Images *image1 = loadTexture(Tekstur[i]);

	if (image1 == NULL) {
		printf("Image was not returned from loadTexture\n");
		exit(0);
	}
	
//------------tekstur rumput---------------
	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[i]);
	//menyesuaikan ukuran textur ketika image lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //
	//menyesuaikan ukuran textur ketika image lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);
 // -- end texture
}

}

void reshape(int w, int h){
glViewport(0, 0 , (GLsizei) w,(GLsizei)h);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();

//glFrustum(-1.0,1.0,-1.0,1.0,1.5,20.0);
gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
glMatrixMode(GL_MODELVIEW);
//glLoadIdentity();
//gluLookAt(100.0,0.0,5.0,0.0,0.0,0.0,0.0,1.0,0.0);
}

static void kibor(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME:
		viewy++;
		break;
	case GLUT_KEY_END:
		viewy--;
		break;
	case GLUT_KEY_UP:
		viewx+=5;
		break;
	case GLUT_KEY_DOWN:
		viewx-=5;
		break;

	case GLUT_KEY_RIGHT:
		viewz++;
		break;
	case GLUT_KEY_LEFT:
		viewz--;
		break;

	case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2); //untuk lighting
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	default:
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'q') {
		viewz++;
	}
	if (key == 'e') {
		viewz--;
	}
	if (key == 's') {
		viewy--;
	}
	if (key == 'w') {
		viewy++;
	}
}

int main(int argc, char** argv){
glutInit(&argc, argv);
//glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
glutInitWindowSize(800,600);
glutInitWindowPosition(100,100);
glutCreateWindow("Kelompok Ahmad Kosasih");
init();
glutDisplayFunc(display);
glutIdleFunc(display);
glutReshapeFunc(reshape);

glutKeyboardFunc (keyboard);
glutSpecialFunc(kibor);

glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
glLightfv(GL_LIGHT0, GL_POSITION, light_position);

glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
glColorMaterial(GL_FRONT, GL_DIFFUSE);

glutMainLoop();
return 0;
}
