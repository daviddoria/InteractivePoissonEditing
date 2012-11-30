/*=========================================================================
 *
 *  Copyright David Doria 2011 daviddoria@gmail.com
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
                                           const std::string& targetImageFileName,
                                           const std::string& maskFileName) : PoissonCloningWidget()
{
  this->SourceImageFileName = sourceImageFileName;
  this->TargetImageFileName = targetImageFileName;
  this->MaskImageFileName = maskFileName;

  OpenImages(this->SourceImageFileName, this->TargetImageFileName, this->MaskImageFileName);
}

PoissonCloningWidget::PoissonCloningWidget() : QMainWindow()
{
  this->setupUi(this);

  this->progressBar->setMinimum(0);
  this->progressBar->setMaximum(0);
  this->progressBar->hide();

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

  this->SourceScene = new QGraphicsScene;
  this->graphicsViewSourceImage->setScene(this->SourceScene);

  this->TargetScene = new QGraphicsScene;
  this->graphicsViewTargetImage->setScene(this->TargetScene);

  this->ResultScene = new QGraphicsScene;
  this->graphicsViewResultImage->setScene(this->ResultScene);
}

void PoissonCloningWidget::showEvent(QShowEvent* event)
{
  if(SourceImagePixmapItem && TargetImagePixmapItem)
  {
    this->graphicsViewSourceImage->fitInView(this->SourceImagePixmapItem, Qt::KeepAspectRatio);
    this->graphicsViewTargetImage->fitInView(this->TargetImagePixmapItem, Qt::KeepAspectRatio);
  }
}

void PoissonCloningWidget::resizeEvent(QResizeEvent* event)
{
  if(SourceImagePixmapItem && TargetImagePixmapItem)
  {
    this->graphicsViewSourceImage->fitInView(this->SourceImagePixmapItem, Qt::KeepAspectRatio);
    this->graphicsViewTargetImage->fitInView(this->TargetImagePixmapItem, Qt::KeepAspectRatio);
  }
}
  
void PoissonCloningWidget::OpenImages(const std::string& sourceImageFileName, const std::string& targetImageFileName, const std::string& maskFileName)
{
  // Load and display source image
  typedef itk::ImageFileReader<ImageType> ImageReaderType;
  ImageReaderType::Pointer sourceImageReader = ImageReaderType::New();
  sourceImageReader->SetFileName(sourceImageFileName);
  sourceImageReader->Update();

  ITKHelpers::DeepCopy(sourceImageReader->GetOutput(), this->SourceImage.GetPointer());

  QImage qimageSourceImage = ITKQtHelpers::GetQImageColor(this->SourceImage.GetPointer());
  this->SourceImagePixmapItem = this->SourceScene->addPixmap(QPixmap::fromImage(qimageSourceImage));
    
  // Load and display target image
  ImageReaderType::Pointer targetImageReader = ImageReaderType::New();
  targetImageReader->SetFileName(targetImageFileName);
  targetImageReader->Update();

  ITKHelpers::DeepCopy(targetImageReader->GetOutput(), this->TargetImage.GetPointer());

  QImage qimageTargetImage = ITKQtHelpers::GetQImageColor(this->TargetImage.GetPointer());
  this->TargetImagePixmapItem = this->TargetScene->addPixmap(QPixmap::fromImage(qimageTargetImage));
  this->TargetScene->setSceneRect(qimageTargetImage.rect());

  // Load and display mask
  this->MaskImage->Read(maskFileName);

  QImage qimageMask = MaskQt::GetQtImage(this->MaskImage.GetPointer());
  this->MaskImagePixmapItem = this->SourceScene->addPixmap(QPixmap::fromImage(qimageMask));
  this->MaskImagePixmapItem->setVisible(this->chkShowMask->isChecked());

  // Setup selection region
  QColor semiTransparentRed(255,0,0, 127);

  this->SelectionImage = QImage(sourceImageReader->GetOutput()->GetLargestPossibleRegion().GetSize()[0],
                                sourceImageReader->GetOutput()->GetLargestPossibleRegion().GetSize()[1], QImage::Format_ARGB32);
  this->SelectionImage.fill(semiTransparentRed.rgba());

  this->SelectionImagePixmapItem = this->TargetScene->addPixmap(QPixmap::fromImage(this->SelectionImage));
  //this->SelectionImagePixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
  this->SelectionImagePixmapItem->setFlag(QGraphicsItem::ItemIsMovable);

}

void PoissonCloningWidget::on_btnClone_clicked()
{
  // Extract the portion of the target image the user has selected.
  
  this->SelectedRegionCorner[0] = this->SelectionImagePixmapItem->pos().x();
  //corner[1] = this->SelectionImagePixmapItem->pos().y();
  this->SelectedRegionCorner[1] = this->TargetImage->GetLargestPossibleRegion().GetSize()[1] -
              (this->SelectionImagePixmapItem->pos().y() + this->SelectionImagePixmapItem->boundingRect().height());
//   std::cout << "Height: " << this->SelectionImagePixmapItem->boundingRect().height() << std::endl;
//   std::cout << "y: " << this->SelectionImagePixmapItem->pos().y() << std::endl;
//   std::cout << "Corner: " << corner << std::endl;
  
  itk::Size<2> size = this->SourceImage->GetLargestPossibleRegion().GetSize();

  ImageType::RegionType desiredRegion(this->SelectedRegionCorner, size);

  std::vector<PoissonEditingParent::GuidanceFieldType::Pointer> guidanceFields =
      PoissonEditingParent::ComputeGuidanceField(this->SourceImage.GetPointer());

  // We must get a function pointer to the overload that would be chosen by the compiler
  // to pass to run().
  void (*functionPointer)(const std::remove_pointer<decltype(this->TargetImage.GetPointer())>::type*,
                          const std::remove_pointer<decltype(this->MaskImage.GetPointer())>::type*,
                          const decltype(guidanceFields)&,
                          decltype(this->ResultImage.GetPointer()),
                          const itk::ImageRegion<2>&)
      = FillImage;

  QFuture<void> future =
      QtConcurrent::run(functionPointer,
                        this->SourceImage.GetPointer(),
                        this->MaskImage.GetPointer(),
                        guidanceFields,
                        this->ResultImage.GetPointer(),
                        desiredRegion);

  this->FutureWatcher.setFuture(future);

  this->ProgressDialog->exec();

}

void PoissonCloningWidget::on_actionSaveResult_activated()
{
  // Get a filename to save
  QString fileName = QFileDialog::getSaveFileName(this, "Save File", ".", "Image Files (*.jpg *.jpeg *.bmp *.png *.mha)");

  if(fileName.toStdString().empty())
  {
    std::cout << "Filename was empty." << std::endl;
    return;
  }

  ITKHelpers::WriteImage(this->ResultImage.GetPointer(), fileName.toStdString());
  ITKHelpers::WriteRGBImage(this->ResultImage.GetPointer(), fileName.toStdString() + ".png");
  this->statusBar()->showMessage("Saved result.");
}

void PoissonCloningWidget::on_actionOpenImages_activated()
{
  std::vector<std::string> namedImages;
  namedImages.push_back("SourceImage");
  namedImages.push_back("TargetImage");
  namedImages.push_back("MaskImage");
  
  ImageFileSelector* fileSelector(new ImageFileSelector(namedImages));
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

void PoissonCloningWidget::on_chkShowMask_clicked()
{
  if(!this->MaskImagePixmapItem)
  {
    return;
  }
  this->MaskImagePixmapItem->setVisible(this->chkShowMask->isChecked());
}


void PoissonCloningWidget::slot_StartProgressBar()
{
  this->progressBar->show();
}

void PoissonCloningWidget::slot_StopProgressBar()
{
  this->progressBar->hide();
}

void PoissonCloningWidget::slot_finished()
{
//   QImage qimage = HelpersQt::GetQImageRGBA<ImageType>(this->ResultImage);
//   this->ResultPixmapItem = this->ResultScene->addPixmap(QPixmap::fromImage(qimage));


  // Paste the result back into the appropriate region of the target image
  typedef itk::PasteImageFilter <ImageType, ImageType > PasteImageFilterType;
  PasteImageFilterType::Pointer pasteImageFilter = PasteImageFilterType::New ();
  pasteImageFilter->SetSourceImage(this->ResultImage);
  pasteImageFilter->SetDestinationImage(this->TargetImage);
  pasteImageFilter->SetSourceRegion(this->ResultImage->GetLargestPossibleRegion());
  pasteImageFilter->SetDestinationIndex(this->SelectedRegionCorner);
  pasteImageFilter->Update();

  // Display the result
  QImage qimage = ITKQtHelpers::GetQImageColor(pasteImageFilter->GetOutput());
  //qimage = HelpersQt::FitToGraphicsView(qimage, this->graphicsViewResultImage);

  this->ResultPixmapItem = this->ResultScene->addPixmap(QPixmap::fromImage(qimage));
  this->graphicsViewResultImage->fitInView(this->ResultPixmapItem, Qt::KeepAspectRatio);
  //this->ResultPixmapItem->setVisible(this->chkShowOutput->isChecked());
}
