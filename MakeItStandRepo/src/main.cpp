#include "viewer/Preview3D.h"
#include "slcrystal.h"
#include "GL_inc.h"

Preview3D * preview_3D;

// Callback function called by GLUT to render screen
void Render(void)
{
	preview_3D->draw(glutGet(GLUT_ELAPSED_TIME));

	glutSwapBuffers();
	glutReportErrors();
}

// Callback function called by GLUT when window size changes
void Reshape(int width, int height)
{
	preview_3D->resize(width,height);

	// initialize the transparency renderer
	slcrystal_init(height, 1.0f, 40.0f);

	glutPostRedisplay();
}

// Function called at exit
void Terminate(void)
{
	delete preview_3D;

	TwDeleteAllBars();
	TwTerminate();

	slcrystal_terminate();

	exit(0);
}

void mouse(int glutButton, int glutState, int mouse_x, int mouse_y)
{
	int key = glutGetModifiers();
	int modifiers = Preview3D::NO_KEY;
	if(key & GLUT_ACTIVE_SHIFT)
		modifiers = modifiers | Preview3D::SHIFT;
	if(key & GLUT_ACTIVE_CTRL)
		modifiers = modifiers | Preview3D::CTRL;
	if(key & GLUT_ACTIVE_ALT)
		modifiers = modifiers | Preview3D::ALT;

	if(glutState==1)
		preview_3D->mouse_up(mouse_x,mouse_y, glutButton,modifiers);
	else if(glutState==0)
		preview_3D->mouse_down(mouse_x,mouse_y, glutButton, modifiers);

	glutPostRedisplay();
}
void mouse_move(int mouse_x, int mouse_y)
{
	preview_3D->mouse_move(mouse_x,mouse_y);
	
	glutPostRedisplay();
}
void keyboard (unsigned char k, int x, int y)
{
	TwEventKeyboardGLUT (k, x, y);

	if(k == 27) {
		Terminate();
		return;
	}

	int key = glutGetModifiers();
	int modifiers = Preview3D::NO_KEY;
	if(key & GLUT_ACTIVE_SHIFT)
		modifiers = modifiers | Preview3D::SHIFT;
	if(key & GLUT_ACTIVE_CTRL)
		modifiers = modifiers | Preview3D::CTRL;
	if(key & GLUT_ACTIVE_ALT)
		modifiers = modifiers | Preview3D::ALT;

	preview_3D->key_down(k, modifiers, x, y);

	glutPostRedisplay ();
}

void keyboardSpec(int key, int x, int y)
{
	TwEventSpecialGLUT (key, x, y);

	if(key == GLUT_KEY_DOWN)
		preview_3D->mouse_scroll(x, y, -1);
	else if(key == GLUT_KEY_UP)
		preview_3D->mouse_scroll(x, y, 1);
	else if(key == GLUT_KEY_LEFT)
		preview_3D->key_down(key, Preview3D::NO_KEY, x, y);
	else if(key == GLUT_KEY_RIGHT)
		preview_3D->key_down(key, Preview3D::NO_KEY, x, y);

	if(key == GLUT_KEY_F1) preview_3D->display_bars = !preview_3D->display_bars;

	if(key == GLUT_KEY_F2 || key == GLUT_KEY_F3 || key == GLUT_KEY_F4 || key == GLUT_KEY_F5 || key == GLUT_KEY_F9 || key == GLUT_KEY_F10)
		preview_3D->key_down(key, Preview3D::NO_KEY, x, y);

	glutPostRedisplay ();
}

void Idle()
{	
	glutPostRedisplay();
}

// Main
int main(int argc, char *argv[])
{
	// Initialize GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA | GLUT_MULTISAMPLE);
	glutInitWindowSize(1280, 720);

	// Center window
	glutInitWindowPosition(
		glutGet(GLUT_SCREEN_WIDTH)/2-glutGet(GLUT_INIT_WINDOW_WIDTH)/2,
		glutGet(GLUT_SCREEN_HEIGHT)/2-glutGet(GLUT_INIT_WINDOW_HEIGHT)/2);
	glutCreateWindow("IGL Viewer : Make It Stand Project");
	glutCreateMenu(NULL);

	glewInit();

	preview_3D = new Preview3D(glutGet(GLUT_ELAPSED_TIME));

	// Set GLUT callbacks
	glutDisplayFunc(Render);
	glutIdleFunc(Idle);
	glutReshapeFunc(Reshape);
	
	glutMouseFunc((GLUTmousebuttonfun)mouse);
	glutMotionFunc(mouse_move);
	glutPassiveMotionFunc(mouse_move);
	glutSpecialFunc(keyboardSpec);
	glutKeyboardFunc(keyboard);
	
	TwGLUTModifiersFunc(glutGetModifiers);

	// Call the GLUT main loop
	glutMainLoop();

	Terminate();

	return 0;
}



