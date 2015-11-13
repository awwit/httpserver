#pragma once

#include "Socket.h"
#include "RequestParameters.h"

namespace HttpServer
{
	class DataVariantAbstract
	{
	protected:
		std::string data_variant_name;

	public:
		inline std::string getName() const
		{
			return data_variant_name;
		};

	public:
		/**
		 * virtual destructor
		 */
        virtual ~DataVariantAbstract() = default;

		/**
		 * @param const Socket & - сокет, откуда можно достать остальные данные
		 * @param std::string & - первая часть полученных данных
		 * @param const size_t - сколько осталось данных (в байтах) получить из сокета
		 * @param const std::unordered_map<std::string, std::string> & - дополнительные параметры, описывающие формат данных
		 * @param request_data & - данные текущего запроса (заголовки, параметры, коллекции для хранения данных)
		 *
		 * @return bool - (true|false) - удачно ли были разобраны данные
		 */
		virtual bool parse
		(
			const Socket &sock,
			std::string &,
			const size_t,
			std::unordered_map<std::string, std::string> &contentParams,
			struct request_parameters &rp
		) = 0;
	};
};