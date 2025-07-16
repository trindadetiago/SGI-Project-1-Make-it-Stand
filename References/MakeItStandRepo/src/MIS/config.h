#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

#include "MIS_inc.h"

typedef enum { PLANE_CARVING, EMPTY, FULL, IMPORTED } BalanceModeEnum;

class Config {

private:
	string filename;

	string meshfile;

	bool multipleObj;

	bool mode[2];
	Quaternion meshRotation[2];
	RowVector3 supportPoint[2];
	Support* supp[2];

	vector<RowVector3> handles;

	int voxRes;
	BalanceModeEnum balanceMode;

public:
	Config(const string _filename, const string _meshfile);
	~Config();

	void saveConfig(const string _filename);
	
	string getConfigfile() const { return filename; }
	string getMeshfile() const { return string(MESH_DIR).append(meshfile); }
	string getVoxfile() const {
		string voxfile = getConfigfile();
		removeExt(voxfile);
		voxfile.append("_").append(IntToString(voxRes)).append(VOX_EXT);
		return voxfile;
	}
	string getBBWfile() const {
		string bbwfile = getConfigfile();
		removeExt(bbwfile);
		bbwfile.append("_").append(IntToString(voxRes)).append(BBW_EXT);
		return bbwfile;
	}
	string getOptimfile(const int id) const {
		string optfile = getConfigfile();
		removeExt(optfile);
		optfile.append("_").append(IntToString(voxRes)).append("_").append(IntToString(id)).append(".opt");
		return optfile;
	}
	string getPrintOuterfile(const int id) const {
		string printfile = getConfigfile();
		removeExt(printfile);
        printfile.replace(printfile.cbegin(), printfile.cbegin()+string(CONFIG_DIR).length(), PRINT_DIR);
		printfile.append("_").append(IntToString(voxRes)).append("_").append(IntToString(id)).append("_outer.stl");
		return printfile;
	}
	string getPrintInnerfile(const int id) const {
		string printfile = getConfigfile();
		removeExt(printfile);
        printfile.replace(printfile.cbegin(), printfile.cbegin()+string(CONFIG_DIR).length(), PRINT_DIR);
		printfile.append("_").append(IntToString(voxRes)).append("_").append(IntToString(id)).append("_inner.stl");
		return printfile;
	}

	bool isMultipleObj() const { return multipleObj; }
	void setMultipleObj(bool mo) { multipleObj = mo; }
	int nbObj() const { return multipleObj ? 2 : 1; }

	Support * getSupport(int idObj = 0) { return supp[idObj]; }
	const Support * getSupportConst(int idObj = 0) const { return supp[idObj]; }
	void clearSupport(const Mesh & mesh, int idObj = 0);
	void updateSupport(const Mesh & mesh, int idObj = 0);
	void getSupportVertices(vector<int> & L, int idObj = 0) const;

	const RowVector3 & getTarget(int idObj = 0) const;
	const RowVector3 & getSupportPoint(int idObj = 0) const;
	void setSupportPoint(const RowVector3 & p, int idObj = 0);
	
	const RowVector3 & getGravity(int idObj) const;
	const RowVector3 & getGravityT1(int idObj) const;
	const RowVector3 & getGravityT2(int idObj) const;

	const Quaternion & getMeshRotation(int idObj = 0) const { return meshRotation[idObj]; }
	void setMeshRotation(const Quaternion & quat, int idObj = 0) {
		meshRotation[idObj] = quat;
		setGravity(idObj);
	}
	void setGravity(int idObj = 0);
	void setMode(bool _mode, int idObj = 0);
	bool isStandingMode(int idObj = 0) const;
	bool isSuspendedMode(int idObj = 0) const;
	float getFlattenThreshold(int idObj = 0) const;
	void setFlattenThreshold(const float t, int idObj = 0);
	float getAngleObj(int idObj = 0) const;
	void setAngleObj(const float angle, int idObj = 0);

	const unsigned int getNbHandles() const { return handles.size(); }
	const RowVector3 & getHandle(int i) const { return handles[i]; }
	void addHandle(const RowVector3 & v) {
		handles.push_back(v);
	}
	void moveHandle(int i, const RowVector3 & t) {
		handles[i] += t;
	}
	void deleteHandle(int i) {
		handles.erase(handles.begin()+i);
	}


	int getVoxRes() const { return voxRes; }
	void setVoxRes(int _voxRes) { voxRes = _voxRes; }
	BalanceModeEnum getBalanceMode() const { return balanceMode; }
	void setBalancingMode(BalanceModeEnum _mode) { balanceMode = _mode; }

	int checkObj(int idObj, const RowVector3 & com, float * topplingAngle = 0);
};


#endif