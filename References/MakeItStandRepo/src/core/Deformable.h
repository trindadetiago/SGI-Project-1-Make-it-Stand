#ifndef __DEFORMABLE_H
#define __DEFORMABLE_H

#include "MIS_inc.h"

class Deformable {

public:
	RowVector3 m_restPose;
	vector<ScalarType> m_coords; 
	ScalarType m_sumCoords;
	RowVector3 m_currentPose;

	Deformable(const RowVector3 & restPose) {
		m_restPose = restPose;
		m_currentPose = m_restPose;
		m_sumCoords = 0;
	}
	~Deformable() {
		m_coords.clear();
	}

	void pushWeight(ScalarType w) {
		m_coords.push_back(w);
		m_sumCoords += w;
	}

	ScalarType getCoord(int idHandle) const { return m_coords[idHandle]; }
	ScalarType getSumCoords() const { return m_sumCoords; }

	void normalizeWeights() {
		for(unsigned int i = 0; i < m_coords.size(); i++) {
			m_coords[i] /= m_sumCoords;
		}
		m_sumCoords = 1.0;
	}

	void computeCurrentPose(const Handles & handles);

	const RowVector3 & getRestPose() const { return m_restPose; }
	const RowVector3 & getCurrentPose() const { return m_currentPose; }

	ScalarType gradPoseForTranslationHandleJ(int j) const {
		return m_coords[j];
	}

};



#endif