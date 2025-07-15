#ifndef __BOXGRID_H
#define __BOXGRID_H

#include "MIS_inc.h"
#include "MIS/common.h"

#include "core/Array3D.h"

// Basic data structures for a 3D grid of regular boxes (not necessarily equilateral -- though some methods silently assume square boxes)
// Some boxes can be empty, so we distinguish all elements (i.e. full 3D array) and non-empty ones (carving a subset of the 3D array)
class BoxGrid {

public:
	BoxGrid(const int res);
	~BoxGrid() {
		freeAll();
	}

	void saveToFile(string voxfile, string bbwfile, const int numHandles) const;

	void initVoxels(const Mesh & mesh, const int res);
	void initFromFile(string filename);
	void initStructure();

	int getNumNodes() const { return nnzNodes; }
	int getNumBoxes() const { return nnzBoxes; }

	void getInterpolatedBBW(const RowVector3 & P, Deformable * deformInfo, const int nbWeights) const;

	void importBBW(string filename, const int numHandles);
	void computeBBW(const Handles & handles);

	int getBoxContainingPoint(const RowVector3 & P, RowVector3 & t) const;
	bool getBoxCoordsContainingPoint(const RowVector3 & P, RowVector3i & t) const;
	int getNodeClosestToPoint(const RowVector3 & P) const;
	
	float getWeight(int idHandle, int idNode) const;
	const RowVector3 & getCurrentPose(int idNode) const;
	const RowVector3 & getRestPose(int idNode) const;

	//ScalarType getNodeVolume(int idNode) const { return m_nodeVolumes[idNode]; }
	//ScalarType getBoxVolume(int idBox) const { return m_boxVolumes[idBox]; }
	RowMatrixX3::ConstRowXpr getBoxPosition(int idBox) const { return m_boxPositions.row(idBox); }

	bool isHull(int idBox) const { return m_depth[idBox] == 0; }
	float getDepthAsFloat(int idBox) const { return m_depth[idBox]/(float)m_maxDepth; }
	int getMaxDepth() const { return m_maxDepth; }
	int getDepth(int idBox) const { return m_depth[idBox]; }

	bool isFilled(int idBox) const { return m_boxStatus[idBox]; }
	void setFilled(int idBox) { m_boxStatus[idBox] = true; }
	void setCarved(int idBox) { m_boxStatus[idBox] = false; }

	void updatePoses(const Handles & handles);
	void computeBoxPositions();

	void getMassAndCenterOfMassBox(int idBox, ScalarType & m, Vector3 & c);
	void getMassAndCenterOfMass(ScalarType & m, Vector3 & c);

	void clearFilling(int hullDepth = 0);
	void clearCarving();
	
	int getBoxBoxes(int idBox, int idNeighbor) const { return m_boxBoxes(idBox,idNeighbor); }
	int getBoxNodes(int idBox, int idNeighbor) const { return m_boxNodes(idBox,idNeighbor); }

protected:
	Vector3i m_size; // number of boxes in x,y,z dimensions
	RowVector3 m_lowerLeft, m_upperRight; // placement in 3D space
	RowVector3 m_frac;

	// all 3 following 3D arrays store either -1 if empty or element index (vertex, edge, box)
	Array3D<int> m_nodeArray; // nodes 
	Array3D<int> m_boxArray; // boxes

	int nnzBoxes, nnzNodes, nnzEdges[3];

	// frees all data structures
	void freeAll();

	vector<Deformable*> m_nodes;
	vector<int> m_depth; // boxes depth (0 is hull, 1 the neighbors of the hull, 2...)
	int m_maxDepth;
	vector<bool> m_boxStatus; // true if box filled, else false
	RowMatrixX3 m_boxPositions; // positions of boxes (isobarycenter)
	
	MatrixX8i m_boxNodes; // numBoxes x 8 int matrix of node indices (incident to a given box)
	MatrixX6i m_nodeNodes; // numNodes x 6 int matrix of node indices (incident to a given node)
	MatrixX6i m_boxBoxes;

};

#endif
