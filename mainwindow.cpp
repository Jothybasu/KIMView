/********************************************************************************
MIT License

Copyright (c) 2021 Jothy Selvaraj

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
********************************************************************************/

#include "mainwindow.h"

#include <QVTKWidget.h>
#include <gdcmAttribute.h>
#include <gdcmDataElement.h>
#include <gdcmItem.h>
#include <gdcmNestedModuleEntries.h>
#include <gdcmTag.h>
#include <itkMetaDataDictionary.h>
#include <omp.h>
#include <stdlib.h>
#include <vtkActorCollection.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkDICOMImageReader.h>
#include <vtkFileOutputWindow.h>
#include <vtkGDCMImageReader.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkXMLImageDataWriter.h>

#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>
#include <QList>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTime>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QUdpSocket>
#include <iostream>
#include <itksys/SystemTools.hxx>

#include "aboutdialog.h"
#include "createobjects.h"
#include "dvhdialog.h"
#include "imageviewer2d.h"
#include "ipconfigdialog.h"
#include "itkCastImageFilter.h"
#include "itkCommand.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImage.h"
#include "itkImageSeriesReader.h"
#include "itkImageToVTKImageFilter.h"
#include "itkMetaDataObject.h"
#include "itkMinimumMaximumImageFilter.h"
#include "itkNumericSeriesFileNames.h"
#include "itkSmartPointer.h"
#include "itkVersion.h"
#include "meshreader.h"
#include "planreader.h"
#include "rangesliderdialog.h"
#include "rtstructreaderdialog.h"
#include "udplistener.h"
#include "ui_mainwindow.h"
#include "wlwwdialog.h"

// Testing
#include <vtkArcSource.h>
#include <vtkArrowSource.h>
#include <vtkAssembly.h>
#include <vtkConeSource.h>
#include <vtkGlyph3D.h>
#include <vtkLineSource.h>
#include <vtkPolarAxesActor.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // Make tabel widget read only
  this->ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Avoid error window popping
  vtkSmartPointer<vtkFileOutputWindow> fileOutputWindow =
      vtkSmartPointer<vtkFileOutputWindow>::New();
  fileOutputWindow->SetFileName("ErrorLog.txt");
  vtkOutputWindow *outputWindow = vtkOutputWindow::GetInstance();
  if (outputWindow) {
    outputWindow->SetInstance(fileOutputWindow);
  }

  this->BeamAngles.clear();

  this->ContextMenus = new QActionGroup(this);
  this->ContextMenus->setEnabled(true);
  this->ContextMenus->setExclusive(false);

  this->ContextMenus->addAction(this->ui->actionBEV);
  this->ContextMenus->addAction(this->ui->action3DView);

  this->RTDose = vtkSmartPointer<vtkImageData>::New();

  this->ui->treeWidget->expandAll();
  this->ui->statusBar->showMessage("No data available to display");

  this->CTImage = vtkSmartPointer<vtkImageData>::New();

  this->MeshActors = vtkSmartPointer<vtkActorCollection>::New();
  this->BeamActors = vtkSmartPointer<vtkActorCollection>::New();

  // Setup BEV widget
  this->BEVViewer = new BEVWidget(this->ui->mdiAreaView, this->ContextMenus);
  // this->ui->mdiAreaView->addSubWindow(this->BEVViewer,Qt::WindowMinMaxButtonsHint|Qt::WindowTitleHint|Qt::FramelessWindowHint);//For
  // frameless window
  this->ui->mdiAreaView->addSubWindow(
      this->BEVViewer, Qt::WindowMaximizeButtonHint | Qt::WindowTitleHint);
  this->BEVViewer->setWindowTitle("Model");

  // By default plan information dock widget is hidden
  this->ui->dockWidget_2->setVisible(true);
  this->ui->mdiAreaView->tileSubWindows();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_actionCT_triggered() {
  const unsigned int InputDimension = 3;
  typedef signed short PixelType;
  typedef itk::Image<PixelType, InputDimension> InputImageType;
  typedef itk::ImageSeriesReader<InputImageType> ReaderType;
  typedef itk::GDCMImageIO ImageIOType;
  typedef itk::GDCMSeriesFileNames InputNamesGeneratorType;
  ImageIOType::Pointer gdcmIO = ImageIOType::New();
  ReaderType::Pointer reader = ReaderType::New();

  try {
    QString imageDirName = QFileDialog::getExistingDirectory(
        this, "Open CT Folder", "D:\\DICOM Test Patients");

    // Start reading DICOM CT data with Phase info
    /********************************************************************************/

    InputNamesGeneratorType::Pointer inputNames =
        InputNamesGeneratorType::New();
    inputNames->SetInputDirectory(imageDirName.toLatin1().data());
    gdcmIO->SetLoadPrivateTags(true);
    gdcmIO->SetLoadSequences(true);

    const ReaderType::FileNamesContainer &filenames =
        inputNames->GetInputFileNames();

    reader->SetImageIO(gdcmIO);
    reader->SetFileNames(filenames);
    reader->Update();

    // qDebug()<<"Reading done!";
    using DictionaryType = itk::MetaDataDictionary;
    typedef itk::MetaDataObject<std::string> MetaDataStringType;
    const DictionaryType &dictionary = gdcmIO->GetMetaDataDictionary();

    // Read Patient Position tag
    std::string PatientPosition = "0018|5100";
    auto tagItr1 = dictionary.Find(PatientPosition);
    MetaDataStringType::ConstPointer PatientPositionValue =
        dynamic_cast<const MetaDataStringType *>(tagItr1->second.GetPointer());
    std::string val1;
    val1 = PatientPositionValue->GetMetaDataObjectValue();
    // qDebug()<<val1.c_str()<<" "<<val1.compare("HFS")<<" Comaprarison";
    QString curIOP;
    curIOP = QString::fromStdString(val1);

    //        //Read Patient Orientation tag, this seems more relibale
    //        std::string PatientOrientation = "0020|0037";
    //        auto tagItr = dictionary.Find(PatientOrientation);
    //        MetaDataStringType::ConstPointer PatientOrientationValue =
    //        dynamic_cast<const MetaDataStringType
    //        *>(tagItr->second.GetPointer() ); std::string val2 =
    //        PatientOrientationValue->GetMetaDataObjectValue();
    //        qDebug()<<"DICOM Cosines: "<<val2.c_str();

    //        QString curIOP;
    //        curIOP=QString::fromStdString(val2);

    QString supportedIOP =
        "HFS "; // DICOM seems to be hvaving a space after as "HFS "
    // QString supportedIOP="1\\0.0\\0.0\\0.0\\1\\0.0 ";//DICOM seems to be
    // hvaving a space at the end "
    // qDebug()<<curIOP<<"**********"<<supportedIOP;

    // Read patient's name tag
    std::string PatientName = "0010|0010";
    auto tagItr2 = dictionary.Find(PatientName);
    MetaDataStringType::ConstPointer PatientNameValue =
        dynamic_cast<const MetaDataStringType *>(tagItr2->second.GetPointer());
    std::string val2;
    val2 = PatientNameValue->GetMetaDataObjectValue();
    QString PatientNameStr;
    PatientInfo["PatientName"] = QString::fromStdString(val2);

    // Read patient's ID
    std::string PatientID = "0010|0020";
    auto tagItr3 = dictionary.Find(PatientID);
    MetaDataStringType::ConstPointer PatientIDValue =
        dynamic_cast<const MetaDataStringType *>(tagItr3->second.GetPointer());
    std::string val3;
    val3 = PatientIDValue->GetMetaDataObjectValue();
    QString PatientIDStr;
    PatientInfo["PatientID"] = QString::fromStdString(val3);

    // 1==match,-1=no match
    if (curIOP == supportedIOP) {
      typedef itk::ImageToVTKImageFilter<InputImageType> ConnectorType;
      ConnectorType::Pointer Converter = ConnectorType::New();
      Converter->SetInput(reader->GetOutput());
      Converter->Update();
      // qDebug()<<"Conversion done!";

      this->CTImage->DeepCopy(Converter->GetOutput());

      // Set patients name and ID to treeWidget
      QString NameIDStr;
      NameIDStr.append(PatientInfo["PatientName"]);
      NameIDStr.append(" | ");
      NameIDStr.append(PatientInfo["PatientID"]);
      this->ui->treeWidget->setHeaderLabel(NameIDStr);
      this->setWindowTitle(NameIDStr);

      // Hide information tree widget
      // this->ui->actionInformation->trigger();

      // Display the data
      this->SagittalViewer =
          new ImageViewer2D(this->ui->mdiAreaView, this->ContextMenus);
      this->SagittalViewer->SetImageData(this->CTImage);
      this->SagittalViewer->SetSliceOrientation(1);
      this->SagittalViewer->SetUpView();
      this->ui->mdiAreaView->addSubWindow(
          this->SagittalViewer,
          Qt::WindowMaximizeButtonHint |
              Qt::WindowTitleHint); // add to make borderless window
                                    // Qt::FramelessWindowHint
      this->SagittalViewer->setWindowTitle("Sagittal");
      this->SagittalViewer->show();

      this->CoronalViewer =
          new ImageViewer2D(this->ui->mdiAreaView, this->ContextMenus);
      this->CoronalViewer->SetImageData(this->CTImage);
      this->CoronalViewer->SetSliceOrientation(2);
      this->CoronalViewer->SetUpView();
      this->ui->mdiAreaView->addSubWindow(this->CoronalViewer,
                                          Qt::WindowMaximizeButtonHint |
                                              Qt::WindowTitleHint);
      this->CoronalViewer->setWindowTitle("Coronal");
      this->CoronalViewer->show();

      this->AxialViewer =
          new ImageViewer2D(this->ui->mdiAreaView, this->ContextMenus);
      this->AxialViewer->SetImageData(this->CTImage);
      this->AxialViewer->SetSliceOrientation(0);
      this->AxialViewer->SetUpView();
      this->ui->mdiAreaView->addSubWindow(this->AxialViewer,
                                          Qt::WindowMaximizeButtonHint |
                                              Qt::WindowTitleHint);
      this->AxialViewer->setWindowTitle("Axial");
      this->AxialViewer->show();

      this->ui->mdiAreaView->tileSubWindows();
      this->ui->statusBar->showMessage("CT imported sucessfully");

      QTreeWidgetItem *wItem = new QTreeWidgetItem((QTreeWidget *)nullptr,
                                                   QStringList(QString("CT")));
      wItem->setCheckState(0, Qt::Checked);
      QIcon icon;
      icon.addFile(QString::fromUtf8(":/Icons/CT.png"), QSize(12, 12),
                   QIcon::Normal, QIcon::Off);
      wItem->setIcon(0, icon);
      this->ui->treeWidget->topLevelItem(0)->addChild(wItem);

      // Set default DoseVOI for RPL calculation
      double bds[6];
      this->CTImage->GetBounds(bds);

      // Enable actions
      this->ui->actionReset_WL_WW->setEnabled(true);
      this->ui->actionGo_To_Isocentre->setEnabled(true);
      this->ui->actionReset_Zoom->setEnabled(true);
      this->ui->actionShowBeams->setEnabled(true);
      this->ui->actionShowContours->setEnabled(true);
      this->ui->actionShowDose->setEnabled(true);
      this->ui->actionZoom_In_All->setEnabled(true);
      this->ui->actionZoom_Out_All->setEnabled(true);
      this->ui->actionBEV->setEnabled(true);
      this->ui->action3DView->setEnabled(true);
      this->ui->actionZoom_In_All->setEnabled(true);
      this->ui->actionZoom_Out_All->setEnabled(true);
      this->ui->actionReset_Zoom->setEnabled(true);
      this->ui->actionReset_WL_WW->setEnabled(true);
      this->ui->actionCalc_DVH->setEnabled(true);
      this->ui->actionStart->setEnabled(true);
      this->ui->actionStop->setEnabled(true);
      this->ui->actionStructures->setEnabled(true);
      this->ui->actionDose->setEnabled(true);
      this->ui->actionAdjust_Range->setEnabled(true);
      this->ui->actionWL_WW->setEnabled(true);
      this->ui->actionPlan->setEnabled(true);

    } else {
      QMessageBox messageBox;
      messageBox.critical(this, "Error",
                          "Unsupported image orientation.\nOnly HFS is "
                          "supported in this version.");
      messageBox.setFixedSize(500, 200);
    }

  } catch (itk::ExceptionObject &excp) {
    std::cerr << "Exception thrown while reading the series" << std::endl;
    std::cerr << excp << std::endl;
  }
}

void MainWindow::on_actionStructures_triggered() {
  if (this->CTImage->GetDimensions()[0] > 0) {
    // Read ROIs
    RTStructReaderDialog *meshReaderDlg = new RTStructReaderDialog(this);
    meshReaderDlg->exec();

    if (meshReaderDlg->ROINames.size() > 0) // Check any ROI exist or not
    {
      QList<int> selectedStructsList = meshReaderDlg->selectedItems;
      // qDebug()<<selectedStructsList[0]<<"ROI";

      meshReader *RTStructReader = new meshReader(this);
      // RTStructReader->getStructFileName();
      RTStructReader->structFileName = meshReaderDlg->structFileName;
      QCoreApplication::processEvents();
      RTStructReader->getROIMeshes(
          this->CTImage, this->CTImage->GetSpacing()[2], this->TargetReduction,
          meshReaderDlg->selectedItems,
          this); // Reads ROI name as well as structs
      QCoreApplication::processEvents();
      this->MeshList = RTStructReader->meshes;
      this->MeshActors = RTStructReader->ROIActors;
      this->ROIVisibleFlag = 1; // structs imported

      for (int i = 0; i < meshReaderDlg->selectedItems.size(); i++) {
        this->ROIColors[i][0] = RTStructReader->ROIColors[i][0];
        this->ROIColors[i][1] = RTStructReader->ROIColors[i][1];
        this->ROIColors[i][2] = RTStructReader->ROIColors[i][2];
      }

      this->ROINum = meshReaderDlg->selectedItems.size();
      this->ROINames = RTStructReader->ROINames;
      this->ROITypes = RTStructReader->ROITypes;
      this->ROINo = RTStructReader->ROINo;

      QList<QTreeWidgetItem *> items;
      for (int i = 0; i < meshReaderDlg->selectedItems.size(); ++i) {
        QTreeWidgetItem *wItem = new QTreeWidgetItem(
            (QTreeWidget *)0,
            QStringList(QString(RTStructReader->ROINames[i])));
        wItem->setCheckState(0, Qt::Checked);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/Icons/Polygon.png"), QSize(12, 12),
                     QIcon::Normal, QIcon::Off);
        wItem->setIcon(0, icon);
        wItem->setBackgroundColor(0, QColor(RTStructReader->ROIColors[i][0],
                                            RTStructReader->ROIColors[i][1],
                                            RTStructReader->ROIColors[i][2]));
        items.append(wItem);
      }
      // Add ROIs to RTSS item
      this->ui->treeWidget->topLevelItem(1)->addChildren(items);
      this->ui->treeWidget->expandAll();

      this->ui->treeWidget->topLevelItem(1)->setText(
          0, QString(RTStructReader->structSetLabel));

      delete RTStructReader;
    }
    delete meshReaderDlg;

    // Dispaly ROIs
    this->AxialViewer->MeshList = this->MeshList;
    this->AxialViewer->ROIColors = this->ROIColors;
    this->AxialViewer->show();
    this->AxialViewer->ContourVisibility = 1;
    this->AxialViewer->DisplayROIs(this->AxialViewer->SliceLoc,
                                   this->AxialViewer->SliceOrientation);
    this->AxialViewer->UpdateView();

    this->SagittalViewer->MeshList = this->MeshList;
    this->SagittalViewer->ROIColors = this->ROIColors;
    this->SagittalViewer->show();
    this->SagittalViewer->ContourVisibility = 1;
    this->SagittalViewer->DisplayROIs(this->SagittalViewer->SliceLoc,
                                      this->SagittalViewer->SliceOrientation);
    this->SagittalViewer->UpdateView();

    this->CoronalViewer->MeshList = this->MeshList;
    this->CoronalViewer->ROIColors = this->ROIColors;
    this->CoronalViewer->show();
    this->CoronalViewer->ContourVisibility = 1;
    this->CoronalViewer->DisplayROIs(this->CoronalViewer->SliceLoc,
                                     this->CoronalViewer->SliceOrientation);
    this->CoronalViewer->UpdateView();

    this->ui->action3DView->trigger();
  } else {
    QMessageBox messageBox;
    messageBox.critical(this, "Error",
                        "Please load a CT image before loading structures");
    messageBox.setFixedSize(500, 200);
  }
}

void MainWindow::on_actionDose_triggered() {
  if (this->CTImage->GetDimensions()[0] > 0) {
    QString DoseFile = QFileDialog::getOpenFileName(this, "Open RT Dose");
    if (DoseFile != nullptr) {
      // qDebug()<<doseFile;
      vtkSmartPointer<vtkGDCMImageReader> DoseReader =
          vtkSmartPointer<vtkGDCMImageReader>::New();
      DoseReader->SetFileName(DoseFile.toLatin1());
      DoseReader->FileLowerLeftOn(); // otherwise flips the image
      DoseReader->SetDataScalarTypeToDouble();
      DoseReader->Update();
      this->RTDose->DeepCopy(DoseReader->GetOutput());
      // qDebug()<<this->RTDose->GetScalarRange()[0]<<":Min"<<this->RTDose->GetScalarRange()[1]<<":Max";
    } else {
      QMessageBox messageBox;
      messageBox.critical(this, "Error", "An error has occured !");
      messageBox.setFixedSize(500, 200);
      messageBox.show();
    }

    this->AxialViewer->SetRTDose(this->RTDose);
    this->SagittalViewer->SetRTDose(this->RTDose);
    this->CoronalViewer->SetRTDose(this->RTDose);
    this->BEVViewer->RTDose = this->RTDose;

    this->AxialViewer->DoseVisibility = true;
    this->AxialViewer->UpdateView();
    this->SagittalViewer->DoseVisibility = true;
    this->SagittalViewer->UpdateView();
    this->CoronalViewer->DoseVisibility = true;
    this->CoronalViewer->UpdateView();

    this->ui->statusBar->showMessage("Dose imported sucessfully");

    double *DoseOrg = this->RTDose->GetOrigin();
    // qDebug()<<DoseOrg[0]<<DoseOrg[1]<<DoseOrg[2]<<"Dose Origin";

    double *ImgOrg = this->CTImage->GetOrigin();
    // qDebug()<<ImgOrg[0]<<ImgOrg[1]<<ImgOrg[2]<<"Image Origin";
  } else {
    QMessageBox messageBox;
    messageBox.critical(this, "Error",
                        "Please load a CT image before loading dose");
    messageBox.setFixedSize(500, 200);
  }
}

void MainWindow::on_actionGo_To_Isocentre_triggered() {
  QList<QMdiSubWindow *> SubWindows = this->ui->mdiAreaView->subWindowList();
  if (SubWindows[1]) {
    ImageViewer2D *Viewer =
        qobject_cast<ImageViewer2D *>(SubWindows[1]->widget());
    if (Viewer->SliceOrientation == 0) {
      Viewer->MoveToLocation(this->Isocentre[2]);

    }

    else if (Viewer->SliceOrientation == 1) {
      Viewer->MoveToLocation(this->Isocentre[0]);

    }

    else if (Viewer->SliceOrientation == 2) {
      Viewer->MoveToLocation(this->Isocentre[1]);
    }
  }

  if (SubWindows[2]) {
    ImageViewer2D *Viewer =
        qobject_cast<ImageViewer2D *>(SubWindows[2]->widget());
    if (Viewer->SliceOrientation == 0) {
      Viewer->MoveToLocation(this->Isocentre[2]);

    }

    else if (Viewer->SliceOrientation == 1) {
      Viewer->MoveToLocation(this->Isocentre[0]);

    }

    else if (Viewer->SliceOrientation == 2) {
      Viewer->MoveToLocation(this->Isocentre[1]);
    }
  }

  if (SubWindows[3]) {
    ImageViewer2D *Viewer =
        qobject_cast<ImageViewer2D *>(SubWindows[3]->widget());
    if (Viewer->SliceOrientation == 0) {
      Viewer->MoveToLocation(this->Isocentre[2]);

    }

    else if (Viewer->SliceOrientation == 1) {
      Viewer->MoveToLocation(this->Isocentre[0]);

    }

    else if (Viewer->SliceOrientation == 2) {
      Viewer->MoveToLocation(this->Isocentre[1]);
    }
  }
}

void MainWindow::on_actionBEV_triggered() {
  this->ui->tableWidget->setFocus();

  // qDebug()<<"No. of beams:"<<this->ui->tableWidget->rowCount();

  if (!(this->ui->tableWidget->rowCount() == 0)) {
    // By default show the first beam if none selected
    int SelectedBeam = 0;
    SelectedBeam = this->ui->tableWidget->currentRow();
    double gantryAngle =
        this->ui->tableWidget->item(SelectedBeam, 2)->text().toDouble();
    // qDebug()<<gantryAngle<<":Selected Beam Angle";

    // Display the data
    this->BEVViewer->MeshActors = this->MeshActors;
    this->BEVViewer->ROIColors = this->ROIColors;
    this->BEVViewer->Isocentre = this->Isocentre;
    this->BEVViewer->GantryAngle = gantryAngle;
    this->BEVViewer->DisplayBEV();
    this->BEVViewer->show();

  }

  else {
    QMessageBox messageBox;
    messageBox.critical(this, "Error",
                        "No beam exist. Please add a beam to view BEV.");
    messageBox.setFixedSize(500, 200);
  }

  // this->ui->mdiAreaView->tileSubWindows();
}

void MainWindow::on_action3DView_triggered() {
  this->ui->tableWidget->setFocus();
  // Display the data
  this->BEVViewer->MeshActors = this->MeshActors;
  this->BEVViewer->BeamActors = this->BeamActors;
  this->BEVViewer->ROIColors = this->ROIColors;
  this->BEVViewer->DisplayMeshes();
  this->BEVViewer->DisplayBeams();

  this->InteractorTrackball =
      vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  this->BEVViewer->ModelRenderer->GetRenderWindow()
      ->GetInteractor()
      ->SetInteractorStyle(this->InteractorTrackball);
  this->BEVViewer->show();
  this->BEVViewer->ModelRenderer->GetRenderWindow()->Render();
}

void MainWindow::on_actionClose_Patient_triggered() {
  QApplication::closeAllWindows();
}

void MainWindow::on_actionReset_Zoom_triggered() {
  this->AxialViewer->ViewRenderer->ResetCamera();
  this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();

  this->SagittalViewer->ViewRenderer->ResetCamera();
  this->SagittalViewer->ViewRenderer->GetRenderWindow()->Render();

  this->CoronalViewer->ViewRenderer->ResetCamera();
  this->CoronalViewer->ViewRenderer->GetRenderWindow()->Render();
}

void MainWindow::on_actionShowBeams_triggered() {
  this->AxialViewer->TriggerActionShowBeams();
  this->SagittalViewer->TriggerActionShowBeams();
  this->CoronalViewer->TriggerActionShowBeams();
}

void MainWindow::on_actionShowDose_triggered() {
  this->AxialViewer->TriggerActionShowDose();
  this->SagittalViewer->TriggerActionShowDose();
  this->CoronalViewer->TriggerActionShowDose();

  if (this->AxialViewer->DoseVisibility == 1) {
    this->BEVViewer->IsodoseSurface->VisibilityOn();
    // qDebug()<<"Dose on";

  } else {
    this->BEVViewer->IsodoseSurface->VisibilityOff();
    // qDebug()<<"Dose off";
  }
  this->BEVViewer->ModelRenderer->GetRenderWindow()->Render();
}

void MainWindow::on_actionShowContours_triggered() {
  this->AxialViewer->TriggerActionShowContours();
  this->SagittalViewer->TriggerActionShowContours();
  this->CoronalViewer->TriggerActionShowContours();
}

void MainWindow::on_actionZoom_In_All_triggered() {
  // qDebug()<<"Zooming in...";
  this->AxialViewer->ViewRenderer->GetActiveCamera()->Zoom(1.1);
  this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();

  this->SagittalViewer->ViewRenderer->GetActiveCamera()->Zoom(1.1);
  this->SagittalViewer->ViewRenderer->GetRenderWindow()->Render();

  this->CoronalViewer->ViewRenderer->GetActiveCamera()->Zoom(1.1);
  this->CoronalViewer->ViewRenderer->GetRenderWindow()->Render();
}

void MainWindow::on_actionZoom_Out_All_triggered() {
  // qDebug()<<"Zooming out...";
  this->AxialViewer->ViewRenderer->GetActiveCamera()->Zoom(0.9);
  this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();

  this->SagittalViewer->ViewRenderer->GetActiveCamera()->Zoom(0.9);
  this->SagittalViewer->ViewRenderer->GetRenderWindow()->Render();

  this->CoronalViewer->ViewRenderer->GetActiveCamera()->Zoom(0.9);
  this->CoronalViewer->ViewRenderer->GetRenderWindow()->Render();
}

void MainWindow::on_actionInformation_triggered() {
  if (this->ui->actionInformation->isChecked()) {
    this->ui->dockWidget->setVisible(true);
  }

  else {
    this->ui->dockWidget->setVisible(false);
  }

  this->ui->mdiAreaView->tileSubWindows();
}

void MainWindow::on_actionPlan_Information_triggered() {
  if (this->ui->actionPlan_Information->isChecked()) {
    this->ui->dockWidget_2->setVisible(true);
  }

  else {
    this->ui->dockWidget_2->setVisible(false);
  }

  this->ui->mdiAreaView->tileSubWindows();
}

void MainWindow::on_actionCalc_DVH_triggered() {
  // qDebug()<<this->rtDoseCube->GetScalarRange()[1];
  if (this->RTDose->GetScalarRange()[1] == 1) {
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setText("Dose not available.");
    msgBox->setIcon(QMessageBox::Critical);
    msgBox->setWindowTitle("Error");
    msgBox->exec();
    delete msgBox;
  }

  else {
    DVHDialog *myDVHDialog = new DVHDialog(this);
    myDVHDialog->doseMatrix = this->RTDose;
    myDVHDialog->maxDose = this->RTDose->GetScalarRange()[1];
    myDVHDialog->meshes = this->MeshList;
    myDVHDialog->setROIColors(this->ROIColors);
    myDVHDialog->setStructureNames(this->ROINames);
    myDVHDialog->ROIType = this->ROITypes;
    myDVHDialog->ROINo = this->ROINo;

    myDVHDialog->exec();
    delete myDVHDialog;
  }
}

void MainWindow::on_actionAdjust_Range_triggered() {
  if (this->RTDose->GetScalarRange()[1] == 1.0) {
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setText("Dose not available.");
    msgBox->setIcon(QMessageBox::Critical);
    msgBox->setWindowTitle("Error");
    msgBox->exec();
    delete msgBox;
  } else {
    RangeSliderDialog *ranger = new RangeSliderDialog(this);
    ranger->minDose = this->RTDose->GetScalarRange()[0];
    ranger->maxDose = this->RTDose->GetScalarRange()[1];
    // Need to set viewers first before SetDoseRange, or else crashes
    ranger->AxialViewer = this->AxialViewer;
    ranger->SagittalViewer = this->SagittalViewer;
    ranger->CoronalViewer = this->CoronalViewer;
    ranger->ModelViewer = this->BEVViewer;
    ranger->SetDoseRange();
    ranger->show();
  }
}

void MainWindow::on_actionReset_WL_WW_triggered() {
  this->AxialViewer->TriggerReset_WL_WW();
  this->SagittalViewer->TriggerReset_WL_WW();
  this->CoronalViewer->TriggerReset_WL_WW();
}

void MainWindow::on_actionRender_Bones_triggered() {}

void MainWindow::on_actionHello_UDP_triggered() {
  //  if (this->ui->actionHello_UDP->isChecked() == true) {
  //    this->listener->TrackingTarget->DeepCopy(this->MeshList[0]);
  //    this->listener->AxialViewer = this->AxialViewer;
  //    this->listener->SagittalViewer = this->SagittalViewer;
  //    this->listener->CoronalViewer = this->CoronalViewer;
  //    this->listener->BEVViewer = this->BEVViewer;
  //    this->listener->StartListening();
  //    this->ui->statusBar->showMessage("Listening to UPD sender...");
  //  }

  //  else {
  //    this->listener->StopListening();
  //    this->ui->statusBar->clearMessage();
  //    QApplication::processEvents();
}

void MainWindow::on_actionMove_ROI_triggered() {
  // double XYZ[3]={-5,10,20};
  QString text = QInputDialog::getText(
      this, tr("Enter shifts (XYZ)"), tr("X,Y,Z:"), QLineEdit::Normal, "0,0,0");
  double x = text.split(',')[0].toDouble();
  double y = text.split(',')[1].toDouble();
  double z = text.split(',')[2].toDouble();

  QElapsedTimer timer;
  timer.start();

  qDebug() << "Rendering took" << timer.elapsed() << "milliseconds";
  QMessageBox messageBox;
  messageBox.information(this, "Error", QString::number(timer.elapsed()));
  messageBox.show();
}

void MainWindow::on_actionRotate_ROI_triggered() {
  // qDebug() << this->AxialViewer->ImageSlice->GetInput()->GetScalarRange()[0];
  // qDebug() << this->AxialViewer->ImageSlice->GetInput()->GetScalarRange()[1];
}

void MainWindow::on_actionAdd_Arc_triggered() {
  double radius = 250;
  double startAngle = 180.1;
  double stopAngle = 179.9;
  QString direction = "CW";
  double arcAngle = 359.8; // stopAngle - startAngle;
  double clipAngle = 360.0 - arcAngle;

  float angle = startAngle - stopAngle;
  float rad = angle * vtkMath::Pi() / 180.0;
  float radOutside = (2 * vtkMath::Pi()) - rad;

  float arc_length = radOutside * radius;

  double xCord1 = radius * cos(vtkMath::RadiansFromDegrees(startAngle));
  double yCord1 = radius * sin(vtkMath::RadiansFromDegrees(startAngle));
  double xCord2 = radius * cos(vtkMath::RadiansFromDegrees(stopAngle));
  double yCord2 = radius * sin(vtkMath::RadiansFromDegrees(stopAngle));

  vtkSmartPointer<vtkAssembly> arcAssembly =
      vtkSmartPointer<vtkAssembly>::New();

  vtkSmartPointer<vtkArcSource> arcSource =
      vtkSmartPointer<vtkArcSource>::New();
  arcSource->SetResolution(360);
  arcSource->SetPoint1(xCord1, yCord1, 0);
  arcSource->SetPoint2(xCord2, yCord2, 0);
  if (clipAngle < arcAngle) {
    arcSource->NegativeOn();
  }

  else if (clipAngle >= arcAngle) {
    arcSource->NegativeOff();
  }
  arcSource->Update();

  // Visualize
  vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(arcSource->GetOutputPort());

  vtkSmartPointer<vtkActor> arcActor = vtkSmartPointer<vtkActor>::New();
  arcActor->SetMapper(mapper);
  arcActor->GetProperty()->SetColor(1, 1, 0);
  arcActor->GetProperty()->SetLineWidth(2.0);
  arcActor->SetPosition(-12, 126, -32);
  arcActor->RotateZ(-90);

  vtkSmartPointer<vtkLineSource> arcStart =
      vtkSmartPointer<vtkLineSource>::New();
  arcStart->SetPoint1(0, 0, 0);
  arcStart->SetPoint2(xCord2, yCord2, 0);

  // Visualize
  vtkSmartPointer<vtkPolyDataMapper> arcStartMapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  arcStartMapper->SetInputConnection(arcStart->GetOutputPort());

  vtkSmartPointer<vtkActor> arcStartActor = vtkSmartPointer<vtkActor>::New();
  arcStartActor->SetMapper(arcStartMapper);
  arcStartActor->GetProperty()->SetColor(1, 0, 0);
  arcStartActor->GetProperty()->SetLineWidth(0.5);
  arcStartActor->SetPosition(-12, 126, -32);
  arcStartActor->RotateZ(-90);

  vtkSmartPointer<vtkLineSource> arcStop =
      vtkSmartPointer<vtkLineSource>::New();
  arcStop->SetPoint2(0, 0, 0);
  arcStop->SetPoint2(xCord1, yCord1, 0);

  // Visualize
  vtkSmartPointer<vtkPolyDataMapper> arcStopMapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  arcStopMapper->SetInputConnection(arcStop->GetOutputPort());

  vtkSmartPointer<vtkActor> arcStopActor = vtkSmartPointer<vtkActor>::New();
  arcStopActor->SetMapper(arcStopMapper);
  arcStopActor->GetProperty()->SetColor(0, 1, 0);
  arcStopActor->GetProperty()->SetLineWidth(0.5);
  arcStopActor->SetPosition(-12, 126, -32);
  arcStopActor->RotateZ(-90);

  arcAssembly->AddPart(arcActor);
  arcAssembly->AddPart(arcStartActor);
  arcAssembly->AddPart(arcStopActor);

  this->BEVViewer->ModelRenderer->AddViewProp(arcAssembly);
  this->BEVViewer->ModelRenderer->GetRenderWindow()->Render();

  this->AxialViewer->ViewRenderer->AddActor(arcAssembly);
  this->AxialViewer->ViewRenderer->ResetCamera();
  this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();

  /***************************************************************/
  double radius2 = 275;
  double startAngle2 = 195;
  double stopAngle2 = 165;
  QString direction2 = "CW";
  double arcAngle2 = 250; // stopAngle - startAngle;
  double clipAngle2 = 360.0 - arcAngle;

  float angle2 = startAngle2 - stopAngle2;
  float rad2 = angle2 * vtkMath::Pi() / 180.0;
  float radOutside2 = (2 * vtkMath::Pi()) - rad;

  float arc_length2 = radOutside2 * radius2;

  double xCord12 = radius2 * cos(vtkMath::RadiansFromDegrees(startAngle2));
  double yCord12 = radius2 * sin(vtkMath::RadiansFromDegrees(startAngle2));
  double xCord22 = radius2 * cos(vtkMath::RadiansFromDegrees(stopAngle2));
  double yCord22 = radius2 * sin(vtkMath::RadiansFromDegrees(stopAngle2));

  vtkSmartPointer<vtkAssembly> arcAssembly2 =
      vtkSmartPointer<vtkAssembly>::New();

  vtkSmartPointer<vtkArcSource> arcSource2 =
      vtkSmartPointer<vtkArcSource>::New();
  arcSource2->SetResolution(360);
  arcSource2->SetPoint1(xCord12, yCord12, 0);
  arcSource2->SetPoint2(xCord22, yCord22, 0);
  if (clipAngle2 < arcAngle2) {
    arcSource2->NegativeOn();
  }

  else if (clipAngle2 >= arcAngle2) {
    arcSource2->NegativeOff();
  }
  arcSource2->Update();

  // Visualize
  vtkSmartPointer<vtkPolyDataMapper> mapper2 =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper2->SetInputConnection(arcSource2->GetOutputPort());

  vtkSmartPointer<vtkActor> arcActor2 = vtkSmartPointer<vtkActor>::New();
  arcActor2->SetMapper(mapper2);
  arcActor2->GetProperty()->SetColor(1, 1, 0);
  arcActor2->GetProperty()->SetLineWidth(2.0);
  arcActor2->SetPosition(-12, 126, -32);
  arcActor2->RotateZ(-90);

  vtkSmartPointer<vtkLineSource> arcStart2 =
      vtkSmartPointer<vtkLineSource>::New();
  arcStart2->SetPoint1(0, 0, 0);
  arcStart2->SetPoint2(xCord22, yCord22, 0);

  // Visualize
  vtkSmartPointer<vtkPolyDataMapper> arcStartMapper2 =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  arcStartMapper2->SetInputConnection(arcStart2->GetOutputPort());

  vtkSmartPointer<vtkActor> arcStartActor2 = vtkSmartPointer<vtkActor>::New();
  arcStartActor2->SetMapper(arcStartMapper2);
  arcStartActor2->GetProperty()->SetColor(1, 0, 0);
  arcStartActor2->GetProperty()->SetLineWidth(0.5);
  arcStartActor2->SetPosition(-12, 126, -32);
  arcStartActor2->RotateZ(-90);

  vtkSmartPointer<vtkLineSource> arcStop2 =
      vtkSmartPointer<vtkLineSource>::New();
  arcStop2->SetPoint2(0, 0, 0);
  arcStop2->SetPoint2(xCord12, yCord12, 0);

  // Visualize
  vtkSmartPointer<vtkPolyDataMapper> arcStopMapper2 =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  arcStopMapper2->SetInputConnection(arcStop2->GetOutputPort());

  vtkSmartPointer<vtkActor> arcStopActor2 = vtkSmartPointer<vtkActor>::New();
  arcStopActor2->SetMapper(arcStopMapper2);
  arcStopActor2->GetProperty()->SetColor(0, 1, 0);
  arcStopActor2->GetProperty()->SetLineWidth(0.5);
  arcStopActor2->SetPosition(-12, 126, -32);
  arcStopActor2->RotateZ(-90);

  arcAssembly2->AddPart(arcActor2);
  arcAssembly2->AddPart(arcStartActor2);
  arcAssembly2->AddPart(arcStopActor2);

  this->BEVViewer->ModelRenderer->AddViewProp(arcAssembly2);
  this->BEVViewer->ModelRenderer->GetRenderWindow()->Render();

  this->AxialViewer->ViewRenderer->AddActor(arcAssembly2);
  this->AxialViewer->ViewRenderer->ResetCamera();
  this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();
}

void MainWindow::on_actionSend_UDP_triggered() {
  //    this->listener->TrackingTarget->DeepCopy(this->MeshList[0]);
  //    this->listener->AxialViewer=this->AxialViewer;
  //    this->listener->SagittalViewer=this->SagittalViewer;
  //    this->listener->CoronalViewer=this->CoronalViewer;
  //    this->listener->BEVViewer=this->BEVViewer;
  //    this->listener->StartListening();
  //    QApplication::processEvents();
}

void MainWindow::on_actionAbout_QT_triggered() { QMessageBox::aboutQt(this); }

void MainWindow::on_actionIP_COnfiguration_triggered() {
  IPConfigDialog *IPDialog = new IPConfigDialog(this);
  IPDialog->exec();
  delete IPDialog;
}

void MainWindow::on_actionStart_triggered() {
  this->ui->statusBar->showMessage("Listening to KIM");

  this->listener = new UDPListener(this);
  this->listener->parent = this;

  this->listener->ROINames = this->ROINames;
  this->listener->ROIColors = this->ROIColors;
  this->listener->MeshList = this->MeshList;

  this->listener->AxialViewer = this->AxialViewer;
  this->listener->SagittalViewer = this->SagittalViewer;
  this->listener->CoronalViewer = this->CoronalViewer;
  this->listener->BEVViewer = this->BEVViewer;
  this->listener->StartListening();

  // QApplication::processEvents();
}

void MainWindow::on_actionStop_triggered() {
  this->ui->statusBar->clearMessage();
  if (this->listener->connectionState) {
    this->listener->StopListening();
  }
  delete this->listener;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  QMessageBox::StandardButton resBtn = QMessageBox::question(
      this, "KIMView", tr("Are you sure you want to close?\n"),
      QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
  if (resBtn != QMessageBox::Yes) {
    event->ignore();
  } else {
    if (this->listener->connectionState) {
      this->listener->StopListening();
      delete this->listener;
    }

    event->accept();
  }
}

void MainWindow::on_actionWL_WW_triggered() {
  WLWWDialog *WLWlDlg = new WLWWDialog(this);
  WLWlDlg->AxialViewer = this->AxialViewer;
  WLWlDlg->SagittalViewer = this->SagittalViewer;
  WLWlDlg->CoronalViewer = this->CoronalViewer;
  WLWlDlg->show();
}

void MainWindow::on_actionAbout_triggered() {
  AboutDialog *abtDlg = new AboutDialog(this);
  abtDlg->exec();
}

void MainWindow::on_actionPlan_triggered() {
  PlanReader *myPlanReader = new PlanReader();
  myPlanReader->readRTPlan();
  // qDebug() << "Frs: " << myPlanReader->fractionsPlanned;
  // qDebug() << "No. of beams: " << myPlanReader->numOfBeams;
  // qDebug() << "Plan name: " << myPlanReader->planDetailStruct[0].beamType;
  // qDebug() << myPlanReader->planDetailStruct[0].beamAngle;
  // qDebug() << myPlanReader->planDetailStruct[0].beamStopAngle;

  this->ui->tableWidget->setRowCount(0);

  this->ui->treeWidget->topLevelItem(2)->setText(
      0, QString(myPlanReader->planLabel));

  this->ui->tableWidget->setRowCount(myPlanReader->numOfBeams);

  for (int i = 0; i < myPlanReader->numOfBeams; i++) {
    // Add a treeWidegtItem for each beam
    QTreeWidgetItem *wItem = new QTreeWidgetItem(
        (QTreeWidget *)nullptr,
        QStringList(QString(myPlanReader->planDetailStruct[i].beamName)));
    wItem->setCheckState(0, Qt::Checked);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/Icons/Plan.png"), QSize(12, 12),
                 QIcon::Normal, QIcon::Off);
    wItem->setIcon(0, icon);
    this->ui->treeWidget->topLevelItem(2)->addChild(wItem);
    this->ui->treeWidget->expandAll();

    QTableWidgetItem *item1 = new QTableWidgetItem;
    item1->setText(QString::number(myPlanReader->planDetailStruct[i].beamNum));
    item1->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 0, item1);

    QTableWidgetItem *item2 = new QTableWidgetItem;
    item2->setText(myPlanReader->planDetailStruct[i].beamName.toLatin1());
    item2->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 1, item2);

    QTableWidgetItem *item3 = new QTableWidgetItem;
    item3->setText(tr(myPlanReader->planDetailStruct[i].mcName.toLatin1()));
    item3->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 2, item3);

    QTableWidgetItem *item4 = new QTableWidgetItem;
    item4->setText(tr(myPlanReader->planDetailStruct[i].beamType.toLatin1()));
    item4->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 3, item4);

    QTableWidgetItem *item5 = new QTableWidgetItem;
    item5->setText(
        QString::number(myPlanReader->planDetailStruct[i].beamEnergy));
    item5->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 4, item5);

    // gantry info goes here later.......................

    QTableWidgetItem *item6 = new QTableWidgetItem;
    item6->setText(
        QString::number(myPlanReader->planDetailStruct[i].beamAngle, 'f', 1));
    item6->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 5, item6);

    QTableWidgetItem *item7 = new QTableWidgetItem;
    item7->setText(QString::number(
        myPlanReader->planDetailStruct[i].beamStopAngle, 'f', 1));
    item7->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 6, item7);

    QTableWidgetItem *item8 = new QTableWidgetItem;
    item8->setText(
        tr(myPlanReader->planDetailStruct[i].arcDirection.toLatin1()));
    item8->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 7, item8);

    QTableWidgetItem *item9 = new QTableWidgetItem;
    item9->setText(
        QString::number(myPlanReader->planDetailStruct[i].collAngle, 'f', 1));
    item9->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 8, item9);

    QTableWidgetItem *item10 = new QTableWidgetItem;
    item10->setText(
        QString::number(myPlanReader->planDetailStruct[i].couchAngle, 'f', 1));
    item10->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 9, item10);

    QTableWidgetItem *item11 = new QTableWidgetItem;
    item11->setText(
        QString::number(myPlanReader->planDetailStruct[i].fieldX1, 'f', 1));
    item11->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 10, item11);

    QTableWidgetItem *item12 = new QTableWidgetItem;
    item12->setText(
        QString::number(myPlanReader->planDetailStruct[i].fieldX2, 'f', 1));
    item12->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 11, item12);

    QTableWidgetItem *item13 = new QTableWidgetItem;
    item13->setText(
        QString::number(myPlanReader->planDetailStruct[i].fieldY1, 'f', 1));
    item13->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 12, item13);

    QTableWidgetItem *item14 = new QTableWidgetItem;
    item14->setText(
        QString::number(myPlanReader->planDetailStruct[i].fieldY2, 'f', 1));
    item14->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 13, item14);

    QTableWidgetItem *item15 = new QTableWidgetItem;
    item15->setText(
        QString::number(myPlanReader->planDetailStruct[i].icX, 'f', 1));
    item15->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 14, item15);

    QTableWidgetItem *item16 = new QTableWidgetItem;
    item16->setText(
        QString::number(myPlanReader->planDetailStruct[i].icY, 'f', 1));
    item16->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 15, item16);

    QTableWidgetItem *item17 = new QTableWidgetItem;
    item17->setText(
        QString::number(myPlanReader->planDetailStruct[i].icZ, 'f', 1));
    item17->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 16, item17);

    QTableWidgetItem *item18 = new QTableWidgetItem;
    item18->setText(
        QString::number(myPlanReader->planDetailStruct[i].ssd, 'f', 1));
    item18->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 17, item18);

    QTableWidgetItem *item19 = new QTableWidgetItem;
    item19->setText(
        QString::number(myPlanReader->planDetailStruct[i].mu, 'f', 1));
    item19->setTextAlignment(Qt::AlignCenter);
    this->ui->tableWidget->setItem(i, 18, item19);
  }

  delete myPlanReader;
}
