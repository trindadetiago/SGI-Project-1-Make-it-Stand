#ifndef __RENDERING_FUNCTIONS_H
#define __RENDERING_FUNCTIONS_H

#include "MIS_inc.h"
#include "utils\meshIO\readOBJ.h"

namespace MIS {

	void render_mesh(const Mesh * mesh, const int idSelectedHandle);
	void render_innermesh(const InnerMesh * imesh);
	
};

	class objModel {
	public:
		objModel(string filename) {
			igl::readOBJ(filename, V, F, N);
		}
		
		void draw() const;

	private:
		PointMatrixType V;
		FaceMatrixType F;
		PointMatrixType N;

	};

	void paintTrackball(const double radius);

	// OpenGL helpers
	void paintPoint(const RowVector3 & a, const double radius, const RowVector4 & color);
	void paintCylinder(const RowVector3 & a, const RowVector3 & b, const double radius, const RowVector4 & color);
	void paintCone(const RowVector3 & bottom, const RowVector3 & top, const double radius, const RowVector4 & color);
	void paintArrow(const RowVector3 & from, const RowVector3 & to, const double radius, const RowVector4 & color);
	
	// Sylvain's transparency library helpers
	void paintPointSL(const RowVector3 & a, const double radius, const RowVector4 & color);
	void paintCylinderSL(const RowVector3 & a, const RowVector3 & b, const double radius, const RowVector4 & color);
	void paintConeSL(const RowVector3 & bottom, const RowVector3 & top, const double radius, const RowVector4 & color);
	void paintArrowSL(const RowVector3 & from, const RowVector3 & to, const double radius, const RowVector4 & color);

#endif