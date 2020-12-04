#include "udplistener.h"

#include<udplistener.h>
#include<QUdpSocket>
#include<QElapsedTimer>

//Tracking includes
#include<vtkProperty.h>
#include<imageviewer2d.h>
#include<vtkRenderWindow.h>

UDPListener::UDPListener(QObject *parent) : QObject(parent)
{
    // create a QUDP socket
    socket = new QUdpSocket(this);

    //Instantiate tracking members
    this->TrackingTranform=vtkSmartPointer<vtkTransform>::New();
    this->TrackingPolydataTransform=vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    this->TrackingPolyData=vtkSmartPointer<vtkPolyData>::New();
    this->TrackingTarget=vtkSmartPointer<vtkPolyData>::New();

    this->TrackingActor3D=vtkSmartPointer<vtkActor>::New();
    this->TrackingActor3D->GetProperty()->SetColor(1,1,0);
    this->TrackingActorAxial=vtkSmartPointer<vtkActor>::New();
    this->TrackingActorSagittal=vtkSmartPointer<vtkActor>::New();
    this->TrackingActorCoronal=vtkSmartPointer<vtkActor>::New();


    this->TrackingMapper=vtkSmartPointer<vtkPolyDataMapper>::New();
    this->TrackingMapper->ImmediateModeRenderingOn();
}

UDPListener::~UDPListener()
{
    socket->close();
    socket->deleteLater();
}

void UDPListener::HelloUDP()
{
    QByteArray Data;
    Data.append("Hello from UDP");

    // Sends the datagram datagram
    // to the host address and at port.
    // qint64 QUdpSocket::writeDatagram(const QByteArray & datagram,
    //                      const QHostAddress & host, quint16 port)
    socket->writeDatagram(Data, QHostAddress::LocalHost,45617);
}

void UDPListener::readMessage()
{
    QElapsedTimer timer;
    timer.start();

    //When data comes in
    QByteArray buffer;
    buffer.resize(socket->pendingDatagramSize());

    QHostAddress sender;
    quint16 senderPort;

    // qint64 QUdpSocket::readDatagram(char * data, qint64 maxSize,
    //                 QHostAddress * address = 0, quint16 * port = 0)
    // Receives a datagram no larger than maxSize bytes and stores it in data.
    // The sender's host address and port is stored in *address and *port
    // (unless the pointers are 0).

    socket->readDatagram(buffer.data(), buffer.size(),
                         &sender, &senderPort);

    //qDebug() << "Message from: " << sender.toString();
    //qDebug() << "Message port: " << senderPort;
    //qDebug() << "Message: " << buffer;
    this->shifts[0]=buffer.split(',')[0].toDouble();
    this->shifts[1]=buffer.split(',')[1].toDouble();
    this->shifts[2]=buffer.split(',')[2].toDouble();
    qDebug()<<"Shifts: "<<this->shifts[0]<<""<<this->shifts[1]<<""<<this->shifts[2];

    /*******************************Tracking.......................................*/
    //Remove previous last actors
    this->BEVViewer->ModelRenderer->RemoveViewProp(this->TrackingActor3D);
    this->AxialViewer->ViewRenderer->RemoveViewProp(this->TrackingActorAxial);
    this->SagittalViewer->ViewRenderer->RemoveViewProp(this->TrackingActorSagittal);
    this->CoronalViewer->ViewRenderer->RemoveViewProp(this->TrackingActorCoronal);

    //Transform actor
    this->TrackingTranform->Identity();
    this->TrackingTranform->Translate(this->shifts);
    this->TrackingPolydataTransform->SetTransform(this->TrackingTranform);
    this->TrackingPolydataTransform->SetInputData(this->TrackingTarget);
    this->TrackingMapper->SetInputConnection(this->TrackingPolydataTransform->GetOutputPort());
    this->TrackingActor3D->SetMapper(this->TrackingMapper);

    this->BEVViewer->ModelRenderer->AddViewProp(this->TrackingActor3D);
    this->BEVViewer->ModelRenderer->GetRenderWindow()->Render();

    this->TrackingPolyData->DeepCopy(this->TrackingPolydataTransform->GetOutput());

    //Update 2D views
    this->TrackingActorAxial=this->AxialViewer->CutROI(this->TrackingPolyData,this->AxialViewer->SliceLoc,1,1,0,0);
    this->TrackingActorAxial->GetProperty()->SetLineWidth(3.0);
    this->AxialViewer->ViewRenderer->AddActor(this->TrackingActorAxial);
    this->AxialViewer->ViewRenderer->GetRenderWindow()->Render();

    this->TrackingActorSagittal=this->SagittalViewer->CutROI(this->TrackingPolyData,this->SagittalViewer->SliceLoc,1,1,0,1);
    this->TrackingActorSagittal->GetProperty()->SetLineWidth(3.0);
    this->SagittalViewer->ViewRenderer->AddActor(this->TrackingActorSagittal);
    this->SagittalViewer->ViewRenderer->GetRenderWindow()->Render();

    this->TrackingActorCoronal=this->CoronalViewer->CutROI(this->TrackingPolyData,this->CoronalViewer->SliceLoc,1,1,0,2);
    this->TrackingActorCoronal->GetProperty()->SetLineWidth(3.0);
    this->CoronalViewer->ViewRenderer->AddActor(this->TrackingActorCoronal);
    this->CoronalViewer->ViewRenderer->GetRenderWindow()->Render();

    qDebug() <<"Rendering took" << timer.elapsed() << "milliseconds";


}

void UDPListener::StartListening()
{
    qDebug()<<"Start";
    socket->bind(QHostAddress::LocalHost,45617);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readMessage()));

}

void UDPListener::StopListening()
{
    socket->close();
}




