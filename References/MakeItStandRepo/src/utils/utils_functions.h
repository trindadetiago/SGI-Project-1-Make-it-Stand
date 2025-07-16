#ifndef __UTILS_FUNCTIONS_H
#define __UTILS_FUNCTIONS_H

#include "STL_inc.h"
#include "EIGEN_inc.h"
#include "utils\dirent.h"

inline void QuaternionToMatrix44(const Quaternion & q, double * mat) {
	Matrix33 m = q.toRotationMatrix().transpose();
	mat[0] = m(0,0); mat[1] = m(0,1); mat[2] = m(0,2); mat[3] = 0; 
	mat[4] = m(1,0); mat[5] = m(1,1); mat[6] = m(1,2); mat[7] = 0; 
	mat[8] = m(2,0); mat[9] = m(2,1); mat[10] = m(2,2); mat[11] = 0; 
	mat[12] = 0; mat[13] = 0; mat[14] = 0; mat[15] = 1; 
}


inline void compute_bounding_box_and_centroid(const PointMatrixType & vertices, RowVector3 & min_point, RowVector3 & max_point, RowVector3 & centroid)
{
  centroid = vertices.colwise().sum()/vertices.rows();
  min_point = vertices.colwise().minCoeff();
  max_point = vertices.colwise().maxCoeff();
}

#undef min
#undef max

inline void get_scale_and_shift_to_fit_mesh(const PointMatrixType & vertices, float& zoom, RowVector3 & shift)
{
  //Compute mesh centroid
  RowVector3 centroid;
  RowVector3 min_point;
  RowVector3 max_point;
  compute_bounding_box_and_centroid(vertices, min_point, max_point, centroid);
  
  shift = centroid;
  double x_scale = fabs(max_point[0] - min_point[0]);
  double y_scale = fabs(max_point[1] - min_point[1]);
  double z_scale = fabs(max_point[2] - min_point[2]);
  zoom = 2.7f/ (float)std::max(z_scale,std::max(x_scale,y_scale));
}

inline bool exists(const string filename) {
	std::ifstream ifile(filename);
	bool result = ifile.is_open();
	ifile.close();
	return result;
}

inline void findFiles(string dirName, string extName, vector<string> & files) {

	DIR *pxDir = opendir(dirName.c_str());
	struct dirent *pxItem = NULL;

	if(pxDir != NULL) {
		files.clear();
		while(pxItem = readdir(pxDir)) {
			string fn(pxItem->d_name);
			if(fn.find(extName) != string::npos) {
				files.push_back(fn);
			}
		}

		closedir(pxDir);
	}
	else
		cout << "Unable to open specified directory." << endl;
}

inline void transpose(float *m)
{
	for (int j=0;j<4;j++) {
	for (int i=0;i<4;i++) {
		if (i<j) { continue; }
		int k  = i+j*4;
		int t  = j+i*4;
		std::swap( m[k],m[t] );
	}
	}
}

inline string IntToString (int Number) {
     std::ostringstream ss;
     ss << Number;
     return ss.str();
}

inline void convertSparseMatrixToBuffer(const SparseMatrix & A, bool symmetric, int & AnumEl, int * & Asubi, int * & Asubj, double * & Aval) {
	AnumEl = 0;
	for (int k=0; k<A.outerSize(); ++k) {
		for (SparseMatrix::InnerIterator it(A,k); it; ++it) {
			if(!symmetric || it.row() >= it.col()) AnumEl++;
		}
	}

	Asubi = new int[AnumEl];
	Asubj = new int[AnumEl];
	Aval = new double[AnumEl];

	int id = 0;
	for (int k=0; k<A.outerSize(); ++k) {
		for (SparseMatrix::InnerIterator it(A,k); it; ++it) {
			if(!symmetric || it.row() >= it.col()) {
				Asubi[id] = it.row();
				Asubj[id] = it.col();
				Aval[id] = (double)it.value();
				id++;
			}
		}
	}
}

inline ScalarType clamp(ScalarType x, ScalarType a, ScalarType b) {
	return ((x<a)?a:((x>b)?b:x));
}

#endif