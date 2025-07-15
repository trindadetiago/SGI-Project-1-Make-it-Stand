#ifndef __MIS_COMMON_H
#define __MIS_COMMON_H

#include "MIS_inc.h"
#include "utils\embree\embree_intersector.h"

namespace MIS {

	extern Config * m_config;
	extern Mesh * m_mesh;
	extern InnerMesh * m_imesh;
	extern BoxGrid * m_voxels;
	extern Handles * m_handles;
	extern Optimizer * m_opt;

	extern EmbreeIntersector * m_embree;
	
	bool isConfigLoaded();
	bool isVoxelized();
	bool isOptimized();
	
	extern ScalarType start_step;
	extern ScalarType step;
	extern ScalarType lambda;
	extern ScalarType mu;
	extern unsigned int hullDepth;
	extern int currentObj;
	
	extern int number_of_iterations;
	extern double time_count;

	extern bool use_scaling;
	extern bool fixed_mu;
	
	void voxelize();
	void uninitialize();
	void balance();
	bool optimize();
	bool optimizeNaive();
	void reset();
	void updateGeometry();
	void exportForPrinting();

};


#endif