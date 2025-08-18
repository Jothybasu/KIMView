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

#include "vtklinecallbackdose.h"

#include <vtkActor.h>
#include <vtkArrayData.h>
#include <vtkCommand.h>
#include <vtkContextScene.h>
#include <vtkDataObjectToDataSetFilter.h>
#include <vtkDatabaseToTableReader.h>
#include <vtkFloatArray.h>
#include <vtkLineRepresentation.h>
#include <vtkLineWidget2.h>
#include <vtkPlot.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProbeFilter.h>
#include <vtkProperty2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTable.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkXYPlotActor.h>

#include <QChartView>
#include <QDebug>
#include <QLineSeries>
#include <QSplineSeries>

#include "doseprofiledialog.h"

//QT_CHARTS_USE_NAMESPACE

vtkLineCallbackDose::vtkLineCallbackDose(QWidget *parent)
{
    this->doseProfiler = new DoseProfileDialog(parent);
    this->doseProfiler->show();
    this->trX = 0.0;
    this->trY = 0.0;
    this->trZ = 0.0;
}

vtkLineCallbackDose::~vtkLineCallbackDose()
{
    this->trX = 0.0;
    this->trY = 0.0;
    this->trZ = 0.0;
    delete doseProfiler;
}

vtkLineCallbackDose *vtkLineCallbackDose::New(QWidget *parent)
{
    return new vtkLineCallbackDose(parent);
}

void vtkLineCallbackDose::Execute(vtkObject *caller, unsigned long, void *)
{
      // qDebug()<<"'Executing...";
      vtkLineWidget2 *lineWidget = reinterpret_cast<vtkLineWidget2 *>(caller);

      // Get the actual box coordinates of the line
      this->lineData = vtkSmartPointer<vtkPolyData>::New();
      static_cast<vtkLineRepresentation *>(lineWidget->GetRepresentation())
          ->GetPolyData(this->lineData);
      this->distance =
          static_cast<vtkLineRepresentation *>(lineWidget->GetRepresentation())
              ->GetDistance();
      // qDebug()<<this->distance<<"Distance";

      vtkSmartPointer<vtkProbeFilter> pfDose =
          vtkSmartPointer<vtkProbeFilter>::New();

      // Axial
      if (this->SliceOrientation == 0) {
        this->transformPolyData();

        pfDose->SetInputData(this->lineData);
        pfDose->SetSourceData(this->dose);
        pfDose->Update();

      }

      // Sagittal
      else if (this->SliceOrientation == 1) {
        // Start modifying line data
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

        int npts = this->lineData->GetNumberOfPoints();
        points->SetNumberOfPoints(npts);
        cells->InsertNextCell(npts);
        for (int p = 0; p < npts; p++) {
          double pt[3];
          this->lineData->GetPoint(p, pt);
          points->SetPoint(p, this->trX, pt[0], -pt[1]);
          cells->InsertCellPoint(p);
        }
        cells->InsertCellPoint(npts);

        vtkSmartPointer<vtkPolyData> newLineData =
            vtkSmartPointer<vtkPolyData>::New();
        newLineData->Initialize();
        newLineData->SetLines(cells);
        newLineData->SetPoints(points);
        // End of modifying line data

        this->transformPolyData();

        pfDose->SetInputData(newLineData);
        pfDose->SetSourceData(this->dose);
        pfDose->Update();

      }

      // Coronal
      else if (this->SliceOrientation == 2) {
        // Start modifying line data
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

        int npts = this->lineData->GetNumberOfPoints();
        points->SetNumberOfPoints(npts);
        cells->InsertNextCell(npts);
        for (int p = 0; p < npts; p++) {
          double pt[3];
          this->lineData->GetPoint(p, pt);
          points->SetPoint(p, pt[0], this->trY, -pt[1]);
          cells->InsertCellPoint(p);
        }
        cells->InsertCellPoint(npts);

        vtkSmartPointer<vtkPolyData> newLineData =
            vtkSmartPointer<vtkPolyData>::New();
        newLineData->Initialize();
        newLineData->SetLines(cells);
        newLineData->SetPoints(points);
        // End of modifying line data

        this->transformPolyData();

        pfDose->SetInputData(newLineData);
        pfDose->SetSourceData(this->dose);
        pfDose->Update();
      }

      vtkSmartPointer<vtkDataObjectToDataSetFilter> polyDataToDoseTable =
          vtkSmartPointer<vtkDataObjectToDataSetFilter>::New();
      polyDataToDoseTable->SetInputConnection(pfDose->GetOutputPort());
      polyDataToDoseTable->SetDataSetTypeToStructuredPoints();
      polyDataToDoseTable->SetPointComponent(0,"Points",0);
      polyDataToDoseTable->SetPointComponent(1,"Points",1);
      polyDataToDoseTable->SetPointComponent(2,"Points",2);

      //polyDataToDoseTable->SetFieldType(vtkDataObjectToDataSetFilter::POINT_DATA);
      polyDataToDoseTable->Update();

      this->doseProfiler->DoseData->clear();

      this->SplineSeries = new QSplineSeries;
      this->SplineSeries->setName("Dose (Spline interpolated)");
      QPen pen = SplineSeries->pen();
      pen.setWidth(2);
      pen.setBrush(QBrush(QColor(255, 127, 39)));
      SplineSeries->setPen(pen);

      int numPoints = this->lineData->GetNumberOfPoints();
      for (int i = 0; i < numPoints; ++i) {
        float dist;
        dist = (this->distance / 1000) * i;
        //float dose = polyDataToDoseTable->GetOutput()->GetValue(i, 0).ToFloat();
        //float dose = polyDataToDoseTable->GetOutput()->GetPointData()->GetScalars();

        //this->SplineSeries->append(dist, dose);
      }

      this->doseProfiler->DoseData = this->SplineSeries;
      this->doseProfiler->plotProfile();
    }

    void vtkLineCallbackDose::transformPolyData() {
      vtkSmartPointer<vtkTransform> transform =
          vtkSmartPointer<vtkTransform>::New();
      // qDebug()<<this->trX<<this->trY<<this->trZ<<"tr";
      transform->Translate(this->trX, this->trY, this->trZ);
      transform->Update();

      vtkSmartPointer<vtkTransformPolyDataFilter> transF =
          vtkSmartPointer<vtkTransformPolyDataFilter>::New();
      transF->SetInputData(this->lineData);
      transF->SetTransform(transform);
      transF->Update();
      this->lineData = transF->GetOutput();
}
