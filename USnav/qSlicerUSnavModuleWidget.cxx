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
#include <QDebug>

// SlicerQt includes
#include "qSlicerUSnavModuleWidget.h"
#include "ui_qSlicerUSnavModuleWidget.h"

#include "vtkSlicerUSnavLogic.h"

// STL includes
#include <set>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerUSnavModuleWidgetPrivate: public Ui_qSlicerUSnavModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerUSnavModuleWidget)
protected:
  qSlicerUSnavModuleWidget* const q_ptr;
public:
  ~qSlicerUSnavModuleWidgetPrivate();
  qSlicerUSnavModuleWidgetPrivate(qSlicerUSnavModuleWidget& object);
  vtkSlicerUSnavLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerUSnavModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerUSnavModuleWidgetPrivate::~qSlicerUSnavModuleWidgetPrivate()
{
}

qSlicerUSnavModuleWidgetPrivate::qSlicerUSnavModuleWidgetPrivate(qSlicerUSnavModuleWidget& object): q_ptr(&object)
{
}

vtkSlicerUSnavLogic* qSlicerUSnavModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerUSnavModuleWidget);
  return vtkSlicerUSnavLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerUSnavModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerUSnavModuleWidget::qSlicerUSnavModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerUSnavModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerUSnavModuleWidget::~qSlicerUSnavModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerUSnavModuleWidget::setup()
{
  Q_D(qSlicerUSnavModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
  
  connect(d->filePathLineEdit, SIGNAL(currentPathChanged(const QString&)), this, SLOT(onFileChanged(const QString&)));
  connect(d->nextPushButton, SIGNAL(clicked()), this, SLOT(onNextImage()));
  connect(d->previousPushButton, SIGNAL(clicked()), this, SLOT(onPreviousImage()));
  
  connect(d->previousValidFrameButton, SIGNAL(clicked()), this, SLOT(onPreviousValidFrame()));
  connect(d->nextValidFrameButton, SIGNAL(clicked()), this, SLOT(onNextValidFrame()));
  connect(d->previousInvalidFrameButton, SIGNAL(clicked()), this, SLOT(onPreviousInvalidFrame()));
  connect(d->nextInvalidFrameButton, SIGNAL(clicked()), this, SLOT(onNextInvalidFrame()));
  
  connect(d->frameSlider, SIGNAL(valueChanged(int)), this, SLOT(onFrameSliderChanged(int)));
  
  connect(d->MRImageNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onMrimageSelected(vtkMRMLNode*)));
  
  d->logic()->setConsole(d->consoleTextEdit);
  
  qvtkConnect(d->logic(), vtkCommand::ModifiedEvent, this, SLOT(updateState()));
}

void qSlicerUSnavModuleWidget::onFileChanged(const QString& path)
{
  Q_D(qSlicerUSnavModuleWidget);
  vtkSlicerUSnavLogic* logic = d->logic();
  logic->setMhaPath(path.toStdString());
}

void qSlicerUSnavModuleWidget::updateState()
{
  Q_D(qSlicerUSnavModuleWidget);
  vtkSlicerUSnavLogic* logic = d->logic();
  ostringstream oss;
  oss << logic->getCurrentFrame() << "/" << logic->getNumberOfFrames();
  d->currentFrameLabel->setText(oss.str().c_str());
  d->transformStatusLabel->setText(logic->getCurrentTransformStatus().c_str());
  oss.clear(); oss.str("");
  oss << logic->getImageWidth() << "x" << logic->getImageHeight();
  d->imageDimensionsLabel->setText(oss.str().c_str());
  d->frameSlider->setMaximum(logic->getNumberOfFrames());
  d->frameSlider->setValue(logic->getCurrentFrame());
  std::set<std::string> availableTransforms = logic->getAvailableTransforms();
  std::string avTransText;
  for(std::set<std::string>::iterator it=availableTransforms.begin(); it!=availableTransforms.end(); it++)
  {
    avTransText+=*it + ", ";
  }
  d->availableTransformsLabel->setText(avTransText.c_str());
}

void qSlicerUSnavModuleWidget::onMrimageSelected(vtkMRMLNode* node)
{
  Q_D(qSlicerUSnavModuleWidget);
  vtkSlicerUSnavLogic* logic = d->logic();
  vtkMRMLScalarVolumeNode* vnode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if(vnode)
    logic->setMrimageNode(vnode);
}

SLOTDEF_0(onNextImage, nextImage);
SLOTDEF_0(onPreviousImage, previousImage);
SLOTDEF_0(onPreviousValidFrame, previousValidFrame);
SLOTDEF_0(onNextValidFrame, nextValidFrame);
SLOTDEF_0(onPreviousInvalidFrame, previousInvalidFrame);
SLOTDEF_0(onNextInvalidFrame, nextInvalidFrame);
SLOTDEF_1(int, onFrameSliderChanged, goToFrame);

