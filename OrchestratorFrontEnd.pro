QT += core
QT -= gui

#CONFIG += c++98
CONFIG += c++11

# these were originally without path.
QMAKE_CC = /home/adr1r17/Prg/mpich-3.2.1/bin/mpicc
QMAKE_CXX = /home/adr1r17/Prg/mpich-3.2.1/bin/mpicxx

TARGET = root
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    ./Source/Parser/i_graph.cpp \
    ./Source/Parser/orchbasedummy.cpp \
    ./Source/Parser/pannotateddef.cpp \
    ./Source/Parser/pcodefragment.cpp \
    ./Source/Parser/pconcretedef.cpp \
    ./Source/Parser/pconcreteinstance.cpp \
    ./Source/Parser/pdeviceinstance.cpp \
    ./Source/Parser/pdevicetype.cpp \
    ./Source/Parser/pedgeinstance.cpp \
    ./Source/Parser/pidatatype.cpp \
    ./Source/Parser/pidatavalue.cpp \
    ./Source/Parser/pigraphbranch.cpp \
    ./Source/Parser/pigraphinstance.cpp \
    ./Source/Parser/pigraphleaf.cpp \
    ./Source/Parser/pigraphobject.cpp \
    ./Source/Parser/pigraphroot.cpp \
    ./Source/Parser/pigraphtype.cpp \
    ./Source/Parser/piinputpin.cpp \
    ./Source/Parser/pioutputpin.cpp \
    ./Source/Parser/pipin.cpp \
    ./Source/Parser/pmessagetype.cpp \
    ./Source/Parser/pmetadatapatch.cpp \
    ./Source/Parser/poetsdatatype.cpp \
    ./Source/Parser/psubobjects.cpp \
    ./Source/Parser/psupervisordevicetype.cpp \
    ./Source/Root/Root.cpp \
    ./Source/Root/RootMain.cpp \
    ./Source/OrchBase/Bin.cpp \
    ./Source/OrchBase/build_defs.cpp \
    ./Source/OrchBase/CMsg_p.cpp \
    ./Source/OrchBase/CFrag.cpp \
    ./Source/OrchBase/Constraints.cpp \
    ./Source/OrchBase/D_graph.cpp \
    ./Source/OrchBase/OrchBase.cpp \   
    ./Source/OrchBase/HardwareConfigurationDeployment/MultiSimpleDeployer.cpp \
    ./Source/OrchBase/HardwareConfigurationDeployment/SimpleDeployer.cpp \
    ./Source/OrchBase/HardwareConfigurationDeployment/Dialect1Deployer.cpp \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderCore.cpp \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderDialect1.cpp \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderDialect3.cpp \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderDialect3Engine.cpp \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderDialect3Getters.cpp \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderDialect3Items.cpp \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderDialect3Sections.cpp \
    ./Source/OrchBase/HardwareFileReader/Validator.cpp \
    ./Source/OrchBase/P_device.cpp \
    ./Source/OrchBase/P_builder.cpp \
    ./Source/OrchBase/P_devtyp.cpp \
    ./Source/OrchBase/P_message.cpp \
    ./Source/OrchBase/P_owner.cpp \
    ./Source/OrchBase/P_pin.cpp \
    ./Source/OrchBase/P_pintyp.cpp \
    ./Source/OrchBase/P_task.cpp \
    ./Source/OrchBase/P_typdcl.cpp \
    ./Source/OrchBase/Placement.cpp \
    ./Source/OrchBase/T_gen.cpp \
    ./Source/OrchBase/P_addr.cpp \
    ./Source/OrchBase/P_super.cpp \
    ./Source/Common/CommonBase.cpp \
    ./Source/Common/Debug.cpp \
    ./Source/Common/DumpUtils.cpp \
    ./Source/Common/Environment.cpp \
    ./Source/Common/P_frame.cpp \
    ./Source/Common/Pglobals.cpp \
    ./Source/Common/PMsg_p.cpp \
    ./Source/Common/ProcMap.cpp \
    ./Source/Common/SoftwareAddress.cpp \
    ./Source/Common/Unrec_t.cpp \
    ./Source/Common/HardwareModel/AddressableItem.cpp \
    ./Source/Common/HardwareModel/HardwareAddress.cpp \
    ./Source/Common/HardwareModel/HardwareAddressFormat.cpp \
    ./Source/Common/HardwareModel/HardwareIterator.cpp \
    ./Source/Common/HardwareModel/P_board.cpp \
    ./Source/Common/HardwareModel/P_box.cpp \
    ./Source/Common/HardwareModel/P_core.cpp \
    ./Source/Common/HardwareModel/P_engine.cpp \
    ./Source/Common/HardwareModel/P_link.cpp \
    ./Source/Common/HardwareModel/P_mailbox.cpp \
    ./Source/Common/HardwareModel/P_thread.cpp \
    ./Source/Common/HardwareModel/P_port.cpp \
    ./Generics/Cli.cpp \
    ./Generics/dfprintf.cpp \
    ./Generics/dumpchan.cpp \
    ./Generics/filename.cpp \
    ./Generics/flat.cpp \
    ./Generics/header.cpp \
    ./Generics/jnj.cpp \
    ./Generics/lex.cpp \
    ./Generics/map2.tpp \
    ./Generics/Msg_p.cpp \
    ./Generics/Msg_p.tpp \
    ./Generics/NameBase.cpp \
    ./Generics/pdigraph.tpp \
    ./Generics/rand.cpp \
    ./Generics/uif.cpp \
    ./Source/NameServer/Ns_el.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    ./Source/Parser/i_graph.h \
    ./Source/Parser/orchbasedummy.h \
    ./Source/Parser/pannotateddef.h \
    ./Source/Parser/pcodefragment.h \
    ./Source/Parser/pconcretedef.h \
    ./Source/Parser/pconcreteinstance.h \
    ./Source/Parser/pdeviceinstance.h \
    ./Source/Parser/pdevicetype.h \
    ./Source/Parser/pedgeinstance.h \
    ./Source/Parser/pidatatype.h \
    ./Source/Parser/pidatavalue.h \
    ./Source/Parser/pigraphbranch.h \
    ./Source/Parser/pigraphinstance.h \
    ./Source/Parser/pigraphleaf.h \
    ./Source/Parser/pigraphobject.h \
    ./Source/Parser/pigraphroot.h \
    ./Source/Parser/pigraphtype.h \
    ./Source/Parser/piinputpin.h \
    ./Source/Parser/pioutputpin.h \
    ./Source/Parser/pipin.h \
    ./Source/Parser/pmessagetype.h \
    ./Source/Parser/pmetadatapatch.h \
    ./Source/Parser/poetsdatatype.h \
    ./Source/Parser/psubobjects.h \
    ./Source/Parser/psupervisordevicetype.h \
    ./Source/Root/Root.h \
    ./Source/Root/RootArgs.h \
    ./Source/Root/P_graph.h \
    ./Source/OrchBase/Bin.h \
    ./Source/OrchBase/build_defs.h \
    ./Source/OrchBase/CFrag.h \
    ./Source/OrchBase/CMsg_p.h \
    ./Source/OrchBase/Constraints.h \
    ./Source/OrchBase/D_graph.h \
    ./Source/OrchBase/OrchBase.h \
    ./Source/OrchBase/P_addr.h \
    ./Source/OrchBase/P_builder.h \
    ./Source/OrchBase/P_device.h \
    ./Source/OrchBase/P_devtyp.h \
    ./Source/OrchBase/Placement.h \
    ./Source/OrchBase/P_message.h \
    ./Source/OrchBase/P_owner.h \
    ./Source/OrchBase/P_pin.h \
    ./Source/OrchBase/P_pintyp.h \
    ./Source/OrchBase/P_super.h \
    ./Source/OrchBase/P_task.h \
    ./Source/OrchBase/P_typdcl.h \
    ./Source/OrchBase/T_gen.h \
    ./Source/OrchBase/tinsel-config.h \
    ./Source/OrchBase/HardwareConfigurationDeployment/MultiSimpleDeployer.h \
    ./Source/OrchBase/HardwareConfigurationDeployment/SimpleDeployer.h \
    ./Source/OrchBase/HardwareConfigurationDeployment/Dialect1Deployer.h \
    ./Source/OrchBase/HardwareFileReader/HardwareFileNotFoundException.h \
    ./Source/OrchBase/HardwareFileReader/HardwareFileNotLoadedException.h \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReader.h \
    ./Source/OrchBase/HardwareFileReader/HardwareFileReaderException.h \
    ./Source/OrchBase/HardwareFileReader/HardwareSemanticException.h \
    ./Source/OrchBase/HardwareFileReader/HardwareSyntaxException.h \
    ./Source/OrchBase/HardwareFileReader/Validator.h \
    ./Source/Common/CommonBase.h \
    ./Source/Common/Debug.h \
    ./Source/Common/DumpUtils.h \
    ./Source/Common/Environment.h \
    ./Source/Common/OrchestratorException.h \
    ./Source/Common/OSFixes.hpp \
    ./Source/Common/P_frame.h \
    ./Source/Common/Pglobals.h \
    ./Source/Common/PMsg_p.hpp \
    ./Source/Common/ProcMap.h \
    ./Source/Common/SoftwareAddress.h \
    ./Source/Common/Unrec_t.h \
    ./Source/Common/HardwareModel/AddressableItem.h \
    ./Source/Common/HardwareModel/HardwareAddress.h \
    ./Source/Common/HardwareModel/HardwareAddressFormat.h \
    ./Source/Common/HardwareModel/HardwareIterator.h \
    ./Source/Common/HardwareModel/HardwareModel.h \
    ./Source/Common/HardwareModel/InvalidAddressException.h \
    ./Source/Common/HardwareModel/IteratorException.h \
    ./Source/Common/HardwareModel/MissingAddressException.h \
    ./Source/Common/HardwareModel/OwnershipException.h \
    ./Source/Common/HardwareModel/P_board.h \
    ./Source/Common/HardwareModel/P_box.h \
    ./Source/Common/HardwareModel/P_core.h \
    ./Source/Common/HardwareModel/P_engine.h \
    ./Source/Common/HardwareModel/P_link.h \
    ./Source/Common/HardwareModel/P_mailbox.h \
    ./Source/Common/HardwareModel/P_port.h \
    ./Source/Common/HardwareModel/P_thread.h \
    ./Generics/Cli.h \
    ./Generics/dfprintf.h \
    ./Generics/dumpchan.h \
    ./Generics/e.h \
    ./Generics/filename.h \
    ./Generics/flat.h \
    ./Generics/header.h \
    ./Generics/jnj.h \
    ./Generics/lex.h \
    ./Generics/macros.h \
    ./Generics/map2.h \
    ./Generics/Msg_p.hpp \
    ./Generics/NameBase.h \
    ./Generics/pdigraph.hpp \
    ./Generics/rand.h \
    ./Generics/uif.h \
    ./Source/Injector/Injector.h \
    ./Source/NameServer/Ns_el.h

# mpich common install directory added
LIBS += "-L/home/adr1r17/Prg/mpich-3.2.1/lib" "-L/usr/lib" -lmpi

# mpi include path changed
INCLUDEPATH += ./Source/Common \
    ./Source/Common/HardwareModel \
    ./Source/OrchBase \
    ./Source/OrchBase/HardwareConfigurationDeployment \
    ./Source/OrchBase/HardwareFileReader \
    ./Source/Parser \
    ./Source/NameServer \
    ./Source/Injector \
    ./Generics \
    /home/adr1r17/adr1r17_Soton/data/code/tinsel/tinsel-0.6.1/include \
    /home/adr1r17/Prg/mpich-3.2.1/include \
    /usr/include
    #/usr/include/mpi

