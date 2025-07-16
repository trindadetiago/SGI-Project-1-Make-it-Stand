#include "Mesh.h"

#include "core/Deformable.h"
#include "core/BoxGrid.h"
#include "core/Handles.h"
#include "core/Support.h"
#include "MIS\config.h"

#include "utils\meshIO\readMesh.h"
#include "utils\meshIO\writeSTL.h"

	Mesh::Mesh(string filename) {
		readMeshfile(filename,vertices,faces);

		nbV = vertices.rows();
		nbF = faces.rows();

		InitNeighboringData();
		ComputeBoundingBox();

		deformInfo.assign(nbV,0);
		for(IndexType i = 0 ; i < nbV ; ++i) {
			deformInfo[i] = new Deformable( vertices.row(i) );
		}
		
		corner_threshold = 32;
		ComputeNormals();
		ComputeLaplacian();

		mList.setZero(1,nbF);
		cList.setZero(3,nbF);
		dmList.setZero(3,3*nbF);
		dcList.setZero(9,3*nbF);
	}

	Mesh::~Mesh() {
		for(unsigned int i = 0 ; i < deformInfo.size() ; ++i)
			delete deformInfo[i];
		deformInfo.clear();
	}

	static inline bool inTheList(const vector<IndexType> & l, IndexType i) {
		return (std::find(l.begin(),l.end(),i) != l.end());
	}

	void Mesh::InitNeighboringData() {

		vertex_to_faces.clear();
		vertex_to_faces.resize(nbV);
		vertex_to_vertices.clear();
		vertex_to_vertices.resize(nbV);

		for(int fi = 0; fi < nbF; fi++) {
			for(IndexType j = 0 ; j < 3 ; ++j) {
				vertex_to_faces[F(fi,j)].push_back(fi);

				IndexType v0 = F(fi,j);
				IndexType v1 = F(fi,(j+1)%3);
				IndexType v2 = F(fi,(j+2)%3);

				if(!inTheList(vertex_to_vertices[v0],v1)) vertex_to_vertices[v0].push_back(v1);
				//if(!inTheList(vertex_to_vertices[v0],v2)) vertex_to_vertices[v0].push_back(v2);
				else cout << "Error : non conform orientation of triangles or non-manifold edge" << endl;
			}
		}

		face_to_faces.setConstant(nbF,3,INDEX_NULL);
		for(int fi = 0 ; fi < nbF ; ++fi) {
			for(int i = 0 ; i < 3 ; ++i) {
				IndexType v0 = faces(fi,i);
				IndexType v1 = faces(fi,(i+1)%3);
				for(unsigned int j = 0 ; j < vertex_to_faces[v0].size() ; ++j) {
					IndexType fj = vertex_to_faces[v0][j];
					if(fj != fi && inTheList(vertex_to_faces[v1],fj)) {
						face_to_faces(fi,i) = fj;
						break;
					}
				}
				if(face_to_faces(fi,i) == INDEX_NULL) cout << "Error : non closed mesh" << endl;
			}
		}
	}

	void Mesh::ComputeNormals() {
		
		FNormals.setZero(nbF,3);
		#pragma omp parallel for
		for(int fi = 0; fi < nbF; ++fi) {
			const RowVector3 & p0 = getCurrentPose(faces(fi,0));
			const RowVector3 & p1 = getCurrentPose(faces(fi,1));
			const RowVector3 & p2 = getCurrentPose(faces(fi,2));
			
			FNormals.row(fi) = ((p1-p0).cross(p2-p0)).normalized();
			//FNormals.row(fi).normalize();
		}

		VNormals.setZero(nbV,3);
		#pragma omp parallel for
		for(int vi = 0; vi < nbV; ++vi) {
			RowVector3 n = RowVector3::Zero();
			for(IndexType j = 0; j < vfNbNeighbors(vi); ++j) {
				n += FNormals.row(vfNeighbor(vi,j));
			}
			VNormals.row(vi) = n.normalized();
		}

		ComputeCornerNormals();

	}

	void Mesh::ComputeCornerNormals() {
		double t = cos(clamp(corner_threshold,0.0,180.0)*M_PI/180.0);
		CNormals.setZero(nbF*3,3);
		#pragma omp parallel for
		for(int fi = 0 ; fi < nbF ; ++fi) {
			for(int j = 0 ; j < 3 ; ++j) {
				IndexType vj = faces(fi,j);
				RowVector3 n = FNormals.row(fi);
				for(int k = 0; k < vfNbNeighbors(vj); ++k) {
					IndexType fk = vfNeighbor(vj,k);
					if(fk != fi && FNormals.row(fi).dot(FNormals.row(fk)) >= t)
						n += FNormals.row(fk);
				}
				CNormals.row(3*fi+j) = n.normalized();
			}
		}

	}

	void Mesh::ComputeBoundingBox() {
		bmin = bmax = vertices.row(0);
		
		for(int i=1; i < nbV; i++) {
			bmin = bmin.cwiseMin(vertices.row(i));
			bmax = bmax.cwiseMax(vertices.row(i));    
		}
		
		//cout << bmin << " " << bmax << endl;

		RowVector3 length = (bmax - bmin).cwiseInverse();
		ScalarType min_scale = 0.95*length.minCoeff();
		RowVector3 center = (bmin+bmax)/2.0;
		
		for(int i=0; i < nbV; i++) {
			vertices.row(i) = min_scale*(vertices.row(i)-center) + RowVector3(0.5,0.5,0.5);
		}

		bmin = bmax = vertices.row(0);
		
		for(int i=1; i < nbV; i++) {
			bmin = bmin.cwiseMin(vertices.row(i));
			bmax = bmax.cwiseMax(vertices.row(i));    
		}

		//cout << bmin << " " << bmax << endl;
	}

	void Mesh::computeBBW(const Handles & handles, const BoxGrid & voxels) {
		for(IndexType i = 0 ; i < nbV ; ++i) {
			voxels.getInterpolatedBBW(vertices.row(i),deformInfo[i],handles.getNumHandles());
		}
	}
	/*void Mesh::sparsifyBBW() {
		for(IndexType i = 0 ; i < nbV ; ++i) {
			deformInfo[i]->sparsify();
		}
	}*/
	
	void Mesh::updatePoses(const Handles & handles) {
		
		#pragma omp parallel for
		for(int i = 0 ; i < nbV ; ++i) {
			deformInfo[i]->computeCurrentPose(handles);
		}

		ComputeNormals();
	}

	float Mesh::getWeight(int idHandle, int idV) const {
		return deformInfo[idV]->getCoord(idHandle);
	}
	const RowVector3 & Mesh::getCurrentPose(int idV) const {
		return deformInfo[idV]->getCurrentPose();
	}
	const RowVector3 & Mesh::getRestPose(int idV) const {
		return deformInfo[idV]->getRestPose();
	}


	
	RowVector3 Mesh::getHigherVertex(const RowVector3 & gravity) const {
		ScalarType highestHeight = -std::numeric_limits<ScalarType>::infinity();

		int idBest = -1;
		for(int i = 0 ; i < nbV ; i++) {
			ScalarType height = -gravity.dot(vertices.row(i));
			if(height > highestHeight) {
				highestHeight = height;
				idBest = i;
			}
		}
		
		return vertices.row(idBest);
	}
	
	void Mesh::glDrawForVoxelizer(const int res) const {
		
		glDisable(GL_LIGHTING);
		glColor4f(1.0f,1.0f,1.0f,1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBegin(GL_TRIANGLES);
		for(int i = 0 ; i < nbF ; ++i) {
			for(int j = 0 ; j < 3 ; ++j) {
				const RowVector3 & p = getCurrentPose(faces(i,j));
				glVertex3f(res*p[0],res*p[1],res*p[2]);
			}
		}
		glEnd();
	}

	void Mesh::exportMesh(string filename, Config & cfg) {
		PointMatrixType vert;
		vert.setZero(nbV,3);
		for(int i = 0 ; i < nbV ; ++i) {
			vert.row(i) = getCurrentPose(i);
		}

		for(int idObj = 0 ; idObj < cfg.nbObj() ; ++idObj) {
			if(cfg.isStandingMode(idObj)) {

				Support * supp = cfg.getSupport(idObj);
				for(int i = 0 ; i < supp->nbVertices() ; ++i) {
					int idV = supp->getVertex(i);
					RowVector3 p = vert.row(idV);
					vert.row(idV) = supp->projectOnSupport(p);
				}
			}
		}


		igl::writeSTLforTriMesh(filename,vert,faces);
	}

	static void assignblock1(const ScalarType w, Eigen::Matrix<ScalarType,3,Eigen::Dynamic> & M, int id) {
		M(0,3*id) = w;
		M(1,3*id+1) = w;
		M(2,3*id+2) = w;
	}

	static void assignblock2(const RowVector3 & p, const ScalarType w, Eigen::Matrix<ScalarType,Eigen::Dynamic,7> & M, int id) {
		M(3*id,0) = p[0];   M(3*id,2) = p[2]; M(3*id,3) = -p[1];     M(3*id,4) = w;
		M(3*id+1,0) = p[1]; M(3*id+1,1) = -p[2]; M(3*id+1,3) = p[0]; M(3*id+1,5) = w;
		M(3*id+2,0) = p[2]; M(3*id+2,1) = p[1]; M(3*id+2,2) = -p[0]; M(3*id+2,6) = w;
	}

	void Mesh::ComputeLaplacian() {
		
		IndexType vi, vj;

		// sparse matrices are initialized using list of triplets (row,col,value)
		/*vector<SparseMatrixTriplet> tripletLw;
		tripletLw.reserve(3*6*nbV);

		for(vi = 0; vi < nbV; ++vi) {
			int Ni = vvNbNeighbors(vi);
			ScalarType wi = (1.0/Ni);

			for(int j = 0 ; j < vvNbNeighbors(vi) ; ++j) {
				vj = vvNeighbor(vi,j);
				tripletLw.push_back(SparseMatrixTriplet(3*vi,3*vj,-wi));
				tripletLw.push_back(SparseMatrixTriplet(3*vi+1,3*vj+1,-wi));
				tripletLw.push_back(SparseMatrixTriplet(3*vi+2,3*vj+2,-wi));
			}
			tripletLw.push_back(SparseMatrixTriplet(3*vi,3*vi,1.0));
			tripletLw.push_back(SparseMatrixTriplet(3*vi+1,3*vi+1,1.0));
			tripletLw.push_back(SparseMatrixTriplet(3*vi+2,3*vi+2,1.0));
		}
		
		// fill the sparse matrix with the list of triplets
		L = SparseMatrix(3*nbV,3*nbV);
		L.setFromTriplets(tripletLw.begin(), tripletLw.end());*/



		M.resize(3*nbV,3*nbV);
		vector<SparseMatrixTriplet> tripletM;
		tripletM.reserve(3*9*nbV*6);

		for(vi = 0 ; vi < nbV ; ++vi) {

			int Ni = vvNbNeighbors(vi);
			ScalarType wi = (1.0/Ni);

			Eigen::Matrix<ScalarType,Eigen::Dynamic,7> Ai;
			Ai.setZero(3*(Ni+1),7);
			Eigen::Matrix<ScalarType,3,Eigen::Dynamic> Li;
			Li.setZero(3,3*(Ni+1));
			
			assignblock2(getCurrentPose(vi),1.0,Ai,0);
			assignblock1(1,Li,0);
			for(int j = 0 ; j < Ni ; ++j) {
				vj = vvNeighbor(vi,j);
				
				assignblock2(getCurrentPose(vj),1.0,Ai,j+1);
				assignblock1(-wi,Li,j+1);
			}

			Eigen::Matrix<ScalarType,Eigen::Dynamic,7> Ki;
			Ki.setZero(3,7);

			RowVector3 deltai = Li * Ai.col(0);
			
			assignblock2(deltai,0.0,Ki,0);

			Eigen::Matrix<ScalarType,7,7> AAinv = Ai.transpose() * Ai;

			/*cout << "AA" << endl;
			cout << AAinv << endl << endl;
			cout << "AA-1" << endl;
			cout << AAinv.inverse() << endl << endl;*/

			AAinv = AAinv.inverse();

			Eigen::Matrix<ScalarType,3,Eigen::Dynamic> Ui;
			Ui.resize(3,3*(Ni+1));
			Ui = (Ki * AAinv * Ai.transpose()) - Li;

			//cout << AAinv << endl;
			
			/*VectorX tt(Ai.rows(),1); tt.setZero();
			for(int l = 0 ; l < Ai.rows() ; l+=3) {
				tt[l] = 8.3;
				tt[l+1] = 7.5;
				tt[l+2] = -3.4;
			}

			cout << endl << endl;
			cout << tt.transpose() << endl;
			cout << deltai << endl;
			cout << (Li * Ai.col(0)).transpose() << endl;
			cout << (Li * (Ai.col(0)+tt)).transpose() << endl;
			cout << (Ui * Ai.col(0)).transpose() << endl;
			cout << (Ui * (Ai.col(0)+tt)).transpose() << endl;
			cout << (AAinv * Ai.transpose() * Ai.col(0)).transpose() << endl;
			cout << (AAinv * Ai.transpose() * (Ai.col(0)+tt)).transpose() << endl;*/

			for(int kr = 0 ; kr < 3 ; ++kr) {
				for(int kc = 0 ; kc < 3 ; ++kc) {
					tripletM.push_back(SparseMatrixTriplet(3*vi+kr,3*vi+kc,Ui(kr,kc)));
				}
			}
			for(int j = 0 ; j < Ni ; ++j) {
				vj = vvNeighbor(vi,j);
				
				for(int kr = 0 ; kr < 3 ; ++kr) {
					for(int kc = 0 ; kc < 3 ; ++kc) {
						tripletM.push_back(SparseMatrixTriplet(3*vi+kr,3*vj+kc,Ui(kr,3*(j+1)+kc)));
					}
				}
			}

			/*cout << "vi = " << vi << ", Ni =" << Ni << endl;
			cout << "v =" << getCurrentPose(vi) << ", d =" << deltai << endl;*/
			//cout << "T(deltai) : " << (Ui * Ai.col(0)).transpose() << endl;
			/*cout << Li * Ai.col(0) << endl;
			cout << "Ai = " << endl;
			cout << Ai << endl;
			cout << "Ki = " << endl;
			cout << Ki << endl;
			cout << "Ui = " << endl;
			cout << Ui << endl;

			cout << endl << endl;*/

		}

		M.setFromTriplets(tripletM.begin(),tripletM.end());

		MM.resize(3*nbV,3*nbV);
		MM = M.transpose() * M;

	}

	/*void Mesh::getDiffLaplacian(PointMatrixType & LL) const {
		for(int i = 0 ; i < nbV ; ++i) {
			LL.row(i) = deformInfo[i]->getCurrentPose() - deformInfo[i]->getRestPose();
		}
		LL = L * LL;
	}
	void Mesh::getWeightLaplacian(int j, VectorX & LW) const {
		for(int i = 0 ; i < nbV ; ++i) {
			LW[i] = deformInfo[i]->getCoord(j);
		}
		LW = L * LW;
	}*/

	
	void Mesh::getMassAndCenterOfMass(ScalarType & m, Vector3 & c, MList & dm, CList & dc) {
				
		#pragma omp parallel for
		for(int idF = 0 ; idF < nbF ; ++idF) {

			getMassAndCenterOfMassTriangleList(
				getCurrentPose(faces(idF,0)),
				getCurrentPose(faces(idF,1)),
				getCurrentPose(faces(idF,2)),
				idF,
				mList,cList,
				dmList,dcList);

		}
		
		m = 0; c.setZero();
		for(int idF = 0 ; idF < nbF ; ++idF) {	
			m += mList[idF];
			c[0] += cList(0,idF);
			c[1] += cList(1,idF);
			c[2] += cList(2,idF);
		}
		m /= 6.0;
		c /= 24.0;

		dm.setZero(3*nbV);
		dc.setZero(3,3*nbV);

		#pragma omp parallel for
		for(int idV = 0 ; idV < nbV ; ++idV) {
			for(int j = 0 ; j < vfNbNeighbors(idV) ; ++j) {
				const int idVF = 3*vfNeighbor(idV,j) + vfNeighborId(idV,j);

				// derivative of mass
				dm[3*idV] += dmList(0,idVF);
				dm[3*idV+1] += dmList(1,idVF);
				dm[3*idV+2] += dmList(2,idVF);

				// derivative of center of mass			
				dc(0,3*idV)   += dcList(0,idVF);
				dc(1,3*idV)   += dcList(1,idVF);
				dc(2,3*idV)   += dcList(2,idVF);
				dc(0,3*idV+1) += dcList(3,idVF);
				dc(1,3*idV+1) += dcList(4,idVF);
				dc(2,3*idV+1) += dcList(5,idVF);
				dc(0,3*idV+2) += dcList(6,idVF);
				dc(1,3*idV+2) += dcList(7,idVF);
				dc(2,3*idV+2) += dcList(8,idVF);
			}
		}
	}
		
	void Mesh::getMassAndCenterOfMass(ScalarType & m, Vector3 & c) {
		

		#pragma omp parallel for
		for(int idF = 0 ; idF < nbF ; ++idF) {

			getMassAndCenterOfMassTriangleList(
				getCurrentPose(faces(idF,0)),
				getCurrentPose(faces(idF,1)),
				getCurrentPose(faces(idF,2)),
				idF,
				mList,cList);

		}
		
		m = 0; c.setZero();
		for(int idF = 0 ; idF < nbF ; ++idF) {	
			m += mList[idF];
			c[0] += cList(0,idF);
			c[1] += cList(1,idF);
			c[2] += cList(2,idF);
		}
		m /= 6.0;
		c /= 24.0;

	}