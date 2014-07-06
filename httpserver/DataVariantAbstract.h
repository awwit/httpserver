#pragma once

#include "Socket.h"
#include "FileIncoming.h"

#include <unordered_map>

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
		virtual ~DataVariantAbstract() {};

		/**
		 * @param const Socket & - сокет, откуда можно достать остальные данные
		 * @param const std::chrono::milliseconds & - максимальное время ожидания данных (на сокете)
		 * @param const std::string & - первая часть полученных данных
		 * @param const size_t - сколько осталось данных (в байтах) получить из сокета
		 * @param const std::unordered_map<std::string, std::string> & - дополнительные параметры, описывающие формат данных
		 * @param std::unordered_multimap<std::string, std::string> & - данные в виде ключ->значение
		 * @param std::unordered_multimap<std::string, FileIncoming> & - имена файлов, в которые записаны данные
		 *
		 * @return bool - (true|false) - удачно ли были разобраны данные
		 */
		virtual bool parse
		(
			const Socket &,
			const std::chrono::milliseconds &,
			const std::string &,
			const size_t,
			const std::unordered_map<std::string, std::string> &,
			std::unordered_multimap<std::string, std::string> &,
			std::unordered_multimap<std::string, FileIncoming> &
		) = 0;
	};
};