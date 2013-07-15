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

// .NAME vtkSlicerUSnavLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerUSnavLogic_h
#define __vtkSlicerUSnavLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

// STD includes
#include <cstdlib>
#include <stdio.h>
#include <set>

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>

#include "vtkSlicerUSnavModuleLogicExport.h"

#include "util_macros.h"

using namespace std;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_USNAV_MODULE_LOGIC_EXPORT vtkSlicerUSnavLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerUSnavLogic *New();
  vtkTypeMacro(vtkSlicerUSnavLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerUSnavLogic();
  virtual ~vtkSlicerUSnavLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
private:

  vtkSlicerUSnavLogic(const vtkSlicerUSnavLogic&); // Not implemented
  void operator=(const vtkSlicerUSnavLogic&);               // Not implemented
  
  // Attributes
  string mhaPath;
  vector<vector<float> > transforms;
  vector<string> filenames;
  vector<bool> transformsValidity;
  set<string> availableTransforms;
  
  vtkSmartPointer<vtkMatrix4x4> ImageToProbeTransform;
  vtkSmartPointer<vtkImageData> imgData;
  vtkMRMLScalarVolumeNode* imageNode;
  vtkMRMLScalarVolumeNode* mrimageNode;
  unsigned char* dataPointer;
  int imageWidth;
  int imageHeight;
  int currentFrame;
  int numberOfFrames;
  
  
  // Private function
  void checkFrame();
public:
  // Read image logic
  void readImage_mha();
  
  // Getters and Setters
  GET(string, mhaPath, MhaPath);
  GET(int, imageWidth, ImageWidth);
  GET(int, imageHeight, ImageHeight);
  GET(int, currentFrame, CurrentFrame);
  GET(int, numberOfFrames, NumberOfFrames);
  GET(set<string>, availableTransforms, AvailableTransforms);
  GETSET(vtkMRMLScalarVolumeNode*, mrimageNode, MrimageNode);
  void setMhaPath(string path);
  string getCurrentTransformStatus();
  void updateImage();
  void nextImage();
  void nextValidFrame();
  void goToFrame(int);
  void previousValidFrame();
  void nextInvalidFrame();
  void previousInvalidFrame();
  void previousImage();
};

#endif
