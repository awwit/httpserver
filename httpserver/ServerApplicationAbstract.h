#pragma once

#include "ServerRequest.h"
#include "ServerResponse.h"
#include "ResourceAbstract.h"

namespace HttpServer
{
	class ServerApplicationAbstract
	{
	protected:
		std::string default_uri_reference;

		std::unordered_map<std::string, ResourceAbstract *> resources;

	private:
		void addResource(ResourceAbstract *);

	public:
		~ServerApplicationAbstract();

		/**
		 * Application entry point
		 *
		 * @param ServerRequest &request - 
		 *		@member Socket & - сокет клиента
		 *		@member const std::string &method - метод применяемый к ресурсу
		 *		@member const std::string &uriReference - ссылка на ресурс
		 *		@member const std::unordered_map<std::string, std::string> &params - параметры ресурса
		 *		@member const std::unordered_map<std::string, std::string> &headers - заголовки запроса
		 *		@member const std::unordered_map<std::string, std::string> &data - входящие данные запроса
		 *		@member const std::unordered_map<std::string, FileIncoming> &files - входящие файлы запроса
		 *
		 * @param ServerResponse &response - 
		 *		...
		 *
		 * @return int - код завершения приложения
		 */
		virtual int main(ServerRequest &, ServerResponse &) = 0;
	};
};