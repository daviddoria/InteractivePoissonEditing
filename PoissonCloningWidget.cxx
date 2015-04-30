/*=========================================================================
 *
 *  Copyright David Doria 2012 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "PoissonCloningWidget.h"

// Custom
#include "ImageFileSelector.h"

// Submodules
#include "Helpers/Helpers.h"
#include "QtHelpers/QtHelpers.h"
#include "ITKQtHelpers/ITKQtHelpers.h"
#include "ITKHelpers/ITKHelpers.h"
#include "Mask/Mask.h"
#include "Mask/MaskQt.h"
#include "PoissonEditing/PoissonEditing.h"
#include "PoissonEditing/PoissonEditingWrappers.h"

// ITK
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkPasteImageFilter.h"

// Qt
#include <QIcon>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QtConcurrentRun>

PoissonCloningWidget::PoissonCloningWidget(const std::string& sourceImageFileName,
                                           const std::string& maskFileName,
                                           const std::string& targetImageFileName) : PoissonCloningWidget()
{
  this->SourceImageFileName = sourceImageFileName;
  this->MaskImageFileName = maskFileName;
  this->TargetImageFileName = targetImageFileName;

  OpenImages(this->SourceImageFileName, this->TargetImageFileName, this->MaskImageFileName);
}

PoissonCloningWidget::PoissonCloningWidget() : QMainWindow()
{
  this->setupUi(this);

  this->ProgressDialog = new QProgressDialog();
  this->ProgressDialog->setMinimum(0);
  this->ProgressDialog->setMaximum(0);
  this->ProgressDialog->setWindowModality(Qt::WindowModal);

  connect(&this->FutureWatcher, SIGNAL(finished()), this, SLOT(slot_finished()));
  connect(&this->FutureWatcher, SIGNAL(finished()), this->ProgressDialog , SLOT(cancel()));
  
  this->SourceImage = ImageType::New();
  this->TargetImage = ImageType::New();
  this->MaskImage = Mask::New();
  this->ResultImage = ImageType::New();

  this->InputScene = new QGraphicsScene;
  this->graphicsViewInputImage->setScene(this->InputScene);

  this->ResultScene = new QGraphicsScene;
  this->graphicsViewResultImage->setScene(this->ResultScene);
}

void PoissonCloningWidget::showEvent(QShowEvent* )
{
  if(SourceImagePixmapItem && TargetImagePixmapItem)
  {
    this->graphicsViewInputImage->fitInView(this->TargetImagePixmapItem,
                                            Qt::KeepAspectRatio);
    this->graphicsViewResultImage->fitInView(this->TargetImagePixmapItem,
                                            Qt::KeepAspectRatio);
  }
}

void PoissonCloningWidget::resizeEvent(QResizeEvent* )
{
  if(this->SourceImagePixmapItem && this->TargetImagePixmapItem)
  {
    this->graphicsViewInputImage->fitInView(this->TargetImagePixmapItem,
                                            Qt::KeepAspectRatio);
    this->graphicsViewResultImage->fitInView(this->TargetImagePixmapItem,
                                            Qt::KeepAspectRatio);
  }
}
  
void PoissonCloningWidget::OpenImages(const std::string& sourceImageFileName,
                                      const std::string& targetImageFileName,
                                      const std::string& maskFileName)
{
  // Load the mask
  this->MaskImage->Read(maskFileName);

  // Load and display source image
  typedef itk::ImageFileReader<ImageType> ImageReaderType;
  ImageReaderType::Pointer sourceImageReader =
      ImageReaderType::New();
  sourceImageReader->SetFileName(sourceImageFileName);
  sourceImageReader->Update();

  ITKHelpers::DeepCopy(sourceImageReader->GetOutput(),
                       this->SourceImage.GetPointer());

  QImage qimageSourceImage =
//      ITKQtHelpers::GetQImageColor(this->SourceImage.GetPointer(), QImage::Format_RGB888);
      ITKQtHelpers::GetQImageColor(this->SourceImage.GetPointer(),
                                   QImage::Format_ARGB32);
  qimageSourceImage =
      MaskQt::SetPixelsToTransparent(qimageSourceImage,
                                     this->MaskImage, HoleMaskPixelTypeEnum::VALID);
  this->SourceImagePixmapItem =
    this->InputScene->addPixmap(QPixmap::fromImage(qimageSourceImage));
  this->SourceImagePixmapItem->setFlag(QGraphicsItem::ItemIsMovable);

  // Load and display target image
  ImageReaderType::Pointer targetImageReader = ImageReaderType::New();
  targetImageReader->SetFileName(targetImageFileName);
  targetImageReader->Update();

  ITKHelpers::DeepCopy(targetImageReader->GetOutput(),
                       this->TargetImage.GetPointer());

  QImage qimageTargetImage =
      ITKQtHelpers::GetQImageColor(this->TargetImage.GetPointer(),
                                   QImage::Format_RGB888);
  this->TargetImagePixmapItem =
      this->InputScene->addPixmap(QPixmap::fromImage(qimageTargetImage));
  this->InputScene->setSceneRect(qimageTargetImage.rect());

  // Size the result image
  this->ResultScene->setSceneRect(qimageTargetImage.rect());

  this->SourceImagePixmapItem->setZValue(this->TargetImagePixmapItem->zValue()+1); // make sure the source image is on top of the target image
}

void PoissonCloningWidget::on_btnClone_clicked()
{
  // Extract the portion of the target image the user has selected.
  
  this->SelectedRegionCorner[0] = this->SourceImagePixmapItem->pos().x();
  this->SelectedRegionCorner[1] = this->SourceImagePixmapItem->pos().y();
  
  ImageType::RegionType desiredRegion(this->SelectedRegionCorner,
                                      this->SourceImage->GetLargestPossibleRegion().GetSize());

  std::vector<PoissonEditingParent::GuidanceFieldType::Pointer> guidanceFields =
      PoissonEditingParent::ComputeGuidanceField(this->SourceImage.GetPointer());

  ITKHelpers::WriteImage(this->SourceImage.GetPointer(), "source.mha");
  ITKHelpers::WriteImage(guidanceFields[0].GetPointer(), "guidanceField.mha");

  // We must get a function pointer to the overload that would be chosen by the compiler
  // to pass to run().

  void (*functionPointer)(const std::remove_pointer<decltype(this->TargetImage.GetPointer())>::type*,
                          const std::remove_pointer<decltype(this->MaskImage.GetPointer())>::type*,
                          const decltype(guidanceFields)&,
                          decltype(this->ResultImage.GetPointer()),
                          const itk::ImageRegion<2>&,
                          const std::remove_pointer<decltype(this->SourceImage.GetPointer())>::type*)
      = FillImage;

  auto functionToRun = std::bind(functionPointer,
                                 this->TargetImage.GetPointer(),
                                 this->MaskImage.GetPointer(),
                                 guidanceFields,
                                 this->ResultImage.GetPointer(),
                                 desiredRegion,
                                 this->SourceImage.GetPointer());

  QFuture<void> future =
      QtConcurrent::run(functionToRun);

  this->FutureWatcher.setFuture(future);

  this->ProgressDialog->exec();
}


void PoissonCloningWidget::on_btnMixedClone_clicked()
{
  // Extract the portion of the target image the user has selected.

  this->SelectedRegionCorner[0] = this->SourceImagePixmapItem->pos().x();
  this->SelectedRegionCorner[1] = this->SourceImagePixmapItem->pos().y();

  ImageType::RegionType desiredRegion(this->SelectedRegionCorner,
                                      this->SourceImage->GetLargestPossibleRegion().GetSize());

  std::vector<PoissonEditingParent::GuidanceFieldType::Pointer> sourceGuidanceFields =
      PoissonEditingParent::ComputeGuidanceField(this->SourceImage.GetPointer());

  std::vector<PoissonEditingParent::GuidanceFieldType::Pointer> targetGuidanceFields =
      PoissonEditingParent::ComputeGuidanceField(this->TargetImage.GetPointer());

  // Create a container for the new guidance fields
  std::vector<PoissonEditingParent::GuidanceFieldType::Pointer> mixedGuidanceFields(sourceGuidanceFields.size());

  ImageType::RegionType targetDesiredRegion = desiredRegion;
  targetDesiredRegion.Crop(this->TargetImage->GetLargestPossibleRegion());
  ImageType::RegionType sourceDesiredRegion = this->SourceImage->GetLargestPossibleRegion();
  ITKHelpers::CropRegionAtPosition(sourceDesiredRegion, this->TargetImage->GetLargestPossibleRegion(), desiredRegion);

  for(unsigned int channel = 0; channel < sourceGuidanceFields.size(); ++channel)
  {
    // Initialize the mixed field with the source field
    mixedGuidanceFields[channel] = PoissonEditingParent::GuidanceFieldType::New();
    ITKHelpers::DeepCopy(sourceGuidanceFields[channel].GetPointer(), mixedGuidanceFields[channel].GetPointer());

    itk::ImageRegionIterator<PoissonEditingParent::GuidanceFieldType>
        sourceGuidanceIterator(sourceGuidanceFields[channel], sourceDesiredRegion);
    itk::ImageRegionIterator<PoissonEditingParent::GuidanceFieldType>
        targetGuidanceIterator(targetGuidanceFields[channel], targetDesiredRegion);
    itk::ImageRegionIterator<PoissonEditingParent::GuidanceFieldType>
        mixedGuidanceIterator(mixedGuidanceFields[channel], sourceDesiredRegion);

    while(!sourceGuidanceIterator.IsAtEnd())
    {
      float sourceMagnitude = sourceGuidanceIterator.Get().GetNorm();
      float targetMagnitude = targetGuidanceIterator.Get().GetNorm();

      if(targetMagnitude > sourceMagnitude)
      {
        mixedGuidanceIterator.Set(targetGuidanceIterator.Get());
      }

      ++sourceGuidanceIterator;
      ++targetGuidanceIterator;
      ++mixedGuidanceIterator;
    }
  }

//  ITKHelpers::WriteImage(this->SourceImage.GetPointer(), "source.mha");
//  ITKHelpers::WriteImage(sourceGuidanceFields[0].GetPointer(), "sourceGuidanceField.mha");

  // We must get a function pointer to the overload that would be chosen by the compiler
  // to pass to run().
  void (*functionPointer)(const std::remove_pointer<decltype(this->TargetImage.GetPointer())>::type*,
                          const std::remove_pointer<decltype(this->MaskImage.GetPointer())>::type*,
                          const decltype(mixedGuidanceFields)&,
                          decltype(this->ResultImage.GetPointer()),
                          const itk::ImageRegion<2>&,
                          const std::remove_pointer<decltype(this->SourceImage.GetPointer())>::type*)
      = FillImage;

  auto functionToRun = std::bind(functionPointer,
                                 this->TargetImage.GetPointer(),
                                 this->MaskImage.GetPointer(),
                                 mixedGuidanceFields,
                                 this->ResultImage.GetPointer(),
                                 desiredRegion,
                                 this->SourceImage.GetPointer());

  QFuture<void> future =
      QtConcurrent::run(functionToRun);

  this->FutureWatcher.setFuture(future);

  this->ProgressDialog->exec();
}

void PoissonCloningWidget::on_actionSaveResult_triggered()
{
  // Get a filename to save
  QString fileName =
      QFileDialog::getSaveFileName(this, "Save File", ".",
                                   "Image Files (*.jpg *.jpeg *.bmp *.png *.mha)");

  if(fileName.toStdString().empty())
  {
    std::cout << "Filename was empty." << std::endl;
    return;
  }

  ITKHelpers::WriteImage(this->ResultImage.GetPointer(),
                         fileName.toStdString());
  ITKHelpers::WriteRGBImage(this->ResultImage.GetPointer(),
                            fileName.toStdString() + ".png");
  this->statusBar()->showMessage("Saved result.");
}

void PoissonCloningWidget::on_actionOpenImages_triggered()
{
  std::vector<std::string> namedImages;
  namedImages.push_back("SourceImage");
  namedImages.push_back("TargetImage");
  namedImages.push_back("MaskImage");
  
  std::vector<std::string> extensionFilters;
  extensionFilters.push_back("png");
  extensionFilters.push_back("png");
  extensionFilters.push_back("mask");

  ImageFileSelector* fileSelector(new ImageFileSelector(namedImages, extensionFilters));
  fileSelector->exec();

  int result = fileSelector->result();
  if(result) // The user clicked 'ok'
  {
    OpenImages(fileSelector->GetNamedImageFileName("SourceImage"),
               fileSelector->GetNamedImageFileName("TargetImage"),
               fileSelector->GetNamedImageFileName("MaskImage"));
  }
  else
  {
    // std::cout << "User clicked cancel." << std::endl;
    // The user clicked 'cancel' or closed the dialog, do nothing.
  }
}

void PoissonCloningWidget::slot_finished()
{
  if(this->ResultImage->GetNumberOfComponentsPerPixel() == 0)
  {
    return;
  }

  QImage qimage = ITKQtHelpers::GetQImageColor(this->ResultImage.GetPointer(),
                                               QImage::Format_RGB888);

  this->ResultPixmapItem = this->ResultScene->addPixmap(QPixmap::fromImage(qimage));
  this->graphicsViewResultImage->fitInView(this->ResultPixmapItem, Qt::KeepAspectRatio);
}
