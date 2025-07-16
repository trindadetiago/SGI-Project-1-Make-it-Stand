#include "InnerMesh.h"

#include "core/BoxGrid.h"
#include "utils\meshIO\writeSTL.h"

	static const int g_quadCube[6][4] = {
		{0,1,3,2},
		{0,4,5,1},
		{0,2,6,4},
		{4,6,7,5},
		{2,3,7,6},
		{1,5,7,3}
	};

	void InnerMesh::compute() {

		nbF = 0;
		for(int i = 0 ; i < voxels.getNumBoxes() ; ++i) {
			if(voxels.isFilled(i)) {
				for(int j = 0 ; j < 6 ; ++j) {
					if(voxels.getBoxBoxes(i,j) != -1 && !voxels.isFilled(voxels.getBoxBoxes(i,j)))
						nbF++;
				}
			}
		}

		faces.setZero(nbF,4);
				
		int idF = 0;
		for(int i = 0 ; i < voxels.getNumBoxes() ; ++i) {
			if(voxels.isFilled(i)) {
				for(int j = 0 ; j < 6 ; ++j) {
					if(voxels.getBoxBoxes(i,j) != -1 && !voxels.isFilled(voxels.getBoxBoxes(i,j))) {
						for(int k = 0 ; k < 4 ; ++k)
							faces(idF,k) = voxels.getBoxNodes(i,g_quadCube[j][k]);
						idF++;
					}
				}
			}
		}

	}

	void InnerMesh::exportMesh(string filename) const {
		PointMatrixType vertices;
		vertices.setZero(voxels.getNumNodes(),3);
		for(int i = 0 ; i < voxels.getNumNodes() ; ++i) {
			vertices.row(i) = voxels.getCurrentPose(i);
		}
		igl::writeSTLforQuadMesh(filename,vertices,faces);
	}

	IndexType InnerMesh::F(int idF, int idV) const {
		return faces(idF,idV);
	}

	const RowVector3 & InnerMesh::V(int idF, int idV) const {
		return voxels.getCurrentPose(faces(idF,idV));
	}

	const RowVector3 & InnerMesh::V(int idV) const {
		return voxels.getCurrentPose(idV);
	}


	void InnerMesh::getMassAndCenterOfMass(ScalarType & m, Vector3 & c, MList & dm, CList & dc) {

		m = 0;
		c.setZero();
		dm.setZero(3*voxels.getNumNodes());
		dc.setZero(3,3*voxels.getNumNodes());

		int quadToTri[2][3] = { {0,1,2}, {2,3,0} };

		//cout << "nbf : " << nbF*2 << endl;

		for(int idQ = 0 ; idQ < nbF ; ++idQ) {
			for(int j = 0 ; j < 2 ; ++j) {
				
				const IndexType v0 = faces(idQ,quadToTri[j][0]);
				const IndexType v1 = faces(idQ,quadToTri[j][1]);
				const IndexType v2 = faces(idQ,quadToTri[j][2]);

				// get vertices of triangle t
				const RowVector3 & p0 = V(v0);
				const RowVector3 & p1 = V(v1);
				const RowVector3 & p2 = V(v2);

				getMassAndCenterOfMassTriangle(p0,p1,p2,m,c);
				
				RowVector3 e1 = p1-p0, e2 = p0-p2, e3 = p2-p1;
				RowVector3 n = -e1.cross(e2);
				RowVector3 f1 = p0+p1+p2;
				RowVector3 f2;
				f2[0] = f1[0]*f1[0] - (p0[0]*p1[0] + p1[0]*p2[0] + p2[0]*p0[0]);
				f2[1] = f1[1]*f1[1] - (p0[1]*p1[1] + p1[1]*p2[1] + p2[1]*p0[1]);
				f2[2] = f1[2]*f1[2] - (p0[2]*p1[2] + p1[2]*p2[2] + p2[2]*p0[2]);

				// derivative of mass
				dm[3*v0] += n[0];
				dm[3*v0+1] -= f1[0]*e3[2];
				dm[3*v0+2] += f1[0]*e3[1];
				dm[3*v1] += n[0];
				dm[3*v1+1] -= f1[0]*e2[2];
				dm[3*v1+2] += f1[0]*e2[1];
				dm[3*v2] += n[0];
				dm[3*v2+1] -= f1[0]*e1[2];
				dm[3*v2+2] += f1[0]*e1[1];


				// derivative of center of mass			
				dc(0,3*v0)   += n[0]*(f1[0]+p0[0]);
				dc(1,3*v0)   += f2[1]*e3[2];
				dc(2,3*v0)   += -f2[2]*e3[1];
				dc(0,3*v0+1) += -f2[0]*e3[2];
				dc(1,3*v0+1) += n[1]*(f1[1]+p0[1]);
				dc(2,3*v0+1) += f2[2]*e3[0];
				dc(0,3*v0+2) += f2[0]*e3[1];
				dc(1,3*v0+2) += -f2[1]*e3[0];
				dc(2,3*v0+2) += n[2]*(f1[2]+p0[2]);
			
				dc(0,3*v1)   += n[0]*(f1[0]+p1[0]);
				dc(1,3*v1)   += f2[1]*e2[2];
				dc(2,3*v1)   += -f2[2]*e2[1];
				dc(0,3*v1+1) += -f2[0]*e2[2];
				dc(1,3*v1+1) += n[1]*(f1[1]+p1[1]);
				dc(2,3*v1+1) += f2[2]*e2[0];
				dc(0,3*v1+2) += f2[0]*e2[1];
				dc(1,3*v1+2) += -f2[1]*e2[0];
				dc(2,3*v1+2) += n[2]*(f1[2]+p1[2]);
			
				dc(0,3*v2)   += n[0]*(f1[0]+p2[0]);
				dc(1,3*v2)   += f2[1]*e1[2];
				dc(2,3*v2)   += -f2[2]*e1[1];
				dc(0,3*v2+1) += -f2[0]*e1[2];
				dc(1,3*v2+1) += n[1]*(f1[1]+p2[1]);
				dc(2,3*v2+1) += f2[2]*e1[0];
				dc(0,3*v2+2) += f2[0]*e1[1];
				dc(1,3*v2+2) += -f2[1]*e1[0];
				dc(2,3*v2+2) += n[2]*(f1[2]+p2[2]);
			}
		}

		m /= 6.0;
		c /= 24.0;
	}
