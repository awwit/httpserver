TEMPLATE = app

CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

lessThan(QT_MAJOR_VERSION, 5) {
	QMAKE_CXXFLAGS += -std=c++1y
}

unix {
	DEFINES += POSIX
}

CONFIG(debug, debug|release):DEFINES += DEBUG

#CONFIG(release, debug|release)
#{
#	QMAKE_CFLAGS_RELEASE -= -O
#	QMAKE_CFLAGS_RELEASE -= -O1
#	QMAKE_CFLAGS_RELEASE -= -O2
#	QMAKE_CFLAGS_RELEASE *= -O3
#}

LIBS += -ldl -pthread

SOURCES += \
    ../../src/ConfigParser.cpp \
    ../../src/DataVariantFormUrlencoded.cpp \
    ../../src/DataVariantMultipartFormData.cpp \
    ../../src/DataVariantTextPlain.cpp \
    ../../src/Event.cpp \
    ../../src/FileIncoming.cpp \
    ../../src/Main.cpp \
    ../../src/Module.cpp \
    ../../src/RequestParameters.cpp \
    ../../src/Server.cpp \
    ../../src/ServerApplicationsTree.cpp \
    ../../src/Socket.cpp \
    ../../src/SocketList.cpp \
    ../../src/System.cpp \
    ../../src/Utils.cpp \
    ../../src/SignalHandlers.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    ../../src/ConfigParser.h \
    ../../src/DataVariantAbstract.h \
    ../../src/DataVariantFormUrlencoded.h \
    ../../src/DataVariantMultipartFormData.h \
    ../../src/DataVariantTextPlain.h \
    ../../src/Event.h \
    ../../src/FileIncoming.h \
    ../../src/Main.h \
    ../../src/Module.h \
    ../../src/RawData.h \
    ../../src/RequestParameters.h \
    ../../src/Server.h \
    ../../src/ServerApplicationDefaultSettings.h \
    ../../src/ServerApplicationSettings.h \
    ../../src/ServerApplicationsTree.h \
    ../../src/ServerRequest.h \
    ../../src/ServerResponse.h \
    ../../src/Socket.h \
    ../../src/SocketList.h \
    ../../src/System.h \
    ../../src/Utils.h \
    ../../src/SignalHandlers.h
