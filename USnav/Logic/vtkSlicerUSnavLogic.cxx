/*==============================================================================

Program: 3D Slicer

Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

==============================================================================*/

// USnav Logic includes
#include "vtkSlicerUSnavLogic.h"

// MRML includes

// VTK includes
#include <vtkNew.h>
#include <vtkImageImport.h>
#include <vtkTransform.h>

// STD includes
#include <cassert>
#include <cfloat>

// vnl include
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_trace.h>


// ==============================================
// Helpers - conversion functions
// ==============================================
vnl_matrix<double> convertVnlVectorToMatrix(const vnl_double_3& v)
{
  vnl_matrix<double> result(3,1);
  result(0,0) = v[0];
  result(1,0) = v[1];
  result(2,0) = v[2];
  return result;
}

vnl_double_3 convertVnlMatrixToVector(const vnl_matrix<double>& m)
{
  vnl_double_3 result;
  if(m.rows()==1 && m.cols()==3) {
    for(int i=0; i<3; i++)
      result[i] = m(0,i);
  }
  else if(m.rows()==3 && m.cols()==1) {
    for(int i=0; i<3; i++)
      result[i] = m(i,0);
  }
  return result;
}

vnl_double_3 arrayToVnlDouble(double arr[4])
{
  vnl_double_3 result;
  result[0]=arr[0];
  result[1]=arr[1];
  result[2]=arr[2];
  return result;
}

void vnlToArrayDouble(vnl_double_3 v, double arr[4])
{
  arr[0]=v[0];
  arr[1]=v[1];
  arr[2]=v[2];
  arr[3]=1.0;
}

std::vector<float> vtkToStdMatrix(vtkMatrix4x4* matrix)
{
  std::vector<float> result;
  for(int i=0; i<3; i++)
  {
    for(int j=0; j<4; j++)
      result.push_back(matrix->GetElement(i,j));
  }
  return result;
}



void vnlToVtkMatrix(const vnl_matrix<double> vnlMatrix , vtkMatrix4x4* vtkMatrix)
{
  vtkMatrix->Identity();
  int rows = vnlMatrix.rows();
  int cols = vnlMatrix.cols();
  if(rows > 4)
    rows = 4;
  if(cols > 4)
    cols = 4;
  for(int i=0; i<rows; i++)
  {
    for(int j=0; j<cols; j++)
      vtkMatrix->SetElement(i,j,vnlMatrix(i,j));
  }
}

void getVtkMatrixFromVector(const std::vector<float>& vec, vtkMatrix4x4* vtkMatrix)
{
  vtkMatrix->Identity();
  if(vec.size() < 12)
    return;
  for(int i=0; i<3; i++)
    for(int j=0; j<4; j++)
    vtkMatrix->SetElement(i,j,vec[i*4+j]);
}

vnl_matrix<double> vtkToVnlMatrix(vtkMatrix4x4* vtkMatrix)
{
  vnl_matrix<double> vnlMatrix(4,4);
  for(int i=0; i<4; i++) {
    for(int j=0; j<4; j++)
    vnlMatrix(i,j)=vtkMatrix->GetElement(i,j);
  }
  return vnlMatrix;
}

// =======================================================
// Geometry functions
// =======================================================
vnl_double_3 projectPoint(vnl_double_3 point, vnl_double_3 normalToPlane, double offset)
{
  normalToPlane.normalize();
  double dist = dot_product(point, normalToPlane) + offset;
  vnl_double_3 vec = dist*normalToPlane;
  return point - vec;
}

// ======================================================= 
// Similarity measure between two matrices
// =======================================================
double matriceDistance(vtkMatrix4x4* m1, vtkMatrix4x4* m2)
{
  vnl_matrix<double> vm1 = vtkToVnlMatrix(m1);
  vnl_matrix<double> vm2 = vtkToVnlMatrix(m2);
  vnl_matrix<double> diff = vm1 - vm2;
  double dist = vnl_trace(diff.inplace_transpose() * diff);
  return dist;
}

double orientationDistance(vtkMatrix4x4* m1, vtkMatrix4x4* m2)
{
  vnl_matrix<double> vm1 = vtkToVnlMatrix(m1);
  vnl_matrix<double> vm2 = vtkToVnlMatrix(m2);
  vnl_matrix<double> sub1 = vm1.extract(3,3);
  vnl_matrix<double> sub2 = vm1.extract(3,3);
  sub1.normalize_columns(); sub2.normalize_columns();
  vnl_matrix<double> diff = sub1 - sub2;
  double dist = vnl_trace(diff.inplace_transpose() * diff);
  return dist;
}

double pointToSliceDistance(vtkMatrix4x4* pointerTransform, vtkMatrix4x4* UStransform)
{
  vnl_matrix<double> pointerTransform_vnl = vtkToVnlMatrix(pointerTransform);
  vnl_matrix<double> UStransform_vnl = vtkToVnlMatrix(UStransform);
  vnl_matrix<double> wm = UStransform_vnl.extract(3,1,0,2);
  vnl_matrix<double> tm = UStransform_vnl.extract(3,1,0,3);
  vnl_matrix<double> pm = pointerTransform_vnl.extract(3,1,0,3);
  
  // Vector normal to the US image
  vnl_double_3 w = convertVnlMatrixToVector(wm);
  // Translation vector (a point of the plane)
  vnl_double_3 t = convertVnlMatrixToVector(tm);
  // Position of the pointer tip
  vnl_double_3 p = convertVnlMatrixToVector(pm);
  
  w.normalize();
  double d = -dot_product(t,w);
  vnl_double_3 p_proj = projectPoint(p,w,d);
  double dist = (p-p_proj).two_norm();
  return dist;
}


// =======================================================
// Reading functions
// =======================================================
std::string getDir(const std::string& filename)
{
  #ifdef WIN32
  const char dlmtr = '\\';
  #else
  const char dlmtr = '/';
  #endif

  std::string dirName;
  size_t pos = filename.rfind(dlmtr);
  dirName = pos == string::npos ? "" : filename.substr(0, pos) + dlmtr;
  return dirName;
}


void readTrainFilenames( const string& filename, string& dirName, vector<string>& trainFilenames )
{

  trainFilenames.clear();

  ifstream file( filename.c_str() );
  if ( !file.is_open() )
    return;

  dirName = getDir(filename);
  while( !file.eof() )
  {
    string str; getline( file, str );
    if( str.empty() ) break;
    trainFilenames.push_back(str);
  }
  file.close();
}

int readImageDimensions_mha(const std::string& filename, int& cols, int& rows, int& count)
{
  ifstream file( filename.c_str() );
  if ( !file.is_open() )
    return 1;

  // Read until get dimensions
  while( !file.eof() )
  {
    string str; getline( file, str );
    if( str.empty() ) break;
    char *pch = &(str[0]);
    if( !pch )
    {
      return 1;
      file.close();
    }

    if( strstr( pch, "DimSize =" ) )
    {
      if( sscanf( pch, "DimSize = %d %d %d", &cols, &rows, &count ) != 3 )
      {
        printf( "Error: could not read dimensions\n" );
        file.close();
        return 1;
      }
      file.close();
      return 0;
    }
  }
  file.close();
  return 1;
}

void readImageTransforms_mha(const std::string& filename, std::vector<std::vector<float> >& transforms, set<string>& availableTransforms,  std::vector<bool>& transformsValidity, std::vector<std::string>& filenames)
{
  std::string dirName = getDir(filename);
  filenames.clear();
  transforms.clear();

  // Vector for reading in transforms
  vector< float > vfTrans;
  vfTrans.resize(12);
  std::string pngFilename;

  ifstream file( filename.c_str() );
  if ( !file.is_open() )
    return;

  while( !file.eof() )
  {
    string str; getline( file, str );
    if( str.empty() ) break;
    char *pch = &(str[0]);
    if( !pch )
      return;
      
    if(strstr(pch, "Seq_Frame") && strstr(pch, "Transform")){
      char* transformName = strstr(pch, "Seq_Frame")+string("Seq_Frame").size()+5;
      char* endTransformName = strstr(pch, "Transform");
      std::string transformNameCpy = string(transformName, endTransformName-transformName);
      availableTransforms.insert(transformNameCpy);
    }

    if( strstr( pch, "ProbeToTrackerTransform =" )
      || strstr( pch, "UltrasoundToTrackerTransform =" ) )
    {
       // Parse name and transform
       // Seq_Frame0000_ProbeToTrackerTransform = -0.224009 -0.529064 0.818481 212.75 0.52031 0.6452 0.559459 -14.0417 -0.824074 0.551188 0.130746 -26.1193 0 0 0 1 

      char *pcName = pch;
      char *pcTrans = strstr( pch, "=" );
      pcTrans[-1] = 0; // End file name string pcName
       //pcTrans++; // Increment to just after equal sign

      pngFilename = dirName + pcName + ".png";// + pcTrans;

      char *pch = pcTrans;

      for( int j =0; j < 12; j++ )
      {
        pch = strchr( pch + 1, ' ' );
        if( !pch )
          return;
        vfTrans[j] = atof( pch );
        pch++;
      }
      transforms.push_back( vfTrans );
      filenames.push_back(pngFilename);
    }
    else if(strstr(pch, "UltrasoundToTrackerTransformStatus") || strstr(pch, "ProbeToTrackerTransformStatus")) {
      if(strstr(pch, "OK")){
        transformsValidity.push_back(true);
      }
      else if(strstr(pch, "INVALID"))
        transformsValidity.push_back(false);
    }
    if( strstr( pch, "ElementDataFile = LOCAL" ) )
    {
       // Done reading
      break;
    }
  }
  for(set<string>::iterator it=availableTransforms.begin(); it!=availableTransforms.end(); it++)
    cout << *it << endl;
}

void vtkSlicerUSnavLogic::readImage_mha()
{

  FILE *infile = fopen( this->mhaPath.c_str(), "rb" );
  char buffer[400];
  
  // Just move the pointer where data starts
  while( fgets( buffer, 400, infile ) )
  {
    if( strstr( buffer, "ElementDataFile = LOCAL" ) )
    {
       // Done reading
      break;
    }
  } 

  #ifdef WIN32
  _fseeki64(infile, (__int64)this->imageHeight*(__int64)this->imageWidth*(__int64)this->currentFrame, SEEK_CUR);
  #else
  fseek(infile, (long int)this->imageHeight*(long int)this->imageWidth*(long int)this->currentFrame, SEEK_CUR);
  #endif

  fread( this->dataPointer, 1, this->imageHeight*this->imageWidth, infile );
  fclose( infile );
}



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerUSnavLogic);

//----------------------------------------------------------------------------
vtkSlicerUSnavLogic::vtkSlicerUSnavLogic()
{
  this->imgData = NULL;
  this->dataPointer = NULL;
  this->imageNode = vtkMRMLScalarVolumeNode::New();
  this->imageNode->SetName("mha image");
  this->mrimageNode = NULL;
  this->stylusTransform = NULL;
  this->imageWidth = 0;
  this->imageHeight = 0;
  this->numberOfFrames = 0;
  
  // Initialize Image to Probe transform
  this->ImageToProbeTransform = vtkSmartPointer<vtkMatrix4x4>::New();
  this->ImageToProbeTransform->Identity();
  this->ImageToProbeTransform->SetElement(0,0,0.107535);
  this->ImageToProbeTransform->SetElement(0,1,0.00094824);
  this->ImageToProbeTransform->SetElement(0,2,0.0044213);
  this->ImageToProbeTransform->SetElement(0,3,-65.9013);
  this->ImageToProbeTransform->SetElement(1,0,0.0044901);
  this->ImageToProbeTransform->SetElement(1,1,-0.00238041);
  this->ImageToProbeTransform->SetElement(1,2,-0.106347);
  this->ImageToProbeTransform->SetElement(1,3,-3.05698);
  this->ImageToProbeTransform->SetElement(2,0,-0.000844189);
  this->ImageToProbeTransform->SetElement(2,1,0.105271);
  this->ImageToProbeTransform->SetElement(2,2,-0.00244457);
  this->ImageToProbeTransform->SetElement(2,3,-17.1613);
}

//----------------------------------------------------------------------------
vtkSlicerUSnavLogic::~vtkSlicerUSnavLogic()
{
  if(this->dataPointer)
    delete [] this->dataPointer;
}

// =======================================================
// Core inherited functions reimplemented
// =======================================================

//----------------------------------------------------------------------------
void vtkSlicerUSnavLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerUSnavLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

void vtkSlicerUSnavLogic::ProcessMRMLNodesEvents( vtkObject* caller, unsigned long event, void * callData )
{
  if ( caller == NULL )
  {
    return;
  }
  if(event == vtkMRMLTransformableNode::TransformModifiedEvent)
  {
    vtkMRMLLinearTransformNode* tnode = vtkMRMLLinearTransformNode::SafeDownCast( caller );
    if(tnode) {
      this->console->insertPlainText("Transform Node Modified\n");
      this->findMatchingUS(tnode->GetMatrixTransformToParent());
      return;
    }
  }
  this->Superclass::ProcessMRMLNodesEvents( caller, event, callData );
}

void vtkSlicerUSnavLogic::setStylusTransform(vtkMRMLLinearTransformNode *tnode)
{
  if(tnode==this->stylusTransform)
    return;
  if(!tnode){
    this->removeStylusTransform();
    return;
  }

  int wasModifying = this->StartModify();
  if(this->stylusTransform)
    vtkSetAndObserveMRMLNodeMacro( this->stylusTransform, 0 );

  vtkMRMLLinearTransformNode* newNode = NULL;
  vtkSmartPointer< vtkIntArray > events = vtkSmartPointer< vtkIntArray >::New();
  //events->InsertNextValue( vtkCommand::ModifiedEvent );
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkSetAndObserveMRMLNodeEventsMacro( newNode, tnode, events );
  this->stylusTransform = newNode;
  this->EndModify( wasModifying );
}

void vtkSlicerUSnavLogic::removeStylusTransform()
{
  if(this->stylusTransform)
    vtkSetAndObserveMRMLNodeMacro( this->stylusTransform, 0 );
  this->stylusTransform = NULL;
}

//-----------------------------------------------------------------------------
void vtkSlicerUSnavLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerUSnavLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerUSnavLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerUSnavLogic
  ::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

void vtkSlicerUSnavLogic::setMhaPath(string path)
{
  if(path != this->mhaPath){
    this->mhaPath = path;
    this->transforms.clear();
    this->filenames.clear();
    this->transformsValidity.clear();
    this->currentFrame = 0;
    int iImgCols = -1;
    int iImgRows = -1;
    int iImgCount = -1;
    if(readImageDimensions_mha(this->mhaPath, iImgCols, iImgRows, iImgCount))
      return;
    this->imageWidth = iImgCols;
    this->imageHeight = iImgRows;
    this->numberOfFrames = iImgCount;
    if(this->dataPointer)
      delete [] this->dataPointer;
    this->dataPointer = new unsigned char[iImgRows*iImgCols];
    readImageTransforms_mha(this->mhaPath, this->transforms, this->availableTransforms, this->transformsValidity, this->filenames);
    this->updateImage();
    this->Modified();
  }
}


string vtkSlicerUSnavLogic::getCurrentTransformStatus()
{
  if(this->transformsValidity[this->currentFrame])
    return "OK";
  else
    return "INVALID";
}

void vtkSlicerUSnavLogic::updateImage()
{
  checkFrame();
  readImage_mha();
  vtkSmartPointer<vtkImageImport> importer = vtkSmartPointer<vtkImageImport>::New();
  importer->SetDataScalarTypeToUnsignedChar();
  importer->SetImportVoidPointer(dataPointer,1); // Save argument to 1 won't destroy the pointer when importer destroyed
  importer->SetWholeExtent(0,this->imageWidth-1,0, this->imageHeight-1, 0, 0);
  importer->SetDataExtentToWholeExtent();
  importer->Update();
  this->imgData = importer->GetOutput();

  if(this->transforms.size() > 0)
  {
    vtkSmartPointer<vtkMatrix4x4> transform = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkTransform> combinedTransform = vtkSmartPointer<vtkTransform>::New();
    getVtkMatrixFromVector(this->transforms[this->currentFrame], transform);
    combinedTransform->Concatenate(transform);
    combinedTransform->Concatenate(this->ImageToProbeTransform);
    vtkSmartPointer<vtkMatrix4x4> matrix = combinedTransform->GetMatrix();
    this->imageNode->SetIJKToRASMatrix(matrix);
  }
  
  this->imageNode->SetAndObserveImageData(this->imgData);




  if(this->GetMRMLScene()) {
    if(!this->GetMRMLScene()->IsNodePresent(this->imageNode))
      this->GetMRMLScene()->AddNode(this->imageNode);
  }
}

// =======================================================
// Interface for navigating through frames
// =======================================================
void vtkSlicerUSnavLogic::nextImage()
{
  this->currentFrame += 1;
  this->updateImage();
  this->Modified();
}

void vtkSlicerUSnavLogic::previousImage()
{
  this->currentFrame -= 1;
  this->updateImage();
  this->Modified();
}

void vtkSlicerUSnavLogic::goToFrame(int frame)
{
  this->currentFrame = frame;
  this->updateImage();
  this->Modified();
}

void vtkSlicerUSnavLogic::nextValidFrame()
{
  int frame = 0;
  for(int i=0; i<this->getNumberOfFrames(); i++)
  {
    frame = (this->currentFrame + i + 1)%this->getNumberOfFrames();
    if(this->transformsValidity[frame])
      break;
  }
  this->currentFrame = frame;
  this->updateImage();
  this->Modified();
}

void vtkSlicerUSnavLogic::previousValidFrame()
{
  int frame = 0;
  for(int i=0; i<this->getNumberOfFrames(); i++)
  {
    frame = this->currentFrame-i-1;
    if(frame < 0)
      frame = this->getNumberOfFrames() + frame;
    if(this->transformsValidity[frame])
      break;
  }
  this->currentFrame = frame;
  this->updateImage();
  this->Modified();
}

void vtkSlicerUSnavLogic::nextInvalidFrame()
{
  int frame = 0;
  for(int i=0; i<this->getNumberOfFrames(); i++)
  {
    frame = (this->currentFrame + i + 1)%this->getNumberOfFrames();
    if(!this->transformsValidity[frame])
      break;
  }
  this->currentFrame = frame;
  this->updateImage();
  this->Modified();
}

void vtkSlicerUSnavLogic::previousInvalidFrame()
{
  int frame = 0;
  for(int i=0; i<this->getNumberOfFrames(); i++)
  {
    frame = this->currentFrame-i-1;
    if(frame < 0)
      frame = this->getNumberOfFrames() + frame;
    if(!this->transformsValidity[frame])
      break;
  }
  this->currentFrame = frame;
  this->updateImage();
  this->Modified();
}

void vtkSlicerUSnavLogic::checkFrame()
{
  if(this->currentFrame >= this->numberOfFrames)
    this->currentFrame = 0;
  if(this->currentFrame < 0)
    this->currentFrame = this->getNumberOfFrames()-1;
}

bool pairlt(const pair<double,int>& p1, const pair<double,int>& p2) {
  return p1.first < p2.first;
}

void vtkSlicerUSnavLogic::findMatchingUS(vtkMatrix4x4* stylusMatrix)
{
  vector<pair<double,int> > distToSlice;
  vector<double> orientationDist;
  for(int i=0; i<this->transforms.size(); i++)
  {
    if(!this->transformsValidity[i]){
      distToSlice.push_back(pair<double,int>(DBL_MAX,i));
      orientationDist.push_back(DBL_MAX);
      continue;
    }
    vtkSmartPointer<vtkMatrix4x4> t = vtkSmartPointer<vtkMatrix4x4>::New();
    getVtkMatrixFromVector(transforms[i], t);
    distToSlice.push_back(pair<double,int>(pointToSliceDistance(stylusMatrix, t),i));
    orientationDist.push_back(orientationDistance(stylusMatrix, t));
  }
  vector<pair<double,int> > sortedDistToSlice = distToSlice;
  sort(sortedDistToSlice.begin(),sortedDistToSlice.end(),pairlt);
}
