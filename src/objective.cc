/*---------------------------------------------------------------
 * objective.cc  
 * compute Torque FOM
 * for a given BEM matrix, OPFT matrix, frequency, and PARMMatrix
 * created 2015.03
 * last updated on 2015.05.19
 *--------------------------------------------------------------*/
#include <stdio.h>
#include <math.h>
#include <complex>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <libhrutil.h>
#include <libscuff.h>
#include "VBeam.h" 

#ifndef cdouble
  typedef std::complex<double> cdouble;
#endif

#define II cdouble(0.0,1.0)
using namespace scuff;
namespace scuff{
  //#define NUMPFT 8
  void GetOPFTMatrices(RWGGeometry *G, int SurfaceIndex, cdouble Omega,
		  HMatrix *QPFT[NUMPFT], bool NeedMatrix[NUMPFT]);
}
void ShowPARMMatirx(HMatrix* PARMMatrix); 
void ShowPARMMatirx(int numL, HMatrix* PARMMatrix);
double objective(RWGGeometry *G, HMatrix *PARMMatrix,
		 HMatrix *M, HMatrix *Q); 
//---------------------------------------------------------------//
//---------------------------------------------------------------//
///LEFTOVER TASKS
//---------------------------------------------------------------//
// Import the Q matrix. 
// then compute C_adj by doing LUSolve(M, QbarCbar) 
//---------------------------------------------------------------//

int main(int argc, char *argv[])
  /// Objective 
  /// 
  /// READ In:
  ///    parameter matrix PARMMatrix 
  ///    BEM matrix A, LU Factorized
  ///    PFT matrix Q 
{
  /*--------------------------------------------------------------*/
  /*- process options  -------------------------------------------*/
  /*--------------------------------------------------------------*/
  /// Input arguments should be filenames. (pointers) 
  char *GeoFile=0; 
  char *PARMMatrixFile=0;
  char *BEMMatrixFile=0;
  char *PFTMatrixFile=0;
  char *LogLevel=0;

  /* name    type   #args  max_instances  storage  count  description*/
  OptStruct OSArray[]=
   { 
     {"geometry",   PA_STRING,  1, 1, (void *)&GeoFile,   0,  ".scuffgeo file"},
     {"PARMMatrix", PA_STRING,  1, 1, (void *)&PARMMatrixFile, 0, "VParameters file"},
     {"BEMMatrix",  PA_STRING,  1, 1, (void *)&BEMMatrixFile, 0,  "BEM Matrix datafile"},
     {"PFTMatrix",  PA_STRING,  1, 1, (void *)&PFTMatrixFile, 0,  "PFT Matrix datafile"},
     {"LogLevel",   PA_STRING,  1, 1, (void *)&LogLevel,   0, "none | terse | verbose | verbose2"},
     //
     {0,0,0,0,0,0,0}
   };
  ProcessOptions(argc, argv, OSArray);

  if (GeoFile==0)
   OSUsage(argv[0],OSArray,"--geometry option is mandatory");
  if (PARMMatrixFile==0)
   OSUsage(argv[0],OSArray,"--PARMMatrix option is mandatory");
  if (BEMMatrixFile==0)
   OSUsage(argv[0],OSArray,"--BEMMatrix option is mandatory");

  RWGGeometry *G = new RWGGeometry(GeoFile);
  HMatrix *PARMMatrix = new HMatrix(PARMMatrixFile, LHM_TEXT);
  ShowPARMMatirx(PARMMatrix); 
  HMatrix *M = new HMatrix(BEMMatrixFile, LHM_TEXT);
  //A->LUFactorize(); //(already done)
  HMatrix *Q = new HMatrix(PFTMatrixFile, LHM_TEXT);
  objective(G, PARMMatrix, M, Q); 

  delete G, PARMMatrix, M, Q; 
}
/***************************************************************/
/***************************************************************/
/***************************************************************/
double objective(RWGGeometry *G, HMatrix *PARMMatrix, HMatrix *M, HMatrix *Q)
{
  printf("OBJECTIVE FUNCTION IS CALLED.\n"); 
  /// Input:
  ///    parameter matrix PARMMatrix 
  ///    BEM matrix M, LU Factorized
  ///    PFT matrix Q 
  /// COMPUTE:
  ///    RHS (sum aM+bN, GetRHSVector )
  ///    KN=M\RHS (Surface current vector KN) 
  /// 

  //--------------------------------------------------------------------//
  /// Simulate for single frequency                                  
  //--------------------------------------------------------------------//
  cdouble Omega= 10.471975511965978;
  double wvnm= 2.0*M_PI*1000.0/std::real(Omega);

  double FOM =0.0; ///will be the output. 
  HVector *KN = G->AllocateRHSVector();     
  HVector *RHS= G->AllocateRHSVector();     
  //--------------------------------------------------------------------//
  /// Assemble and compute RHS 
  //--------------------------------------------------------------------//
  VBeam *VBInit = 0; //new GHBeam(HM,HN); 
  VBInit=new VBeam(PARMMatrix); 
  printf("VBInit properties\n");  
  printf("VBInit->numL=%d\n",VBInit->numL); 
  ShowPARMMatirx(VBInit->numL, VBInit->PMatrix); 
  /// set up the incident field profile and assemble the RHS vector 
  //--------------------------------------------------------------------//
  IncField *IF=0; /// Assemble IF from PARMMatrix
  IF=VBInit; ///
  printf("IF is constructed\n");  
  // double *kbloch; 
  // double kblochval = 0.0; 
  // kbloch = &kblochval; 
  G->AssembleRHSVector(Omega, IF, RHS); //KN and RHS are formed
  KN->Copy(RHS); // this is the RHS vector assembled. 
  M->LUSolve(KN);// solved KN. 

  // Need Qmatrix and Cadj. this was already done in MatrixFOM old version. 
  // you can use it again 
  /*------------------------------------------------------------*/
  /*------------------------------------------------------------*/
  /*------------------------------------------------------------*/
  PFTOptions *MyPFTOptions=InitPFTOptions();
  HMatrix *QPFT[8]={0,0,0,0,0,0,0,0};
  printf(" Getting PFT matrix Q...\n");
  bool NeedMatrix[8]={false, false, false, false, false, false, false, false};

  NeedMatrix[SCUFF_PABS]=true; //which matrices are needed. 
  //NeedMatrix[SCUFF_PSCA]=true;
  NeedMatrix[SCUFF_XFORCE]=true;
  //NeedMatrix[SCUFF_ZFORCE]=true;
  NeedMatrix[SCUFF_ZTORQUE]=true;

  GetOPFTMatrices(G, 0, Omega, QPFT, NeedMatrix);
  delete VBInit; 
  return FOM; 
}//end main 
//--------------------------------------------------------------------//
//--------------------------------------------------------------------//
//--------------------------------------------------------------------//
void ShowPARMMatirx(HMatrix* PARMMatrix)
{
  int iL, L; 
  double aIn, aL, bL; 
  printf("PARMMatrix: \n"); 
  printf("L \t aIn \t\t aL \t\t bL\n"); 
  for(iL=0; iL<PARMMatrix->NR; iL++) //for each row of PARMMatrix
    {
      L   =PARMMatrix->GetEntryD(iL,0);
      aIn =PARMMatrix->GetEntryD(iL,1);
      aL  =PARMMatrix->GetEntryD(iL,2);
      bL  =PARMMatrix->GetEntryD(iL,3);
      printf("%d \t %f \t %f \t %f\n",L,aIn,aL,bL);
  }   
}
//--------------------------------------------------------------------//
void ShowPARMMatirx(int numL, HMatrix* PARMMatrix)
{//length of PARMMatrix specified 
  int iL, L; 
  double aIn, aL, bL; 
  printf("PARMMatrix: \n"); 
  printf("L \t aIn \t\t aL \t\t bL\n"); 
  for(iL=0; iL<numL; iL++) //for each row of PARMMatrix
    {
      L   =PARMMatrix->GetEntryD(iL,0);
      aIn =PARMMatrix->GetEntryD(iL,1);
      aL  =PARMMatrix->GetEntryD(iL,2);
      bL  =PARMMatrix->GetEntryD(iL,3);
      printf("%d \t %f \t %f \t %f\n",L,aIn,aL,bL);
  }   
}
// //--------------------------------------------------------------------//
//   //    Step 1: create an SSData struct
//   //    Step 2: for SSData, define or compute: G, Omega, RHS, IF.
//   //    Step 3: run SSD->GetFields 
//   // typedef struct SSData
//   // { RWGGeometry * G ;
//   // HMatrix * M ;
//   // HVector * RHS , * KN ;
//   // cdouble Omega ;
//   // double * kBloch ;
//   // IncField * IF ;
//   // double PowerRadius ;
//   // } SSData ;

//   //--------------------------------------------------------------//
//   //- process options  -------------------------------------------//
//   //--------------------------------------------------------------//
//   char *GeoFile=0, *PARMMatrixFile=0, *MeshFileName=0; 
//   cdouble OmegaVals[1];        int nOmegaVals;
//   OptStruct OSArray[]=
//    { /* name    type   #args  max_instances  storage  count  description*/
//      {"geometry",   PA_STRING,  1, 1, (void *)&GeoFile,   0,  ".scuffgeo file"},
//      {"Omega",      PA_CDOUBLE, 1, MAXFREQ, (void *)OmegaVals, &nOmegaVals,  "(angular) frequency"},
//      {0,0,0,0,0,0,0} };
//   ProcessOptions(argc, argv, OSArray);
//   //--------------------------------------------------------------//
//   if (GeoFile==0)
//    OSUsage(argv[0],OSArray,"--geometry option is mandatory");
//   if (nOmegaVals==0)
//     OSUsage(argv[0], OSArray, "you must specify at least one frequency");
//   //--------------------------------------------------------------//
//   char PARMMatrixFile[100],PPFile[100];
//   snprintf(PARMMatrixFile,MAXSTR,"VParameters");  
//   HMatrix *PARMMatrix = new HMatrix(PARMMatrixFile, LHM_TEXT);
//   //    Step 1: create an SSData struct
//   SSData MySSDforTestMesh, *SSD = &MySSDforTestMesh; 
//   //    Step 2: for SSData, define or compute: G, Omega, RHS, IF.
//   RWGGeometry *G = SSD->G = new RWGGeometry(GeoFile);
//   SSD->RHS = SSD->G->AllocateRHSVector();  
//   cdouble Omega = SSD->Omega = OmegaVals[1]; 
//   VBeam *VB = new VBEam(PARMMatrix); 
//   IncField *IF = SSD->IF = VB; 
