
#include "ServerHttp2Stream.h"

#include "extensions/Sendfile.h"

namespace HttpServer
{
	ServerHttp2Stream::ServerHttp2Stream(
		Socket::Adapter &sock,
		const ServerSettings &settings,
		ServerControls &controls,
		Http2::IncStream *stream
	) noexcept
		: ServerHttp2Protocol(sock, settings, controls, stream)
	{

	}

	void ServerHttp2Stream::close() {
		this->stream->close();
	}

	ServerProtocol *ServerHttp2Stream::process()
	{
		struct Request req;

		req.timeout = std::chrono::milliseconds(5000);
		req.protocol_variant = Transfer::ProtocolVariant::HTTP_2;

		req.incoming_headers = std::move(this->stream->incoming_headers);
		req.incoming_data = std::move(this->stream->incoming_data);
		req.incoming_files = std::move(this->stream->incoming_files);

		auto const &headers = req.incoming_headers;

		auto const it_scheme = headers.find(":scheme");

		if (headers.cend() == it_scheme) {
			return this;
		}

		const std::string &scheme = it_scheme->second;

		const int default_port = (scheme == "https")
			? 443
			: (scheme == "http")
				? 80 : 0;

		auto const it_host = headers.find(":authority");

		if (headers.cend() == it_host) {
			return this;
		}

		const std::string &host_header = it_host->second;

		// Поиск разделителя, за которым помещается номер порта, если указан
		const size_t delimiter = host_header.find(':');

		// Получить имя (или адрес)
		req.host = host_header.substr(0, delimiter);

		// Получить номер порта
		const int port = std::string::npos != delimiter
			? std::atoi(host_header.substr(delimiter + 1).c_str())
			: default_port;

		const ServerApplicationSettings *app_sets = this->settings.apps_tree.find(req.host);

		// Если приложение найдено
		if (nullptr == app_sets || (
				app_sets->ports.cend() == app_sets->ports.find(port) &&
				app_sets->tls_ports.cend() == app_sets->tls_ports.find(port)
			)
		) {
			return this;
		}

		auto const it_method = headers.find(":method");

		if (headers.cend() == it_method) {
			return this;
		}

		req.method = it_method->second;

		auto const it_path = headers.find(":path");

		if (headers.cend() == it_path) {
			return this;
		}

		req.path = it_path->second;

		req.app_exit_code = EXIT_FAILURE;

		this->runApplication(req, *app_sets);

		for (auto const &it : req.incoming_files) {
			std::remove(it.second.getTmpName().c_str() );
		}

		if (EXIT_SUCCESS == req.app_exit_code) {
		//	Http2::OutStream out(*stream);

		//	auto tmp = req.protocol_data;
		//	req.protocol_data = &out;

			Sendfile::xSendfile(
				std::ref(*this),
				req,
				this->settings.mimes_types
			);

		//	req.protocol_data = tmp;
		}

		return this;
	}
}
