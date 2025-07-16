#include "rendering_functions.h"

#include "MIS/common.h"
#include "core/Mesh.h"
#include "core/BoxGrid.h"
#include "core/InnerMesh.h"
#include "core/Handles.h"
#include "core/Deformable.h"

#include "utils/utils_functions.h"

#include "slcrystal.h"

	const static objModel * MyGLSphere = new objModel("resources/sphere.obj");
	const static objModel * MyGLCylinder = new objModel("resources/cylinder.obj");
	const static objModel * MyGLCone = new objModel("resources/cone.obj");

void objModel::draw() const {

	glBegin(GL_TRIANGLES);
	for(int fi = 0 ; fi < F.rows() ; ++fi) {
		for(int j = 0 ; j < 3 ; ++j) {
			glNormal3d(N(F(fi,j),0),N(F(fi,j),1),N(F(fi,j),2));
			glVertexAttrib3d(1,N(F(fi,j),0),N(F(fi,j),1),N(F(fi,j),2));
			glVertex3d(V(F(fi,j),0),V(F(fi,j),1),V(F(fi,j),2));
		}
	}
	glEnd();

}

static void paintPlaneHandle ()
{
	float r = 1.0;
	float dr = r / 10.0f;

	glBegin (GL_LINE_STRIP);
	glVertex3f (+r + dr, +r, 0.0);
	glVertex3f (+r, +r + dr, 0.0);
	glVertex3f (+r - dr, +r, 0.0);
	glVertex3f (+r, +r - dr, 0.0);
	glVertex3f (+r + dr, +r, 0.0);
	glEnd ();
}
static void paintCircle()
{
	int nside = 100;
	const double pi2 = 3.14159265 * 2.0;
	glBegin (GL_LINE_LOOP);
	for (double i = 0; i < nside; i++) {
		glNormal3d (cos (i * pi2 / nside), sin (i * pi2 / nside), 0.0);
		glVertex3d (cos (i * pi2 / nside), sin (i * pi2 / nside), 0.0);
	}
	glEnd ();
	paintPlaneHandle();
}

void paintTrackball(const double radius)
{
	glPushMatrix();

	glScalef(radius, radius, radius);

	glEnable (GL_LINE_SMOOTH);
	glLineWidth(1.5f);
	glDisable(GL_LIGHTING);

	glColor3f(1.0f,0.0f,0.0f);
	paintCircle();
	glRotatef (90, 1, 0, 0);
	glColor3f(0.0f,1.0f,0.0f);
	paintCircle();
	glRotatef (90, 0, 1, 0);
	glColor3f(0.0f,0.0f,1.0f);
	paintCircle();
	
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
	glEnable(GL_LIGHTING);

	glPopMatrix ();
}

	//paints a cylinder with radius r between points a and b
	void paintCylinder(const RowVector3 & a, const RowVector3 & b, const double radius, const RowVector4 & color)
	{
		glPushMatrix();

		// This is the default direction for the cylinders to face in OpenGL
		RowVector3 z = RowVector3(0,0,1);        
		// Get diff between two points you want cylinder along
		RowVector3 p = (a - b);                              
		// Get CROSS product (the axis of rotation)
		RowVector3 t = z.cross(p); 

		// Get angle. LENGTH is magnitude of the vector
		double angle = 180 / M_PI * acos ((z.dot(p)) / p.norm());

		glTranslatef(b[0],b[1],b[2]);
		glRotated(angle,t[0],t[1],t[2]);
		glScaled(radius,radius,p.norm()/2);
		glTranslated(0,0,1);
		
		glColor4f(color[0],color[1],color[2],color[3]);
		MyGLCylinder->draw();

		glPopMatrix();
	}

	void paintCylinderSL(const RowVector3 & a, const RowVector3 & b, const double radius, const RowVector4 & color)
	{
		// This is the default direction for the cylinders to face in OpenGL
		RowVector3 z = RowVector3(0,0,1);        
		// Get diff between two points you want cylinder along
		RowVector3 p = (a - b);                              
		// Get CROSS product (the axis of rotation)
		RowVector3 t = z.cross(p); 

		// Get angle. LENGTH is magnitude of the vector
		double angle = 180 / M_PI * acos ((z.dot(p)) / p.norm());

		float model[16];
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();
			glTranslatef(b[0],b[1],b[2]);
			glRotated(angle,t[0],t[1],t[2]);
			glScaled(radius,radius,p.norm()/2);
			glTranslated(0,0,1);
			glGetFloatv(GL_MODELVIEW_MATRIX, model);
			transpose( model );
		glPopMatrix();

		slcrystal_set_model_matrix(model);
		slcrystal_begin(color[0],color[1],color[2],color[3]);
		MyGLCylinder->draw();
		slcrystal_end();
	}

	//paints a cone given its bottom and top points and its bottom radius
	void paintCone(const RowVector3 & bottom, const RowVector3 & top, const double radius, const RowVector4 & color)
	{
		glPushMatrix();

		// This is the default direction for the cylinders to face in OpenGL
		RowVector3 z = Vector3(0,0,1);        
		// Get diff between two points you want cylinder along
		RowVector3 p = (top - bottom);                              
		// Get CROSS product (the axis of rotation)
		RowVector3 t = z.cross(p); 

		// Get angle. LENGTH is magnitude of the vector
		double angle = 180 / M_PI * acos ((z.dot(p)) / p.norm());

		glTranslatef(bottom[0],bottom[1],bottom[2]);
		glRotated(angle,t[0],t[1],t[2]);
		glScaled(radius,radius,p.norm()/2);
		glTranslated(0,0,1);
		
		glColor4f(color[0],color[1],color[2],color[3]);
		MyGLCone->draw();

		glPopMatrix();
	}

	void paintConeSL(const RowVector3 & bottom, const RowVector3 & top, const double radius, const RowVector4 & color)
	{
		// This is the default direction for the cylinders to face in OpenGL
		RowVector3 z = Vector3(0,0,1);        
		// Get diff between two points you want cylinder along
		RowVector3 p = (top - bottom);                              
		// Get CROSS product (the axis of rotation)
		RowVector3 t = z.cross(p); 

		// Get angle. LENGTH is magnitude of the vector
		double angle = 180 / M_PI * acos ((z.dot(p)) / p.norm());

		float model[16];
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();
			glTranslatef(bottom[0],bottom[1],bottom[2]);
			glRotated(angle,t[0],t[1],t[2]);
			glScaled(radius,radius,p.norm()/2);
			glTranslated(0,0,1);
			glGetFloatv(GL_MODELVIEW_MATRIX, model);
			transpose( model );
		glPopMatrix();

		slcrystal_set_model_matrix(model);
		slcrystal_begin(color[0],color[1],color[2],color[3]);
			MyGLCone->draw();
		slcrystal_end();
	}

	//paints an arrow between from and to with a given radius
	void paintArrow(const RowVector3 & from, const RowVector3 & to, const double radius, const RowVector4 & color)
	{
		double length = (to - from).norm();
		RowVector3 axis = (to-from).normalized();
		if(length > 0.03) {
			paintCylinder(from, to-0.03*axis, radius,color);
			paintCone(to-0.03*axis, to, 2.0*radius,color);
		}
	}

	void paintArrowSL(const RowVector3 & from, const RowVector3 & to, const double radius, const RowVector4 & color)
	{
		double length = (to - from).norm();
		RowVector3 axis = (to-from).normalized();
		if(length > 0.03) {
			paintCylinderSL(from, to-0.03*axis, radius, color);
			paintConeSL(to-0.03*axis, to, 2.0*radius, color);
		}
	}


	void paintPoint(const RowVector3 & a, const double radius, const RowVector4 & color)
	{
		glPushMatrix();
		glTranslated(a[0],a[1],a[2]);
		glScaled(radius,radius,radius);
		glColor4f(color[0],color[1],color[2],color[3]);
		MyGLSphere->draw();
		glPopMatrix();
	}

	void paintPointSL(const RowVector3 & a, const double radius, const RowVector4 & color)
	{
		float model[16];
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();
			glTranslated(a[0],a[1],a[2]);
			glScaled(radius,radius,radius);
			glGetFloatv(GL_MODELVIEW_MATRIX, model);
			transpose( model );
		glPopMatrix();

		slcrystal_set_model_matrix(model);
		slcrystal_begin(color[0],color[1],color[2],color[3]);
			MyGLSphere->draw();
		slcrystal_end();
	}



void MIS::render_mesh(const Mesh * mesh, const int idSelectedHandle) {

	//glLineWidth((GLfloat)1.0f);

	// loop over the faces
	for(IndexType fi = 0; fi < mesh->nbF ; ++fi) {
		// loop over vertices in this face
		glBegin(GL_TRIANGLES);
		for(IndexType j = 0; j < 3; ++j)
		{
			IndexType vj = mesh->F(fi,j);

			if(MIS::isVoxelized() && idSelectedHandle != -1) {
				float wi = mesh->getWeight(idSelectedHandle,vj);
				glColor3f(wi,0,1.0f-wi);
			}

			const RowVector3 & p = mesh->getCurrentPose(vj);
			const RowVector3 & n = mesh->cN(fi,j);
			glNormal3f(n[0],n[1],n[2]);
			glVertexAttrib3f(1,n[0],n[1],n[2]);
			glVertex3f(p[0],p[1],p[2]);
		}
		glEnd();
	}

}


void MIS::render_innermesh(const InnerMesh * imesh) {

	glLineWidth((GLfloat)1.0f);

	// loop over the faces
	for(IndexType fi = 0; fi < imesh->nbF ; ++fi) {
		// loop over vertices in this face
		glBegin(GL_QUADS);
		const RowVector3 & v0 = imesh->V(fi,0);
		const RowVector3 & v1 = imesh->V(fi,1);
		const RowVector3 & v2 = imesh->V(fi,2);
		const RowVector3 & v3 = imesh->V(fi,3);
		RowVector3 n = (v0-v2).cross(v3-v1);
		n.normalize();
		glNormal3f(n[0],n[1],n[2]);
		glVertexAttrib3f(1,n[0],n[1],n[2]);
		glVertex3d(v0[0], v0[1], v0[2]);
		glVertex3d(v1[0], v1[1], v1[2]);
		glVertex3d(v2[0], v2[1], v2[2]);
		glVertex3d(v3[0], v3[1], v3[2]);
		glEnd();
	}

}
