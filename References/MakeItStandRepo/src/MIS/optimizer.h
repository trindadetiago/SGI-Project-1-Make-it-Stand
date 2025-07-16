#ifndef __MIS_OPTIMIZER_H
#define __MIS_OPTIMIZER_H

#include "MIS_inc.h"

#include "utils\mass_functions.h"

class Optimizer {
	
public:
	Optimizer::Optimizer(const Config & _cfg, Mesh & _mesh, InnerMesh & _imesh, BoxGrid & _voxels, Handles & _handles) :
		cfg(_cfg), mesh(_mesh), imesh(_imesh), voxels(_voxels), handles(_handles)
	{
		reset();
		prepare();
	}
	~Optimizer() {}
	
	void reset() {
		mass = 0.0;
		centerOfMass = RowVector3::Zero();
		EC[0] = EC[1] = EL = 0;
	}

	const ScalarType getMass() const { return mass; }
	const RowVector3 getCenterOfMassMO() const { return (cO/mO); }
	const RowVector3 getCenterOfMass() const { return centerOfMass; }

	void balanceByPlaneCarving(int hullDepth = 0);
	void balanceByPlaneCarvingMulti(int hullDepth = 0);
	
	void prepare();
	
	void compute_COM_MO();
	void compute_COM_MI();
	void compute_COM();

	void compute_EC();
	void compute_EL();
	void compute_Energy();

	void compute_fake_dEC();
	void compute_dEC();
	void compute_dEL();
	void compute_GradEnergy();
	
	void importOptimization(string optfile);
	void exportOptimization(string optfile) const;
	
	void apply_Energy();
	void apply_GradEnergy();

	RowVector3 getGradT(int i) const;
	ScalarType getGradS(int i) const;

	ScalarType getCurrentEnergy() const;
	ScalarType getEnergy1(int idObj) const { return EC[idObj]; }
	ScalarType getEnergy2() const { return EL; }

private:
	const Config & cfg;
	Mesh & mesh;
	InnerMesh & imesh;
	BoxGrid & voxels;
	Handles & handles;

	ScalarType EC[2];
	ScalarType EL;

	RowVectorX vO;

	MatrixXX dvO_dTx,dvO_dTy,dvO_dTz,dvO_dS;
	MatrixXX dvI_dTx,dvI_dTy,dvI_dTz,dvI_dS;
	MatrixXX MM_dvO_dTx,MM_dvO_dTy,MM_dvO_dTz,MM_dvO_dS;

	MatrixXX dc_dvO, dc_dvI;
	MatrixXX dc_dTx, dc_dTy, dc_dTz, dc_dS;
	
	ScalarType mI; Vector3 cI; MList dmI; CList dcI;
	ScalarType mO; Vector3 cO; MList dmO; CList dcO;

	ScalarType mass;
	Vector3 centerOfMass;
	
	RowVectorX dEC_dTx;
	RowVectorX dEC_dTy;
	RowVectorX dEC_dTz;
	RowVectorX dEC_dS;

	RowVectorX dEL_dTx;
	RowVectorX dEL_dTy;
	RowVectorX dEL_dTz;
	RowVectorX dEL_dS;

};

#endif