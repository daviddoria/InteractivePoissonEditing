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

#ifndef PoissonCloningWidget_H
#define PoissonCloningWidget_H

#include "ui_PoissonCloningWidget.h"

// ITK
#include "itkVectorImage.h"

// Custom
#include "Mask.h"

// Qt
#include <QMainWindow>
#include <QFutureWatcher>
#include <QProgressDialog>
class QGraphicsPixmapItem;

class PoissonCloningWidget : public QMainWindow, public Ui::PoissonCloningWidget
{
  Q_OBJECT
public:

  PoissonCloningWidget();
  PoissonCloningWidget(const std::string& sourceImageFileName,
                       const std::string& targetImageFileName, const std::string& maskFileName);
  
  typedef itk::VectorImage<float,2> ImageType;
    
public slots:

  void on_actionOpenImages_triggered();
  void on_actionSaveResult_triggered();
  
  void on_btnClone_clicked();
  void on_btnMixedClone_clicked();

  void slot_finished();
  
protected:

  itk::Index<2> SelectedRegionCorner;

  void OpenImages(const std::string& sourceImageFileName,
                  const std::string& maskFileName,
                  const std::string& targetImageFileName);

  void showEvent ( QShowEvent * event );
  void resizeEvent ( QResizeEvent * event );
  
  ImageType::Pointer ResultImage;
  ImageType::Pointer SourceImage;
  ImageType::Pointer TargetImage;
  Mask::Pointer MaskImage;

  QGraphicsPixmapItem* SourceImagePixmapItem = nullptr;
  QGraphicsPixmapItem* TargetImagePixmapItem = nullptr;
  QGraphicsPixmapItem* ResultPixmapItem = nullptr;
  
  QGraphicsScene* InputScene;
  QGraphicsScene* ResultScene;

  std::string SourceImageFileName;
  std::string TargetImageFileName;
  std::string MaskImageFileName;

  QFutureWatcher<void> FutureWatcher;
  QProgressDialog* ProgressDialog;
};

#endif // PoissonEditingWidget_H
