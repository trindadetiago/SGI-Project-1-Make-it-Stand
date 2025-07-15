//
//  IGL Lib - Simple C++ mesh library 
//
//  Copyright 2011, Daniele Panozzo. All rights reserved.

// History:
//  return type changed from void to bool  Alec 20 Sept 2011

#ifndef IGL_WRITESTL_H
#define IGL_WRITESTL_H

#include <Eigen/Core>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>

namespace igl 
{
  // Write a mesh in an ascii obj file
  // Inputs:
  //   str  path to outputfile
  //   V  eigen double matrix #V by 3 (mesh vertices)
  //   F  eigen int matrix #F by 3 (mesh indices)
  // Returns true on success, false on error
  template <typename DerivedV, typename DerivedF>
  inline bool writeSTLforTriMesh(
    const std::string str,
    const Eigen::PlainObjectBase<DerivedV>& V,
    const Eigen::PlainObjectBase<DerivedF>& F);

  template <typename DerivedV, typename DerivedF>
  inline bool writeSTLforQuadMesh(
    const std::string str,
    const Eigen::PlainObjectBase<DerivedV>& V,
    const Eigen::PlainObjectBase<DerivedF>& F);

}



template <typename DerivedV, typename DerivedF>
inline bool igl::writeSTLforTriMesh(
  const std::string str,
  const Eigen::PlainObjectBase<DerivedV>& V,
  const Eigen::PlainObjectBase<DerivedF>& F)
{
  std::ofstream s(str.c_str());

  if(!s.is_open())
  {
    fprintf(stderr,"IOError: writeSTL() could not open %s\n",str.c_str());
    return false;
  }

  s << "solid MIS" << std::endl;
  
  for(int i=0;i<(int)F.rows();++i)
  {
		Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> A(V(F(i,0),0),V(F(i,0),1),V(F(i,0),2));
		Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> B(V(F(i,1),0),V(F(i,1),1),V(F(i,1),2));
		Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> C(V(F(i,2),0),V(F(i,2),1),V(F(i,2),2));

		Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> N = (B-A).cross(C-A);
		N.normalize();

		s << "facet normal " << N[0] << " " << N[1] << " " << N[2] << std::endl;
		s << "   outer loop" << std::endl;
		s << "      vertex " << A[0] << " " << A[1] << " " << A[2] << std::endl;
		s << "      vertex " << B[0] << " " << B[1] << " " << B[2] << std::endl;
		s << "      vertex " << C[0] << " " << C[1] << " " << C[2] << std::endl;
		s << "   endloop" << std::endl;
		s << "endfacet" << std::endl;
  }
  
  s << "endsolid MIS" << std::endl;

  s.close();
  return true;
}


template <typename DerivedV, typename DerivedF>
inline bool igl::writeSTLforQuadMesh(
  const std::string str,
  const Eigen::PlainObjectBase<DerivedV>& V,
  const Eigen::PlainObjectBase<DerivedF>& F)
{
  std::ofstream s(str.c_str());

  if(!s.is_open())
  {
    fprintf(stderr,"IOError: writeSTL() could not open %s\n",str.c_str());
    return false;
  }

  s << "solid MIS" << std::endl;
  
  int id[2][3] = { {0,1,2}, {2,3,0} };

  for(int i=0;i<(int)F.rows();++i)
  {
	  for(int j = 0 ; j < 2 ; ++j)
	  {
		  Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> A(V(F(i,id[j][0]),0),V(F(i,id[j][0]),1),V(F(i,id[j][0]),2));
		  Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> B(V(F(i,id[j][1]),0),V(F(i,id[j][1]),1),V(F(i,id[j][1]),2));
		  Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> C(V(F(i,id[j][2]),0),V(F(i,id[j][2]),1),V(F(i,id[j][2]),2));

		  Eigen::Matrix<ScalarType,1,3,Eigen::RowMajor> N = (B-A).cross(C-A);
		  N.normalize();

		  s << "facet normal " << N[0] << " " << N[1] << " " << N[2] << std::endl;
		  s << "   outer loop" << std::endl;
		  s << "      vertex " << A[0] << " " << A[1] << " " << A[2] << std::endl;
		  s << "      vertex " << B[0] << " " << B[1] << " " << B[2] << std::endl;
		  s << "      vertex " << C[0] << " " << C[1] << " " << C[2] << std::endl;
		  s << "   endloop" << std::endl;
		  s << "endfacet" << std::endl;
	  }
  }
  
  s << "endsolid MIS" << std::endl;

  s.close();
  return true;
}

#endif
