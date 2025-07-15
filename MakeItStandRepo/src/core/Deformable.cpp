#include "Deformable.h"

#include "core/Handles.h"
#include "MIS/common.h"

	void Deformable::computeCurrentPose(const Handles & handles) {
		m_currentPose = RowVector3::Zero();

		RowVector3 Tjv;
		for(int j = 0; j < handles.getNumHandles(); j++) {
			handles.getTransformed(j,m_restPose,Tjv);
			m_currentPose += m_coords[j] * Tjv;
		}
	}