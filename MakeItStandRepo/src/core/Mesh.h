#ifndef __MESH_H
#define __MESH_H

#include "MIS_inc.h"
#include "utils\mass_functions.h"

class Mesh {
	friend class MISPlugin;

protected:
	PointMatrixType vertices; // list of vertices (each row is a 3D point)
	FaceMatrixType faces; // list of triangle (each row is a triplet of vertex indices)

	vector<Deformable*> deformInfo;

	vector< vector<IndexType> > vertex_to_vertices; // vertex_to_vertices[i][j] is the j-th neighboring vertex of the vertex i
	vector< vector<IndexType> > vertex_to_faces; // vertex_to_faces[i][j] is the j-th neighboring face of the vertex i
	FaceMatrixType face_to_faces; // face_to_faces(i,j) is the j-th neighboring face of the face i (only 3)
	
	RowMatrixX3 FNormals; // face normals
	RowMatrixX3 VNormals; // vertex normals
	RowMatrixX3 CNormals; // corner normals

	RowVector3 bmin, bmax;
	
	MList mList;
	CList cList;
	DMList dmList;
	DCList dcList;

public:
	IndexType nbV; // number of vertices
	IndexType nbF; // number of faces
	SparseMatrix M, MM;

	ScalarType corner_threshold;
	
	Mesh(string filename); // this construction loads list of vertices and triangles from a filename and initialize the data structure
	~Mesh();
	
	const PointMatrixType & getV() const {
		return vertices;
	}
	const FaceMatrixType & getF() const {
		return faces;
	}
	
	/*
		Getters for vertices
	*/

	RowVector3 V(IndexType v) const {
		return vertices.row(v);
	}
	RowVector3 V(IndexType t, IndexType vt) const {
		return vertices.row(faces(t,vt));
	}
	RowVector3 V(IndexType t, const RowVector3 & bc) const {
		return bc.x() * V(t,0) + bc.y() * V(t,1) + bc.z() * V(t,2);
	}

	IndexType F(IndexType t, IndexType vt) const {
		return faces(t,vt);
	}

	/*
		Getters for normals and area
	*/

	RowVector3 fN(IndexType t) const {
		return FNormals.row(t);
	}
	RowVector3 vN(IndexType v) const {
		return VNormals.row(v);
	}
	RowVector3 vN(IndexType t, IndexType vt) const {
		return VNormals.row(faces(t,vt));
	}
	RowVector3 vN(IndexType t, const RowVector3 & bc) const {
		return bc.x() * vN(t,0) + bc.y() * vN(t,1) + bc.z() * vN(t,2);
	}
	RowVector3 cN(IndexType t, IndexType vt) const {
		return CNormals.row(3*t+vt);
	}

	/*
		Getters for neighboring information
	*/

	IndexType vvNbNeighbors(IndexType v) const {
		return vertex_to_vertices[v].size();
	}
	IndexType vvNeighbor(IndexType v, IndexType i) const {
		return vertex_to_vertices[v][i];
	}
	IndexType vfNbNeighbors(IndexType v) const {
		return vertex_to_faces[v].size();
	}
	IndexType vfNeighbor(IndexType v, IndexType i) const {
		return vertex_to_faces[v][i];
	}
	int vfNeighborId(IndexType v, IndexType i) const {
		if(faces(vertex_to_faces[v][i],0) == v) return 0;
		if(faces(vertex_to_faces[v][i],1) == v) return 1;
		if(faces(vertex_to_faces[v][i],2) == v) return 2;
		
		cout << "ERROR : vertex not in the face" << endl;
		return -1;
	}
	IndexType ffNeighbor(IndexType f, IndexType i) const {
		return face_to_faces(f,i);
	}

	void InitNeighboringData(); // initialize all neighboring information
	void ComputeNormals(); // compute all normals
	void ComputeCornerNormals(); // compute corner normals
	void ComputeBoundingBox();
	void ComputeLaplacian();


	void computeBBW(const Handles & handles, const BoxGrid & voxels);
	void updatePoses(const Handles & handles);

	RowVector3 getHigherVertex(const RowVector3 & gravity) const;
	
	float getWeight(int idHandle, int idV) const;
	const RowVector3 & getCurrentPose(int idV) const;
	const RowVector3 & getRestPose(int idV) const;

	void getMassAndCenterOfMass(ScalarType & m, Vector3 & c, MList & dm, CList & dc);
	void getMassAndCenterOfMass(ScalarType & m, Vector3 & c);

	void getDiffLaplacian(PointMatrixType & LL) const;
	void getWeightLaplacian(int j, VectorX & LW) const;

	void glDrawForVoxelizer(const int res) const;
	void exportMesh(string filename, Config & cfg);

};

#endif