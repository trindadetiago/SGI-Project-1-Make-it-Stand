#ifndef __INNERMESH_H
#define __INNERMESH_H

#include "MIS_inc.h"

#include "utils\mass_functions.h"

class InnerMesh {

protected:
	const BoxGrid & voxels;
	QuadMatrixType faces; // list of quad (each row is a quadruplet of vertex indices)

public:
	IndexType nbF; // number of faces
	
	InnerMesh(const BoxGrid & vox) : voxels(vox)
	{
		nbF = 0;	
	}
	~InnerMesh()
	{
		faces.setZero(0,4);
	}

	void compute();
	void exportMesh(string filename) const;
	IndexType F(int idF, int idV) const;
	const RowVector3 & V(int idF, int idV) const;
	const RowVector3 & V(int idV) const;
		
	void getMassAndCenterOfMass(ScalarType & m, Vector3 & c, MList & dm, CList & dc);

};

#endif