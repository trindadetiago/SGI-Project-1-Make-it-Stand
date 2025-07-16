#include "MIS/optimizer.h"

#include "MIS/common.h"
#include "MIS\config.h"
#include "core/BoxGrid.h"
#include "core/Handles.h"
#include "core/Mesh.h"
#include "core/InnerMesh.h"

	static bool sortVoxel(pair<int,ScalarType> & a, pair<int,ScalarType> & b) { return (a.second>b.second); }

	void Optimizer::balanceByPlaneCarving(int hullDepth) {

		voxels.clearCarving();
		voxels.computeBoxPositions();
				
		RowVector3 com = cO/mO;

		const RowVector3 & g = cfg.getGravity(0);
		const RowVector3 & target = cfg.getTarget(0);
		RowVector3 cc = com - target;
		cc -= (cc.dot(g)) * g;

		ScalarType bestEnergy = 0.5 * cc.squaredNorm();
		ScalarType currentEnergy;

		cc.normalize();

		vector< pair<int,ScalarType> > listVoxels;
		for(int i = 0 ; i < voxels.getNumBoxes() ; ++i) {
			if(voxels.getDepth(i) > hullDepth) {
				ScalarType d = (voxels.getBoxPosition(i)-target).dot(cc);
				if(d > 0) {
					listVoxels.push_back(std::make_pair(i,d));
				}
			}
		}

		std::sort(listVoxels.begin(),listVoxels.end(),sortVoxel);

		ScalarType m2 = 0;
		Vector3 c2 = RowVector3::Zero();
		
		ScalarType mTmp; Vector3 cTmp;
		int bestId = -1;
		for(unsigned int i = 0 ; i < listVoxels.size() ; ++i) {
			int idBox = listVoxels[i].first;

			voxels.getMassAndCenterOfMassBox(idBox,mTmp,cTmp);
			m2 -= mTmp; c2 -= cTmp;

			com = (cO+c2)/(mO+m2);
			 
			cc = com - target;
			cc -= (cc.dot(g)) * g;
			currentEnergy = 0.5 * cc.squaredNorm();

			if(currentEnergy < bestEnergy) {
				bestEnergy = currentEnergy;
				bestId = i;
			}
		}

		for(int i = 0 ; i <= bestId ; ++i) {
			voxels.setCarved(listVoxels[i].first);
		}

	}

	void Optimizer::balanceByPlaneCarvingMulti(int hullDepth) {

		voxels.clearCarving();
		voxels.computeBoxPositions();
		
		RowVector3 com = cO/mO;

		const RowVector3 & g0 = cfg.getGravity(0);
		const RowVector3 & target0 = cfg.getTarget(0);
		RowVector3 cc0 = com - target0;
		cc0 -= (cc0.dot(g0)) * g0;
		
		const RowVector3 & g1 = cfg.getGravity(1);
		const RowVector3 & target1 = cfg.getTarget(1);
		RowVector3 cc1 = com - target1;
		cc1 -= (cc1.dot(g1)) * g1;

		ScalarType bestEnergy = 0.5 * (cc0.squaredNorm() + cc1.squaredNorm());
		ScalarType currentEnergy;

		cc0.normalize();
		cc1.normalize();

		vector< pair<int,ScalarType> > listVoxels;
		for(int i = 0 ; i < voxels.getNumBoxes() ; ++i) {
			if(voxels.getDepth(i) > hullDepth && voxels.isFilled(i)) {
				ScalarType d0 = (voxels.getBoxPosition(i)-target0).dot(cc0);
				ScalarType d1 = (voxels.getBoxPosition(i)-target1).dot(cc1);
				if(d0 > 0 && d1 > 0) {
					listVoxels.push_back(std::make_pair(i,d0+d1));
				}
			}
		}

		std::sort(listVoxels.begin(),listVoxels.end(),sortVoxel);

		ScalarType m2 = 0;
		Vector3 c2 = RowVector3::Zero();
		
		ScalarType mTmp; Vector3 cTmp;
		int bestId = -1;
		for(unsigned int i = 0 ; i < listVoxels.size() ; ++i) {
			int idBox = listVoxels[i].first;

			voxels.getMassAndCenterOfMassBox(idBox,mTmp,cTmp);
			m2 -= mTmp; c2 -= cTmp;

			com = (cO+c2)/(mO+m2);
			cc0 = com - target0;
			cc0 -= (cc0.dot(g0)) * g0;
			cc1 = com - target1;
			cc1 -= (cc1.dot(g1)) * g1;
			currentEnergy = 0.5 * (cc0.squaredNorm() + cc1.squaredNorm());

			if(currentEnergy < bestEnergy) {
				bestEnergy = currentEnergy;
				bestId = i;
			}
		}

		for(int i = 0 ; i <= bestId ; ++i) {
			voxels.setCarved(listVoxels[i].first);
		}

	}

	void Optimizer::prepare() {
		
		RowVector3 p;

		dvO_dTx.setZero(3*mesh.nbV,handles.getNumHandles());
		dvO_dTy.setZero(3*mesh.nbV,handles.getNumHandles());
		dvO_dTz.setZero(3*mesh.nbV,handles.getNumHandles());
		dvO_dS.setZero(3*mesh.nbV,handles.getNumHandles());
		for(int i = 0 ; i < mesh.nbV ; ++i) {
			for(int j = 0 ; j < handles.getNumHandles() ; ++j) {

				float wij = mesh.getWeight(j,i);

				dvO_dTx(3*i,j) = wij;
				dvO_dTy(3*i+1,j) = wij;
				dvO_dTz(3*i+2,j) = wij;

				handles.getGradScale(j,mesh.getRestPose(i),p);
				dvO_dS(3*i,j) = wij*p[0];
				dvO_dS(3*i+1,j) = wij*p[1];
				dvO_dS(3*i+2,j) = wij*p[2];
			}
		}
		
		MM_dvO_dTx = mesh.MM * dvO_dTx;
		MM_dvO_dTy = mesh.MM * dvO_dTy;
		MM_dvO_dTz = mesh.MM * dvO_dTz;
		MM_dvO_dS = mesh.MM * dvO_dS;
	
		dvI_dTx.setZero(3*voxels.getNumNodes(),handles.getNumHandles());
		dvI_dTy.setZero(3*voxels.getNumNodes(),handles.getNumHandles());
		dvI_dTz.setZero(3*voxels.getNumNodes(),handles.getNumHandles());
		dvI_dS.setZero(3*voxels.getNumNodes(),handles.getNumHandles());
		for(int i = 0 ; i < voxels.getNumNodes() ; ++i) {
			for(int j = 0 ; j < handles.getNumHandles() ; ++j) {

				float wij = voxels.getWeight(j,i);

				dvI_dTx(3*i,j) = wij;
				dvI_dTy(3*i+1,j) = wij;
				dvI_dTz(3*i+2,j) = wij;

				handles.getGradScale(j,voxels.getRestPose(i),p);
				dvI_dS(3*i,j) = wij*p[0];
				dvI_dS(3*i+1,j) = wij*p[1];
				dvI_dS(3*i+2,j) = wij*p[2];
			}
		}

		dEC_dTx.setZero(handles.getNumHandles());
		dEC_dTy.setZero(handles.getNumHandles());
		dEC_dTz.setZero(handles.getNumHandles());
		dEC_dS.setZero(handles.getNumHandles());

		dEL_dTx.setZero(handles.getNumHandles());
		dEL_dTy.setZero(handles.getNumHandles());
		dEL_dTz.setZero(handles.getNumHandles());
		dEL_dS.setZero(handles.getNumHandles());
	}

	RowVector3 Optimizer::getGradT(int i) const {
		return (1.0-MIS::mu) * RowVector3(dEC_dTx[i],dEC_dTy[i],dEC_dTz[i]) + MIS::mu * MIS::lambda * RowVector3(dEL_dTx[i],dEL_dTy[i],dEL_dTz[i]);
	}
	ScalarType Optimizer::getGradS(int i) const {
		return (1.0-MIS::mu) * dEC_dS[i] + MIS::mu * MIS::lambda * dEL_dS[i];
	}


	void Optimizer::compute_COM_MO() {
		mesh.getMassAndCenterOfMass(mO,cO,dmO,dcO);
		
		vO.setZero(3*mesh.nbV);
		#pragma omp parallel for
		for(int i = 0 ; i < mesh.nbV ; ++i) {
			const RowVector3 & v = mesh.getCurrentPose(i);
			vO[3*i] = v[0];
			vO[3*i+1] = v[1];
			vO[3*i+2] = v[2];
		}
	}

	void Optimizer::compute_COM_MI() {
		imesh.getMassAndCenterOfMass(mI,cI,dmI,dcI);
	}

	void Optimizer::compute_COM() {
		mass = (mO+mI);
		centerOfMass = (cO+cI)/mass;
		
		//cout << mass << " " << centerOfMass.transpose() << endl;
	}

	void Optimizer::compute_dEC() {
		dc_dvO  = ((dcO/24.0f) - ((centerOfMass/6.0f)*dmO))/mass;
		dc_dvI = ((dcI/24.0f) - ((centerOfMass/6.0f)*dmI))/mass;
		
		dc_dTx = dc_dvO * dvO_dTx + dc_dvI * dvI_dTx;
		dc_dTy = dc_dvO * dvO_dTy + dc_dvI * dvI_dTy;
		dc_dTz = dc_dvO * dvO_dTz + dc_dvI * dvI_dTz;
		dc_dS = dc_dvO * dvO_dS  + dc_dvI * dvI_dS;

		RowVector3 ccs = RowVector3::Zero();
		for(int idObj = 0 ; idObj < cfg.nbObj() ; ++idObj) {
			const RowVector3 & g = cfg.getGravity(idObj);
			RowVector3 cc = centerOfMass.transpose() - cfg.getTarget(idObj);
			cc -= (cc.dot(g)) * g;
			ccs += cc;
		}

		dEC_dTx = ccs * dc_dTx;
		dEC_dTy = ccs * dc_dTy;
		dEC_dTz = ccs * dc_dTz;
		dEC_dS  = ccs * dc_dS;

	}
	void Optimizer::compute_dEL() {
		dEL_dTx = (vO * MM_dvO_dTx);
		dEL_dTy = (vO * MM_dvO_dTy);
		dEL_dTz = (vO * MM_dvO_dTz);
		dEL_dS  = (vO * MM_dvO_dS);
	}
	void Optimizer::compute_GradEnergy() {
		compute_dEC();
		compute_dEL();
	}
	void Optimizer::apply_GradEnergy() {
		for(int j = cfg.nbObj() ; j < handles.getNumHandles() ; ++j) {
			if(!handles.isLocked(j)) {
				if(j >= cfg.nbObj()) handles.translate(j,-MIS::step*getGradT(j));
				if(MIS::use_scaling) handles.scale(j,-MIS::step*getGradS(j));
			}
		}
	}

	void Optimizer::compute_EC() {
		EC[0] = EC[1] = 0;

		for(int idObj = 0 ; idObj < cfg.nbObj() ; ++idObj) {
			const RowVector3 & g = cfg.getGravity(idObj);
			RowVector3 cc = centerOfMass.transpose() - cfg.getTarget(idObj);
			cc -= (cc.dot(g)) * g;
			EC[idObj] = 0.5 * cc.squaredNorm();
		}
	}
	void Optimizer::compute_EL() {
		EL = 0.5 * (mesh.M * vO.transpose()).squaredNorm();
	}
	void Optimizer::compute_Energy() {
		compute_EC();
		compute_EL();
	}
	ScalarType Optimizer::getCurrentEnergy() const {
		return (1.0-MIS::mu) * (EC[0]+EC[1]) + MIS::mu * MIS::lambda * EL;
	}


	
	void Optimizer::importOptimization(string optfile) {
		ifstream fin;

		fin.open(optfile,std::ios::in);
		if(fin.fail()) {
			cout << "I/O error" << endl;
		}
		else {
			string tag;
			int numBoxes;
			int numHandles;
			fin >> tag;
			if(tag == "balancing") {
				fin >> numBoxes;
				if(voxels.getNumBoxes() != numBoxes) cout << "Error : number of boxes incorrect" << endl;
				for(int i = 0 ; i < numBoxes ; ++i) {
					bool state;
					fin >> state;
					if(state)	voxels.setFilled(i);
					else		voxels.setCarved(i);
				}
			}
			else cout << "Error : opt file incorrect" << endl;
			
			fin >> tag;
			if(tag == "handles") {
				fin >> numHandles;
				if(handles.getNumHandles() != numHandles) cout << "Error : number of handles incorrect" << endl;
			
				RowMatrixX3 translations;
				RowMatrixX1 scales;
				translations.setZero(numHandles,3);
				scales.setZero(numHandles,1);

				for(int i = 0 ; i < numHandles ; ++i) {
					fin >> translations(i,0) >> translations(i,1) >> translations(i,2) >> scales[i];
				}
				handles.setState(translations,scales);
			}
			else cout << "Error : opt file incorrect" << endl;
		}
		fin.close();

	}
	void Optimizer::exportOptimization(string optfile) const {
		ofstream fout;
		fout.open(optfile, std::ios::out);
		if(fout.fail()) {
			cout << "I/O error" << endl;
		}
		else {
			fout << "balancing " << voxels.getNumBoxes() << endl;
			for(int i = 0 ; i < voxels.getNumBoxes() ; ++i) {
				fout << voxels.isFilled(i) << endl;
			}
			fout << "handles " << handles.getNumHandles() << endl;
			
			RowMatrixX3 translations;
			RowMatrixX1 scales;
			handles.getState(translations,scales);

			for(int i = 0 ; i < handles.getNumHandles() ; ++i) {
				fout << translations.row(i) << " " << scales[i] << endl;
			}
		}
		fout.close();
	}