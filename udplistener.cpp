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

#include "udplistener.h"

#include <selecttargetdialog.h>
#include <udplistener.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QSettings>
#include <QTextBrowser>
#include <QUdpSocket>

// Tracking includes
#include <imageviewer2d.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>

UDPListener::UDPListener(QObject *parent)
    : QObject(parent)
{
    // create a QUDP socket
    socket = new QUdpSocket(this);

    // Instantiate tracking members
    this->TrackingTransform = vtkSmartPointer<vtkTransform>::New();
    this->TrackingPolydataTransform = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    this->TrackingPolyData = vtkSmartPointer<vtkPolyData>::New();
    this->TrackingTarget = vtkSmartPointer<vtkPolyData>::New();

    this->TrackingActor3D = vtkSmartPointer<vtkActor>::New();
    this->TrackingActor3D->GetProperty()->SetColor(1, 1, 0);
    this->TrackingActorAxial = vtkSmartPointer<vtkActor>::New();
    this->TrackingActorSagittal = vtkSmartPointer<vtkActor>::New();
    this->TrackingActorCoronal = vtkSmartPointer<vtkActor>::New();

    this->TrackingMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
}

UDPListener::~UDPListener()
{
    socket->close();
    socket->deleteLater();
}

void UDPListener::HelloUDP()
{
    //    QByteArray Data;
    //    Data.append("Hello from UDP");
    //    //Sends the datagram datagram
    //    socket->writeDatagram(Data, QHostAddress::LocalHost,45617);
    //    qDebug()<<"Writing...";
}

void UDPListener::readMessage()
{
    //    QElapsedTimer timer;
    //    timer.start();

    // When data comes in
    QByteArray datagram;
    datagram.resize(socket->pendingDatagramSize());

    QHostAddress sender;
    quint16 senderPort;

    socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

    //qDebug() << "Message from: " << sender.toString();
    //qDebug() << "Message port: " << senderPort;
    //qDebug() << "Message: " << datagram;

    UDPMsg *UDPShifts = new UDPMsg;

    // Unpack the message
    //double x, y, z, rx, ry, rz, gantry;
    //unsigned short flag;
    std::memcpy(&UDPShifts->shiftX, datagram.data(), sizeof(double));
    std::memcpy(&UDPShifts->shiftY, datagram.data() + sizeof(double), sizeof(double));
    std::memcpy(&UDPShifts->shiftZ, datagram.data() + 2 * sizeof(double), sizeof(double));
    std::memcpy(&UDPShifts->rotationX, datagram.data() + 3 * sizeof(double), sizeof(double));
    std::memcpy(&UDPShifts->rotationY, datagram.data() + 4 * sizeof(double), sizeof(double));
    std::memcpy(&UDPShifts->rotationZ, datagram.data() + 5 * sizeof(double), sizeof(double));
    std::memcpy(&UDPShifts->gantryAngle, datagram.data() + 6 * sizeof(double), sizeof(double));
    std::memcpy(&UDPShifts->beamHold, datagram + 7 * sizeof(double), sizeof(unsigned short));

    // qDebug() << UDPShifts->shiftX << "" << UDPShifts->shiftY << ""
    //          << UDPShifts->shiftZ << "" << UDPShifts->rotationX << ""
    //        << UDPShifts->rotationY << "" << UDPShifts->rotationZ << ""
    //          << UDPShifts->gantryAngle << "" << UDPShifts->beamHold;

    //   The UDP format is [X,Y,Z,Gantry] in IEC(cm) and Varian degrees
    //   IEC to LPS conversion, simple approach as it only supports HFS
    //   orientation now
    this->shifts[0] = UDPShifts->shiftX * 10;  // cm to mm
    this->shifts[1] = -UDPShifts->shiftZ * 10; // cm to mm
    this->shifts[2] = UDPShifts->shiftY * 10;  // cm to mm
    //qDebug() << this->shifts[0] << this->shifts[1] << this->shifts[2];
    //           << UDPShifts->gantryAngle << " :Shifts";

    delete UDPShifts;

    this->UpdateViews();
    QCoreApplication::processEvents();

    //qDebug() <<"Rendering took" << timer.elapsed() << "milliseconds";
}

void UDPListener::StartListening()
{
    // Close any existing connection
    if (socket->state() == QUdpSocket::BoundState) {
        this->StopListening();
        // qDebug() << "Disconnecting...";
    }

    SelectTargetDialog *SelectedTarget = new SelectTargetDialog(this->parent);
    SelectedTarget->ROINames = this->ROINames;
    SelectedTarget->ROIColors = this->ROIColors;
    SelectedTarget->setROINames();
    SelectedTarget->exec();

    if (SelectedTarget->ROISelected) {
        this->SelectROINum = SelectedTarget->selectedROINum;
        this->TrackingTarget->DeepCopy(this->MeshList[this->SelectROINum]);

        // Receiver port
        QSettings settings("ImageX", "KIMView");
        // KIM IP and KIMView port
        QString KIMViewIP = settings.value("KIMViewIP").toString();
        int KIMViewPort = settings.value("KIMViewPort").toInt();

        socket->bind(QHostAddress(KIMViewIP), KIMViewPort);
        connect(socket, SIGNAL(readyRead()), this, SLOT(readMessage()));
        this->connectionState = true;

    }

    else {
        QMessageBox messageBox;
        messageBox.critical(this->parent, "Error", "No target selected");
        messageBox.setFixedSize(500, 200);
    }
}

void UDPListener::StopListening()
{
    if (socket->state() == QUdpSocket::BoundState) {
        socket->close();
        this->connectionState = false;
    }

    try {
        // Remove previous last actors
        this->BEVViewer->ModelRenderer->RemoveViewProp(this->TrackingActor3D);
        this->AxialViewer->ViewRenderer->RemoveViewProp(this->TrackingActorAxial);
        this->SagittalViewer->ViewRenderer->RemoveViewProp(this->TrackingActorSagittal);
        this->CoronalViewer->ViewRenderer->RemoveViewProp(this->TrackingActorCoronal);

        this->BEVViewer->ModelRenderer->GetRenderWindow()->Render();
        this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();
        this->SagittalViewer->ViewRenderer->GetRenderWindow()->Render();
        this->CoronalViewer->ViewRenderer->GetRenderWindow()->Render();

    } catch (...) {
        // qDebug()<<"No ROI exists";
    }
}

void UDPListener::UpdateViews()
{
    /*******************************Tracking.......................................*/

    // Remove previous last actors
    this->BEVViewer->ModelRenderer->RemoveViewProp(this->TrackingActor3D);
    this->AxialViewer->ViewRenderer->RemoveViewProp(this->TrackingActorAxial);
    this->SagittalViewer->ViewRenderer->RemoveViewProp(this->TrackingActorSagittal);
    this->CoronalViewer->ViewRenderer->RemoveViewProp(this->TrackingActorCoronal);

    // Transform actor
    this->TrackingTransform->Identity();
    this->TrackingTransform->Translate(this->shifts);
    this->TrackingPolydataTransform->SetTransform(this->TrackingTransform);
    this->TrackingPolydataTransform->SetInputData(this->TrackingTarget);
    this->TrackingMapper->SetInputConnection(this->TrackingPolydataTransform->GetOutputPort());
    this->TrackingActor3D->SetMapper(this->TrackingMapper);

    this->BEVViewer->ModelRenderer->AddViewProp(this->TrackingActor3D);
    this->BEVViewer->ModelRenderer->GetRenderWindow()->Render();

    this->TrackingPolyData->DeepCopy(this->TrackingPolydataTransform->GetOutput());
    // Update 2D views
    this->TrackingActorAxial = this->AxialViewer->CutROI(this->TrackingPolyData,
                                                         this->AxialViewer->SliceLoc,
                                                         1,
                                                         1,
                                                         0,
                                                         this->AxialViewer->SliceOrientation);
    this->TrackingActorAxial->GetProperty()->SetLineWidth(3.0);
    this->AxialViewer->ViewRenderer->AddActor(this->TrackingActorAxial);
    this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();

    this->TrackingActorSagittal = this->SagittalViewer
                                      ->CutROI(this->TrackingPolyData,
                                               this->SagittalViewer->SliceLoc,
                                               1,
                                               1,
                                               0,
                                               this->SagittalViewer->SliceOrientation);
    this->TrackingActorSagittal->GetProperty()->SetLineWidth(3.0);
    this->SagittalViewer->ViewRenderer->AddActor(this->TrackingActorSagittal);
    this->SagittalViewer->ViewRenderer->GetRenderWindow()->Render();

    this->TrackingActorCoronal = this->CoronalViewer->CutROI(this->TrackingPolyData,
                                                             this->CoronalViewer->SliceLoc,
                                                             1,
                                                             1,
                                                             0,
                                                             this->CoronalViewer->SliceOrientation);
    this->TrackingActorCoronal->GetProperty()->SetLineWidth(3.0);
    this->CoronalViewer->ViewRenderer->AddActor(this->TrackingActorCoronal);
    this->CoronalViewer->ViewRenderer->GetRenderWindow()->Render();
}
