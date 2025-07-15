#include "MIS/common.h"

#include "core/Mesh.h"
#include "core/BoxGrid.h"
#include "core/Handles.h"
#include "core/InnerMesh.h"
#include "core/Support.h"
#include "MIS/optimizer.h"
#include "MIS/config.h"

#include "utils\timer.h"

namespace MIS {

	Config * m_config = 0;
	Mesh * m_mesh = 0;
	EmbreeIntersector *m_embree = 0;

	InnerMesh * m_imesh = 0;
	BoxGrid * m_voxels = 0;
	Handles * m_handles = 0;
	Optimizer * m_opt = 0;
	
	ScalarType start_step = 1.0;
	ScalarType step = start_step;
	ScalarType lambda = 20.0;
	ScalarType mu = 0.75;
	unsigned int hullDepth = 2;
	int currentObj = 0;
	int number_of_iterations = 0;

	double time_count = 0;
	
	bool use_gradCC = true;
	bool use_gradLap = true;
	bool use_scaling = true;

	bool fixed_mu = false;

	bool isConfigLoaded() { return m_config != 0; }
	bool isVoxelized() { return m_voxels != 0; }
	bool isOptimized() { return m_opt != 0; }

	void voxelize() {
		const int res = m_config->getVoxRes();
		string voxfile = m_config->getVoxfile();
		string bbwfile = m_config->getBBWfile();

		// create voxelization
		m_voxels = new BoxGrid(res);

		if(exists(voxfile)) {
			cout << "Reading voxels from file" << endl;
			m_voxels->initFromFile(voxfile);
		}
		else {
			cout << "Voxelizing mesh" << endl;
			m_voxels->initVoxels(*m_mesh,res);
		}

		cout << "Initializing voxel grid data structure" << endl;
		m_voxels->initStructure();

		// place handles constraints
		cout << "Creating handles in the voxel grid" << endl;
		m_handles = new Handles(*m_config,*m_voxels,*m_mesh);
		// create inner surface
		cout << "Creating inner surface mesh" << endl;
		m_imesh = new InnerMesh(*m_voxels);
		
		// compute BBW weights
		if(exists(bbwfile)) {
			cout << "Reading BBW from file" << endl;
			m_voxels->importBBW(bbwfile,m_handles->getNumHandles());
		}
		else {
			cout << "Computing BBW" << endl;
			m_voxels->computeBBW(*m_handles);
		}

		cout << "Interpolating BBW for mesh vertices" << endl;
		m_mesh->computeBBW(*m_handles,*m_voxels);

		// initial positioning
		m_voxels->updatePoses(*m_handles);
		m_mesh->updatePoses(*m_handles);

		// create optimizer
		m_opt = new Optimizer(*m_config,*m_mesh,*m_imesh,*m_voxels,*m_handles);
		
		updateGeometry();
		
		for(int idObj = 0 ; idObj < m_config->nbObj() ; ++idObj) {
			if(m_config->isStandingMode(idObj))
				m_config->getSupport(idObj)->updateStabilityZoneAndTarget(m_opt->getCenterOfMassMO());
		}

		m_voxels->clearCarving();
		m_imesh->compute();
		m_opt->compute_COM_MI();
		m_opt->compute_COM();
		m_opt->compute_GradEnergy();
		m_opt->compute_Energy();
	}
	
	void uninitialize() {		
		if(m_opt) { delete m_opt; m_opt = 0; }
		if(m_embree) { delete m_embree; m_embree = 0; }
		if(m_imesh) { delete m_imesh; m_imesh = 0; }
		if(m_handles) { delete m_handles; m_handles = 0; }
		if(m_voxels) { delete m_voxels; m_voxels = 0; }
		if(m_mesh) { delete m_mesh; m_mesh = 0; }
		if(m_config) { delete m_config; m_config = 0; }

		step = start_step;
		lambda = 20.0;
		mu = 0.75;
		hullDepth = 2;
		currentObj = 0;
	}

	void updateGeometry() {
		m_voxels->updatePoses(*m_handles);
		m_mesh->updatePoses(*m_handles);
		
		m_opt->compute_COM_MO();
	}

	void reset() {
		step = start_step;
		mu = 0.75;
		m_handles->reset();
		updateGeometry();
		m_config->setBalancingMode(FULL);
		balance();
	}

	bool optimizeNaive() {
		m_handles->saveState();

		ScalarType energyBefore = m_opt->getCurrentEnergy();

		// apply the gradient of the energy to move the handles
		m_opt->apply_GradEnergy();
		// update mesh vertices and voxels positions
		updateGeometry();
		// inner carving
		balance();

		ScalarType energyAfter = m_opt->getCurrentEnergy();
		
		if(energyAfter > energyBefore) {
			m_handles->restoreState();
			updateGeometry();
			balance();
			return false;
		}
		return true;
	}

	bool optimize() {
		
		bool finished = true;
		for(int idObj = 0 ; idObj < MIS::m_config->nbObj() ; ++idObj) {
			int state = MIS::m_config->checkObj(idObj,MIS::m_opt->getCenterOfMass());
			finished = finished && (state == 2);
			if(state >= 1) cout << "Standing ---- ";
		}
		if(finished) {
			cout << "Average iteration time : " << time_count/number_of_iterations << endl;
			return false;
		}

		m_handles->saveState();

		ScalarType energyBefore = m_opt->getCurrentEnergy();

		igl::Timer t; t.start();

		// apply the gradient of the energy to move the handles
		m_opt->apply_GradEnergy();
		// update mesh vertices and voxels positions
		updateGeometry();
		// inner carving
		balance();

		t.stop();

		ScalarType energyAfter = m_opt->getCurrentEnergy();
		
		if(energyAfter > energyBefore) {
			m_handles->restoreState();
			updateGeometry();
			balance();
		}

		cout << number_of_iterations << " -> " << energyBefore << " " << energyAfter << " " << step << " " << mu << " " << t.getElapsedTimeInMilliSec() << endl;
		number_of_iterations++;
		time_count += t.getElapsedTimeInMilliSec();

		if((energyAfter - energyBefore) / energyBefore > -0.03) {
			if(step > 0.4) {
				step *= 0.8;
				return true;
			}
			else {
				if(!fixed_mu && mu > 0.05) {
					step = start_step;
					mu -= 0.05;
					return true;
				}
				cout << "Average iteration time : " << time_count/number_of_iterations << endl;
				return false;
			}
		}

		return true;
	}

	void balance() {
		if(m_config->getBalanceMode() == PLANE_CARVING) {
			if(m_config->isMultipleObj())
				m_opt->balanceByPlaneCarvingMulti(hullDepth);
			else
				m_opt->balanceByPlaneCarving(hullDepth);
		}
		else if(m_config->getBalanceMode() == EMPTY)
			m_voxels->clearFilling(hullDepth);
		else if(m_config->getBalanceMode() == FULL)
			m_voxels->clearCarving();
		

		m_imesh->compute();
		m_opt->compute_COM_MI();
		
		m_opt->compute_COM();

		m_opt->compute_GradEnergy();
		
		m_opt->compute_Energy();
	}





};