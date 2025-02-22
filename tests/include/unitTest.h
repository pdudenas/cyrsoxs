//
// Created by maksbh on 4/11/21.
//

#ifndef CY_RSOXS_UNITTEST_H
#define CY_RSOXS_UNITTEST_H

#include <Input/InputData.h>
#include <Rotation.h>
#include "testUtils.h"
#ifdef _WIN32
#include <direct.h>
// MSDN recommends against using getcwd & chdir names
#define cwd _getcwd
#define cd _chdir
#else
#include "unistd.h"
#define cwd getcwd
#define cd chdir
#endif

static constexpr UINT maxERotation = 2;
static constexpr Real angleOfERotation[maxERotation]{0,20};
static const char * oracleDirName[]{"0degree","20degree"};

TEST(CyRSoXS, polarization) {
  const std::string root = CMAKE_ROOT ;
  const std::string fname = root + "/Data/sample.h5";
  const std::string configPath = root + "/Data/config/";
  if(cd(configPath.c_str()) != 0){
    throw std::runtime_error("Wrong path for config");
  }
  std::vector<Material<NUM_MATERIAL>> refractiveIndexData;
  InputData inputData;
  inputData.readRefractiveIndexData(refractiveIndexData);
  const UINT voxelSize[3]{32,32,16};
  Voxel* voxelData,*d_voxelData;
  const uint3 vx{voxelSize[0],voxelSize[1],voxelSize[2]};


  H5::readFile(fname,voxelSize,voxelData,MorphologyType::VECTOR_MORPHOLOGY,false);
  const BigUINT  numVoxels = voxelSize[0]*voxelSize[1]*voxelSize[2];
  Complex *polarizationX, *polarizationY,*polarizationZ;
  Complex *d_polarizationX, *d_polarizationY,*d_polarizationZ;

  mallocGPU(d_voxelData,numVoxels);
  mallocGPU(d_polarizationX,numVoxels);
  mallocGPU(d_polarizationY,numVoxels);
  mallocGPU(d_polarizationZ,numVoxels);
  mallocCPU(polarizationX,numVoxels);
  mallocCPU(polarizationY,numVoxels);
  mallocCPU(polarizationZ,numVoxels);

  hostDeviceExchange(d_voxelData, voxelData, numVoxels, cudaMemcpyHostToDevice);


  const Real wavelength = static_cast<Real>(1239.84197 / inputData.energies[0]);
  const Real kMagnitude = static_cast<Real>(2 * M_PI / wavelength);;
  UINT blockSize = static_cast<UINT >(ceil(numVoxels * 1.0 / NUM_THREADS));
  static constexpr Real3 kVec{0,0,1};
  Matrix rotationMatrixK,rotationMatrix;
  computeRotationMatrixK(kVec,rotationMatrixK);
  Real baseRotAngle;
  computeRotationMatrixBaseConfiguration(kVec,rotationMatrixK,rotationMatrix,baseRotAngle);

  Complex *pXOracle = new Complex[numVoxels];
  Complex *pYOracle = new Complex[numVoxels];
  Complex *pZOracle = new Complex[numVoxels];
  Matrix ERotationMatrix;
  for(int i = 0; i < maxERotation; i++) {
    Real angle = angleOfERotation[i]*180.0/M_PI;
    computeRotationMatrix(kVec,rotationMatrixK,ERotationMatrix,angle);
    computePolarization(refractiveIndexData[0], d_voxelData, vx, d_polarizationX, d_polarizationY,
                        d_polarizationZ, FFT::FFTWindowing::NONE,
                        false, MorphologyType::VECTOR_MORPHOLOGY, blockSize,ReferenceFrame::MATERIAL,rotationMatrix,numVoxels);
    hostDeviceExchange(polarizationX, d_polarizationX, numVoxels, cudaMemcpyDeviceToHost);
    hostDeviceExchange(polarizationY, d_polarizationY, numVoxels, cudaMemcpyDeviceToHost);
    hostDeviceExchange(polarizationZ, d_polarizationZ, numVoxels, cudaMemcpyDeviceToHost);

    std::string pathOfOracle = root + "/Data/regressionData/SingleAngle/"  + oracleDirName[i] +"/polarization/";
    readFile(pXOracle, pathOfOracle + "polarizeX.dmp", numVoxels);
    readFile(pYOracle, pathOfOracle + "polarizeY.dmp", numVoxels);
    readFile(pZOracle, pathOfOracle + "polarizeZ.dmp", numVoxels);
    Complex linfError;
    linfError = computeLinfError(pXOracle, polarizationX, numVoxels);
    EXPECT_LE(linfError.x, TOLERANCE_CHECK);
    EXPECT_LE(linfError.y, TOLERANCE_CHECK);
    linfError = computeLinfError(pYOracle, polarizationY, numVoxels);
    EXPECT_LE(linfError.x, TOLERANCE_CHECK);
    EXPECT_LE(linfError.y, TOLERANCE_CHECK);
    linfError = computeLinfError(pZOracle, polarizationZ, numVoxels);
    EXPECT_LE(linfError.x, TOLERANCE_CHECK);
    EXPECT_LE(linfError.y, TOLERANCE_CHECK);
  }

  freeCudaMemory(d_voxelData);
  freeCudaMemory(d_polarizationX);
  freeCudaMemory(d_polarizationY);
  freeCudaMemory(d_polarizationZ);
  delete [] pXOracle;
  delete [] pYOracle;
  delete [] pZOracle;

  delete [] voxelData;
  delete [] polarizationX;
  delete [] polarizationY;
  delete [] polarizationZ;
}

TEST(CyRSoXS, polarizationEulerAngle) {
//  const std::string root = CMAKE_ROOT ;
//  const std::string fname = root + "/Data/sample.h5";
//  const std::string configPath = root + "/Data/config/";
//  if(cd(configPath.c_str()) != 0){
//    throw std::runtime_error("Wrong path for config");
//  }
//  std::vector<Material<NUM_MATERIAL>> refractiveIndexData;
//  InputData inputData;
//  inputData.readRefractiveIndexData(refractiveIndexData);
//  const UINT voxelSize[3]{32,32,16};
//  Voxel* voxelData,*d_voxelData;
//  const uint3 vx{voxelSize[0],voxelSize[1],voxelSize[2]};
//  const BigUINT  numVoxels = voxelSize[0]*voxelSize[1]*voxelSize[2];
//
//  H5::readFile(fname,voxelSize,voxelData,MorphologyType::VECTOR_MORPHOLOGY,false);
//  for(int i = 0; i < numVoxels; i++){
//    Real   sx     = voxelData[i].s1.x;
//    Real   sy     = voxelData[i].s1.y;
//    Real   sz     = voxelData[i].s1.z;
//    Real   phi_ui = voxelData[i].s1.w;
//    Real magS      = sqrt(sx*sx + sy*sy +sz*sz);
//    voxelData[i].s1.x = magS;
//    voxelData[i].s1.y = acos(sx/magS);
//    voxelData[i].s1.z = atan(sy/sz);
//    voxelData[i].s1.w = magS*magS + phi_ui;
//
//  }
//
//  Complex *polarizationX, *polarizationY,*polarizationZ;
//  Complex *d_polarizationX, *d_polarizationY,*d_polarizationZ;
//
//  mallocGPU(d_voxelData,numVoxels);
//  mallocGPU(d_polarizationX,numVoxels);
//  mallocGPU(d_polarizationY,numVoxels);
//  mallocGPU(d_polarizationZ,numVoxels);
//  mallocCPU(polarizationX,numVoxels);
//  mallocCPU(polarizationY,numVoxels);
//  mallocCPU(polarizationZ,numVoxels);
//
//  hostDeviceExchange(d_voxelData, voxelData, numVoxels, cudaMemcpyHostToDevice);
//
//
//  const Real wavelength = static_cast<Real>(1239.84197 / inputData.energies[0]);
//  const Real kMagnitude = static_cast<Real>(2 * M_PI / wavelength);;
//  UINT blockSize = static_cast<UINT >(ceil(numVoxels * 1.0 / NUM_THREADS));
//  static constexpr Real3 kVec{0,0,1};
//  Matrix rotationMatrixK,rotationMatrix;
//  computeRotationMatrixK(kVec,rotationMatrixK);
//  Real baseRotAngle;
//  computeRotationMatrixBaseConfiguration(kVec,rotationMatrixK,rotationMatrix,baseRotAngle);
//
//  Complex *pXOracle = new Complex[numVoxels];
//  Complex *pYOracle = new Complex[numVoxels];
//  Complex *pZOracle = new Complex[numVoxels];
//  Matrix ERotationMatrix;
//  for(int i = 0; i < maxERotation; i++) {
//    Real angle = angleOfERotation[i]*180.0/M_PI;
//    computeRotationMatrix(kVec,rotationMatrixK,ERotationMatrix,angle);
//    computePolarization(refractiveIndexData[0], d_voxelData, vx, d_polarizationX, d_polarizationY,
//                        d_polarizationZ, FFT::FFTWindowing::NONE,
//                        false, MorphologyType::EULER_ANGLES, blockSize,ReferenceFrame::MATERIAL,rotationMatrix,numVoxels);
//    hostDeviceExchange(polarizationX, d_polarizationX, numVoxels, cudaMemcpyDeviceToHost);
//    hostDeviceExchange(polarizationY, d_polarizationY, numVoxels, cudaMemcpyDeviceToHost);
//    hostDeviceExchange(polarizationZ, d_polarizationZ, numVoxels, cudaMemcpyDeviceToHost);
//
//    std::string pathOfOracle = root + "/Data/regressionData/SingleAngle/"  + oracleDirName[i] +"/polarization/";
//    readFile(pXOracle, pathOfOracle + "polarizeX.dmp", numVoxels);
//    readFile(pYOracle, pathOfOracle + "polarizeY.dmp", numVoxels);
//    readFile(pZOracle, pathOfOracle + "polarizeZ.dmp", numVoxels);
//    Complex linfError;
//    linfError = computeLinfError(pXOracle, polarizationX, numVoxels);
//    EXPECT_LE(linfError.x, TOLERANCE_CHECK);
//    EXPECT_LE(linfError.y, TOLERANCE_CHECK);
//    linfError = computeLinfError(pYOracle, polarizationY, numVoxels);
//    EXPECT_LE(linfError.x, TOLERANCE_CHECK);
//    EXPECT_LE(linfError.y, TOLERANCE_CHECK);
//    linfError = computeLinfError(pZOracle, polarizationZ, numVoxels);
//    EXPECT_LE(linfError.x, TOLERANCE_CHECK);
//    EXPECT_LE(linfError.y, TOLERANCE_CHECK);
//  }
//
//  freeCudaMemory(d_voxelData);
//  freeCudaMemory(d_polarizationX);
//  freeCudaMemory(d_polarizationY);
//  freeCudaMemory(d_polarizationZ);
//  delete [] pXOracle;
//  delete [] pYOracle;
//  delete [] pZOracle;
//
//  delete [] voxelData;
//  delete [] polarizationX;
//  delete [] polarizationY;
//  delete [] polarizationZ;
}


TEST(CyRSoXS, FFT) {

  const UINT voxelSize[3]{32,32,16};
  const BigUINT  numVoxels = voxelSize[0]*voxelSize[1]*voxelSize[2];
  const std::string root = CMAKE_ROOT ;
  Complex *pX = new Complex [numVoxels];
  Complex *pY = new Complex [numVoxels];
  Complex *pZ = new Complex [numVoxels];

  Complex *d_pX, *d_pY,*d_pZ;
  mallocGPU(d_pX,numVoxels);
  mallocGPU(d_pY,numVoxels);
  mallocGPU(d_pZ,numVoxels);

  Complex *fftPX = new Complex [numVoxels];
  Complex *fftPY = new Complex [numVoxels];
  Complex *fftPZ = new Complex [numVoxels];

  static constexpr int NUM_STREAMS=3;
  cudaStream_t streams[NUM_STREAMS];
  cufftHandle plan[NUM_STREAMS];
  for (int i = 0; i < NUM_STREAMS; i++) {
    gpuErrchk(cudaStreamCreate(&streams[i]));
  }
  for(int i = 0; i < NUM_STREAMS; i++){
    cufftPlan3d(&plan[i], voxelSize[2], voxelSize[1], voxelSize[0], fftType);
    cufftSetStream(plan[i], streams[i]);
  }

  for(int i = 0 ; i < maxERotation; i++){
    const std::string pathOfpolarization = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] + "/polarization/";
    const std::string pathOfFFT = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] +"/FFT/";
    readFile(pX,pathOfpolarization+"polarizeX.dmp",numVoxels);
    readFile(pY,pathOfpolarization+"polarizeY.dmp",numVoxels);
    readFile(pZ,pathOfpolarization+"polarizeZ.dmp",numVoxels);

    hostDeviceExchange(d_pX, pX, numVoxels, cudaMemcpyHostToDevice);
    hostDeviceExchange(d_pY, pY, numVoxels, cudaMemcpyHostToDevice);
    hostDeviceExchange(d_pZ, pZ, numVoxels, cudaMemcpyHostToDevice);

    auto res1 = performFFT(d_pX,plan[0]);
    auto res2 = performFFT(d_pY,plan[1]);
    auto res3 = performFFT(d_pZ,plan[2]);

    EXPECT_EQ(res1,CUFFT_SUCCESS);
    EXPECT_EQ(res2,CUFFT_SUCCESS);
    EXPECT_EQ(res3,CUFFT_SUCCESS);

    hostDeviceExchange(pX, d_pX, numVoxels, cudaMemcpyDeviceToHost);
    hostDeviceExchange(pY, d_pY, numVoxels, cudaMemcpyDeviceToHost);
    hostDeviceExchange(pZ, d_pZ, numVoxels, cudaMemcpyDeviceToHost);
    readFile(fftPX,pathOfFFT+"fftpolarizeXbshift.dmp",numVoxels);
    readFile(fftPY,pathOfFFT+"fftpolarizeYbshift.dmp",numVoxels);
    readFile(fftPZ,pathOfFFT+"fftpolarizeZbshift.dmp",numVoxels);
    Complex linfError;
    linfError = computeLinfError(pX,fftPX,numVoxels);
    EXPECT_LE(linfError.x,TOLERANCE_CHECK);
    EXPECT_LE(linfError.y,TOLERANCE_CHECK);
    linfError = computeLinfError(pY,fftPY,numVoxels);
    EXPECT_LE(linfError.x,TOLERANCE_CHECK);
    EXPECT_LE(linfError.y,TOLERANCE_CHECK);
    linfError = computeLinfError(pZ,fftPZ,numVoxels);
    EXPECT_LE(linfError.x,TOLERANCE_CHECK);
    EXPECT_LE(linfError.y,TOLERANCE_CHECK);
    UINT blockSize = static_cast<UINT >(ceil(numVoxels * 1.0 / NUM_THREADS));
    const uint3 vx{voxelSize[0],voxelSize[1],voxelSize[2]};
    int suc1 = performFFTShift(d_pX,blockSize,vx,streams[0]);
    int suc2 = performFFTShift(d_pY,blockSize,vx,streams[1]);
    int suc3 = performFFTShift(d_pZ,blockSize,vx,streams[2]);
    EXPECT_EQ(suc1,EXIT_SUCCESS);
    EXPECT_EQ(suc2,EXIT_SUCCESS);
    EXPECT_EQ(suc3,EXIT_SUCCESS);
    hostDeviceExchange(pX, d_pX, numVoxels, cudaMemcpyDeviceToHost);
    hostDeviceExchange(pY, d_pY, numVoxels, cudaMemcpyDeviceToHost);
    hostDeviceExchange(pZ, d_pZ, numVoxels, cudaMemcpyDeviceToHost);

    readFile(fftPX,pathOfFFT+"fftpolarizeX.dmp",numVoxels);
    readFile(fftPY,pathOfFFT+"fftpolarizeY.dmp",numVoxels);
    readFile(fftPZ,pathOfFFT+"fftpolarizeZ.dmp",numVoxels);

    linfError = computeLinfError(pX,fftPX,numVoxels);
    EXPECT_LE(linfError.x,TOLERANCE_CHECK);
    EXPECT_LE(linfError.y,TOLERANCE_CHECK);
    linfError = computeLinfError(pY,fftPY,numVoxels);
    EXPECT_LE(linfError.x,TOLERANCE_CHECK);
    EXPECT_LE(linfError.y,TOLERANCE_CHECK);
    linfError = computeLinfError(pZ,fftPZ,numVoxels);
    EXPECT_LE(linfError.x,TOLERANCE_CHECK);
    EXPECT_LE(linfError.y,TOLERANCE_CHECK);


  }




  delete [] pX;
  delete [] pY;
  delete [] pZ;

  delete [] fftPX;
  delete [] fftPY;
  delete [] fftPZ;
  for(int i = 0; i < NUM_STREAMS; i++){
    cufftDestroy(plan[i]);
    gpuErrchk(cudaStreamDestroy(streams[i]))
  }

  freeCudaMemory(d_pX);
  freeCudaMemory(d_pY);
  freeCudaMemory(d_pZ);
}

TEST(CyRSoXS, scatter) {
  const UINT voxelSize[3]{32,32,16};
  uint3 vx{voxelSize[0],voxelSize[1],voxelSize[2]};
  const BigUINT numVoxel = voxelSize[0]*voxelSize[1]*voxelSize[2];

  Complex  * d_polarizationX,* d_polarizationY,* d_polarizationZ;
  Complex  * polarizationX,* polarizationY,* polarizationZ;
  Real * scatter_3D, * d_scatter3D, *scatterOracle;

  polarizationX = new Complex[numVoxel];
  polarizationY = new Complex[numVoxel];
  polarizationZ = new Complex[numVoxel];
  scatter_3D = new Real[numVoxel];
  scatterOracle = new Real[numVoxel];

  const std::string root = CMAKE_ROOT ;



  const Real energy = 280.0;

  const Real wavelength = static_cast<Real>(1239.84197 / energy);
  const Real kMagnitude = static_cast<Real>(2 * M_PI / wavelength);

  mallocGPU(d_polarizationX,numVoxel);
  mallocGPU(d_polarizationY,numVoxel);
  mallocGPU(d_polarizationZ,numVoxel);
  mallocGPU(d_scatter3D,numVoxel);
  static constexpr Real3 kVec{0,0,1};
  for(int i = 0; i < maxERotation; i++){
    const std::string pathOfFFT = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] + "/FFT/";

    readFile(polarizationX,pathOfFFT+"fftpolarizeX.dmp",numVoxel);
    readFile(polarizationY,pathOfFFT+"fftpolarizeY.dmp",numVoxel);
    readFile(polarizationZ,pathOfFFT+"fftpolarizeZ.dmp",numVoxel);

    hostDeviceExchange(d_polarizationX, polarizationX, numVoxel, cudaMemcpyHostToDevice);
    hostDeviceExchange(d_polarizationY, polarizationY, numVoxel, cudaMemcpyHostToDevice);
    hostDeviceExchange(d_polarizationZ, polarizationZ, numVoxel, cudaMemcpyHostToDevice);
    const UINT blockSize = static_cast<UINT>(ceil(numVoxel * 1.0 / NUM_THREADS));

    int suc1 = performScatter3DComputation(d_polarizationX,d_polarizationY,d_polarizationZ,d_scatter3D,kMagnitude,numVoxel,vx,5.0,false,blockSize,kVec);
    EXPECT_EQ(suc1,EXIT_SUCCESS);
    hostDeviceExchange(scatter_3D, d_scatter3D, numVoxel, cudaMemcpyDeviceToHost);

    const std::string pathOfScatter = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] + "/Scatter/";
    readFile(scatterOracle,pathOfScatter+"scatter_3D.dmp",numVoxel);
    Real linfError = computeLinfError(scatterOracle,scatter_3D,numVoxel);
    EXPECT_LE(linfError,TOLERANCE_CHECK);
  }


  freeCudaMemory(d_polarizationX);
  freeCudaMemory(d_polarizationY);
  freeCudaMemory(d_polarizationZ);
  freeCudaMemory(d_scatter3D);

  delete [] polarizationX;
  delete [] polarizationY;
  delete [] polarizationZ;
  delete [] scatter_3D;
  delete [] scatterOracle;
}

TEST(CyRSoXS, ewaldProjectionFull){
  const UINT voxelSize[3]{32,32,16};
  uint3 vx{voxelSize[0],voxelSize[1],voxelSize[2]};
  const BigUINT numVoxel = voxelSize[0]*voxelSize[1]*voxelSize[2];
  const BigUINT numVoxel2D = voxelSize[0]*voxelSize[1];

  const Real energy = 280.0;
  const Real wavelength = static_cast<Real>(1239.84197 / energy);
  const Real kMagnitude = static_cast<Real>(2 * M_PI / wavelength);

  const std::string root = CMAKE_ROOT ;
  Real * scatter = new Real [numVoxel];
  Real * projection = new Real[numVoxel2D];
  Real * projectionOracle = new Real[numVoxel2D];
  Real * d_scatter, *d_projection;

  static constexpr Real3 kVec{0,0,1};
  mallocGPU(d_scatter,numVoxel);
  mallocGPU(d_projection,numVoxel2D);

  for(int i = 0; i < maxERotation; i++){
    const std::string pathOfScatter = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] + "/Scatter/";
    const std::string pathOfEwaldGPU = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] + "/Ewald/";
    readFile(scatter,pathOfScatter+"scatter_3D.dmp",numVoxel);
    hostDeviceExchange(d_scatter, scatter, numVoxel, cudaMemcpyHostToDevice);
    const UINT blockSize =  static_cast<UINT>(ceil(numVoxel2D * 1.0 / NUM_THREADS));
    cudaZeroEntries(d_projection,numVoxel2D);
    int suc = peformEwaldProjectionGPU(d_projection,d_scatter,kMagnitude,vx,5.0,Interpolation::EwaldsInterpolation::NEARESTNEIGHBOUR,false,blockSize,kVec);
    EXPECT_EQ(suc,EXIT_SUCCESS);

    readFile(projectionOracle,pathOfEwaldGPU+"Ewald.dmp",numVoxel2D);
    hostDeviceExchange(projection, d_projection, numVoxel2D, cudaMemcpyDeviceToHost);
    Real linfError = computeLinfError(projectionOracle,projection,numVoxel2D);
    EXPECT_LE(linfError,TOLERANCE_CHECK);
  }



  freeCudaMemory(d_scatter);
  freeCudaMemory(d_projection);
  delete [] scatter;
  delete [] projection;
  delete [] projectionOracle;



}
TEST(CyRSoXS, ewaldProjectionPartial){
  const UINT voxelSize[3]{32,32,16};
  uint3 vx{voxelSize[0],voxelSize[1],voxelSize[2]};
  const BigUINT numVoxel = voxelSize[0]*voxelSize[1]*voxelSize[2];
  const BigUINT numVoxel2D = voxelSize[0]*voxelSize[1];

  const Real energy = 280.0;

  const Real wavelength = static_cast<Real>(1239.84197 / energy);
  const Real kMagnitude = static_cast<Real>(2 * M_PI / wavelength);

  const std::string root = CMAKE_ROOT ;


  Complex  * d_polarizationX,* d_polarizationY,* d_polarizationZ;
  Complex  * polarizationX,* polarizationY,* polarizationZ;
  Real *projectionOracle, *d_projection, *projection;
  static constexpr Real3 kVec{0,0,1};
  polarizationX = new Complex[numVoxel];
  polarizationY = new Complex[numVoxel];
  polarizationZ = new Complex[numVoxel];
  projection = new Real[numVoxel2D];
  projectionOracle = new Real[numVoxel2D];

  mallocGPU(d_polarizationX,numVoxel);
  mallocGPU(d_polarizationY,numVoxel);
  mallocGPU(d_polarizationZ,numVoxel);
  mallocGPU(d_projection,numVoxel2D);

  for(int i = 0; i < maxERotation; i++){
    const std::string pathOfFFT = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] + "/FFT/";
    const std::string pathOfEwaldGPU = root+"/Data/regressionData/SingleAngle/" + oracleDirName[i] + "/Ewald/";
    readFile(polarizationX,pathOfFFT+"fftpolarizeX.dmp",numVoxel);
    readFile(polarizationY,pathOfFFT+"fftpolarizeY.dmp",numVoxel);
    readFile(polarizationZ,pathOfFFT+"fftpolarizeZ.dmp",numVoxel);

    hostDeviceExchange(d_polarizationX, polarizationX, numVoxel, cudaMemcpyHostToDevice);
    hostDeviceExchange(d_polarizationY, polarizationY, numVoxel, cudaMemcpyHostToDevice);
    hostDeviceExchange(d_polarizationZ, polarizationZ, numVoxel, cudaMemcpyHostToDevice);
    cudaZeroEntries(d_projection,numVoxel2D);

    const UINT blockSize =  static_cast<UINT>(ceil(numVoxel2D * 1.0 / NUM_THREADS));
    int suc = peformEwaldProjectionGPU(d_projection,d_polarizationX,d_polarizationY,d_polarizationZ,
                                       kMagnitude,vx,5.0,Interpolation::EwaldsInterpolation::NEARESTNEIGHBOUR,false,blockSize,kVec);
    EXPECT_EQ(suc,EXIT_SUCCESS);

    hostDeviceExchange(projection, d_projection, numVoxel2D, cudaMemcpyDeviceToHost);
    readFile(projectionOracle,pathOfEwaldGPU+"Ewald.dmp",numVoxel2D);
    Real linfError = computeLinfError(projection,projectionOracle,numVoxel2D);
    EXPECT_LE(linfError,TOLERANCE_CHECK);


  }


  freeCudaMemory(d_polarizationX);
  freeCudaMemory(d_polarizationY);
  freeCudaMemory(d_polarizationZ);
  freeCudaMemory(d_projection);
  delete [] polarizationX;
  delete [] polarizationY;
  delete [] polarizationZ;
  delete [] projection;
  delete [] projectionOracle;
}

TEST(CyRSoXS, RotationBaseCase) {

  Real3 k{0, 0, 1};
  normalizeVec(k);
  Matrix rotationMatrix,rotationMatrixK;
  computeRotationMatrixK(k,rotationMatrixK);
  Real rotAngle;
  bool res = computeRotationMatrixBaseConfiguration(k, rotationMatrixK,rotationMatrix,rotAngle);
  EXPECT_EQ(res, true);
  static constexpr Real3 origK{0, 0, 1};
  Real3 shiftedK;
  doMatVec<false>(rotationMatrix, origK, shiftedK);
  EXPECT_LE(fabs(shiftedK.x - k.x), TOLERANCE_CHECK);
  EXPECT_LE(fabs(shiftedK.y - k.y), TOLERANCE_CHECK);
  EXPECT_LE(fabs(shiftedK.z - k.z), TOLERANCE_CHECK);
  static constexpr Real3 X{1, 0, 0};
  static constexpr Real3 Y{0, 1, 0};
  static constexpr Real3 Z{0, 0, 1};
  Real3 shiftedX, shiftedY, shiftedZ;
  doMatVec<false>(rotationMatrix, X, shiftedX);
  doMatVec<false>(rotationMatrix, Y, shiftedY);
  doMatVec<false>(rotationMatrix, Z, shiftedZ);
  EXPECT_LE(fabs(shiftedX.y), TOLERANCE_CHECK);
  EXPECT_EQ(FEQUALS(computeDotProduct(shiftedX, shiftedY), 0), true);
  EXPECT_EQ(FEQUALS(computeDotProduct(shiftedX, shiftedZ), 0), true);
  EXPECT_EQ(FEQUALS(computeDotProduct(shiftedY, shiftedZ), 0), true);
  EXPECT_LE(fabs(computeVecNorm(shiftedX) - 1), TOLERANCE_CHECK);
  EXPECT_LE(fabs(computeVecNorm(shiftedY) - 1), TOLERANCE_CHECK);
  EXPECT_LE(fabs(computeVecNorm(shiftedZ) - 1), TOLERANCE_CHECK);
  for(Real angle = rotAngle; angle < rotAngle + 2*M_PI; angle+= (M_PI/200.) ){
    Matrix rotationMatrixX;
    computeRotationMatrix(k,rotationMatrixK,rotationMatrixX,rotAngle);
    static constexpr Real3 Y{0,1,0};
    static constexpr Real3 Z{0,0,1};
    static constexpr Real3 X{1,0,0};
    Real3 rotatedX,rotatedY,rotatedZ,rotatedK;
    doMatVec<false>(rotationMatrixX, X, rotatedX);
    doMatVec<false>(rotationMatrixX, Y, rotatedY);
    doMatVec<false>(rotationMatrixX, Z, rotatedZ);
    doMatVec<false>(rotationMatrixX, origK, rotatedK);
    EXPECT_EQ(FEQUALS(rotatedK.x,k.x),true);
    EXPECT_LE(FEQUALS(rotatedK.y,k.y),true);
    EXPECT_LE(FEQUALS(rotatedK.z,k.z),true);
    EXPECT_EQ(FEQUALS(computeDotProduct(rotatedX,rotatedY),0),true);
    EXPECT_EQ(FEQUALS(computeDotProduct(rotatedX,rotatedZ),0),true);
    EXPECT_EQ(FEQUALS(computeDotProduct(rotatedY,rotatedZ),0),true);
  }

}


TEST(CyRSoXS,Rotation){
  srand(123458);
  for(int i = 0; i < 1000; i++) {
    Real rand1 = rand()/(RAND_MAX*1.0);
    Real rand2 = rand()/(RAND_MAX*1.0);
    Real rand3 = rand()/(RAND_MAX*1.0);
    Real3 k{rand1,rand2,rand3};
    normalizeVec(k);
    Real rotAngle;
    Matrix rotationMatrix, rotationMatrixK;
    computeRotationMatrixK(k,rotationMatrixK);
    bool res = computeRotationMatrixBaseConfiguration(k,rotationMatrixK, rotationMatrix,rotAngle);
    EXPECT_EQ(res, true);
    static constexpr Real3 origK{0,0,1};
    Real3 shiftedK;
    doMatVec<false>(rotationMatrix,origK,shiftedK);
    EXPECT_LE(fabs(shiftedK.x-k.x),TOLERANCE_CHECK);
    EXPECT_LE(fabs(shiftedK.y-k.y),TOLERANCE_CHECK);
    EXPECT_LE(fabs(shiftedK.z-k.z),TOLERANCE_CHECK);
    static constexpr Real3 X{1,0,0};
    static constexpr Real3 Y{0,1,0};
    static constexpr Real3 Z{0,0,1};
    Real3 shiftedX,shiftedY,shiftedZ;
    doMatVec<false>(rotationMatrix, X, shiftedX);
    doMatVec<false>(rotationMatrix, Y, shiftedY);
    doMatVec<false>(rotationMatrix, Z, shiftedZ);
    EXPECT_LE(fabs(shiftedX.y),TOLERANCE_CHECK);
    EXPECT_EQ(FEQUALS(computeDotProduct(shiftedX,shiftedY),0),true);
    EXPECT_EQ(FEQUALS(computeDotProduct(shiftedX,shiftedZ),0),true);
    EXPECT_EQ(FEQUALS(computeDotProduct(shiftedY,shiftedZ),0),true);
    EXPECT_LE(fabs(computeVecNorm(shiftedX) - 1),TOLERANCE_CHECK);
    EXPECT_LE(fabs(computeVecNorm(shiftedY) - 1),TOLERANCE_CHECK);
    EXPECT_LE(fabs(computeVecNorm(shiftedZ) - 1),TOLERANCE_CHECK);

    for(Real angle = rotAngle; angle < rotAngle + 2*M_PI; angle+= (M_PI/200.) ){
      Matrix rotationMatrixX;
      computeRotationMatrix(k,rotationMatrixK,rotationMatrixX,rotAngle);
      static constexpr Real3 Y{0,1,0};
      static constexpr Real3 Z{0,0,1};
      static constexpr Real3 X{1,0,0};
      Real3 rotatedX,rotatedY,rotatedZ,rotatedK;
      doMatVec<false>(rotationMatrixX, X, rotatedX);
      doMatVec<false>(rotationMatrixX, Y, rotatedY);
      doMatVec<false>(rotationMatrixX, Z, rotatedZ);
      doMatVec<false>(rotationMatrixX, origK, rotatedK);
      EXPECT_EQ(FEQUALS(rotatedK.x,k.x),true);
      EXPECT_LE(FEQUALS(rotatedK.y,k.y),true);
      EXPECT_LE(FEQUALS(rotatedK.z,k.z),true);
      EXPECT_EQ(FEQUALS(computeDotProduct(rotatedX,rotatedY),0),true);
      EXPECT_EQ(FEQUALS(computeDotProduct(rotatedX,rotatedZ),0),true);
      EXPECT_EQ(FEQUALS(computeDotProduct(rotatedY,rotatedZ),0),true);
    }
  }
}

TEST(CyRSoXS,Matrix){

  static const Real A[]{1.989280944991925e-01,2.201612876712853e-01,2.353900385650833e-01,
                        6.408539631980653e-01,8.866997439207192e-01,5.068600287362459e-01,
                        7.338419444133517e-01,2.275676457594085e-01,3.538748695968068e-01};
  static const Real B[]{5.349974950541927e-01, 2.435111987827461e-01,2.015348702948165e-02,
                        2.353224419569240e-01,5.987761391373778e-01,8.715335173739702e-01,
                        8.089931247803992e-01,5.059348436123204e-01,2.285771703331268e-01};

  static const Real AXB[]{3.486638469331048e-01,2.993605669317151e-01,2.496918251458391e-01,
                          9.615618925021381e-01,9.434279154339934e-01,9.015606198324255e-01,
                          7.324377126737106e-01,4.939984347293485e-01,2.940100212020774e-01};

  static const Real ATXB[]{8.509064395163947e-01,8.034454899806208e-01,7.302743185671113e-01,
                           5.105467472758688e-01,6.996807895938478e-01,8.292423328563572e-01,
                           5.314909572219765e-01,5.398534283700407e-01,5.273771500943050e-01};

  static const Real AXBT[]{1.647817014284501e-01,3.837898790561351e-01,3.261235163627835e-01,
                           5.689915796239037e-01,1.123487472457263e+00,1.082915377610618e+00,
                           4.551506848443609e-01,6.173653645001284e-01,7.896952053188341e-01};

  static const Real ATXBT[]{2.772706231844306e-01,1.070108157817134e+00,6.529013255502248e-01,
                            3.382933366116734e-01,7.810763718273485e-01,6.787380328675401e-01,
                            2.564909868001286e-01,6.673020595871796e-01,5.277547885573965e-01};

  Matrix matrixA(A);
  Matrix matrixB(A);
  Real res[9];
  Matrix G;
  G.performMatrixMultiplication<false,false>(A,B);
  G.getValue(res);
  EXPECT_LE(computeLinfError(AXB,res,9),TOLERANCE_CHECK);
  G.performMatrixMultiplication<true,false>(A,B);
  G.getValue(res);
  EXPECT_LE(computeLinfError(ATXB,res,9),TOLERANCE_CHECK);
  G.performMatrixMultiplication<false,true>(A,B);
  G.getValue(res);
  EXPECT_LE(computeLinfError(AXBT,res,9),TOLERANCE_CHECK);
  G.performMatrixMultiplication<true,true>(A,B);
  G.getValue(res);
  EXPECT_LE(computeLinfError(ATXBT,res,9),TOLERANCE_CHECK);
}

TEST(CyRSoXS,warpAffineMatrix){
  const double A[3][2]{{50,50},{200,50},{50,200}};
  const double B[3][2]{{10,100},{200,50},{100,250}};
  double warpAffineMatrix[2][3];
  computeWarpAffineMatrix(A,B,warpAffineMatrix);

  static const double res[] {1.2667,0.6000,-83.3333,-0.3333,1.0000,66.6667};
  EXPECT_LE(computeLinfErrorReal(res,&warpAffineMatrix[0][0],6),TOLERANCE_CHECK);

}
#endif //CY_RSOXS_UNITTEST_H
