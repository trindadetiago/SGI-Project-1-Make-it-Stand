#ifndef __HANDLES_H
#define __HANDLES_H

#include "MIS_inc.h"
#include "core/BoxGrid.h"

class Handles {

private:	
	vector<int> m_nodeIndicesSupp[2];
	vector<int> m_nodeIndices;
	
	int m_numHandles;

	vector<bool> m_locks;
	RowMatrixX3 m_restposes;
	RowMatrixX3 m_translations;
	RowMatrixX1 m_scales;

	RowMatrixX3 m_prevTranslations;
	RowMatrixX1 m_prevScales;

public:
	Handles(const Config & cfg, const BoxGrid & vox, const Mesh & mesh);
	~Handles() {}

	void reset() {
		m_scales.setConstant(m_numHandles,1,1.0);
		m_translations = m_restposes;

		for(int j=0 ; j < m_numHandles ; ++j) {
			m_locks[j] = false;
		}
	}

	int getIdFirstConstraint(int j) const {
		if(j == 0)
			return 0;
		if(m_nodeIndicesSupp[1].empty()) {
			return m_nodeIndicesSupp[0].size() + j-1;
		}
		else {
			if(j == 1)
				return m_nodeIndicesSupp[0].size();
			return m_nodeIndicesSupp[0].size() + m_nodeIndicesSupp[1].size() + j-2;
		}
	}
	int getNumConstraints(int j) const {
		if(j == 0)
			return m_nodeIndicesSupp[0].size();
		if(j == 1 && !m_nodeIndicesSupp[1].empty())
			return m_nodeIndicesSupp[1].size();
		
		return 1;
	}
	int getConstraint(unsigned int i) const { 
		if(i < m_nodeIndicesSupp[0].size())
			return m_nodeIndicesSupp[0][i];

		i -= m_nodeIndicesSupp[0].size();
		if(i < m_nodeIndicesSupp[1].size())
			return m_nodeIndicesSupp[1][i];
		
		i -= m_nodeIndicesSupp[1].size();
		return m_nodeIndices[i];
	}
	int getNumConstraints() const { return m_nodeIndices.size() + m_nodeIndicesSupp[0].size() + m_nodeIndicesSupp[1].size(); }

	int getNumHandles() const { return m_numHandles; }

	bool isLocked(int j) const { return m_locks[j]; }
	void changeLock(int j) { m_locks[j] = !m_locks[j]; }

	void getFullTransform(int j, RowVector3 & t, ScalarType & s) const {
		t = m_translations.row(j);
		s = m_scales[j];
	}

	void getTransformed(int j, const RowVector3 & v0, RowVector3 & v) const {
		v = m_scales[j] * (v0 - m_restposes.row(j)) + m_translations.row(j);
	}

	void getGradScale(int j, const RowVector3 & v0, RowVector3 & v) const {
		v = (v0 - m_restposes.row(j));
	}

	RowVector3 getHandlePose(int j) const { return m_translations.row(j); }

	void translate(int j, const RowVector3 & t) {
		m_translations.row(j) += t;
	}
	void scale(int j, const ScalarType s) {
		m_scales[j] = clamp(m_scales[j]+s,0.8,1.4);
	}

	void setT(int j, const RowVector3 & t) {
		m_translations.row(j) = t;
	}
	void setS(int j, const ScalarType s) {
		m_scales[j] = clamp(s,0.8,1.4);
	}
	
	void getState(RowMatrixX3 & t, RowMatrixX1 & s) {
		t = m_translations;
		s = m_scales;
	}
	void setState(const RowMatrixX3 & t, const RowMatrixX1 & s) {
		m_translations = t;
		m_scales = s;
	}
	
	void saveState() {
		getState(m_prevTranslations,m_prevScales);
	}
	void restoreState() {
		setState(m_prevTranslations,m_prevScales);
	}
};

#endif