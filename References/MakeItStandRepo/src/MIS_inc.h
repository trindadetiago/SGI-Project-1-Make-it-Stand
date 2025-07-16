#ifndef __MIS_INCLUDE_H
#define __MIS_INCLUDE_H

#include "STL_inc.h"
#include "EIGEN_inc.h"
#include "GL_inc.h"

#include <omp.h>

class Mesh;
class BoxGrid;
class Deformable;
class Handles;
class Support;
class InnerMesh;
class Optimizer;
class Config;

#undef min
#undef max

#include "utils\utils_functions.h"

const static IndexType INDEX_NULL = 999999;

#define DEFAULT_FLATTEN_THRESHOLD 0.02
#define DEFAULT_ANGLE_OBJ 5.0

#define CONFIG_DIR "data/configs/"
#define MESH_DIR "data/meshes/"
#define PRINT_DIR "data/prints/"
#define CONFIG_EXT ".mis"
#define CONFIG_EXT_LENGTH 4
#define OBJ_EXT ".obj"
#define VOX_EXT ".vox"
#define BBW_EXT ".bbw"

inline void removeExt(string & s) {
	s.erase(s.cend()-CONFIG_EXT_LENGTH,s.cend());
}

#endif