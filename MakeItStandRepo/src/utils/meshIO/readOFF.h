//
//  IGL Lib - Simple C++ mesh library 
//
//  Copyright 2011, Daniele Panozzo. All rights reserved.

// History:
//  return type changed from void to bool  Alec 18 Sept 2011

#ifndef IGL_READOFF_H
#define IGL_READOFF_H

#include <Eigen/Core>
#include <string>
#include <vector>
#include <cstdio>
#include "list_to_matrix.h"

namespace igl 
{
  // read mesh from a ascii off file
  // Inputs:
  //   str  path to .off file
  // Outputs:
  //   V  eigen double matrix #V by 3
  //   F  eigen int matrix #F by 3
  template <typename DerivedV, typename DerivedF>
  inline bool readOFF(
    const std::string str,
    Eigen::PlainObjectBase<DerivedV>& V,
    Eigen::PlainObjectBase<DerivedF>& F);

}

template <typename DerivedV, typename DerivedF>
inline bool igl::readOFF(
                             const std::string str,
                             Eigen::PlainObjectBase<DerivedV>& V,
                             Eigen::PlainObjectBase<DerivedF>& F)
{
  std::vector<std::vector<double> > vV;  
  std::vector<std::vector<int> > vF;

  FILE * off_file = fopen(str.c_str(),"r");                                       
  if(NULL==off_file)
  {
    printf("IOError: %s could not be opened...",str.c_str());
    return false; 
  }
  vV.clear();
  vF.clear();

  // First line is always OFF
  char header[1000];
  const std::string OFF("OFF");
  const std::string NOFF("NOFF");
  if(!fscanf(off_file,"%s\n",header)==1
     || !(OFF == header || NOFF == header))
  {
    printf("Error: %s's first line should be OFF or NOFF not %s...",str.c_str(),header);
    fclose(off_file);
    return false; 
  }
  bool has_normals = NOFF==header;
  // Second line is #vertices #faces #edges
  int number_of_vertices;
  int number_of_faces;
  int number_of_edges;
  char tic_tac_toe;
  char line[1000];
  bool still_comments = true;
  while(still_comments)
  {
    fgets(line,1000,off_file);
    still_comments = line[0] == '#';
  }
  sscanf(line,"%d %d %d",&number_of_vertices,&number_of_faces,&number_of_edges);
  vV.resize(number_of_vertices);
  vF.resize(number_of_faces);

  // Read vertices
  for(int i = 0;i<number_of_vertices;)
  {
    float x,y,z,nx,ny,nz;
    if((has_normals && fscanf(off_file, "%g %g %g %g %g %g\n",&x,&y,&z,&nx,&ny,&nz)==6) || 
       (!has_normals && fscanf(off_file, "%g %g %g\n",&x,&y,&z)==3))
    {
      std::vector<double > vertex;
      vertex.resize(3);
      vertex[0] = x;
      vertex[1] = y;
      vertex[2] = z;
      vV[i] = vertex;
      i++;
    }else if(
             fscanf(off_file,"%[#]",&tic_tac_toe)==1)
    {
      char comment[1000];
      fscanf(off_file,"%[^\n]",comment);
    }else
    {
      printf("Error: bad line in %s\n",str.c_str());
      fclose(off_file);
      return false;
    }
  }
  // Read faces
  for(int i = 0;i<number_of_faces;)
  {
    std::vector<int > face;
    int valence;
    if(fscanf(off_file,"%d",&valence)==1)
    {
      face.resize(valence);
      for(int j = 0;j<valence;j++)
      {
        int index;
        if(j<valence-1)
        {
          fscanf(off_file,"%d",&index);
        }else{
          fscanf(off_file,"%d%*[^\n]",&index);
        }
        
        face[j] = index;
      }
      vF[i] = face;
      i++;
    }else if(
             fscanf(off_file,"%[#]",&tic_tac_toe)==1)
    {
      char comment[1000];
      fscanf(off_file,"%[^\n]",comment);
    }else
    {
      printf("Error: bad line in %s\n",str.c_str());
      fclose(off_file);
      return false;
    }
  }
  fclose(off_file);

  bool V_rect = igl::list_to_matrix(vV,V);
  if(!V_rect)
  {
    // igl::list_to_matrix(vV,V) already printed error message to std err
    return false;
  }
  bool F_rect = igl::list_to_matrix(vF,F);
  if(!F_rect)
  {
    // igl::list_to_matrix(vF,F) already printed error message to std err
    return false;
  }
  return true;
}


#endif
