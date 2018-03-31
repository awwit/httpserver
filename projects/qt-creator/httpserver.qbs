import qbs

Project {
    CppApplication {
        name: "httpserver"

        cpp.cxxLanguageVersion: "c++14"

        cpp.defines: qbs.buildVariant == "debug" ? ["DEBUG"] : original

        cpp.dynamicLibraries: ["gnutls"]

        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.defines: outer.concat(["POSIX"])
            cpp.dynamicLibraries: outer.concat(["dl", "pthread", "rt"])
        }
        Properties {
            condition: qbs.targetOS.contains("windows")
            cpp.defines: outer.concat(["WIN32", "NOMINMAX"])
        }

        files: [
            "../../src/transfer/AppRequest.h",
            "../../src/transfer/AppResponse.h",
            "../../src/server/ServerControls.cpp",
            "../../src/server/ServerControls.h",
            "../../src/server/ServerSettings.cpp",
            "../../src/server/ServerSettings.h",
            "../../src/server/SocketsQueue.h",
            "../../src/server/config/ConfigParser.cpp",
            "../../src/server/config/ConfigParser.h",
            "../../src/server/data-variant/Abstract.cpp",
            "../../src/server/data-variant/Abstract.h",
            "../../src/server/data-variant/FormUrlencoded.cpp",
            "../../src/server/data-variant/FormUrlencoded.h",
            "../../src/server/data-variant/MultipartFormData.cpp",
            "../../src/server/data-variant/MultipartFormData.h",
            "../../src/server/data-variant/TextPlain.cpp",
            "../../src/server/data-variant/TextPlain.h",
            "../../src/server/protocol/ServerHttp2Protocol.cpp",
            "../../src/server/protocol/ServerHttp2Protocol.h",
            "../../src/server/protocol/ServerHttp2Stream.cpp",
            "../../src/server/protocol/ServerHttp2Stream.h",
            "../../src/server/protocol/ServerWebSocket.cpp",
            "../../src/server/protocol/ServerWebSocket.h",
            "../../src/socket/Adapter.cpp",
            "../../src/socket/Adapter.h",
            "../../src/socket/AdapterDefault.cpp",
            "../../src/socket/AdapterDefault.h",
            "../../src/socket/AdapterTls.cpp",
            "../../src/socket/AdapterTls.h",
            "../../src/socket/List.cpp",
            "../../src/socket/List.h",
            "../../src/system/Cache.h",
            "../../src/utils/Event.cpp",
            "../../src/utils/Event.h",
            "../../src/transfer/FileIncoming.cpp",
            "../../src/transfer/FileIncoming.h",
            "../../src/system/GlobalMutex.cpp",
            "../../src/system/GlobalMutex.h",
            "../../src/transfer/http2/HPack.cpp",
            "../../src/transfer/http2/HPack.h",
            "../../src/transfer/http2/Http2.cpp",
            "../../src/transfer/http2/Http2.h",
            "../../src/transfer/HttpStatusCode.h",
            "../../src/Main.cpp",
            "../../src/Main.h",
            "../../src/system/Module.cpp",
            "../../src/system/Module.h",
            "../../src/transfer/ProtocolVariant.h",
            "../../src/server/Request.cpp",
            "../../src/server/Request.h",
            "../../src/server/Server.cpp",
            "../../src/server/Server.h",
            "../../src/server/ServerApplicationSettings.h",
            "../../src/server/ServerApplicationsTree.cpp",
            "../../src/server/ServerApplicationsTree.h",
            "../../src/server/protocol/ServerHttp1.cpp",
            "../../src/server/protocol/ServerHttp1.h",
            "../../src/server/protocol/ServerHttp2.cpp",
            "../../src/server/protocol/ServerHttp2.h",
            "../../src/server/protocol/ServerProtocol.cpp",
            "../../src/server/protocol/ServerProtocol.h",
            "../../src/server/protocol/extensions/Sendfile.cpp",
            "../../src/server/protocol/extensions/Sendfile.h",
            "../../src/server/ServerStructuresArguments.h",
            "../../src/system/SharedMemory.cpp",
            "../../src/system/SharedMemory.h",
            "../../src/SignalHandlers.cpp",
            "../../src/SignalHandlers.h",
            "../../src/socket/Socket.cpp",
            "../../src/socket/Socket.h",
            "../../src/system/System.cpp",
            "../../src/system/System.h",
            "../../src/utils/Utils.cpp",
            "../../src/utils/Utils.h",
        ]
    }
}
