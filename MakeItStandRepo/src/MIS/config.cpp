#include "MIS/config.h"

#include "core/Mesh.h"
#include "core/Support.h"

	Config::Config(const string _filename, const string _meshfile) {
		filename = string(CONFIG_DIR).append(_filename);
		
		multipleObj = false;

		meshRotation[0] = Quaternion::Identity();
		mode[0] = true;
		supp[0] = new Support(DEFAULT_FLATTEN_THRESHOLD,DEFAULT_ANGLE_OBJ);
		
		meshRotation[1] = Quaternion::Identity();
		mode[1] = true;
		supp[1] = new Support(DEFAULT_FLATTEN_THRESHOLD,DEFAULT_ANGLE_OBJ);

		if(exists(filename)) {
			ifstream fin;

			fin.open(filename,std::ios::in);
			if(fin.fail()) {
				cout << "I/O error" << endl;
			}
			else {
				while(!fin.eof()) {
					string tag;
					fin >> tag;
					if(tag == "mesh") {
						fin >> meshfile;
					}
					else if(tag == "multiobj") {
						multipleObj = true;
					}
					else if(tag == "mesh_rotation" || tag == "mesh_rotation1") {
						fin >> meshRotation[0].w() >> meshRotation[0].x() >> meshRotation[0].y() >> meshRotation[0].z();
					}
					else if(tag == "mesh_rotation2") {
						fin >> meshRotation[1].w() >> meshRotation[1].x() >> meshRotation[1].y() >> meshRotation[1].z();
					}
					else if(tag == "support_point" || tag == "support_point1") {
						fin >> supportPoint[0][0] >> supportPoint[0][1] >> supportPoint[0][2];
						mode[0] = false;
					}
					else if(tag == "support_point2") {
						fin >> supportPoint[1][0] >> supportPoint[1][1] >> supportPoint[1][2];
						mode[1] = false;
					}
					else if(tag == "flatten_threshold" || tag == "flatten_threshold1") {
						ScalarType tmp;
						fin >> tmp;
						supp[0]->setThreshold(tmp);
						mode[0] = true;
					}
					else if(tag == "flatten_threshold2") {
						ScalarType tmp;
						fin >> tmp;
						supp[1]->setThreshold(tmp);
						mode[1] = true;
					}
					else if(tag == "vertex_handle") {
						RowVector3 v = RowVector3::Zero();
						fin >> v[0] >> v[1] >> v[2];
						handles.push_back(v);
					}
					else if(!tag.empty()) {
						cout << "Tag " << tag << " unknown" << endl;
					}
				}
			}
			fin.close();  
		}
		else {
			meshfile = _meshfile;
		}
		
		setGravity(0);
		setGravity(1);

		voxRes = 0;

		string shortConfigName = _filename;
		removeExt(shortConfigName);
		vector<string> voxFiles;
		findFiles(CONFIG_DIR,VOX_EXT,voxFiles);
		for(unsigned int i = 0 ; i < voxFiles.size() ; ++i) {
			if(voxFiles[i].find(shortConfigName) != string::npos) {
				string tmp = voxFiles[i];
				tmp.erase(tmp.begin(),tmp.begin()+shortConfigName.length()+1);
				removeExt(tmp);
				voxRes = std::max(voxRes,atoi(tmp.c_str()));
			}
		}

		if(voxRes == 0) voxRes = 50;

		balanceMode = FULL;
	}

	Config::~Config() {}

	void Config::saveConfig(const string _filename) {
		filename = string(CONFIG_DIR).append(_filename);
		ofstream fout;
		fout.open(filename, std::ios::out);
		if(fout.fail()) {
		   cout << "I/O error" << endl;
		}
		else {
			fout << "mesh " << meshfile << endl;
			
			if(multipleObj)
				fout << "multiobj" << endl;

			if(!multipleObj) {
				fout << "mesh_rotation " << meshRotation[0].w() << " " << meshRotation[0].x() << " " << meshRotation[0].y() << " " << meshRotation[0].z() << endl;
				if(mode[0]) {
					fout << "flatten_threshold " << supp[0]->getThreshold() << endl;
				}
				else {
					fout << "support_point " << supportPoint[0] << endl;
				}
			}
			else {
				for(int i = 0 ; i <= 1 ; ++i) {
					fout << "mesh_rotation" << i+1 << " " << meshRotation[i].w() << " " << meshRotation[i].x() << " " << meshRotation[i].y() << " " << meshRotation[i].z() << endl;
					if(mode[i]) {
						fout << "flatten_threshold" << i+1 << " " << supp[i]->getThreshold() << endl;
					}
					else {
						fout << "support_point" << i+1 << " " << supportPoint[i] << endl;
					}
				}
			}


			for(unsigned int i = 0 ; i < handles.size() ; ++i) {
				fout << "vertex_handle " << handles[i] << endl;
			}
		}
		fout.close();
	}

	
	void Config::clearSupport(const Mesh & mesh, int idObj) {
		supp[idObj]->clear();

		if(isStandingMode(idObj)) {
			supp[idObj]->updateStandingZone(mesh);
		}
		else if(isSuspendedMode(idObj)) {
			RowVector3 p = mesh.getHigherVertex(supp[idObj]->getDir());
			setSupportPoint(p);
		}
	}

	void Config::updateSupport(const Mesh & mesh, int idObj) {
		if(isStandingMode(idObj)) {
			supp[idObj]->clear();
			supp[idObj]->updateStandingZone(mesh);
		}
	}

	
	void Config::getSupportVertices(vector<int> & L, int idObj) const {
		for(int i = 0 ; i < supp[idObj]->nbVertices() ; ++i) {
			L.push_back(supp[idObj]->getVertex(i));
		}
	}

	
	const RowVector3 & Config::getTarget(int idObj) const { 
		if(isStandingMode(idObj))
			return supp[idObj]->getTarget();
		else
			return supportPoint[idObj];
	}

	const RowVector3 & Config::getSupportPoint(int idObj) const { return supportPoint[idObj]; }
	void Config::setSupportPoint(const RowVector3 & p, int idObj) {
		supportPoint[idObj] = p;
	}

	int Config::checkObj(int idObj, const RowVector3 & com, float * topplingAngle) {
		if(isStandingMode(idObj)) {
			bool isSatisfied = supp[idObj]->isStanding(com);
			if(isSatisfied) {
				float angle = supp[idObj]->computeToppling(com);
				if(topplingAngle != 0) *topplingAngle = angle;
				if(angle >= supp[idObj]->getAngleObj()) return 2;
				else return 1;
			}
			else {
				if(topplingAngle != 0) *topplingAngle = 0.0f;
				return 0;
			}
		}
		else if(isSuspendedMode(idObj)) {
			float angle = supp[idObj]->computeDeviation(com);
			if(topplingAngle != 0) *topplingAngle = angle;
			if(angle <= supp[idObj]->getAngleObj()) return 2;
			else return 1;
		}
		return 0; // this cannot happen
	}



	const RowVector3 & Config::getGravity(int idObj) const { return supp[idObj]->getDir(); }
	const RowVector3 & Config::getGravityT1(int idObj) const { return supp[idObj]->getDirU(); }
	const RowVector3 & Config::getGravityT2(int idObj) const { return supp[idObj]->getDirV(); }

	void Config::setGravity(int idObj) {
		Matrix33 mat = meshRotation[idObj].conjugate().toRotationMatrix();
		RowVector3 g = (mat * Vector3(0,-1,0));
		RowVector3 g1 = (mat * Vector3(1,0,0));
		RowVector3 g2 = (mat * Vector3(0,0,1));
		supp[idObj]->setDir(g,g1,g2);
	}
	void Config::setMode(bool _mode, int idObj) { mode[idObj] = _mode; }
	bool Config::isStandingMode(int idObj) const { return mode[idObj]; }
	bool Config::isSuspendedMode(int idObj) const { return !mode[idObj]; }
	float Config::getFlattenThreshold(int idObj) const { return supp[idObj]->getThreshold(); }
	void Config::setFlattenThreshold(const float t, int idObj) { 
		supp[idObj]->setThreshold(t);
	}
	float Config::getAngleObj(int idObj) const { return supp[idObj]->getAngleObj(); }
	void Config::setAngleObj(const float angle, int idObj) {
		supp[idObj]->setAngleObj(angle); 
	}