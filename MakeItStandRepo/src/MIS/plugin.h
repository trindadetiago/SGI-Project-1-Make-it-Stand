#ifndef __MIS_PLUGIN_H
#define __MIS_PLUGIN_H
#include "viewer/PreviewPlugin.h"
#include "viewer/PluginManager.h"

#include "MIS_inc.h"

enum MouseMode { NOTHING, TRANSLATE_HANDLE, SCALE_HANDLE, HANDLE_CREATION, SUPPORT_POINT, MOVE_TARGET };

class MISPlugin : public PreviewPlugin
{
public:
	MISPlugin()
	{
		PluginManager().register_plugin(this);

		cfg_configfile = 0;
		cfg_confignewfile = ".mis";
		cfg_meshfile = 0;
		scriptfile = 0;
		optfile = 0;

		enable_shadows = true;
		enable_transparency = false;
		naive_mode = false;
		resetVars();
	}

	void resetVars() {
		deforming = false;
		mouse_down = false;
		mouse_button = 0;
		m_mouse_mode = NOTHING;
		
		enable_shadows = true;
		enable_transparency = true;
		display_info = false;
		display_mesh = true;
		
		tmp_id_handle = -1;

		optimization_in_progress = false;
	}

	~MISPlugin() {
	}

	void init(Preview3D *preview);
	void updateAfterConfigLoading();
	void updateAfterVoxelizing();

	void get_translation(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, RowVector3 & translation);
	void get_scale(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, ScalarType & scale);
	void get_rotation(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, Quaternion & rotation);

	// keyboard callback
	bool keyDownEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y);
	bool keyUpEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y);

	//mouse callback
	bool mouseDownEvent(int mouse_x, int mouse_y, int button, int modifiers);
	bool mouseUpEvent(int mouse_x, int mouse_y, int button, int modifiers);
	bool mouseMoveEvent(int mouse_x, int mouse_y);
	bool mouseScrollEvent(int mouse_x, int mouse_y, float delta);
	void preDraw(int currentTime);
	void postDraw(int currentTime);
	void resize(int w, int h);
	
	void drawMesh();
	void drawPlane();
	void resetCamera();
	void drawMIS_UI();
	void drawWithTransparency();

	void close();

	void setOpenglLight();
	void setOpenglMatrices();
	
	void initOpenglMatricesForLight();
	void initOpenglMatricesForCamera(bool forceSquared = false);
	void initOpenglMatricesForWorld();

	string getConfigfile() const {
		if(cfg_configfile == 0) return cfg_confignewfile;
		return cfgFiles[cfg_configfile-1];
	}
	string getMeshfile() const { return objFiles[cfg_meshfile]; }
	
	MouseMode m_mouse_mode;
	
	std::string cfg_confignewfile;
	int cfg_configfile;
	int cfg_meshfile;
	int scriptfile;
	int optfile;
	
	int previousTime;

	int log_nbV;
	int log_nbF;
	int log_nbVox;
	int log_FPS;

	int log_mouse_mode;

	float log_mass;
	float log_energy1obj1;
	float log_energy1obj2;
	float log_energy1sum;
	float log_energy2;
	float log_energy;

	int log_obj[2];
	float log_toppling[2];
	float log_objc[2][3];

	bool enable_shadows;
	bool enable_transparency;
	bool naive_mode;

protected:
	TwBar *mybar;
	TwBar *stats;

	Preview3D::KeyModifier m_key_modifier;

	bool mouse_down;
	int mouse_button;
	bool deforming;
	int from_x, from_y;
	int to_x, to_y;

	Quaternion rotation_down;
	RowVector3 translation_down;
	ScalarType scale_down;
	
	ScalarType scale;
	RowVector3 translation;
	Quaternion rotation;

	bool tmp_ha_set; RowVector3 tmp_handle;
	bool tmp_sp_set; RowVector3 tmp_support_point;
	int tmp_id_handle;

	bool display_info;
	bool display_mesh;
	float media_exponent;
	float media_density;
	float media_color[3];
	float imesh_color[4];
	float mesh_color[4];
	float plane_offset;

	int m_viewport[4];
	double m_projMatrix[16];
	double m_projSquaredMatrix[16];
	double m_cameraViewMatrix[16];
	double m_lightViewMatrix[16];
	double m_modelViewMatrix[16];
	double m_fullViewMatrix[16];

	bool optimization_in_progress;

	vector<string> objFiles;
	vector<string> cfgFiles;

};

#endif
