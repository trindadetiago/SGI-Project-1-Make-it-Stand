#include "BoxGrid.h"

#include "core/Deformable.h"
#include "core/Mesh.h"
#include "core/InnerMesh.h"
#include "core/Handles.h"

#include "slvoxlib.h"
#include "utils\mosek_solver.h"

BoxGrid::BoxGrid(const int res)
{
	m_size[0] = m_size[1] = m_size[2] = res;
	m_lowerLeft.setZero();
	m_upperRight.setConstant(1.0f);
	
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];

	m_boxArray.init(Xs, Ys, Zs);
	m_nodeArray.init(Xs+1, Ys+1, Zs+1);

	m_frac = (m_upperRight-m_lowerLeft).cwiseQuotient(RowVector3(Xs,Ys,Zs));
}

void BoxGrid::initVoxels(const Mesh & mesh, const int res) {
	
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];
	
    slvox_set_voxel_res( res );
	  // begin voxelizing
	  slvox_begin();
	  // draw object to voxelize
	  mesh.glDrawForVoxelizer(res);
	  // done
	  slvox_end();
	  
	  unsigned char *vox = new unsigned char[res*res*res];
	  slvox_getvoxels( vox );
	  	m_boxArray.setAllTo(-1);
		nnzBoxes = 0;
		for (int x=0; x<Xs; x++) {
			for (int y=0; y<Ys; y++) {
				for (int z=0; z<Zs; z++) {
					if(vox[res*res*z+res*y+x] != 0) {
						m_boxArray(x,y,z) = nnzBoxes++;
					}
				}
			}
		}
		delete vox;
	
	// THIS IS A HACK BECAUSE SOME VERTICES OF THE MESH MIGHT BE IN A BOX WHICH IS NOT MARKED
	for(int i = 0 ; i < mesh.nbV ; ++i) {
		RowVector3i t = RowVector3i::Zero();
		if(getBoxCoordsContainingPoint(mesh.V(i),t)) {
			if(m_boxArray(t[0],t[1],t[2]) == -1) {
				m_boxArray(t[0],t[1],t[2]) = nnzBoxes++;
				//cout << "One vertex of the mesh was not in an active voxel" << endl;
			}
		}
		else {
			cout << "Error !" << endl;
		}
	}

}

void BoxGrid::initStructure() {
	
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];

	m_nodeArray.setAllTo(-1);
	nnzNodes = 0;
	for (int x=0; x<Xs+1; x++) {
		for (int y=0; y<Ys+1; y++) {
			for (int z=0; z<Zs+1; z++) {
				bool occupiedNeighboringBox = false;
				for (int dx=-1; dx<1; dx++) {
					for (int dy=-1; dy<1; dy++) {
						for (int dz=-1; dz<1; dz++) {
							if (m_boxArray.validIndices(x + dx, y + dy, z + dz)) {
								if (m_boxArray(x + dx, y + dy, z + dz) != -1) occupiedNeighboringBox = true;								
							}
						}
					}
				}				

				if (occupiedNeighboringBox) m_nodeArray(x, y, z) = nnzNodes++;
			}
		}
	}

	m_nodes.assign(nnzNodes, 0);
	for (int x=0; x<Xs+1; x++) {
		for (int y=0; y<Ys+1; y++) {
			for (int z=0; z<Zs+1; z++) {
				const int nIdx = m_nodeArray(x, y, z);
				if (nIdx != -1) {
					m_nodes[nIdx] = new Deformable( m_lowerLeft + m_frac.cwiseProduct(RowVector3(x,y,z)) );
				}
			}
		}
	}

	m_nodeNodes.setConstant(nnzNodes,6,-1);
	for (int x=0; x<Xs+1; x++) {
		for (int y=0; y<Ys+1; y++) {
			for (int z=0; z<Zs+1; z++) {
				if(m_nodeArray(x,y,z) == -1) continue;

				int idNode = m_nodeArray(x,y,z);

				int k = 0;
				for (int dw=-1; dw<=1; dw+=2) {
					if (m_nodeArray.validIndices(x + dw, y, z)) {
						m_nodeNodes(idNode,k) = m_nodeArray(x + dw, y, z);								
					}
					k++;
					if (m_nodeArray.validIndices(x, y + dw, z)) {
						m_nodeNodes(idNode,k) = m_nodeArray(x, y + dw, z);								
					}
					k++;
					if (m_nodeArray.validIndices(x, y, z + dw)) {
						m_nodeNodes(idNode,k) = m_nodeArray(x, y, z + dw);								
					}
					k++;
				}
			}
		}
	}

	m_boxNodes.setConstant(nnzBoxes, 8, -1);
	for (int x=0; x<Xs; x++) {
		for (int y=0; y<Ys; y++) {
			for (int z=0; z<Zs; z++) {
				const int idBox = m_boxArray(x,y,z);
				if (idBox == -1) continue;
				
				int cnt = 0;
				for (int dx=0; dx<2; dx++) {
					for (int dy=0; dy<2; dy++) {
						for (int dz=0; dz<2; dz++) {
							m_boxNodes(idBox,cnt++) = m_nodeArray(x + dx, y + dy, z + dz);
						}
					}
				}
			}
		}
	}

	m_boxBoxes.setConstant(nnzBoxes,6,-1);
	for (int x=0; x<Xs; x++) {
		for (int y=0; y<Ys; y++) {
			for (int z=0; z<Zs; z++) {
				if(m_boxArray(x,y,z) == -1) continue;

				int idBox = m_boxArray(x,y,z);

				int k = 0;
				for (int dw=-1; dw<=1; dw+=2) {
					if (m_boxArray.validIndices(x + dw, y, z)) {
						m_boxBoxes(idBox,k) = m_boxArray(x + dw, y, z);								
					}
					k++;
					if (m_boxArray.validIndices(x, y + dw, z)) {
						m_boxBoxes(idBox,k) = m_boxArray(x, y + dw, z);								
					}
					k++;
					if (m_boxArray.validIndices(x, y, z + dw)) {
						m_boxBoxes(idBox,k) = m_boxArray(x, y, z + dw);								
					}
					k++;
				}
			}
		}
	}

	m_depth.assign(nnzBoxes,-1);
	std::queue<int> voxBFS;
	m_maxDepth = 0;
	for (int x=0; x<Xs; x++) {
		for (int y=0; y<Ys; y++) {
			for (int z=0; z<Zs; z++) {
				if(m_boxArray(x,y,z) == -1) continue;

				int idBox = m_boxArray(x,y,z);

				bool hull = false;
				for (int dx=-1; !hull && dx<=1; ++dx) {
					for (int dy=-1; !hull && dy<=1; ++dy) {
						for (int dz=-1; !hull && dz<=1; ++dz) {
							if (dx == 0 && dy == 0 && dz == 0) continue;
							if (!m_boxArray.validIndices(x + dx, y + dy, z + dz) || m_boxArray(x + dx, y + dy, z + dz) == -1) {
								hull = true;							
							}
						}
					}
				}
				
				if(hull) {
					m_depth[idBox] = 0;
					voxBFS.push(idBox);
				}
			}
		}
	}

	while(!voxBFS.empty()) {
		int idBox = voxBFS.front();
		voxBFS.pop();
		for(int i = 0 ; i < 6 ; ++i) {
			int idNeighbor = m_boxBoxes(idBox,i);
			if(idNeighbor != -1 && m_depth[idNeighbor] == -1) {
				m_depth[idNeighbor] = m_depth[idBox]+1;
				voxBFS.push(idNeighbor);
			}
		}
	}
	for(int idBox = 0 ; idBox < getNumBoxes() ; ++idBox) {
		if(m_depth[idBox] == -1) {
			cout << "Error : there is a voxel with uninitizialized depth, this should not be possible !" << endl;
		}
		else if(m_depth[idBox] > m_maxDepth)
			m_maxDepth = m_depth[idBox];
	}
}

void BoxGrid::initFromFile(string filename) {
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];

	ifstream fin;

	fin.open(filename,std::ios::in);
	if(fin.fail()) {
		cout << "I/O error" << endl;
	}
	else {
		m_boxArray.setAllTo(-1);
		nnzBoxes = 0;
		for (int x=0; x<Xs; x++) {
			for (int y=0; y<Ys; y++) {
				for (int z=0; z<Zs; z++) {
					int idBox;
					fin >> idBox;
					if(idBox >= 0) {
						m_boxArray(x,y,z) = idBox;
						nnzBoxes++;
					}
				}
			}
		}
	}
	fin.close();
}

void BoxGrid::saveToFile(string voxfile, string bbwfile, const int numHandles) const {
	const int & Xs = m_size[0];
	const int & Ys = m_size[1];
	const int & Zs = m_size[2];

	ofstream fout;
	fout.open(voxfile, std::ios::out);
	if(fout.fail()) {
		cout << "I/O error" << endl;
	}
	else {
		for (int x=0; x<Xs; x++) {
			for (int y=0; y<Ys; y++) {
				for (int z=0; z<Zs; z++) {
					fout <<	m_boxArray(x,y,z) << " ";
				}
				fout << endl;
			}
		}
	}
	fout.close();

	fout.open(bbwfile, std::ios::out);
	if(fout.fail()) {
		cout << "I/O error" << endl;
	}
	else {
		for(int idNode = 0 ; idNode < getNumNodes() ; ++idNode) {
			for(int j = 0 ; j < numHandles ; ++j) {
				fout <<	m_nodes[idNode]->getCoord(j) << " ";
			}
			fout << endl;
		}
	}
	fout.close();
}

void BoxGrid::clearFilling(int hullDepth) {
	
	m_boxStatus.assign(getNumBoxes(), false);
	for(int i = 0 ; i < getNumBoxes() ; ++i) {
		m_boxStatus[i] = (m_depth[i] <= hullDepth);
	}

}

void BoxGrid::clearCarving() {
	
	m_boxStatus.assign(getNumBoxes(), true);

}

void BoxGrid::freeAll()
{
	m_nodeArray.free();
	m_boxArray.free();

	for(unsigned int i = 0 ; i < m_nodes.size() ; ++i)
		if(m_nodes[i]) delete m_nodes[i];
	m_nodes.clear();

	m_boxStatus.clear();
}

int BoxGrid::getBoxContainingPoint(const RowVector3 & P, RowVector3 & t) const
{
	if(P.cwiseMin(m_lowerLeft) != m_lowerLeft || P.cwiseMax(m_upperRight) != m_upperRight) {
		cout << "Error : point is outside the voxelgrid" << endl;
		//P = P.cwiseMax(m_lowerLeft).cwiseMin(m_upperRight);
		return -1;
	}
	RowVector3 floatIndices = (P - m_lowerLeft).cwiseQuotient(m_frac);
	RowVector3i intIndices((int)floor(floatIndices[0]),(int)floor(floatIndices[1]),(int)floor(floatIndices[2]));
	
	if(!m_boxArray.validIndices(intIndices[0],intIndices[1],intIndices[2])) {
		cout << "Error : position in voxelgrid incorrect" << endl;
		return -1;
	}
	t = floatIndices - RowVector3(intIndices[0],intIndices[1],intIndices[2]);
	
	return m_boxArray(intIndices[0],intIndices[1],intIndices[2]);
}

bool BoxGrid::getBoxCoordsContainingPoint(const RowVector3 & P, RowVector3i & t) const
{
	if(P.cwiseMin(m_lowerLeft) != m_lowerLeft || P.cwiseMax(m_upperRight) != m_upperRight) {
		cout << "Error : point is outside the voxelgrid" << endl;
		//P = P.cwiseMax(m_lowerLeft).cwiseMin(m_upperRight);
		return false;
	}
	RowVector3 floatIndices = (P - m_lowerLeft).cwiseQuotient(m_frac);
	t = RowVector3i((int)floor(floatIndices[0]),(int)floor(floatIndices[1]),(int)floor(floatIndices[2]));
	
	if(!m_boxArray.validIndices(t[0],t[1],t[2])) {
		cout << "Error : position in voxelgrid incorrect" << endl;
		return false;
	}
	return true;
}

int BoxGrid::getNodeClosestToPoint(const RowVector3 & P) const
{

	if(P.cwiseMin(m_lowerLeft) != m_lowerLeft || P.cwiseMax(m_upperRight) != m_upperRight) {
		cout << "Error : point is outside the voxelgrid" << endl;
		//P = P.cwiseMax(m_lowerLeft).cwiseMin(m_upperRight);
		return -1;
	}
	RowVector3 floatIndices = (P - m_lowerLeft).cwiseQuotient(m_frac);
	RowVector3i intIndices((int)floor(floatIndices[0]),(int)floor(floatIndices[1]),(int)floor(floatIndices[2]));
	
	if(!m_boxArray.validIndices(intIndices[0],intIndices[1],intIndices[2])) {
		cout << "Error : position in voxelgrid incorrect" << endl;
		return -1;
	}
	RowVector3 t = floatIndices - RowVector3(intIndices[0],intIndices[1],intIndices[2]);
	int dx = (t[0]<=0.5)?0:1;
	int dy = (t[1]<=0.5)?0:1;
	int dz = (t[2]<=0.5)?0:1;
	
	return m_nodeArray(intIndices[0]+dx,intIndices[1]+dy,intIndices[2]+dz);
}


	float BoxGrid::getWeight(int idHandle, int idNode) const {
		if(idNode >= 0)
			return m_nodes[idNode]->getCoord(idHandle);
		else return 1.0f;
	}

	void BoxGrid::getInterpolatedBBW(const RowVector3 & P, Deformable * deformInfo, const int nbWeights) const {
		RowVector3 t;
		int idBox = getBoxContainingPoint(P,t);
		if(idBox == -1) {
			cout << "Error cannot create BBW for this mesh vertex" << endl;
			return;
		}

		for(int j = 0 ; j < nbWeights ; ++j) {
			float wj = 0.0f;
			for(int i = 0 ; i < 8 ; ++i) {
				const int idNode = m_boxNodes(idBox,i);
				float alpha = 1.0f;
				alpha *= (((i/4) % 2 == 0)?(1.0f-t[0]):t[0]);
				alpha *= (((i/2) % 2 == 0)?(1.0f-t[1]):t[1]);
				alpha *= ((i % 2 == 0)?(1.0f-t[2]):t[2]);
				wj += alpha * m_nodes[idNode]->getCoord(j);
			}
			deformInfo->pushWeight(wj);
		}
	}

	void BoxGrid::importBBW(string filename, const int numHandles) {
		ifstream fin;

		fin.open(filename,std::ios::in);
		if(fin.fail()) {
			cout << "I/O error" << endl;
		}
		else {
			ScalarType wi;
			for(int idNode = 0 ; idNode < getNumNodes() ; ++idNode) {
				for(int j = 0 ; j < numHandles ; ++j) {
					fin >> wi; m_nodes[idNode]->pushWeight(wi);
				}
				m_nodes[idNode]->normalizeWeights();
			}
		}
		fin.close();
		
	}

	void BoxGrid::computeBBW(const Handles & handles) {
		
		SparseMatrix L, L2;
		L.resize(getNumNodes(), getNumNodes());

		vector<SparseMatrixTriplet> L_MEL;
		L_MEL.clear();	
		vector<int> valences;
		valences.assign(getNumNodes(), 0);
		for(int v1 = 0 ; v1<getNumNodes() ; ++v1) {
			for(int k = 0 ; k < 6 ; ++k) {
				int v2 = m_nodeNodes(v1,k);
				if(v2 != -1) {
					L_MEL.push_back(SparseMatrixTriplet(v1,v2,-1.0f));
					valences[v1]++;
				}
			}
		}
		for (int v=0; v<getNumNodes(); v++) {
			L_MEL.push_back(SparseMatrixTriplet(v, v, valences[v]));
		}

		L.setFromTriplets(L_MEL.begin(),L_MEL.end());	
		L2 = L*L;

		SparseMatrix A(handles.getNumConstraints(),getNumNodes());
		vector<SparseMatrixTriplet> A_MEL;
		A_MEL.reserve(handles.getNumConstraints());
		for(int i = 0 ; i < handles.getNumConstraints() ; ++i) {
			A_MEL.push_back(SparseMatrixTriplet(i, handles.getConstraint(i), 1.0));
		}
		A.setFromTriplets(A_MEL.begin(), A_MEL.end());
	
		VectorX x;
		VectorX bj;
		MOSEKinterface mi0;
		MatrixXX zeros0(L2.rows(), 1); zeros0.setZero();

		int constraintRow = 0;
		for(int j=0; j < handles.getNumHandles() ; ++j) {
			bj.setZero(handles.getNumConstraints(),1);

			int idFirst = handles.getIdFirstConstraint(j);
			int idLast = idFirst + handles.getNumConstraints(j) -1;

			for(int k = idFirst ; k <= idLast ; ++k)
				bj[k] = 1.0f;

			x.setZero(getNumNodes(),1);
			mi0.solveQP_BBW_type(x, L2, zeros0, A, bj, 1, MOSEKinterface::PRINT_LOG);

			for (int i=0; i<getNumNodes(); i++) {
				m_nodes[i]->pushWeight((ScalarType) x[i]);
			}
		}

		for (int i=0; i<getNumNodes(); i++) {
			m_nodes[i]->normalizeWeights();
		}
	}
	
	const RowVector3 & BoxGrid::getCurrentPose(int idNode) const {
		return m_nodes[idNode]->getCurrentPose();
	}
	const RowVector3 & BoxGrid::getRestPose(int idNode) const {
		return m_nodes[idNode]->getRestPose();
	}

	void BoxGrid::updatePoses(const Handles & handles) {
		#pragma omp parallel for
		for(int i = 0 ; i < getNumNodes() ; ++i) {
			m_nodes[i]->computeCurrentPose(handles);
		}
	}

	static const int g_triangulateCube[12][3] = {
		{1,2,0},{1,3,2},
		{4,7,5},{4,6,7},
		{3,6,2},{3,7,6},
		{5,1,0},{5,0,4},
		{0,2,6},{0,6,4},
		{5,7,3},{5,3,1}
	};

	void BoxGrid::computeBoxPositions() {

		m_boxPositions.setZero(getNumBoxes(),3);

		#pragma omp parallel for
		for(int i = 0 ; i < getNumBoxes() ; i++) {
			for(int k = 0 ; k < 8 ; ++k) {
				m_boxPositions.row(i) += getCurrentPose(m_boxNodes(i,k));
			}
		}

		m_boxPositions *= 0.125;

	}

	void BoxGrid::getMassAndCenterOfMassBox(int idBox, ScalarType & m, Vector3 & c) {
		
		m = 0;
		c.setZero();

		getMassAndCenterOfMassVoxel(
			getCurrentPose(m_boxNodes(idBox,0)),
			getCurrentPose(m_boxNodes(idBox,1)),
			getCurrentPose(m_boxNodes(idBox,2)),
			getCurrentPose(m_boxNodes(idBox,3)),
			getCurrentPose(m_boxNodes(idBox,4)),
			getCurrentPose(m_boxNodes(idBox,5)),
			getCurrentPose(m_boxNodes(idBox,6)),
			getCurrentPose(m_boxNodes(idBox,7)),
			m,c);

		m /= 6.0;
		c /= 24.0;
	}

	void BoxGrid::getMassAndCenterOfMass(ScalarType & m, Vector3 & c) {

		m = 0;
		c.setZero();

		for(int idBox = 0 ; idBox < getNumBoxes() ; idBox++) {
			if(isFilled(idBox)) {
				ScalarType mTmp; Vector3 cTmp;
				getMassAndCenterOfMassBox(idBox,mTmp,cTmp);
				m += mTmp;
				c += cTmp;
			}
		}

	}