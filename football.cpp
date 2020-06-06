#include <bits/stdc++.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define KEY_ESC 27
using namespace std;

float pi = 3.1415926f;

int fullscreen = 0;
int mouseDown = 0;

int showGoal = 0;
int goal = 0;

// screen size
int width;
int height;

// game variables
int points = 0;
int shoot = 0;
int showHitPos = 0;

// hit pos
float hit_x = 0.0f;
float hit_y = 0.0f;
float hit_z = -300.0f;

// shoot circle
float shoot_circle_x = 0.0f;
float shoot_circle_y = 50.0f;
float shoot_circle_z = -250.0f;

// keeper
int keeper_dir = 5;
float keeper_x = 0.0f;
float keeper_z = -450.0f;
float keeper_rot_z = 0.0f;
float post_limit = 160;

// camera
int camera_pos = 2;
float camera_x = 0.0f;
float camera_y = 1500.0f;
float camera_z = 0.0f;
float camera_angle = 0.0f;

// penalty spot
float penalty_spot_x = 0.0f;
float penalty_spot_y = 1.0f;
float penalty_spot_z = -210.0f;

// ball
int ballmove = 0;
float ball_x = penalty_spot_x;
float ball_y = penalty_spot_y + 7;
float ball_z = penalty_spot_z;
float ball_dir = -1.0f;
float ball_vel_x = 5.0f;
float ball_vel_y = 10.0f;
float ball_vel_z = -10.0f;

float gravity = 0.98;

GLuint *textures;

void LoadTexture(GLuint texture, const char *filename)
{
	int width, height;

	unsigned char info[54];

	FILE *file;
	file = fopen(filename, "rb");
	if (file == NULL)
		return;
	fread(info, sizeof(unsigned char), 54, file);
	width = *(int *)&info[18];
	height = *(int *)&info[22];

	unsigned char *data;
	data = (unsigned char *)malloc(width * height * 3);

	fread(data, width * height * 3, 1, file);
	fclose(file);

	for (int i = 0; i < width * height; ++i)
	{
		int index = i * 3;
		unsigned char R, B;
		R = data[index];
		B = data[index + 2];

		data[index] = B;
		data[index + 2] = R;
	}

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
	free(data);
}

void bitmap_output(int x, int y, string string, void *font)
{
	int len, i;

	glRasterPos2f(x, y);
	len = string.length();
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(font, string[i]);
	}
}
void stroke_output(GLfloat x, GLfloat y, char *format, ...)
{
	va_list args;
	char buffer[200], *p;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	glPushMatrix();
	glTranslatef(x, y, 0);
	glScalef(0.5, 0.5, 0.5);
	glLineWidth(50);
	for (p = buffer; *p; p++)
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
	glutStrokeWidth(GLUT_STROKE_MONO_ROMAN, 65);
	glPopMatrix();
}

void vxHitCircle(GLfloat x, GLfloat y, GLfloat z, GLfloat radius)
{
	int i;
	int triangleAmount = 20; // of triangles used to draw circle
	GLfloat twicePi = 2.0f * pi;

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.9f, 0.1f, 0.0f);
	glVertex3f(x, y, z); // center of circle
	for (i = 0; i <= triangleAmount; i++)
	{
		glVertex3f(x + (radius * cos(i * twicePi / triangleAmount)),
				   y + (radius * sin(i * twicePi / triangleAmount)),
				   z);
	}
	glEnd();
}

void vxShootCircle(GLfloat x, GLfloat y, GLfloat z, GLfloat radius)
{
	int i;
	int triangleAmount = 10; // of triangles used to draw circle
	GLfloat twicePi = 2.0f * pi;

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(0.9f, 0.9f, 0.9f, 0.8f);
	glVertex3f(x, y, z); // center of circle
	for (i = 0; i <= triangleAmount; i++)
	{
		glVertex3f(x + (radius * cos(i * twicePi / triangleAmount)),
				   y + (radius * sin(i * twicePi / triangleAmount)),
				   z);
	}
	glEnd();
}
// power bar
void vxPower()
{
	glColor3f(0.9f, 0.3f, 0.0f);
	glLineWidth(50);

	glBegin(GL_LINES);
	glVertex3f(penalty_spot_x - 5, penalty_spot_y, penalty_spot_z + 15);
	glVertex3f(penalty_spot_x + abs(ball_vel_z) * 3.0f,
			   penalty_spot_y,
			   penalty_spot_z + 15);
	glEnd();
}
// direction bar
void vxDirectionBar()
{
	glColor3f(1.0f, 1.0f, 0.5f);
	glLineWidth(50);

	glBegin(GL_LINES);
	glVertex3f(penalty_spot_x, penalty_spot_y, penalty_spot_z);
	glVertex3f(penalty_spot_x + (ball_vel_x)*5,
			   penalty_spot_y + (ball_vel_y)*5,
			   penalty_spot_z - 15);
	glEnd();
}

// keeper-------------------------------------
void vxKeeper()
{
	glPushMatrix();
	float height = 50.0f, width = 15.0f, thickness = 10.0f;
	glTranslatef(keeper_x, 0, keeper_z);
	glRotatef(keeper_rot_z, 0.0f, 0.0f, 1.0f);
	glBegin(GL_QUADS);

	// FRONT
	glColor3f(.82f, .41f, .16f);
	glVertex3f(-width, -height, thickness);
	glVertex3f(width, -height, thickness);
	glVertex3f(width, height, thickness);
	glVertex3f(-width, height, thickness);

	// BACK
	glColor3f(.54f, .26f, .07f);
	thickness = -thickness;
	glVertex3f(-width, -height, thickness);
	glVertex3f(width, -height, thickness);
	glVertex3f(width, height, thickness);
	glVertex3f(-width, height, thickness);
	thickness = -thickness;

	// LEFT
	glVertex3f(-width, -height, thickness);
	glVertex3f(-width, height, thickness);
	glVertex3f(-width, height, -thickness);
	glVertex3f(-width, -height, -thickness);

	// RIGHT
	glVertex3f(width, -height, -thickness);
	glVertex3f(width, height, -thickness);
	glVertex3f(width, height, thickness);
	glVertex3f(width, -height, thickness);

	// TOP
	glColor3f(.49f, .26f, .07f);
	glVertex3f(-width, height, thickness);
	glVertex3f(width, height, thickness);
	glVertex3f(width, height, -thickness);
	glVertex3f(-width, height, -thickness);

	// BOTTOM
	glVertex3f(-width, -height, thickness);
	glVertex3f(-width, -height, -thickness);
	glVertex3f(width, -height, -thickness);
	glVertex3f(width, -height, thickness);

	glEnd();
	glTranslatef(-keeper_x, 0, -keeper_z);
	glRotatef(keeper_rot_z, 0.0f, 0.0f, -1.0f);
	glPopMatrix();
}

void moveKeeper()
{
	if (shoot)
	{
		if (abs(keeper_rot_z) < 90)
		{
			if (abs(keeper_rot_z) > 45)
			{
				// if (keeper_x > 0)
				// 	keeper_rot_z -= abs(keeper_rot_z * 0.2;
				// else
				keeper_rot_z += keeper_rot_z * 0.2;
			}
			else
			{
				keeper_rot_z += (keeper_x - ball_x) * 0.5;
			}
		}
		else
		{
			if (keeper_rot_z < 0)
				keeper_rot_z = -90.0f;
			else
				keeper_rot_z = 90.0f;
		}
		if (abs(keeper_rot_z) < 40)
		{
			if (ball_x <= keeper_x)
			{
				keeper_x -= abs(keeper_x - ball_x) * 0.9;
			}
			else
			{
				keeper_x += abs(keeper_x - ball_x) * 0.9;
			}
		}

		// keeper_x += keeper_dir;
		// if (keeper_x >= post_limit || keeper_x <= -post_limit)
		// {
		// 	keeper_dir = (-1) * keeper_dir;
		// }
	}
	else
	{
		keeper_rot_z = 0.0f;
		keeper_x = 0.0f;
	}
}
void drawKeeper()
{
	vxKeeper();
	moveKeeper();
}

void drawHitPos()
{
	if (showHitPos)
	{
		vxHitCircle(hit_x, hit_y, hit_z, 7);
	}
}

void drawShootingBars()
{
	if (!shoot)
	{
		vxDirectionBar();
		vxPower();
	}
}

// ball--------------------------------------
int checkCollision()
{
	// std::cout << cosf(abs(keeper_rot_z) / (2 * pi)) << endl;
	if (ball_z < keeper_z + 10 &&
		ball_x > keeper_x - 10 && ball_x < keeper_x + 10 &&
		ball_y <= 50.0 * cosf(abs(keeper_rot_z) / (2 * pi)))
		return 1;
	return 0;
}
void vxBall()
{
	glColor3f(0.8f, 0.8f, 0.8f);
	// glColor3f(1.0f, 0.6f, 0.2f);
	glTranslated(ball_x, ball_y, ball_z);
	glutSolidSphere(10, 20, 20);
	glTranslated(-ball_x, -ball_y, -ball_z);
}
void moveBall()
{

	if (shoot && ballmove)
	{
		if (checkCollision() && ball_z > -480)
		{
			ball_vel_x = 0.0f;
			ball_vel_y = 0.0f;
			ball_vel_z = -1 * ball_vel_z;
			ballmove = 0;
			return;
		}
		ball_x += ball_vel_x;
		ball_z += ball_vel_z;

		// confine in x direction
		if (ball_x > 160 || ball_x < -160)
		{
			ball_vel_x = -ball_vel_x * 0.9;
		}
		// if ball too slow, stop it
		if (abs(ball_vel_x) < 1)
		{
			ball_vel_x = 0.0f;
		}

		// confine in z direction
		if (ball_z < -480 || ball_z > 0)
		{
			ball_vel_z = -ball_vel_z * 0.9;
		}
		// if ball too slow, stop it
		if (abs(ball_vel_z) < 1)
		{
			ball_vel_z = 0.0f;
		}

		// show hit position
		if (ball_z <= -480 && showHitPos == 0)
		{
			if (abs(ball_x) < post_limit && abs(ball_y) < 150)
			{
				points++;
				goal = 1;
				showGoal = 1;
			}
			else
			{
				goal = 0;
			}
			showHitPos = 1;
			hit_x = ball_x;
			hit_y = ball_y;
			hit_z = ball_z;
		}

		// y movements
		ball_y += ball_vel_y;
		if (abs(ball_vel_y) > 1 || ball_y > 10)
			ball_vel_y -= gravity;
		if (ball_y < 10) // ground
		{
			if (abs(ball_vel_y) > 1)
				ball_vel_y = -ball_vel_y * 0.70;
			else
			{
				ball_vel_y = 0.0f;
			}
			ball_vel_x = ball_vel_x * 0.8;
			ball_vel_z = ball_vel_z * 0.8;
			// decrease velocity in all directions
		}
		if (abs(ball_vel_x) + abs(ball_vel_y) + abs(ball_vel_z) < 1)
		{
			// shoot = 0;
			ballmove = 0;
		}
	}
	else
	{
		ballmove = 0;
		ball_x = penalty_spot_x;
		ball_y = penalty_spot_y + 7;
		ball_z = penalty_spot_z;
	}
}
void drawBall()
{
	vxBall();
	moveBall();
}

// ground ------------------------------------------------------------
// penalty point
void vxFilledCircle(GLfloat x, GLfloat y, GLfloat z, GLfloat radius)
{
	int i;
	int triangleAmount = 20; // of triangles used to draw circle
	GLfloat twicePi = 2.0f * pi;

	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(x, 1, z); // center of circle
	for (i = 0; i <= triangleAmount; i++)
	{
		glVertex3f(x + (radius * cos(i * twicePi / triangleAmount)), y,
				   z + (radius * sin(i * twicePi / triangleAmount)));
	}
	glEnd();
}
// outer arc
void vxArc(float cx, float cz, float r)
{
	float degInRad = (pi / 90);
	for (int i = 0; i < 90; i++)
	{
		vxFilledCircle(cx + cos(i * degInRad) * r, 1, cz + sin(i * degInRad) * r, 4);
	}
}

// goal post
void vxPost()
{
	//using array of spheres
	float post_x = -float(post_limit);
	float post_y = 10.0f;
	float post_z = -455;
	float height = 100.0f;
	float angle = 0.0f;
	float r = 5.0f;
	glColor3f(0.9f, 0.9f, 0.9f);

	//left post
	glBegin(GL_POLYGON);
	for (int i = 0; i < 20; i++)
	{
		float x1 = r * cosf(angle);
		float z1 = r * sinf(angle);
		float x2 = r * cosf(angle + pi / 10);
		float z2 = r * sinf(angle + pi / 10);
		glVertex3f(post_x + x1, 0, post_z + z1);
		glVertex3f(post_x + x2, 0, post_z + z2);
		glVertex3f(post_x + x2, height, post_z + z2);
		glVertex3f(post_x + x1, height, post_z + z1);
		angle += pi / 10;
	}
	glEnd();

	// right post
	angle = 0.0f;
	post_x = 160.0f;

	glBegin(GL_POLYGON);
	for (int i = 0; i < 20; i++)
	{
		float x1 = r * cosf(angle);
		float z1 = r * sinf(angle);
		float x2 = r * cosf(angle + pi / 10);
		float z2 = r * sinf(angle + pi / 10);
		glVertex3f(post_x + x1, 0, post_z + z1);
		glVertex3f(post_x + x2, 0, post_z + z2);
		glVertex3f(post_x + x2, height, post_z + z2);
		glVertex3f(post_x + x1, height, post_z + z1);
		angle += pi / 10;
	}
	glEnd();

	// upper post
	angle = 0.0f;
	post_x = -165.0f;
	post_y = 100.0f;
	float post_length = 330.0f;
	glBegin(GL_POLYGON);
	for (int i = 0; i < 20; i++)
	{
		float y1 = r * cosf(angle);
		float z1 = r * sinf(angle);
		float y2 = r * cosf(angle + pi / 10);
		float z2 = r * sinf(angle + pi / 10);
		glVertex3f(post_x, post_y + y1, post_z + z1);
		glVertex3f(post_x, post_y + y2, post_z + z2);
		glVertex3f(post_x + post_length, post_y + y2, post_z + z2);
		glVertex3f(post_x + post_length, post_y + y1, post_z + z1);
		angle += pi / 10;
	}
	glEnd();
}
// ground white lines
void vxMarks()
{
	// back white line
	glColor3f(1.0f, 1.0f, 1.0f);
	glLineWidth(20);
	glBegin(GL_POLYGON);
	glVertex3f(-300, 1.0f, -460);
	glVertex3f(300, 1.0f, -460);
	glVertex3f(300, 1.0f, -450);
	glVertex3f(-300, 1.0f, -450);
	glEnd();

	//6 yard box x(-170 t0 170) z(-450 t0 -300)
	glBegin(GL_POLYGON);
	glVertex3f(-180, 1.0f, -450);
	glVertex3f(-180, 1.0f, -300);
	glVertex3f(-175, 1.0f, -300);
	glVertex3f(-175, 1.0f, -450);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex3f(180, 1.0f, -450);
	glVertex3f(180, 1.0f, -300);
	glVertex3f(175, 1.0f, -300);
	glVertex3f(175, 1.0f, -450);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex3f(-180, 1.0f, -300);
	glVertex3f(180, 1.0f, -300);
	glVertex3f(180, 1.0f, -290);
	glVertex3f(-180, 1.0f, -290);
	glEnd();

	//D box	x(-270 t0 270) z(-450 t0 -150)
	glBegin(GL_POLYGON);
	glVertex3f(-270, 1.0f, -450);
	glVertex3f(-270, 1.0f, -150);
	glVertex3f(-260, 1.0f, -150);
	glVertex3f(-260, 1.0f, -450);
	glEnd();
	glBegin(GL_POLYGON);
	glVertex3f(270, 1.0f, -450);
	glVertex3f(270, 1.0f, -150);
	glVertex3f(260, 1.0f, -150);
	glVertex3f(260, 1.0f, -450);
	glEnd();

	glBegin(GL_POLYGON);
	glVertex3f(-270, 1.0f, -150);
	glVertex3f(270, 1.0f, -150);
	glVertex3f(270, 1.0f, -140);
	glVertex3f(-270, 1.0f, -140);
	glEnd();

	//for arc of D box
	vxArc(0, -145, 65);

	//penalty spot
	vxFilledCircle(penalty_spot_x, penalty_spot_y, penalty_spot_z, 7);
}
void texturedSky()
{
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[2]);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-800.0, -200.0, -700.0);

	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(800.0, -200.0, -700.0);

	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(800.0, 600.0, -700.0);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-800.0, 600.0, -700.0);

	glEnd();
	glFlush();
	glDisable(GL_TEXTURE_2D);
}
void texturedStatus()
{
	// cout << 3 + goal << " " << ballmove << " " << shoot << endl;
	if (ballmove == 0 && shoot == 1)
	{

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0.0, 500, 0.0, 500);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		//
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glBindTexture(GL_TEXTURE_2D, textures[3 + goal]);

		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(100.0f, 170.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(400.0, 170.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(400.0f, 300.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(100.0f, 300.0);

		glEnd();
		glFlush();
		glDisable(GL_TEXTURE_2D);
		//
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
}

void texturedGround()
{
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-500.0, 0.0, 0.0);

	glTexCoord2f(0.0f, 12.0f);
	glVertex3f(-500.0, 0.0, -600.0);

	glTexCoord2f(12.0f, 12.0f);
	glVertex3f(500.0, 0.0, -600.0);

	glTexCoord2f(12.0f, 0.0f);
	glVertex3f(500.0, 0.0, 0.0);

	glEnd();
	glFlush();
	glDisable(GL_TEXTURE_2D);
}

void texturedBoundary()
{

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[1]);

	// back boundary
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-300, 0, -500);

	glTexCoord2f(5.0f, 0.0f);
	glVertex3f(300, 0, -500);

	glTexCoord2f(5.0f, 1.0f);
	glVertex3f(300, 50, -500);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-300, 50, -500);
	glEnd();

	// left side boundary
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-300, 0, -500);

	glTexCoord2f(5.0f, 0.0f);
	glVertex3f(-300, 0, 0);

	glTexCoord2f(5.0f, 1.0f);
	glVertex3f(-300, 50, 0);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-300, 50, -500);
	glEnd();

	// right side boundary
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(300, 0, -500);

	glTexCoord2f(5.0f, 0.0f);
	glVertex3f(300, 0, 0);

	glTexCoord2f(5.0f, 1.0f);
	glVertex3f(300, 50, 0);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(300, 50, -500);
	glEnd();

	glFlush();
	glDisable(GL_TEXTURE_2D);
}
void drawStaticGround()
{
	texturedGround();
	texturedBoundary();
	texturedSky();
	vxMarks();
	vxPost();
}

void printHelp(int key, int x, int y)
{

	printf("U -----------> Increase Z Power\n");
	printf("Y -----------> Decrease Z Power\n");
	printf("W or w ------> Aim higher\n");
	printf("S or s ------> Aim lower\n");
	printf("D or d ------> Aim towards right\n");
	printf("A or a ------> Aim towards left\n");
	printf("X or x ------> Shoot\n");
	printf("R or r ------> Reset\n");
	printf("V or v ------> Change camera angle\n");
	printf("LeftClick ------> Shoot\n");
	printf("Escape Key ---> exit the program\n\n");
}

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		if (!shoot)
		{
			shoot = 1;
			ballmove = 1;
		}
	}
}
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(1);
		break;

		// case 'w':
		// case 'W':
		// 	shoot_circle_y = shoot_circle_y + 0.5f <= 100.0f ? shoot_circle_y + 1.0f : 100.0f;
		// 	break;
		// case 's':
		// case 'S':
		// 	shoot_circle_y = shoot_circle_y - 0.5f >= 2.0 ? shoot_circle_y - 0.5f : 2.0f;
		// 	break;

		// case 'a':
		// case 'A':
		// 	shoot_circle_x = shoot_circle_x - 1.5f >= -post_limit ? shoot_circle_x - 1.5f : -post_limit;
		// 	break;
		// case 'd':
		// case 'D':
		// 	shoot_circle_x = shoot_circle_x + 1.5f <= post_limit ? shoot_circle_x + 1.5f : post_limit;
		// 	break;

	case 'w':
	case 'W':
		ball_vel_y = ball_vel_y + 0.5f <= 15 ? ball_vel_y + 0.5f : 15.0f;
		break;
	case 's':
	case 'S':
		ball_vel_y = ball_vel_y - 0.5f >= 2.0 ? ball_vel_y - 0.5f : 2.0f;
		break;

	case 'a':
	case 'A':
		ball_vel_x = ball_vel_x - 0.5 >= -10 ? ball_vel_x - 0.5 : -10.0f;
		break;

	case 'd':
	case 'D':
		ball_vel_x = ball_vel_x + 0.5 <= 10 ? ball_vel_x + 0.5 : 10.0f;
		break;

	case 'u':
	case 'U':
		ball_vel_z = ball_vel_z + 0.5 <= -2 ? ball_vel_z + 0.5 : -2.0f;
		break;

	case 'y':
	case 'Y':
		ball_vel_z = ball_vel_z - 0.5 >= -20.0f ? ball_vel_z - 0.5 : -20.0f;
		break;

	case 'h':
	case 'H':
		printHelp(key, x, y);
		break;
	case 'R':
	case 'r':
		showHitPos = 0;
		shoot = 0;
		goal = 0;
		showGoal = 0;
		break;
	case 'X':
	case 'x':
		if (!shoot)
		{
			shoot = 1;
			ballmove = 1;
		}

		break;
	case 'V':
	case 'v':
		camera_pos = (camera_pos + 1) % 3;
		break;

	// case 'F':
	// case 'f':
	// 	camera_pos = 2;
	// 	ball_vel_x = 5.0f;
	// 	ball_vel_y = 15.0f;
	// 	ball_vel_z = -10.0f;
	// 	break;
	}
}

void resize(int w, int h)
{
	width = w;
	height = h;
	if (h == 0)
		h = 1;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, w, h);
	gluPerspective(60, w / float(h), 10, 1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void setCamera(int pos)
{
	switch (pos)
	{
	// case 0: // bird view
	// 	gluLookAt(
	// 		0.0f, 700.0f, -250.0f, //eye position
	// 		0.0f, 0.0f, -250.0f,   //object position
	// 		0.0f, 0.0f, -1.0f);
	// 	break;

	case 0: //close
		gluLookAt(
			0.0f, 300.0f, 100.0f, //eye position
			0.0f, 0.0f, -400.0f,  //object position
			0.0f, 0.0f, -1.0f);
		break;

	case 1: // very close
		gluLookAt(
			0.0f, 270.0f, 100.0f, //eye position
			0.0f, 0.0f, -450.0f,  //object position
			0.0f, 0.0f, -1.0f);
		break;

	case 2: // realistic
		gluLookAt(
			0.0f, 100.0f, 100.0f, //eye position
			0.0f, 0.0f, -1000.0f, //object position
			0.0f, 0.0f, -1.0f);
		break;

	// case 4: // side
	// 	gluLookAt(
	// 		300.0f, 100.0f, -250.0f, //eye position
	// 		-1000.0f, 0.0f, -250.0f, //object position
	// 		0.0f, 1.0f, 0.0f);
	// 	break;
	}
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	setCamera(camera_pos);

	drawStaticGround();
	drawKeeper();
	drawBall();
	drawHitPos();
	drawShootingBars();
	texturedStatus();

	// ---------------
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	gluOrtho2D(0.0, 500, 0.0, 500);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glColor3f(1.0f, 1.0f, 1.0f);
	std::string scoreBoard = "Points: " + std ::to_string(points);
	bitmap_output(10, 480, scoreBoard, GLUT_BITMAP_HELVETICA_18);
	// char *status;
	// if (showGoal)
	// {
	// 	status = "GOAL";
	// }
	// else
	// {
	// 	status = "";
	// }
	// stroke_output(170, 240, status);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
	// ---------
	glFlush();
	glutSwapBuffers();
	glutPostRedisplay();
}

int init(void)
{
	glClearColor(0.4, 0.4, 1, 0);					   // Fr background colour
	glClearDepth(1.0f);								   // Set background depth to farthest
	glEnable(GL_DEPTH_TEST);						   // Enable depth testing for z-culling
	glDepthFunc(GL_LEQUAL);							   // Set the type of depth-test
	glShadeModel(GL_SMOOTH);						   // Enable smooth shading
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Nice perspective corrections
	textures = new GLuint[5];
	glGenTextures(5, textures);

	LoadTexture(textures[0], "./textures/grass.bmp");
	LoadTexture(textures[1], "./textures/fence.bmp");
	LoadTexture(textures[2], "./textures/sky.bmp");
	LoadTexture(textures[3], "./textures/miss.bmp");
	LoadTexture(textures[4], "./textures/goal.bmp");

	return 1;
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitWindowPosition(150, 100);
	glutInitWindowSize(900, 900);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	glutCreateWindow("Penalty Shootout");

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutReshapeFunc(resize);
	glutIdleFunc(display);

	if (!init())
		return 1;

	glutMainLoop();

	return 0;
}