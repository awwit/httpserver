TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += POSIX

CONFIG(debug, debug|release):DEFINES += DEBUG

QMAKE_CXXFLAGS += -std=c++11

CONFIG(release, debug|release)
{
	QMAKE_CFLAGS_RELEASE -= -O
	QMAKE_CFLAGS_RELEASE -= -O1
	QMAKE_CFLAGS_RELEASE -= -O2
	QMAKE_CFLAGS_RELEASE *= -O3
}

LIBS += -ldl -pthread

SOURCES += \
	httpserver/Main.cpp \
	httpserver/DataVariantFormUrlencoded.cpp \
	httpserver/DataVariantMultipartFormData.cpp \
	httpserver/DataVariantTextPlain.cpp \
	httpserver/Event.cpp \
	httpserver/FileIncoming.cpp \
	httpserver/Module.cpp \
	httpserver/Server.cpp \
	httpserver/ServerApplicationsTree.cpp \
	httpserver/SignalsHandles.cpp \
	httpserver/Socket.cpp \
	httpserver/SocketList.cpp \
	httpserver/System.cpp \
	httpserver/Utils.cpp \
    httpserver/ConfigParser.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
	httpserver/DataVariantAbstract.h \
	httpserver/DataVariantFormUrlencoded.h \
	httpserver/DataVariantMultipartFormData.h \
	httpserver/DataVariantTextPlain.h \
	httpserver/Event.h \
	httpserver/FileIncoming.h \
	httpserver/Main.h \
	httpserver/Module.h \
	httpserver/RawData.h \
	httpserver/Server.h \
	httpserver/ServerApplicationDefaultSettings.h \
	httpserver/ServerApplicationSettings.h \
	httpserver/ServerApplicationsTree.h \
	httpserver/ServerRequest.h \
	httpserver/ServerResponse.h \
	httpserver/SignalsHandles.h \
	httpserver/Socket.h \
	httpserver/SocketList.h \
	httpserver/System.h \
	httpserver/Utils.h \
    httpserver/ConfigParser.h

