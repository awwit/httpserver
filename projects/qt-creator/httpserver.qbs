import qbs

Project {
	CppApplication {
		name: "httpserver"

		cpp.cxxLanguageVersion: "c++14"

		cpp.defines: qbs.buildVariant == "debug" ? base : base.concat(["DEBUG"])

		cpp.dynamicLibraries: base.concat(["gnutls"])

		Properties {
			condition: qbs.targetOS.contains("linux")
			cpp.defines: outer.concat(["POSIX"])
			cpp.dynamicLibraries: outer.concat(["dl", "pthread"])
		}

		Properties {
			condition: qbs.targetOS.contains("windows")
			cpp.defines: outer.concat(["WIN32"])
		}

		files: [
			"../../src/ConfigParser.cpp",
			"../../src/ConfigParser.h",
			"../../src/DataVariantAbstract.h",
			"../../src/DataVariantFormUrlencoded.cpp",
			"../../src/DataVariantFormUrlencoded.h",
			"../../src/DataVariantMultipartFormData.cpp",
			"../../src/DataVariantMultipartFormData.h",
			"../../src/DataVariantTextPlain.cpp",
			"../../src/DataVariantTextPlain.h",
			"../../src/Event.cpp",
			"../../src/Event.h",
			"../../src/FileIncoming.cpp",
			"../../src/FileIncoming.h",
			"../../src/Main.cpp",
			"../../src/Main.h",
			"../../src/Module.cpp",
			"../../src/Module.h",
			"../../src/RawData.h",
			"../../src/RequestParameters.cpp",
			"../../src/RequestParameters.h",
			"../../src/Server.cpp",
			"../../src/Server.h",
			"../../src/ServerApplicationDefaultSettings.h",
			"../../src/ServerApplicationSettings.h",
			"../../src/ServerApplicationsTree.cpp",
			"../../src/ServerApplicationsTree.h",
			"../../src/ServerRequest.h",
			"../../src/ServerResponse.h",
			"../../src/SignalHandlers.cpp",
			"../../src/SignalHandlers.h",
			"../../src/Socket.cpp",
			"../../src/Socket.h",
			"../../src/SocketAdapter.cpp",
			"../../src/SocketAdapter.h",
			"../../src/SocketAdapterDefault.cpp",
			"../../src/SocketAdapterDefault.h",
			"../../src/SocketAdapterTls.cpp",
			"../../src/SocketAdapterTls.h",
			"../../src/SocketList.cpp",
			"../../src/SocketList.h",
			"../../src/System.cpp",
			"../../src/System.h",
			"../../src/Utils.cpp",
			"../../src/Utils.h",
		]
	}
}
