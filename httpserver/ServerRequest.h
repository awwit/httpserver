#pragma once

#include "RawData.h"
#include "Socket.h"
#include "FileIncoming.h"

#include <unordered_map>

namespace HttpServer
{
	struct server_request
	{
		const System::native_socket_type socket;
		const char *method;
		const char *uri_reference;
		const char *document_root;
		const size_t params_count;
		const Utils::raw_pair *params;
		const size_t headers_count;
		const Utils::raw_pair *headers;
		const size_t data_count;
		const Utils::raw_pair *data;
		const size_t files_count;
		const Utils::raw_fileinfo *files;
	};

	/**
	 * Структура запроса (входные данные)
	 *
	 * @member const Socket socket - сокет клиента
	 * @member const std::string method - метод применяемый к ресурсу
	 * @member const std::string uri_reference - ссылка на ресурс
	 * @member const std::string document_root - корневая директория приложения
	 * @member const std::unordered_multimap<std::string, std::string> params - параметры ресурса
	 * @member const std::unordered_map<std::string, std::string> headers - заголовки запроса
	 * @member const std::unordered_multimap<std::string, std::string> data - входящие данные запроса
	 * @member const std::unordered_multimap<std::string, FileIncoming> files - входящие файлы запроса
	 */
	struct ServerRequest
	{
		const Socket socket;
		const std::string method;
		const std::string uri_reference;
		const std::string document_root;
		const std::unordered_multimap<std::string, std::string> params;
		const std::unordered_map<std::string, std::string> headers;
		const std::unordered_multimap<std::string, std::string> data;
		const std::unordered_multimap<std::string, FileIncoming> files;
	};
};
