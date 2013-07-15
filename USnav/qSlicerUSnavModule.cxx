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

// Qt includes
#include <QtPlugin>

// USnav Logic includes
#include <vtkSlicerUSnavLogic.h>

// USnav includes
#include "qSlicerUSnavModule.h"
#include "qSlicerUSnavModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerUSnavModule, qSlicerUSnavModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerUSnavModulePrivate
{
public:
  qSlicerUSnavModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerUSnavModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerUSnavModulePrivate
::qSlicerUSnavModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerUSnavModule methods

//-----------------------------------------------------------------------------
qSlicerUSnavModule
::qSlicerUSnavModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerUSnavModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerUSnavModule::~qSlicerUSnavModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerUSnavModule::helpText()const
{
  return "This is a loadable module bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerUSnavModule::acknowledgementText()const
{
  return "This work was was partially funded by NIH grant 3P41RR013218-12S1";
}

//-----------------------------------------------------------------------------
QStringList qSlicerUSnavModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Jean-Christophe Fillion-Robin (Kitware)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerUSnavModule::icon()const
{
  return QIcon(":/Icons/USnav.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerUSnavModule::categories() const
{
  return QStringList() << "Examples";
}

//-----------------------------------------------------------------------------
QStringList qSlicerUSnavModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerUSnavModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerUSnavModule
::createWidgetRepresentation()
{
  return new qSlicerUSnavModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerUSnavModule::createLogic()
{
  return vtkSlicerUSnavLogic::New();
}
