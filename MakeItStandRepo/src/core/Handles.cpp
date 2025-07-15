#include "Handles.h"

#include "MIS/config.h"
#include "core/BoxGrid.h"
#include "core/Mesh.h"
#include "core/Support.h"

	Handles::Handles(const Config & cfg, const BoxGrid & vox, const Mesh & mesh) {
		
		m_numHandles = cfg.nbObj()+cfg.getNbHandles();
		m_restposes.setZero(m_numHandles,3);

		int idHandle = 0;

		for(int idObj = 0 ; idObj < cfg.nbObj() ; ++idObj) {
			if(cfg.isStandingMode(idObj)) {
				cfg.getSupportVertices(m_nodeIndicesSupp[idObj],idObj);
				for(unsigned int i = 0 ; i < m_nodeIndicesSupp[idObj].size() ; ++i) {
					int idV = m_nodeIndicesSupp[idObj][i];
					m_nodeIndicesSupp[idObj][i] = vox.getNodeClosestToPoint(mesh.getCurrentPose(idV));
				}
				m_restposes.row(idHandle) = cfg.getSupportConst(idObj)->getCentroid();
			}
			else if(cfg.isSuspendedMode(idObj)) {
				m_nodeIndicesSupp[idObj].push_back(vox.getNodeClosestToPoint(cfg.getSupportPoint(idObj)));
				m_restposes.row(idHandle) = cfg.getSupportPoint(idObj);
			}
			idHandle++;
		}
		for(unsigned int i = 0 ; i < cfg.getNbHandles() ; ++i) {
			int idNode = vox.getNodeClosestToPoint(cfg.getHandle(i));
			if(idNode != -1) {
				m_nodeIndices.push_back(idNode);
				m_restposes.row(idHandle) = vox.getRestPose(m_nodeIndices[i]);
			}
			else {
				cout << "Error : Handle not in the voxel grid" << endl;
			}
			idHandle++;
		}

		m_locks.resize(m_numHandles,false);
		reset();

	}