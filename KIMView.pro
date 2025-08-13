QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets charts network

CONFIG += c++19

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES +=\
    aboutdialog.cpp \
    alignimageinteractorstyle.cpp \
    bevwidget.cpp \
    createobjects.cpp \
    ipconfigdialog.cpp \
    doseprofiledialog.cpp \
    dvhcalc.cpp \
    dvhdialog.cpp \
    imageviewer2d.cpp \
    planreader.cpp \
    presetwlwwdialog.cpp \
    selecttargetdialog.cpp \
    utilities.cpp   \
    main.cpp \
    mainwindow.cpp \
    meshreader.cpp \
    qDoubleSlider.cpp \
    rangesliderdialog.cpp \
    wlwwdialog.cpp  \
    rtstructreaderdialog.cpp \    
    udplistener.cpp \
    vtkinteractorstyleimagecustom.cpp \    
    vtklinecallbackdose.cpp



HEADERS +=\    
    aboutdialog.h \
    alignimageinteractorstyle.h \
    bevwidget.h \
    ipconfigdialog.h \
    createobjects.h \
    doseprofiledialog.h \
    dvhcalc.h \
    planreader.h \
    presetwlwwdialog.h \
    selecttargetdialog.h \
    utilities.h \
    dvhdialog.h \
    wlwwdialog.h \
    imageviewer2d.h \   
    mainwindow.h \
    meshreader.h \
    qDoubleSlider.h \
    rangesliderdialog.h \
    rtstructreaderdialog.h \   
    udplistener.h \
    vtkinteractorstyleimagecustom.h \   
    vtklinecallbackdose.h



FORMS += \
    aboutdialog.ui \
    bevwidget.ui \
    doseprofiledialog.ui \
    dvhdialog.ui \
    imageviewer2d.ui \
    ipconfigdialog.ui \
    mainwindow.ui \
    presetwlwwdialog.ui \
    rangesliderdialog.ui \
    rtstructreaderdialog.ui \
    selecttargetdialog.ui \
    wlwwdialog.ui

TRANSLATIONS += 
    KIMView_en_AU.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


INCLUDEPATH+= C:\Data\Libs\VTK\Install\include\vtk-9.5

LIBS+= -LC:\Data\Libs\VTK\Install\lib -lvtkcgns-9.5  -lvtkChartsCore-9.5  -lvtkCommonColor-9.5  -lvtkCommonComputationalGeometry-9.5\
-lvtkCommonCore-9.5 -lvtkCommonDataModel-9.5  -lvtkCommonExecutionModel-9.5  -lvtkCommonMath-9.5  -lvtkCommonMisc-9.5  -lvtkCommonSystem-9.5\
-lvtkCommonTransforms-9.5  -lvtkDICOMParser-9.5  -lvtkDomainsChemistry-9.5  -lvtkDomainsChemistryOpenGL2-9.5\
-lvtkdoubleconversion-9.5  -lvtkexodusII-9.5  -lvtkexpat-9.5  -lvtkFiltersAMR-9.5  -lvtkFiltersCellGrid-9.5\
-lvtkFiltersCore-9.5  -lvtkFiltersExtraction-9.5  -lvtkFiltersFlowPaths-9.5  -lvtkFiltersGeneral-9.5  -lvtkFiltersGeneric-9.5\
-lvtkFiltersGeometry-9.5  -lvtkFiltersGeometryPreview-9.5  -lvtkFiltersHybrid-9.5  -lvtkFiltersHyperTree-9.5  -lvtkFiltersImaging-9.5\
-lvtkFiltersModeling-9.5  -lvtkFiltersParallel-9.5  -lvtkFiltersParallelImaging-9.5  -lvtkFiltersPoints-9.5  -lvtkFiltersProgrammable-9.5\
-lvtkFiltersReduction-9.5  -lvtkFiltersSelection-9.5  -lvtkFiltersSMP-9.5  -lvtkFiltersSources-9.5  -lvtkFiltersStatistics-9.5\
-lvtkFiltersTemporal-9.5  -lvtkFiltersTensor-9.5  -lvtkFiltersTexture-9.5  -lvtkFiltersTopology-9.5  -lvtkFiltersVerdict-9.5\
-lvtkfmt-9.5  -lvtkfreetype-9.5  -lvtkGeovisCore-9.5  -lvtkgl2ps-9.5  -lvtkglad-9.5  -lvtkGUISupportQt-9.5  -lvtkGUISupportQtQuick-9.5\
-lvtkGUISupportQtSQL-9.5  -lvtkhdf5-9.5  -lvtkhdf5_hl-9.5  -lvtkImagingColor-9.5  -lvtkImagingCore-9.5  -lvtkImagingFourier-9.5\
-lvtkImagingGeneral-9.5  -lvtkImagingHybrid-9.5  -lvtkImagingMath-9.5  -lvtkImagingMorphological-9.5  -lvtkImagingSources-9.5\
-lvtkImagingStatistics-9.5  -lvtkImagingStencil-9.5  -lvtkInfovisCore-9.5  -lvtkInfovisLayout-9.5  -lvtkInteractionImage-9.5\
-lvtkInteractionStyle-9.5  -lvtkInteractionWidgets-9.5  -lvtkIOAMR-9.5  -lvtkIOAsynchronous-9.5  -lvtkIOCellGrid-9.5  -lvtkIOCesium3DTiles-9.5\
-lvtkIOCGNSReader-9.5  -lvtkIOChemistry-9.5  -lvtkIOCityGML-9.5  -lvtkIOCONVERGECFD-9.5  -lvtkIOCore-9.5  -lvtkIOEngys-9.5\
-lvtkIOEnSight-9.5  -lvtkIOERF-9.5  -lvtkIOExodus-9.5  -lvtkIOExport-9.5  -lvtkIOExportGL2PS-9.5  -lvtkIOExportPDF-9.5  -lvtkIOFDS-9.5\
-lvtkIOFLUENTCFF-9.5  -lvtkIOGeometry-9.5  -lvtkIOHDF-9.5  -lvtkIOImage-9.5  -lvtkIOImport-9.5  -lvtkIOInfovis-9.5  -lvtkIOIOSS-9.5\
-lvtkIOLANLX3D-9.5  -lvtkIOLegacy-9.5  -lvtkIOLSDyna-9.5  -lvtkIOMINC-9.5  -lvtkIOMotionFX-9.5  -lvtkIOMovie-9.5  -lvtkIONetCDF-9.5\
-lvtkIOOggTheora-9.5  -lvtkIOParallel-9.5  -lvtkIOParallelXML-9.5  -lvtkIOPLY-9.5  -lvtkIOSegY-9.5  -lvtkIOSQL-9.5  -lvtkioss-9.5\
-lvtkIOTecplotTable-9.5  -lvtkIOVeraOut-9.5  -lvtkIOVideo-9.5  -lvtkIOXML-9.5  -lvtkIOXMLParser-9.5  -lvtkjpeg-9.5  -lvtkjsoncpp-9.5\
-lvtkkissfft-9.5  -lvtklibharu-9.5  -lvtklibproj-9.5  -lvtklibxml2-9.5  -lvtkloguru-9.5  -lvtklz4-9.5  -lvtklzma-9.5  -lvtkmetaio-9.5\
-lvtknetcdf-9.5  -lvtkogg-9.5  -lvtkParallelCore-9.5  -lvtkParallelDIY-9.5  -lvtkpng-9.5  -lvtkpugixml-9.5  -lvtkRenderingAnnotation-9.5\
-lvtkRenderingCellGrid-9.5  -lvtkRenderingContext2D-9.5  -lvtkRenderingContextOpenGL2-9.5  -lvtkRenderingCore-9.5  -lvtkRenderingFreeType-9.5\
-lvtkRenderingGL2PSOpenGL2-9.5  -lvtkRenderingGridAxes-9.5  -lvtkRenderingHyperTreeGrid-9.5  -lvtkRenderingImage-9.5  -lvtkRenderingLabel-9.5\
-lvtkRenderingLICOpenGL2-9.5  -lvtkRenderingLOD-9.5  -lvtkRenderingOpenGL2-9.5  -lvtkRenderingQt-9.5  -lvtkRenderingSceneGraph-9.5\
-lvtkRenderingUI-9.5  -lvtkRenderingVolume-9.5  -lvtkRenderingVolumeOpenGL2-9.5  -lvtkRenderingVtkJS-9.5  -lvtksqlite-9.5\
-lvtksys-9.5  -lvtkTestingCore-9.5  -lvtkTestingRendering-9.5  -lvtktheora-9.5  -lvtktiff-9.5  -lvtktoken-9.5  -lvtkverdict-9.5\
-lvtkViewsContext2D-9.5  -lvtkViewsCore-9.5  -lvtkViewsInfovis-9.5  -lvtkViewsQt-9.5  -lvtkWrappingTools-9.5  -lvtkzlib-9.5\

INCLUDEPATH+= C:\Data\Libs\ITK\Install\include\ITK-5.4

LIBS+= -LC:\Data\Libs\ITK\Install\lib   -litkAdaptiveDenoising-5.4 -litkAnalyzeObjectLabelMap-5.4 -litkColormap-5.4 -litkCommon-5.4\
-litkConvolution-5.4 -litkDeformableMesh-5.4  -litkDenoising-5.4  -litkDICOMParser-5.4  -litkDiffusionTensorImage-5.4  -litkdouble-conversion-5.4\
-litkEXPAT-5.4  -litkFastMarching-5.4  -litkFFT-5.4  -litkgdcmcharls-5.4  -litkgdcmCommon-5.4  -litkgdcmDICT-5.4\
-litkgdcmDSED-5.4  -litkgdcmIOD-5.4  -litkgdcmjpeg12-5.4  -litkgdcmjpeg16-5.4  -litkgdcmjpeg8-5.4  -litkgdcmMEXD-5.4\
-litkgdcmMSFF-5.4  -litkgdcmopenjp2-5.4  -litkgdcmsocketxx-5.4  -litkgiftiio-5.4  -litkImageFeature-5.4  -litkImageIntensity-5.4\
-litkIOBioRad-5.4  -litkIOBMP-5.4  -litkIOBruker-5.4  -litkIOCSV-5.4  -litkIOGDCM-5.4  -litkIOGE-5.4  -litkIOGIPL-5.4  -litkIOHDF5-5.4\
-litkIOImageBase-5.4  -litkIOIPL-5.4  -litkIOJPEG-5.4  -litkIOJPEG2000-5.4  -litkIOLSM-5.4  -litkIOMeshBase-5.4  -litkIOMeshBYU-5.4\
-litkIOMeshFreeSurfer-5.4  -litkIOMeshGifti-5.4  -litkIOMeshOBJ-5.4  -litkIOMeshOFF-5.4  -litkIOMeshVTK-5.4  -litkIOMeta-5.4\
-litkIOMINC-5.4  -litkIOMRC-5.4  -litkIONIFTI-5.4  -litkIONRRD-5.4  -litkIOPNG-5.4  -litkIOSiemens-5.4  -litkIOSpatialObjects-5.4\
-litkIOStimulate-5.4  -litkIOTIFF-5.4  -litkIOTransformBase-5.4  -litkIOTransformHDF5-5.4  -litkIOTransformInsightLegacy-5.4  -litkIOTransformMatlab-5.4\
-litkIOVTK-5.4  -litkIOXML-5.4  -litkjpeg-5.4  -litkKLMRegionGrowing-5.4  -litkLabelMap-5.4  -litklbfgs-5.4  -litkMarkovRandomFieldsClassifiers-5.4\
-litkMathematicalMorphology-5.4  -litkMesh-5.4  -litkMetaIO-5.4  -litkminc2-5.4  -litkNetlibSlatec-5.4  -litkniftiio-5.4  -litkNrrdIO-5.4\
-litkopenjpeg-5.4  -litkOptimizers-5.4  -litkOptimizersv4-5.4  -litkPath-5.4  -litkPDEDeformableRegistration-5.4  -litkpng-5.4  -litkPolynomials-5.4\
-litkQuadEdgeMesh-5.4  -litkQuadEdgeMeshFiltering-5.4  -litkRegionGrowing-5.4  -litkRegistrationMethodsv4-5.4  -litkSmoothing-5.4  -litkSpatialObjects-5.4\
-litkStatistics-5.4  -litksys-5.4  -litkTestKernel-5.4  -litktestlib-5.4  -litktiff-5.4  -litkTransform-5.4  -litkTransformFactory-5.4  -litkv3p_netlib-5.4\
-litkvcl-5.4  -litkVideoCore-5.4  -litkVideoIO-5.4  -litkvnl-5.4  -litkVNLInstantiation-5.4  -litkvnl_algo-5.4  -litkVTK-5.4  -litkVtkGlue-5.4\
-litkWatersheds-5.4  -litkzlib-5.4  -litkznz-5.4

##Needed to link ITK libs. This is advanced windows api
LIBS += advapi32.lib

INCLUDEPATH+= C:\Data\Libs\GDCM\Install\include\gdcm-3.0

LIBS+= -LC:\Data\Libs\GDCM\Install\lib -lgdcmcharls  -lgdcmCommon  -lgdcmDICT  -lgdcmDSED -lgdcmexpat  -lgdcmgetopt  -lgdcmIOD\
-lgdcmjpeg12  -lgdcmjpeg16 -lgdcmjpeg8  -lgdcmMEXD  -lgdcmMSFF  -lgdcmopenjp2  -lgdcmzlib -lsocketxx

DISTFILES += \    
    Human.vtp \
    LicenseTemplate \
    Notes \
    Styles/AMOLED.qss \
    Styles/Aqua.qss \
    Styles/ConsoleStyle.qss \
    Styles/ElegantDark.qss \
    Styles/ImageX.qss \
    Styles/ManjaroMix.qss \
    Styles/MaterialDark.qss \
    Styles/darkstyle.qss

RESOURCES += \
    RC.qrc
