#ifndef __MASS_FUNCTIONS_H
#define __MASS_FUNCTIONS_H

#include "MIS_inc.h"

typedef Eigen::Matrix<ScalarType,1,Eigen::Dynamic> MList;
typedef Eigen::Matrix<ScalarType,3,Eigen::Dynamic,Eigen::ColMajor> CList;
typedef	Eigen::Matrix<ScalarType,3,Eigen::Dynamic,Eigen::ColMajor> DMList;
typedef Eigen::Matrix<ScalarType,9,Eigen::Dynamic,Eigen::ColMajor> DCList;

inline void getMassAndCenterOfMassTriangle(const RowVector3 & p0, const RowVector3 & p1, const RowVector3 & p2, ScalarType & m, Vector3 & c) {

	// get edges and cross product of edges
	RowVector3 e1 = p1-p0, e2 = p0-p2, e3 = p2-p1;
	RowVector3 n = -e1.cross(e2);
	RowVector3 f1 = p0+p1+p2;
	RowVector3 f2;
	f2[0] = f1[0]*f1[0] - (p0[0]*p1[0] + p1[0]*p2[0] + p2[0]*p0[0]);
	f2[1] = f1[1]*f1[1] - (p0[1]*p1[1] + p1[1]*p2[1] + p2[1]*p0[1]);
	f2[2] = f1[2]*f1[2] - (p0[2]*p1[2] + p1[2]*p2[2] + p2[2]*p0[2]);
			
	// mass
	m += f1[0]*n[0];

	// center of mass
	c[0] += f2[0]*n[0];
	c[1] += f2[1]*n[1];
	c[2] += f2[2]*n[2];

}

inline void getMassAndCenterOfMassTriangleList(const RowVector3 & p0, const RowVector3 & p1, const RowVector3 & p2, int id, MList & m, CList & c) {

	// get edges and cross product of edges
	RowVector3 e1 = p1-p0, e2 = p0-p2, e3 = p2-p1;
	RowVector3 n = -e1.cross(e2);
	RowVector3 f1 = p0+p1+p2;
	RowVector3 f2;
	f2[0] = f1[0]*f1[0] - (p0[0]*p1[0] + p1[0]*p2[0] + p2[0]*p0[0]);
	f2[1] = f1[1]*f1[1] - (p0[1]*p1[1] + p1[1]*p2[1] + p2[1]*p0[1]);
	f2[2] = f1[2]*f1[2] - (p0[2]*p1[2] + p1[2]*p2[2] + p2[2]*p0[2]);
			
	// mass
	m[id] = f1[0]*n[0];

	// center of mass
	c(0,id) = f2[0]*n[0];
	c(1,id) = f2[1]*n[1];
	c(2,id) = f2[2]*n[2];

}

inline void getMassAndCenterOfMassTriangleList(const RowVector3 & p0, const RowVector3 & p1, const RowVector3 & p2, int id, MList & m, CList & c, DMList & dm, DCList & dc) {

	// get edges and cross product of edges
	RowVector3 e1 = p1-p0, e2 = p0-p2, e3 = p2-p1;
	RowVector3 n = -e1.cross(e2);
	RowVector3 f1 = p0+p1+p2;
	RowVector3 f2;
	f2[0] = f1[0]*f1[0] - (p0[0]*p1[0] + p1[0]*p2[0] + p2[0]*p0[0]);
	f2[1] = f1[1]*f1[1] - (p0[1]*p1[1] + p1[1]*p2[1] + p2[1]*p0[1]);
	f2[2] = f1[2]*f1[2] - (p0[2]*p1[2] + p1[2]*p2[2] + p2[2]*p0[2]);
			
	// mass
	m[id] = f1[0]*n[0];

	// center of mass
	c(0,id) = f2[0]*n[0];
	c(1,id) = f2[1]*n[1];
	c(2,id) = f2[2]*n[2];

	// derivative of mass
	dm(0,3*id) = n[0];
	dm(1,3*id) = -f1[0]*e3[2];
	dm(2,3*id) = f1[0]*e3[1];

	dm(0,3*id+1) = n[0];
	dm(1,3*id+1) = -f1[0]*e2[2];
	dm(2,3*id+1) = f1[0]*e2[1];
			
	dm(0,3*id+2) = n[0];
	dm(1,3*id+2) = -f1[0]*e1[2];
	dm(2,3*id+2) = f1[0]*e1[1];

	// derivative of center of mass			
	dc(0,3*id) = n[0]*(f1[0]+p0[0]);
	dc(1,3*id) = f2[1]*e3[2];
	dc(2,3*id) = -f2[2]*e3[1];
	dc(3,3*id) = -f2[0]*e3[2];
	dc(4,3*id) = n[1]*(f1[1]+p0[1]);
	dc(5,3*id) = f2[2]*e3[0];
	dc(6,3*id) = f2[0]*e3[1];
	dc(7,3*id) = -f2[1]*e3[0];
	dc(8,3*id) = n[2]*(f1[2]+p0[2]);
			
	dc(0,3*id+1) = n[0]*(f1[0]+p1[0]);
	dc(1,3*id+1) = f2[1]*e2[2];
	dc(2,3*id+1) = -f2[2]*e2[1];
	dc(3,3*id+1) = -f2[0]*e2[2];
	dc(4,3*id+1) = n[1]*(f1[1]+p1[1]);
	dc(5,3*id+1) = f2[2]*e2[0];
	dc(6,3*id+1) = f2[0]*e2[1];
	dc(7,3*id+1) = -f2[1]*e2[0];
	dc(8,3*id+1) = n[2]*(f1[2]+p1[2]);
			
	dc(0,3*id+2) = n[0]*(f1[0]+p2[0]);
	dc(1,3*id+2) = f2[1]*e1[2];
	dc(2,3*id+2) = -f2[2]*e1[1];
	dc(3,3*id+2) = -f2[0]*e1[2];
	dc(4,3*id+2) = n[1]*(f1[1]+p2[1]);
	dc(5,3*id+2) = f2[2]*e1[0];
	dc(6,3*id+2) = f2[0]*e1[1];
	dc(7,3*id+2) = -f2[1]*e1[0];
	dc(8,3*id+2) = n[2]*(f1[2]+p2[2]);
}


inline void getMassAndCenterOfMassQuad(const RowVector3 & p0, const RowVector3 & p1, const RowVector3 & p2, const RowVector3 & p3, ScalarType & m, Vector3 & c) {

	getMassAndCenterOfMassTriangle(p0, p1, p2, m, c);
	getMassAndCenterOfMassTriangle(p2, p3, p0, m, c);

}


inline void getMassAndCenterOfMassVoxel(const RowVector3 & p0, const RowVector3 & p1, const RowVector3 & p2, const RowVector3 & p3, 
										const RowVector3 & p4, const RowVector3 & p5, const RowVector3 & p6, const RowVector3 & p7, 
										ScalarType & m, Vector3 & c) {

	getMassAndCenterOfMassQuad(p0,p1,p3,p2,m,c);
	getMassAndCenterOfMassQuad(p0,p4,p5,p1,m,c);
	getMassAndCenterOfMassQuad(p0,p2,p6,p4,m,c);
	getMassAndCenterOfMassQuad(p4,p6,p7,p5,m,c);
	getMassAndCenterOfMassQuad(p2,p3,p7,p6,m,c);
	getMassAndCenterOfMassQuad(p1,p5,p7,p3,m,c);
	
}

#endif