#include "plugin.h"

#include "core/Mesh.h"
#include "core/InnerMesh.h"
#include "core/Handles.h"
#include "core/Support.h"

#include "MIS/common.h"
#include "MIS/config.h"
#include "MIS/optimizer.h"

#include "viewer/PCFShadow.h"
#include "viewer/rendering_functions.h"
#include "viewer/trackball.h"
#include "slcrystal.h"

#include "utils\embree\embree_intersector.h"

MISPlugin MISPluginInstance;

// callbacks for UI buttons

void TW_CALL SetConfigFileCB(const void *value, void *clientData)
{
	MISPluginInstance.cfg_configfile = *(const int *)value;
	
	if(MISPluginInstance.cfg_configfile == 0) {
		TwDefine(" MakeItStand/cfgNewFile visible=true ");
		TwDefine(" MakeItStand/meshFile visible=true ");
	}
	else {
		TwDefine(" MakeItStand/cfgNewFile visible=false ");
		TwDefine(" MakeItStand/meshFile visible=false ");
	}

}
void TW_CALL GetConfigFileCB(void *value, void *clientData)
{ 
	*(int *)value = MISPluginInstance.cfg_configfile;
}

void TW_CALL OpenConfigCB(void *clientData)
{
	MISPluginInstance.close();

	MIS::m_config = new Config(MISPluginInstance.getConfigfile(),MISPluginInstance.getMeshfile());
	MIS::m_mesh = new Mesh(MIS::m_config->getMeshfile());
	MIS::m_embree = new EmbreeIntersector(MIS::m_mesh->getV(),MIS::m_mesh->getF());

	MISPluginInstance.updateAfterConfigLoading();
}
void TW_CALL SaveConfigCB(void *clientData)
{
	if(!MIS::isConfigLoaded()) return;

	MIS::m_config->saveConfig(MISPluginInstance.getConfigfile());

	if(MIS::isVoxelized()) {
		
		const int res = MIS::m_config->getVoxRes();
		string voxfile = MIS::m_config->getVoxfile();
		string bbwfile = MIS::m_config->getBBWfile();
		
		if(!exists(bbwfile))
			MIS::m_voxels->saveToFile(voxfile,bbwfile,MIS::m_handles->getNumHandles());
	}

	if(MIS::isOptimized()) {
		string optfile;
		int id = 0;
		do {
			optfile = MIS::m_config->getOptimfile(id);
			id++;
		} while(exists(optfile));

		MIS::m_opt->exportOptimization(optfile);

		string printOuterfile = MIS::m_config->getPrintOuterfile(id);
		MIS::m_mesh->exportMesh(printOuterfile,*MIS::m_config);
		string printInnerfile = MIS::m_config->getPrintInnerfile(id);
		MIS::m_imesh->exportMesh(printInnerfile);
	}
}

void TW_CALL VoxelizeCB(void *clientData)
{
	if(!MIS::isConfigLoaded() || MIS::isVoxelized()) return;

	MIS::voxelize();

	MISPluginInstance.updateAfterVoxelizing();
}
void TW_CALL ResetHandlesCB(void *clientData)
{
	if(!MIS::isVoxelized()) return;
	
	MIS::m_handles->reset();
	MIS::updateGeometry();

	if(MIS::isOptimized()) {
		MIS::balance();
	}
}

void TW_CALL SetMultipleObjCB(const void *value, void *clientData)
{ 
	MIS::m_config->setMultipleObj(*(const bool *)value);
	if(MIS::m_config->isMultipleObj()) {
		TwDefine(" MakeItStand/currentObj visible=true ");
		MIS::currentObj = 0;
	}
	else {
		TwDefine(" MakeItStand/currentObj visible=false ");
		MIS::currentObj = 0;
	}
}
void TW_CALL GetMultipleObjCB(void *value, void *clientData)
{ 
	*(bool *)value = MIS::m_config->isMultipleObj();
}

void TW_CALL SetCurrentObjCB(const void *value, void *clientData)
{ 
	MIS::currentObj = *(const int *)value;
}
void TW_CALL GetCurrentObjCB(void *value, void *clientData)
{ 
	*(int *)value = MIS::currentObj;
}

void TW_CALL SetMeshRotationCB(const void *value, void *clientData)
{ 
    const float* fvalue = (const float *)value;
	Quaternion quat(fvalue[3],fvalue[0],fvalue[1],fvalue[2]);

	MIS::m_config->setMeshRotation(quat.normalized(),MIS::currentObj);
	MIS::m_config->updateSupport(*MIS::m_mesh,MIS::currentObj);
}
void TW_CALL GetMeshRotationCB(void *value, void *clientData)
{ 
	const Quaternion & mRot = MIS::m_config->getMeshRotation(MIS::currentObj);
	((float *)value)[0] = mRot.x();
    ((float *)value)[1] = mRot.y();
    ((float *)value)[2] = mRot.z();
    ((float *)value)[3] = mRot.w();
}

void TW_CALL SetAngleObjCB(const void *value, void *clientData)
{
	MIS::m_config->setAngleObj(*(const float *)value,MIS::currentObj);
	if(MIS::isOptimized()) {
		MIS::m_config->getSupport(MIS::currentObj)->updateStabilityZoneAndTarget(MIS::m_opt->getCenterOfMass());
		MIS::balance();
	}
}
void TW_CALL GetAngleObjCB(void *value, void *clientData)
{ 
	*(float *)value = MIS::m_config->getAngleObj(MIS::currentObj);
}

void TW_CALL SetFlattenThresholdCB(const void *value, void *clientData)
{
	MIS::m_config->setFlattenThreshold(*(const float *)value,MIS::currentObj);
	MIS::m_config->updateSupport(*MIS::m_mesh,MIS::currentObj);
}
void TW_CALL GetFlattenThresholdCB(void *value, void *clientData)
{ 
	*(float *)value = MIS::m_config->getFlattenThreshold(MIS::currentObj);
}

void TW_CALL SetSupportModeCB(const void *value, void *clientData) {
    const bool bvalue = ((*(const int *)value) == 1);

	MIS::m_config->setMode(bvalue,MIS::currentObj);
	MIS::m_config->clearSupport(*MIS::m_mesh,MIS::currentObj);

	if(MIS::m_config->isStandingMode(MIS::currentObj)) {
		TwDefine(" MakeItStand/flatThres readwrite ");
	}
	else if(MIS::m_config->isSuspendedMode(MIS::currentObj)) {
		TwDefine(" MakeItStand/flatThres readonly ");
	}

	MISPluginInstance.resetCamera();
}
void TW_CALL GetSupportModeCB(void *value, void *clientData) {
	*(bool *)value = MIS::m_config->isStandingMode(MIS::currentObj);
}

void TW_CALL SetVoxResCB(const void *value, void *clientData)
{ 
	MIS::m_config->setVoxRes(*(const int *)value);
}
void TW_CALL GetVoxResCB(void *value, void *clientData)
{ 
	*(int *)value = MIS::m_config->getVoxRes();
}

void TW_CALL SetHullDepthCB(const void *value, void *clientData)
{ 
	MIS::hullDepth = *(const int *)value;
	if(MIS::isOptimized()) {
		MIS::balance();
	}
}
void TW_CALL GetHullDepthCB(void *value, void *clientData)
{ 
	*(int *)value = MIS::hullDepth;
}

void TW_CALL SetBalanceModeCB(const void *value, void *clientData)
{
	int val = *(const int *)value;
	BalanceModeEnum mode = EMPTY;
	if(val == 0) mode = PLANE_CARVING;
	else if(val == 1) mode = EMPTY;
	else if(val == 2) mode = FULL;
	else if(val == 3) mode = IMPORTED;
	
	BalanceModeEnum prevMode = MIS::m_config->getBalanceMode();
	MIS::m_config->setBalancingMode(mode);
	if(prevMode != mode && MIS::isOptimized()) {
		if(mode == IMPORTED) {
			string optfile = MIS::m_config->getOptimfile(MISPluginInstance.optfile);
			if(exists(optfile)) {
				MIS::m_opt->importOptimization(optfile);
				MIS::updateGeometry();
				MIS::balance();
			}
		}
		else {
			MIS::balance();
		}
	}

	if(mode == IMPORTED) {
		TwDefine(" MakeItStand/optFile visible=true ");
	}
	else {
		TwDefine(" MakeItStand/optFile visible=false ");
	}

}
void TW_CALL GetBalanceModeCB(void *value, void *clientData)
{ 
	BalanceModeEnum mode = MIS::m_config->getBalanceMode();

	if(mode == PLANE_CARVING)
		*(int *)value = 0;
	else if(mode == EMPTY)
		*(int *)value = 1;
	else if(mode == FULL)
		*(int *)value = 2;
	else if(mode == IMPORTED)
		*(int *)value = 3;
}

void TW_CALL SetOptfileCB(const void *value, void *clientData)
{
	MISPluginInstance.optfile = *(const int *)value;
	string optfile = MIS::m_config->getOptimfile(MISPluginInstance.optfile);
	
	if(MIS::isOptimized() && exists(optfile)) {
		MIS::m_opt->importOptimization(optfile);
		MIS::updateGeometry();
		MIS::balance();
	}
}
void TW_CALL GetOptfileCB(void *value, void *clientData)
{ 
	*(int *)value = MISPluginInstance.optfile;
}

void TW_CALL SetCornerThresCB(const void *value, void *clientData)
{ 
	if(!MIS::isConfigLoaded()) return;

	MIS::m_mesh->corner_threshold = *(const ScalarType *)value;
	MIS::m_mesh->ComputeCornerNormals();
}
void TW_CALL GetCornerThresCB(void *value, void *clientData)
{ 
	if(MIS::isConfigLoaded())
		*(ScalarType *)value = MIS::m_mesh->corner_threshold;
	else
		*(ScalarType *)value = 20.0;
}

void TW_CALL CopyStdStringToClient(std::string& destinationClientString, const std::string& sourceLibraryString)
{
  destinationClientString = sourceLibraryString;
}

	void MISPlugin::init(Preview3D *preview)
	{
		cout << "LOADING MIS PLUGIN..." << endl;

		findFiles(CONFIG_DIR,CONFIG_EXT,cfgFiles);
		findFiles(MESH_DIR,OBJ_EXT,objFiles);

		PreviewPlugin::init(preview);

		mybar = TwNewBar("MakeItStand");
		TwDefine(" MakeItStand size='300 397' text=light alpha=255 color='40 40 40' position='3 3'");
 
		TwCopyStdStringToClientFunc(CopyStdStringToClient);
		
		
		std::stringstream ssEnumCfgFile;
		ssEnumCfgFile << "New...";
		for(unsigned int i = 0 ; i < cfgFiles.size() ; ++i)
			ssEnumCfgFile << "," << cfgFiles[i];

		TwType cfgFileType = TwDefineEnumFromString("CfgFileType", ssEnumCfgFile.str().c_str());
		TwAddVarCB(mybar, "cfgFile", cfgFileType, SetConfigFileCB, GetConfigFileCB, this, " label='Config file :' ");

		TwAddVarRW(mybar, "cfgNewFile", TW_TYPE_STDSTRING, &cfg_confignewfile, " label=' ' ");
		
		std::stringstream ssEnumMeshFile;
		ssEnumMeshFile << objFiles[0];
		for(unsigned int i = 1 ; i < objFiles.size() ; ++i)
			ssEnumMeshFile << "," << objFiles[i];

		TwType meshFileType = TwDefineEnumFromString("MeshFileType", ssEnumMeshFile.str().c_str());
		TwAddVarRW(mybar, "meshFile", meshFileType, &cfg_meshfile, " label='Mesh file :' ");

		TwAddButton(mybar, "openCfg", OpenConfigCB, this, " label='Open/Create' ");
		TwAddButton(mybar, "saveCfg", SaveConfigCB, this, " label='Save' ");

		TwAddSeparator(mybar, "sep1", NULL);
		
		TwAddVarCB(mybar, "multiObj", TW_TYPE_BOOLCPP, SetMultipleObjCB, GetMultipleObjCB, this, " visible=false label='Multiple objectives' ");
		TwAddVarCB(mybar, "currentObj", TW_TYPE_UINT32, SetCurrentObjCB, GetCurrentObjCB, this, " visible=false label='Current objective' min=0 max=1 step=1 ");

		TwType supportModeType = TwDefineEnum("SupportModeType", NULL, 0);
		TwAddVarCB(mybar, "supportMode", supportModeType, SetSupportModeCB, GetSupportModeCB, this, " visible=false label='Stand/Suspend' enum='0 {Suspended}, 1 {Standing}' ");
		TwAddVarCB(mybar, "flatThres", TW_TYPE_FLOAT, SetFlattenThresholdCB, GetFlattenThresholdCB, this, " visible=false label='Flatten threshold' min=0.0 max=0.1 step=0.005 ");
		TwAddVarCB(mybar, "angleObj", TW_TYPE_FLOAT, SetAngleObjCB, GetAngleObjCB, this, " visible=false label='Angle objective' min=0 max=45 step=1 ");
		
		TwAddVarCB(mybar, "meshRotation", TW_TYPE_QUAT4F, SetMeshRotationCB, GetMeshRotationCB, this, " visible=false label='Mesh rotation' opened=true ");
		
		TwAddVarCB(mybar, "voxRes", TW_TYPE_UINT32, SetVoxResCB, GetVoxResCB, this, " visible=false label='Voxel grid resolution' max=256 step=2 ");
		TwAddButton(mybar, "voxelize", VoxelizeCB, this, " label='Voxelize' ");

		TwAddSeparator(mybar, "sep2", " ");
		
		TwType balanceModeType = TwDefineEnum("BalanceModeType", NULL, 0);
		TwAddVarCB(mybar, "balanceMode", balanceModeType, SetBalanceModeCB, GetBalanceModeCB, this, " label='Balancing algorithm' visible=false enum='0 {Plane Carving}, 1 {Empty}, 2 {Full}, 3 {Imported}' ");
		
		TwAddVarCB(mybar, "optFile", TW_TYPE_UINT32, SetOptfileCB, GetOptfileCB, this, " label='Optimization file :' visible=false min=0 max=100 step=1 ");

		TwAddVarCB(mybar, "hullDepth", TW_TYPE_UINT32, SetHullDepthCB, GetHullDepthCB, this, " visible=false label='Hull voxel thickness' min=0 max=10 step=1 ");
		
		TwAddVarRW(mybar, "Use scaling", TW_TYPE_BOOLCPP, &MIS::use_scaling, "");

		TwAddButton(mybar, "reset", ResetHandlesCB, this, " label='Reset' ");
				
		TwAddSeparator(mybar, "sep3", NULL);
		
		TwAddVarRW(mybar, "Naive mode", TW_TYPE_BOOLCPP, &naive_mode, "key=F8 ");
		TwAddVarRW(mybar, "Fixed mu", TW_TYPE_BOOLCPP, &MIS::fixed_mu, "key=F7 ");
		TwAddVarRW(mybar, "Display info", TW_TYPE_BOOLCPP, &display_info, "key=F6 ");


		stats = TwNewBar("Statistics");
		TwDefine(" Statistics size='300 314' text=light alpha=255 color='40 40 40' position='3 403' valueswidth=100");
		
		TwAddVarRO(stats, "FPS", TW_TYPE_UINT32, &log_FPS, " label='FPS :' ");
		TwAddVarRO(stats, "nbV", TW_TYPE_UINT32, &log_nbV, " label='Vertices :' ");
		TwAddVarRO(stats, "nbF", TW_TYPE_UINT32, &log_nbF, " label='Triangles :' ");
		TwAddVarRO(stats, "nbVox", TW_TYPE_UINT32, &log_nbVox, " label='Voxels :' ");
		
		TwType mouseModeType = TwDefineEnum("MouseModeType", NULL, 0);
		TwAddVarRO(stats, "mouseMode", mouseModeType, &log_mouse_mode, " label='Mode :' enum='0 {NOTHING}, 1 {TRANSLATE}, 2 {SCALE}, 3 {CREATE}, 4 {SET_SUPPORT}, 5 {MOVE_TARGET}'  ");
		
		TwAddVarRO(stats, "mass", TW_TYPE_FLOAT, &log_mass, " label='Mass :' ");
		TwAddVarRO(stats, "E1o1", TW_TYPE_FLOAT, &log_energy1obj1, " label='Energy 1 (obj1) :' ");
		TwAddVarRO(stats, "E1o2", TW_TYPE_FLOAT, &log_energy1obj2, " label='Energy 1 (obj2) :' ");
		TwAddVarRO(stats, "E1s", TW_TYPE_FLOAT, &log_energy1sum, " label='Energy 1 :' ");
		TwAddVarRO(stats, "E2", TW_TYPE_FLOAT, &log_energy2, " label='Energy 2 :' ");
		TwAddVarRO(stats, "E", TW_TYPE_FLOAT, &log_energy, " label='Energy :' ");
		TwAddVarRW(stats, "step", TW_TYPE_SCALARTYPE, &MIS::step, " label='Stepsize :' precision=2 min=0 step=0.1 max=2");
		TwAddVarRW(stats, "startstep", TW_TYPE_SCALARTYPE, &MIS::start_step, " label='Start stepsize :' precision=2 min=0 step=0.1 max=2");
		TwAddVarRW(stats, "lambda", TW_TYPE_SCALARTYPE, &MIS::lambda, " label='Lambda :' precision=2 min=0 step=0.1 max=500");
		TwAddVarRW(stats, "mu", TW_TYPE_SCALARTYPE, &MIS::mu, " label='Mu :' precision=2 min=0 max=1 step=0.1");
		
		TwAddVarRO(stats, "stabObj1", TW_TYPE_FLOAT, &log_toppling[0], " label='Stab. obj 1 :' precision=2 ");
		TwAddVarRO(stats, "stabObj2", TW_TYPE_FLOAT, &log_toppling[1], " label='Stab. obj 2 :' precision=2 ");
		
		log_objc[0][0] = log_objc[0][1] = log_objc[0][2] = 0;
		log_objc[1][0] = log_objc[1][1] = log_objc[1][2] = 0;
		TwAddVarRO(stats, "Obj. 1", TW_TYPE_COLOR3F, &log_objc[0], " colormode=hls ");
		TwAddVarRO(stats, "Obj. 2", TW_TYPE_COLOR3F, &log_objc[1], " colormode=hls ");
				
		media_exponent = 0.3;
		media_density = 0.7;
		media_color[0] = 158.0/255.0;
		media_color[1] = 189.0/255.0;
		media_color[2] = 248.0/255.0;
		mesh_color[0] = 158.0/255.0;
		mesh_color[1] = 189.0/255.0;
		mesh_color[2] = 248.0/255.0;
		mesh_color[3] = 74.0/255.0;
		imesh_color[0] = 238.0/255.0;
		imesh_color[1] = 250.0/255.0;
		imesh_color[2] = 0.0/255.0;
		imesh_color[3] = 138.0/255.0;

		TwAddVarRW(m_preview->bar, "Media exponent", TW_TYPE_FLOAT, &media_exponent, " group='Transparency' min=0.0 max=2.0 step=0.02 ");
		TwAddVarRW(m_preview->bar, "Media density", TW_TYPE_FLOAT, &media_density, " group='Transparency' min=0.0 max=5.0 step=0.1 ");
		TwAddVarRW(m_preview->bar, "Media color", TW_TYPE_COLOR3F, media_color, " group='Transparency' colormode=hls");
		TwAddVarRW(m_preview->bar, "Outer surface color", TW_TYPE_COLOR4F, mesh_color, " group='Transparency'");
		TwAddVarRW(m_preview->bar, "Inner surface color", TW_TYPE_COLOR4F, imesh_color, " group='Transparency'");
		TwAddVarCB(m_preview->bar, "Corner threshold", TW_TYPE_SCALARTYPE, SetCornerThresCB, GetCornerThresCB, this, " min=0 max=180 step=2 ");
		TwAddVarRW(m_preview->bar, "Plane offset", TW_TYPE_FLOAT, &plane_offset, " min=-0.7 max=0.7 step=0.02 ");

		cout << "\t [done]" << endl;
	}

	void MISPlugin::updateAfterConfigLoading() {
		
		log_nbV = MIS::m_mesh->nbV;
		log_nbF = MIS::m_mesh->nbF;

		deforming = false;
		mouse_down = false;
		mouse_button = 0;
		m_mouse_mode = NOTHING;
		optimization_in_progress = false;

		// IGL stuff to adapt the camera to the data
		get_scale_and_shift_to_fit_mesh(MIS::m_mesh->vertices,m_preview->zoom,m_preview->mesh_center);
		m_preview->radius = 1/m_preview->zoom;
		m_preview->shift = RowVector3(0,-0.1,0);
		
		MIS::m_config->updateSupport(*MIS::m_mesh,0);
		MIS::m_config->updateSupport(*MIS::m_mesh,1);

		resetCamera();

		if(MISPluginInstance.cfg_configfile == 0) {
			TwDefine(" MakeItStand/cfgNewFile visible=true ");
			TwDefine(" MakeItStand/meshFile visible=true ");
		}
		else {
			TwDefine(" MakeItStand/cfgNewFile visible=false ");
			TwDefine(" MakeItStand/meshFile visible=false ");
		}
		TwDefine(" MakeItStand/multiObj visible=true ");
		if(MIS::m_config->isMultipleObj()) TwDefine(" MakeItStand/currentObj visible=true ");
		TwDefine(" MakeItStand/supportMode visible=true ");
		TwDefine(" MakeItStand/flatThres visible=true ");
		TwDefine(" MakeItStand/angleObj visible=true ");
		TwDefine(" MakeItStand/meshRotation visible=true ");
		TwDefine(" MakeItStand/voxRes visible=true ");
	}

	void MISPlugin::updateAfterVoxelizing() {	
		log_nbVox = MIS::m_voxels->getNumBoxes();	
		
		deforming = false;
		mouse_down = false;
		mouse_button = 0;
		m_mouse_mode = NOTHING;
		optimization_in_progress = false;
		
		TwDefine(" MakeItStand/balanceMode visible=true ");
		TwDefine(" MakeItStand/hullDepth visible=true ");
		TwDefine(" MakeItStand/multiObj readonly ");
		TwDefine(" MakeItStand/supportMode readonly ");
		TwDefine(" MakeItStand/flatThres readonly ");
		TwDefine(" MakeItStand/meshRotation readonly ");
		TwDefine(" MakeItStand/voxRes readonly ");

		slcrystal_init(m_preview->height, 1.0f, 40.0f);
	}

	static bool embree_create_handle(int mouse_x, int mouse_y, const EmbreeIntersector *emb,
		double *modelview_matrix, double *projection_matrix, int *viewport, 
		RowVector3 & P)
	{

		if (!emb) 
			return -1;
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates

		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;
		
		double direction[3];
		gluUnProject(winX, winY, 1, modelview_matrix,projection_matrix,viewport, &(direction[0]),&(direction[1]),&(direction[2]));
		Vector3 dir(direction[0],direction[1],direction[2]);

		Vector3 origin;
		Matrix44 modelViewMat;
		for (int i = 0; i<4; ++i)
			for (int j = 0; j<4; ++j)
				modelViewMat(i,j) = modelview_matrix[4*j+i];

		origin = modelViewMat.inverse().block(0,3,3,1);
		dir -= origin;
		dir.normalize();

		embree::Ray hit1(toVec3f(origin),toVec3f(dir));
		int fi = -1;
		RowVector3 bc;

		if (emb->intersectRay(hit1))
		{
			fi = hit1.id0;
			bc << 1 - hit1.u - hit1.v, hit1.u, hit1.v;
			RowVector3 P1 = MIS::m_mesh->V(fi,bc);

			origin = origin + 50.0f*dir;
			dir = -dir;
			embree::Ray hit2(toVec3f(origin),toVec3f(dir));
			if (emb->intersectRay(hit2))
			{
				fi = hit2.id0;
				bc << 1 - hit2.u - hit2.v, hit2.u, hit2.v;
				RowVector3 P2 = MIS::m_mesh->V(fi,bc);
				P = 0.5*(P1+P2);
				return true;
			}
		}

		return false;
	}

	static bool embree_pick_support_point(int mouse_x, int mouse_y, const EmbreeIntersector *emb,
		double *modelview_matrix, double *projection_matrix, int *viewport, 
		RowVector3 & P)
	{

		if (!emb) 
			return -1;
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates

		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;
		
		double direction[3];
		gluUnProject(winX, winY, 1, modelview_matrix,projection_matrix,viewport, &(direction[0]),&(direction[1]),&(direction[2]));
		Vector3 dir(direction[0],direction[1],direction[2]);

		Vector3 origin;
		Matrix44 modelViewMat;
		for (int i = 0; i<4; ++i)
			for (int j = 0; j<4; ++j)
				modelViewMat(i,j) = modelview_matrix[4*j+i];

		origin = modelViewMat.inverse().block(0,3,3,1);
		dir -= origin;
		dir.normalize();

		embree::Ray hit(toVec3f(origin),toVec3f(dir));
		int fi = -1;
		RowVector3 bc;

		if (emb->intersectRay(hit))
		{
			fi = hit.id0;
			bc << 1 - hit.u - hit.v, hit.u, hit.v;
			P = MIS::m_mesh->V(fi,bc);
			return true;
		}

		return false;
	}

	static bool pick_id_handle(int mouse_x, int mouse_y, const Handles & handles,
		double *modelview_matrix, double *projection_matrix, int *viewport, 
		int & idHandle)
	{
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates

		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;

		idHandle = -1;
		ScalarType best = 50.0; double dist;
		double posX, posY, posZ;

		for(int j = 0 ; j < handles.getNumHandles() ; ++j) {
			RowVector3 p = handles.getHandlePose(j);
			gluProject(p[0],p[1],p[2],modelview_matrix,projection_matrix,viewport,&posX,&posY,&posZ);
			dist = sqrt((posX-winX)*(posX-winX) + (posY-winY)*(posY-winY));
			if(dist < best) {
				idHandle = j;
				best = dist;
			}
		}

		return idHandle != -1;
	}

	static bool pick_id_handle(int mouse_x, int mouse_y, const Config & cfg,
		double *modelview_matrix, double *projection_matrix, int *viewport, 
		int & idHandle)
	{
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates

		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;

		idHandle = -1;
		ScalarType best = 50.0;
		double posX, posY, posZ;

		for(unsigned int j = 0 ; j < cfg.getNbHandles() ; ++j) {
			const RowVector3 & p = cfg.getHandle(j);
			gluProject(p[0],p[1],p[2],modelview_matrix,projection_matrix,viewport,&posX,&posY,&posZ);
			double dist = sqrt((posX-winX)*(posX-winX) + (posY-winY)*(posY-winY));
			if(dist < best) {
				idHandle = j;
				best = dist;
			}
		}

		return idHandle != -1;
	}

	static bool check_close_to(
		const RowVector3 & pos,
		int mouse_x, int mouse_y,
		double *modelview_matrix, double *projection_matrix, int *viewport)
	{
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates

		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;

		double posX, posY, posZ;
		gluProject(pos[0],pos[1],pos[2],modelview_matrix,projection_matrix,viewport,&posX,&posY,&posZ);
		return sqrt((posX-winX)*(posX-winX) + (posY-winY)*(posY-winY)) < 100.0;
	}

	void MISPlugin::get_translation(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, RowVector3 & translation)
	{
		GLdouble winX, winY, winZ;
		gluProject(pos0[0],pos0[1],pos0[2],m_fullViewMatrix,m_projMatrix,m_viewport, &winX, &winY, &winZ);

		double x,y,z;
		RowVector3 p1, p2;
		winX = from_x;
		winY = m_viewport[3] - from_y;
		gluUnProject(winX, winY, winZ, m_fullViewMatrix,m_projMatrix,m_viewport, &x,&y,&z);
		p1 << x,y,z;
		winX = mouse_x;
		winY = m_viewport[3] - mouse_y;
		gluUnProject(winX, winY, winZ, m_fullViewMatrix,m_projMatrix,m_viewport, &x,&y,&z);
		p2 << x,y,z;

		translation = p2-p1;
	}

	void MISPlugin::get_scale(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, ScalarType & scale)
	{
		GLdouble winX, winY, winZ;

		// get depth of the 3D point
		gluProject(pos0[0],pos0[1],pos0[2],m_fullViewMatrix,m_projMatrix,m_viewport, &winX, &winY, &winZ);
		
		scale = clamp(0.02*(mouse_x-from_x), -0.5 , 1.0);
	}

	// calculate a rotation given the mouse location and the mouse location when the dragging began
	void MISPlugin::get_rotation(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, Quaternion & rotation)
	{

				double origin_x, origin_y, origin_z;
				gluProject(
					pos0[0],pos0[1],pos0[2],
					m_fullViewMatrix, m_projMatrix,
					m_viewport, &origin_x, &origin_y, &origin_z);

				float q[4];
				float center_x=0., center_y=0., half_width=0., half_height=0.;
				// Trackball centered at object's centroid
				center_x = (float)origin_x;
				center_y = (float)origin_y;

				half_width =  ((float)(m_viewport[2]))/4.f;
				half_height = ((float)(m_viewport[3]))/4.f;

				trackball(q,
					(center_x-from_x)/half_width,
					(from_y-center_y)/half_height,
					(center_x-mouse_x)/half_width,
					(mouse_y-center_y)/half_height);
				// I think we need to do this because we have z pointing out of the
				// screen rather than into the screen
				q[2] = -q[2];
				
				rotation = Quaternion(q[3],q[0],q[1],q[2]);

	}

	// keyboard callback
	bool MISPlugin::keyDownEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y) {
		

		//cout << (unsigned int) key << endl;

		if(key == 2) enable_shadows = !enable_shadows;
		if(key == 3) enable_transparency = !enable_transparency;
		if(key == 4) m_preview->SetAutoRotate(!m_preview->g_AutoRotate);
		if(key == 5) resetCamera();


		if(!MIS::isConfigLoaded()) return false;

		if(m_mouse_mode == SUPPORT_POINT)
			return false;
		
		if(key == 100) {
			m_preview->g_RotationAngle -= 0.1;
		}
		if(key == 102) {
			m_preview->g_RotationAngle += 0.1;
		}



		if(key == '0') {
			m_mouse_mode = NOTHING;
			return true;
		}

		if(key == 13 && MIS::m_config->isMultipleObj()) {
			MIS::currentObj = 1-MIS::currentObj;
			return true;
		}

		if(!MIS::isVoxelized()) {
			if( modifiers == (Preview3D::ALT) ) {

				if(key == 'e')
				{
					m_mouse_mode = HANDLE_CREATION;
					return true;
				}
				if(key == 'g')
				{
					if(MIS::m_config->isSuspendedMode(MIS::currentObj)) {
						m_mouse_mode = SUPPORT_POINT;
						return true;
					}
				}
				if(key == 'h')//ALT + T: next ALT+click will be translation of the selected region
				{
					m_mouse_mode = MOVE_TARGET;
					return true;
				}
			}
		}
		else if(MIS::isVoxelized()) {
			if( modifiers == (Preview3D::ALT) ) {
				scale = 0.;
				translation = RowVector3::Zero();
				rotation = Quaternion::Identity();

				if(key == 't')//ALT + T: next ALT+click will be translation of the selected region
				{
					m_mouse_mode = TRANSLATE_HANDLE;
					return true;
				}
				if(key == 'z')//ALT + R: next ALT+click will be rotation of the selected region
				{
					m_mouse_mode = SCALE_HANDLE;
					return true;
				}
			}


		
			if(key == ' ') {
				optimization_in_progress = !optimization_in_progress;
				MIS::number_of_iterations = 0;
				MIS::time_count = 0;
				return true;
			}

			m_mouse_mode = NOTHING;
		}

		if(key == 'b') {
			MIS::m_config->setBalancingMode(PLANE_CARVING);
			if(MIS::isOptimized()) MIS::balance();
		}
		if(key == 'r') {
			MIS::reset();
		}

		return false;
	}
	bool MISPlugin::keyUpEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y) {
		return false;
	}

	//mouse callback
	bool MISPlugin::mouseDownEvent(int mouse_x, int mouse_y, int button, int modifiers) { 
		if(!MIS::isConfigLoaded()) return false;

		mouse_down = true;
		mouse_button = button;
		from_x = mouse_x;
		from_y = mouse_y;

		if(button == 2) {
			if(m_mouse_mode == HANDLE_CREATION) {
				if(tmp_id_handle >= 0) {
					MIS::m_config->deleteHandle(tmp_id_handle);
					tmp_id_handle = -1;
				}
				else {
					MIS::m_config->addHandle(tmp_handle);
				}
				return true;
			}
			if(m_mouse_mode == SUPPORT_POINT) {
				MIS::m_config->setSupportPoint(tmp_support_point,MIS::currentObj);
				m_mouse_mode = NOTHING;
				return true;
			}
			if(MIS::isVoxelized() && tmp_id_handle >= 0) {
				MIS::m_handles->changeLock(tmp_id_handle);
				return true;
			}
		}
		if(button == 0) {
			if(m_mouse_mode == TRANSLATE_HANDLE || m_mouse_mode == SCALE_HANDLE) {
				deforming = (tmp_id_handle >= 0);
				MIS::m_handles->getFullTransform(tmp_id_handle,translation_down,scale_down);
			}
			else if(m_mouse_mode == HANDLE_CREATION) {
				deforming = (tmp_id_handle >= 0);
			}
			else if(m_mouse_mode == MOVE_TARGET) {
				deforming = true;
				rotation_down = MIS::m_config->getMeshRotation(MIS::currentObj);
			}
		}

		return false;

	};
	bool MISPlugin::mouseUpEvent(int mouse_x, int mouse_y, int button, int modifiers) {
		if(!MIS::isConfigLoaded()) return false;

		mouse_down = false;
		if(deforming) {
			deforming = false;
			scale = 0.;
			translation = RowVector3::Zero();
			rotation = Quaternion::Identity();

			return true;
		}

		return false;
	};
	bool MISPlugin::mouseMoveEvent(int mouse_x, int mouse_y) {
		if(!MIS::isConfigLoaded()) return false;

		if(mouse_down) {
			if(deforming) {
				if(m_mouse_mode == TRANSLATE_HANDLE) {
					get_translation(
						MIS::m_handles->getHandlePose(tmp_id_handle),
						mouse_x,mouse_y, from_x, from_y, translation);

					MIS::m_handles->setT(tmp_id_handle,translation_down+translation);
					MIS::updateGeometry();
					if(MIS::isOptimized()) {
						MIS::balance();
						if(MIS::m_config->checkObj(0,MIS::m_opt->getCenterOfMass()) == 0) {
							optimization_in_progress = true;
							MIS::number_of_iterations = 0;
							MIS::time_count = 0;
						}
					}
					return true;
				}
				else if(m_mouse_mode == SCALE_HANDLE) {
					get_scale(
						MIS::m_handles->getHandlePose(tmp_id_handle),
						mouse_x,mouse_y, from_x, from_y, scale);

					MIS::m_handles->setS(tmp_id_handle,scale_down + scale);
					MIS::updateGeometry();
					if(MIS::isOptimized()) {
						MIS::balance();
						if(MIS::m_config->checkObj(0,MIS::m_opt->getCenterOfMass()) == 0) {
							optimization_in_progress = true;
							MIS::number_of_iterations = 0;
							MIS::time_count = 0;
						}
					}
					return true;
				}
				else if(m_mouse_mode == MOVE_TARGET) {
					get_rotation(
						m_preview->mesh_center,
						mouse_x,mouse_y, from_x, from_y, rotation);
					
					MIS::m_config->setMeshRotation((rotation * rotation_down).normalized(),MIS::currentObj);
					MIS::m_config->updateSupport(*MIS::m_mesh,MIS::currentObj);
					
					return true;
				}
				else if(m_mouse_mode == HANDLE_CREATION) {
					get_translation(
						MIS::m_config->getHandle(tmp_id_handle),
						mouse_x,mouse_y, from_x, from_y, translation);
					MIS::m_config->moveHandle(tmp_id_handle,translation);
					
					from_x = mouse_x;
					from_y = mouse_y;
				}

				return true;
			}
			return false;
		}
		else {
			if(m_mouse_mode == TRANSLATE_HANDLE || m_mouse_mode == SCALE_HANDLE) {
				pick_id_handle(mouse_x,mouse_y,*MIS::m_handles,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_id_handle);
			}
			else if(m_mouse_mode == HANDLE_CREATION) {
				if(pick_id_handle(mouse_x,mouse_y,*MIS::m_config,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_id_handle)) {
					tmp_ha_set = false;
				}
				else {
					tmp_ha_set = embree_create_handle(mouse_x,mouse_y,MIS::m_embree,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_handle);
				}
			}
			else if(m_mouse_mode == SUPPORT_POINT) {
				tmp_sp_set = embree_pick_support_point(mouse_x,mouse_y,MIS::m_embree,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_support_point);
			}
			else if(MIS::isVoxelized() && m_mouse_mode == NOTHING) {
				pick_id_handle(mouse_x,mouse_y,*MIS::m_handles,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_id_handle);
			}
		}
		
		return false;
	};
	bool MISPlugin::mouseScrollEvent(int mouse_x, int mouse_y, float delta) {
		return false;
	};

	const static RowVector4 green(0,1,0,1);
	const static RowVector4 red(1,0,0,1);
	const static RowVector4 blue(0,0,1,1);
	const static RowVector4 yellow(1,1,0,1);
	const static RowVector4 white(1,1,1,1);
	const static RowVector4 black(0,0,0,1);

	void MISPlugin::drawMesh() {
		
		glColor3f(mesh_color[0],mesh_color[1],mesh_color[2]);
		MIS::render_mesh(MIS::m_mesh,tmp_id_handle);

	}
	void MISPlugin::drawPlane() {
		const RowVector3 & g = MIS::m_config->getGravity(MIS::currentObj);
		const RowVector3 & g1 = MIS::m_config->getGravityT1(MIS::currentObj);
		const RowVector3 & g2 = MIS::m_config->getGravityT2(MIS::currentObj);

		RowVector3 cn = -g;

		RowVector3 ref;
		if(MIS::m_config->isSuspendedMode(MIS::currentObj))
			ref = MIS::m_config->getSupportPoint(MIS::currentObj) - (1.0+plane_offset)*cn;
		else if(MIS::m_config->isStandingMode(MIS::currentObj))
			ref = MIS::m_config->getSupportConst(MIS::currentObj)->projectOnSupport(RowVector3(0.5,0.5,0.5)) - 0.001*cn;
		RowVector3 c1 = ref + 100.0*g1 + 100.0*g2;
		RowVector3 c2 = ref + 100.0*g1 - 100.0*g2;
		RowVector3 c3 = ref - 100.0*g1 - 100.0*g2;
		RowVector3 c4 = ref - 100.0*g1 + 100.0*g2;

		glBegin(GL_QUADS);
		glNormal3f(cn[0],cn[1],cn[2]);
		glVertex3f(c1[0],c1[1],c1[2]);
		glVertex3f(c2[0],c2[1],c2[2]);
		glVertex3f(c3[0],c3[1],c3[2]);
		glVertex3f(c4[0],c4[1],c4[2]);
		glEnd();
	};

	void MISPlugin::setOpenglLight() {
		float v[4]; 
		glEnable(GL_LIGHT0);
		v[0] = v[1] = v[2] = m_preview->g_LightMultiplier*0.4f; v[3] = 1.0f;
		glLightfv(GL_LIGHT0, GL_AMBIENT, v);
		v[0] = v[1] = v[2] = m_preview->g_LightMultiplier*1.0f; v[3] = 1.0f;
		glLightfv(GL_LIGHT0, GL_DIFFUSE, v);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_cameraViewMatrix);

		v[0] = - m_preview->g_LightDistance*m_preview->g_LightDirection[0];
		v[1] = - m_preview->g_LightDistance*m_preview->g_LightDirection[1];
		v[2] = - m_preview->g_LightDistance*m_preview->g_LightDirection[2];
		v[3] = 1.0f;
		glLightfv(GL_LIGHT0, GL_POSITION, v);
	}

	void MISPlugin::setOpenglMatrices() {
		
		double mat[16];

		glViewport(300, 0, m_preview->width-300, m_preview->height);
		glGetIntegerv(GL_VIEWPORT, m_viewport);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		double fH = tan( m_preview->view_angle / 360.0 * M_PI ) * m_preview->dnear;
		double fW = fH * (double)(m_preview->width-300)/(double)m_preview->height;

		glFrustum( -fW, fW, -fH, fH, m_preview->dnear, m_preview->dfar);

		glGetDoublev(GL_PROJECTION_MATRIX, m_projMatrix);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glFrustum( -fH, fH, -fH, fH, m_preview->dnear, m_preview->dfar);

		glGetDoublev(GL_PROJECTION_MATRIX, m_projSquaredMatrix);



		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	
		gluLookAt(m_preview->eye[0], m_preview->eye[1], m_preview->eye[2], m_preview->center[0], m_preview->center[1], m_preview->center[2], m_preview->up[0], m_preview->up[1], m_preview->up[2]);
		
		glScaled(m_preview->g_Zoom, m_preview->g_Zoom, m_preview->g_Zoom);
		glScaled(m_preview->zoom, m_preview->zoom, m_preview->zoom);
	
		glTranslated(m_preview->shift[0],m_preview->shift[1],m_preview->shift[2]);

		QuaternionToMatrix44(m_preview->g_Rotation,mat);
		glMultMatrixd(mat);

		glGetDoublev(GL_MODELVIEW_MATRIX, m_cameraViewMatrix);
		


		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		Quaternion q0 = MIS::m_config->getMeshRotation(MIS::currentObj);
		QuaternionToMatrix44(q0,mat);
		glMultMatrixd(mat);
		
		Quaternion q(AngleAxis(m_preview->g_RotationAngle,MIS::m_config->getGravity(MIS::currentObj).normalized()));
		QuaternionToMatrix44(q,mat);
		glMultMatrixd(mat);
		
		glTranslated(-m_preview->mesh_center[0],-m_preview->mesh_center[1],-m_preview->mesh_center[2]);

		glGetDoublev(GL_MODELVIEW_MATRIX, m_modelViewMatrix);



		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	
		RowVector3 l_LookAt = m_preview->mesh_center;
		RowVector3 l_Pos = l_LookAt - m_preview->g_LightDistance*RowVector3(m_preview->g_LightDirection[0],m_preview->g_LightDirection[1],m_preview->g_LightDirection[2]);
		gluLookAt(l_Pos[0], l_Pos[1], l_Pos[2], l_LookAt[0], l_LookAt[1], l_LookAt[2], 1, 0, 0);
		
		glGetDoublev(GL_MODELVIEW_MATRIX, m_lightViewMatrix);



		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_cameraViewMatrix);
		glMultMatrixd(m_modelViewMatrix);

		glGetDoublev(GL_MODELVIEW_MATRIX, m_fullViewMatrix);
	}

	void MISPlugin::initOpenglMatricesForLight() {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixd(m_projMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_lightViewMatrix);
		glMultMatrixd(m_modelViewMatrix);
	}

	void MISPlugin::initOpenglMatricesForCamera(bool forceSquared) {
		glViewport(300,0,m_preview->width-300,m_preview->height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if(forceSquared) {
			glMultMatrixd(m_projSquaredMatrix);
		}
		else {
			glMultMatrixd(m_projMatrix);
		}
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_fullViewMatrix);
	}

	void MISPlugin::initOpenglMatricesForWorld() {
		glViewport(300,0,m_preview->width-300,m_preview->height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixd(m_projMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_cameraViewMatrix);
	}

	void MISPlugin::resize(int w, int h) {}

	// draw pre/post-process
	void MISPlugin::preDraw(int currentTime) { 
		if(!MIS::isConfigLoaded()) return;
		
		if(optimization_in_progress) {
			if(naive_mode)
			optimization_in_progress = MIS::optimizeNaive();
			else 
			optimization_in_progress = MIS::optimize();
		}

		if(MIS::isOptimized()) {
			log_mass = MIS::m_opt->getMass();
			log_energy1obj1 = MIS::m_opt->getEnergy1(0);
			log_energy1obj2 = MIS::m_opt->getEnergy1(1);
			log_energy1sum = log_energy1obj1+log_energy1obj2;
			log_energy2 = MIS::lambda * MIS::m_opt->getEnergy2();
			log_energy = MIS::m_opt->getCurrentEnergy();
			
			for(int idObj = 0 ; idObj < MIS::m_config->nbObj() ; ++idObj) {
				log_obj[idObj] = MIS::m_config->checkObj(idObj,MIS::m_opt->getCenterOfMass(),&log_toppling[idObj]);
				if(log_obj[idObj] == 0) {
					log_objc[idObj][0] = 1.0f; log_objc[idObj][1] = 0.0f; log_objc[idObj][2] = 0.0f;
				}
				else if(log_obj[idObj] == 1) {
					log_objc[idObj][0] = 1.0f; log_objc[idObj][1] = 0.64f; log_objc[idObj][2] = 0.0f;
				}
				else if(log_obj[idObj] == 2) {
					log_objc[idObj][0] = 0.0f; log_objc[idObj][1] = 1.0f; log_objc[idObj][2] = 0.0f;
				}
			}
		}
		else {
			for(int idObj = 0 ; idObj < MIS::m_config->nbObj() ; ++idObj) {
				log_objc[idObj][0] = 0.0f; log_objc[idObj][1] = 0.0f; log_objc[idObj][2] = 0.0f;
			}
		}

		log_mouse_mode = m_mouse_mode;
		
		log_FPS = (int) (1000.0f/(currentTime-previousTime));

		if( m_preview->g_AutoRotate ) 
		{
			ScalarType delta_angle = (currentTime-previousTime)/1000.0f;
			m_preview->g_RotationAngle += delta_angle;
			if(m_preview->g_RotationAngle > 2.0*M_PI)
				m_preview->g_RotationAngle -= 2.0*M_PI;
			
			Quaternion q(AngleAxis(m_preview->g_RotationAngle,MIS::m_config->getGravity(MIS::currentObj).normalized()));
		}

		previousTime = currentTime;

		setOpenglMatrices();

		initOpenglMatricesForWorld();
		glGetIntegerv(GL_VIEWPORT, m_preview->m_viewport);
		glGetDoublev(GL_PROJECTION_MATRIX, m_preview->m_projection_matrix);
		glGetDoublev(GL_MODELVIEW_MATRIX, m_preview->m_modelview_matrix);
		
		// Set material
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_preview->g_MatAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_preview->g_MatDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_preview->g_MatSpecular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_preview->g_MatShininess);

		return;
	};

	void MISPlugin::postDraw(int currentTime) {
		if(!MIS::isConfigLoaded()) return;

		if(enable_shadows) {
			PCFShadow::beforeShadow();
			initOpenglMatricesForLight();
			drawMesh();
			PCFShadow::afterShadow();
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_NORMALIZE);
		glEnable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glDisable(GL_COLOR_MATERIAL);
		glDepthMask( GL_TRUE );
		
		glPushAttrib(GL_ENABLE_BIT);
		
		setOpenglLight();
		initOpenglMatricesForCamera();

		if(enable_shadows)
			PCFShadow::beforeRender();
		else 
			glUseProgram(m_preview->shader_id); 
		
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Preview3D::WHITE);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Preview3D::WHITE);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Preview3D::BLACK);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_preview->g_MatShininess);
		glColor3f(Preview3D::WHITE[0],Preview3D::WHITE[1],Preview3D::WHITE[2]);
		drawPlane();

		if(MIS::m_config->isStandingMode(MIS::currentObj)) {
			glUseProgram(0);
			glDisable(GL_LIGHTING);
			glColor3f(log_objc[MIS::currentObj][0],log_objc[MIS::currentObj][1],log_objc[MIS::currentObj][2]);
			MIS::m_config->getSupport(MIS::currentObj)->draw();
		}
		else {
			glUseProgram(m_preview->shader_id);
			glEnable(GL_LIGHTING);
			paintCylinder(MIS::m_config->getTarget(MIS::currentObj),MIS::m_config->getTarget(MIS::currentObj)-MIS::m_config->getGravity(MIS::currentObj),0.01,yellow); 
			paintPoint(MIS::m_config->getTarget(MIS::currentObj),0.015,yellow); 
		}

		if(enable_transparency) {
			drawWithTransparency();
		}
		else {
			glUseProgram(m_preview->shader_id);
			glEnable(GL_LIGHTING);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_preview->g_MatAmbient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_preview->g_MatDiffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_preview->g_MatSpecular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_preview->g_MatShininess);
			drawMesh();

			drawMIS_UI();
		}

		glPopAttrib();

		return;
	}

	void MISPlugin::drawWithTransparency() {
		
		glUseProgram(m_preview->shader_id);
		glEnable(GL_LIGHTING);
		if(MIS::isOptimized()) {
			const RowVector3 & com = MIS::m_opt->getCenterOfMass();
			for(int idObj = 0 ; idObj < MIS::m_config->nbObj() ; ++idObj) {
				if(MIS::m_config->isStandingMode(idObj)) {
					RowVector4 color(log_objc[idObj][0],log_objc[idObj][1],log_objc[idObj][2],1.0f);
					const RowVector3 & cstar = MIS::m_config->getSupport(idObj)->getTarget();
					RowVector3 comProj = MIS::m_config->getSupport(idObj)->projectOnSupport(com);
					paintPoint(comProj,0.02,color);
					paintArrow(comProj,cstar,0.01,color);
				}
			}
		}

		float persp[16];
		float view[16];
		initOpenglMatricesForCamera(true);
		glGetFloatv(GL_PROJECTION_MATRIX, persp);
		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		transpose(persp);
		transpose(view);

		// setup crystal
		int offset = 300 + (m_preview->width - 300 - m_preview->height)/2;
		slcrystal_set_viewport_offset(offset,0);
		slcrystal_set_perspective( persp ); // perspective matrix
		slcrystal_set_view( view );  // view (camera) matrix 
		
		float lightPos[3];
		RowVector3 lpos = -m_preview->g_LightDistance*RowVector3(m_preview->g_LightDirection[0],m_preview->g_LightDirection[1],m_preview->g_LightDirection[2]);
		Quaternion q1 = MIS::m_config->getMeshRotation(MIS::currentObj);
		Quaternion q2(AngleAxis(m_preview->g_RotationAngle,MIS::m_config->getGravity(MIS::currentObj)));
		lpos = m_preview->mesh_center + lpos * (q1*q2).toRotationMatrix();
		lightPos[0] = lpos[0];
		lightPos[1] = lpos[1];
		lightPos[2] = lpos[2];
		
		slcrystal_set_lightpos( lightPos );  // light direction
		slcrystal_set_media_color(media_color[0],media_color[1],media_color[2]); // media color inside objects
		slcrystal_set_media_exponent(media_exponent); // exponent: the higher the less visible the media is
		slcrystal_set_media_density(media_density);
		slcrystal_frame_begin(1.0f,1.0f,1.0f);

		slcrystal_begin(mesh_color[0],mesh_color[1],mesh_color[2],mesh_color[3]);
		drawMesh();
		slcrystal_end();
		
		if(MIS::isVoxelized()) {	
			slcrystal_begin(imesh_color[0],imesh_color[1],imesh_color[2],imesh_color[3]); 
			MIS::render_innermesh(MIS::m_imesh);
			slcrystal_end();
		}

		if(m_mouse_mode == HANDLE_CREATION && tmp_ha_set) {
			paintPointSL(tmp_handle,0.02,yellow);
		}

		if(m_mouse_mode == SUPPORT_POINT && tmp_sp_set) {
			paintPointSL(tmp_support_point,0.02,yellow);
		}

		if(!MIS::isVoxelized()) {
			for(unsigned int i = 0 ; i < MIS::m_config->getNbHandles() ; ++i) {
				if(i == tmp_id_handle)
					paintPointSL(MIS::m_config->getHandle(i),0.02,yellow);
				else
					paintPointSL(MIS::m_config->getHandle(i),0.02,white);
			}
		}
		else {
			for(int j = MIS::m_config->nbObj() ; j < MIS::m_handles->getNumHandles() ; ++j) {
				RowVector4 color;
				if(j == tmp_id_handle) color = yellow;
				else if(MIS::m_handles->isLocked(j)) color = red;
				else color = green;
				RowVector3 P = MIS::m_handles->getHandlePose(j);
				paintPointSL(P, 0.02,color);
				if(display_info && MIS::isOptimized())
					paintArrowSL(P, P - MIS::step*MIS::m_opt->getGradT(j),0.01,color);
			}
		}
		
		
		if(MIS::isOptimized()) {
			const RowVector3 & com = MIS::m_opt->getCenterOfMass();

			paintPointSL(com,0.03,black);
			
		}

		slcrystal_frame_end();
		
	};
	
	void MISPlugin::drawMIS_UI() {
		glUseProgram(m_preview->shader_id);

		if(display_info) {
			if(m_mouse_mode == HANDLE_CREATION && tmp_ha_set) {
				paintPoint(tmp_handle,0.02,yellow);
			}

			if(m_mouse_mode == SUPPORT_POINT && tmp_sp_set) {
				paintPoint(tmp_support_point,0.02,yellow);
			}

			if(!MIS::isVoxelized()) {
				for(unsigned int i = 0 ; i < MIS::m_config->getNbHandles() ; ++i) {
					if(i == tmp_id_handle)
						paintPoint(MIS::m_config->getHandle(i),0.02,yellow);
					else
						paintPoint(MIS::m_config->getHandle(i),0.02,white);
				}
			}
			else {
				for(int j = MIS::m_config->nbObj() ; j < MIS::m_handles->getNumHandles() ; ++j) {
					RowVector4 color;
					if(j == tmp_id_handle) color = yellow;
					else if(MIS::m_handles->isLocked(j)) color = red;
					else color = green;
					RowVector3 P = MIS::m_handles->getHandlePose(j);
					paintPoint(P, 0.02, color);
					if(display_info && MIS::isOptimized())
						paintArrow(P, P - MIS::step*MIS::m_opt->getGradT(j),0.01,color);
				}
			}
		}
		
		
		if(MIS::isOptimized()) {
			const RowVector3 & com = MIS::m_opt->getCenterOfMass();
			if(display_info)
				paintPoint(com,0.03,black);

			
			for(int idObj = 0 ; idObj < MIS::m_config->nbObj() ; ++idObj) {
				if(MIS::m_config->isStandingMode(idObj)) {
					RowVector4 color(log_objc[idObj][0],log_objc[idObj][1],log_objc[idObj][2],1.0f);
					const RowVector3 & cstar = MIS::m_config->getSupport(idObj)->getTarget();
					RowVector3 comProj = MIS::m_config->getSupport(idObj)->projectOnSupport(com);
					paintPoint(comProj,0.02,color);
					paintArrow(comProj,cstar,0.01,color);
				}
			}
		}

		glUseProgram(0);
		glDisable(GL_LIGHTING);
		if(MIS::isOptimized() && tmp_id_handle >= MIS::m_config->nbObj()) {
			glPushMatrix();
		
			RowVector3 t; ScalarType s;
			MIS::m_handles->getFullTransform(tmp_id_handle,t,s);

			glTranslated(t[0],t[1],t[2]);
			glScaled(s,s,s);
			
			double mat[16];
			QuaternionToMatrix44(MIS::m_config->getMeshRotation(MIS::currentObj).inverse(),mat);
			glMultMatrixd(mat);

			paintTrackball(0.2);

			glPopMatrix();
		}
		
	};
	
	void MISPlugin::resetCamera() {
		m_preview->g_Rotation.setIdentity();
	}

	void MISPlugin::close() {
			if(!MIS::isConfigLoaded()) return;
	
			if(cfg_configfile == 0) TwDefine(" MakeItStand/meshFile visible=true ");
			TwDefine(" MakeItStand/meshFile readwrite ");
			TwDefine(" MakeItStand/multiObj visible=false ");
			TwDefine(" MakeItStand/currentObj visible=false ");
			TwDefine(" MakeItStand/supportMode visible=false ");
			TwDefine(" MakeItStand/flatThres visible=false ");
			TwDefine(" MakeItStand/angleObj visible=false ");
			TwDefine(" MakeItStand/meshRotation visible=false ");
			TwDefine(" MakeItStand/voxRes visible=false ");
			TwDefine(" MakeItStand/balanceMode visible=false ");
			TwDefine(" MakeItStand/hullDepth visible=false ");

			TwDefine(" MakeItStand/cfgFile readwrite ");	
			TwDefine(" MakeItStand/multiObj readwrite ");
			TwDefine(" MakeItStand/supportMode readwrite ");
			TwDefine(" MakeItStand/flatThres readwrite ");
			TwDefine(" MakeItStand/meshRotation readwrite ");
			TwDefine(" MakeItStand/voxRes readwrite ");

			resetVars();

			MIS::uninitialize();
	}
