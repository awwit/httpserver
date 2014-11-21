
#include "ConfigParser.h"
#include "ServerApplicationSettings.h"

#include "Utils.h"

#include <iostream>
#include <fstream>

namespace HttpServer
{
	/**
	 * Config - include file
	 */
	bool ConfigParser::includeConfigFile(const std::string &fileName, std::string &strBuf, const std::size_t offset = 0)
	{
		std::ifstream file(fileName);

		if ( ! file)
		{
			file.close();

			std::cout << "Error: " << fileName << " - cannot be open;" << std::endl;

			return false;
		}

		file.seekg(0, std::ifstream::end);
		std::streamsize file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		std::streamsize file_size_max = 2048 * 1024;

		if (file_size_max < file_size)
		{
			file.close();

			std::cout << "Error: " << fileName << " - is too large; max include file size = " << file_size_max << " bytes;" << std::endl;

			return false;
		}

		if (file_size)
		{
			std::vector<std::string::value_type> buf(file_size);
			file.read(reinterpret_cast<char *>(buf.data() ), file_size);

			strBuf.insert(strBuf.begin() + offset, buf.cbegin(), buf.cend() );
		}

		file.close();

		return true;
	}

	/**
	 * Config - add application
	 */
	bool ConfigParser::addApplication(
			const std::unordered_map<std::string, std::string> &app,
			const ServerApplicationDefaultSettings &defaults,
			std::vector<Module> &modules,
			ServerApplicationsTree &apps_tree
		)
	{
		auto it_name = app.find("server_name");

		if (app.cend() == it_name)
		{
			std::cout << "Error: application parameter 'server_name' is not specified;" << std::endl;
			return false;
		}

		std::vector<std::string> names;

		std::string whitespace(" \t\n\v\f\r");

		const std::string &app_name = it_name->second;

		size_t delimiter = app_name.find_first_of(whitespace);

		if (delimiter)
		{
			size_t cur_pos = 0;

			while (std::string::npos != delimiter)
			{
				std::string name = app_name.substr(cur_pos, delimiter - cur_pos);
				names.emplace_back(std::move(name) );
				cur_pos = app_name.find_first_not_of(whitespace, delimiter + 1);
				delimiter = app_name.find_first_of(whitespace, cur_pos);
			}

			std::string name = app_name.substr(cur_pos);
			names.emplace_back(std::move(name) );
		}

		auto it_port = app.find("listen");

		if (app.cend() == it_port)
		{
			std::cout << "Error: application port is not set;" << std::endl;
			return false;
		}

		auto it_root_dir = app.find("root_dir");

		if (app.cend() == it_root_dir || it_root_dir->second.empty() )
		{
			std::cout << "Error: application parameter 'root_dir' is not specified;" << std::endl;
			return false;
		}

		auto it_module = app.find("server_module");

		if (app.cend() == it_module)
		{
			std::cout << "Error: application parameter 'server_module' is not specified;" << std::endl;
			return false;
		}

		// TODO: get module realpath

		Module module(it_module->second);

		if (false == module.is_open() )
		{
			std::cout << "Error: module '" << it_module->second << "' cannot be open;" << std::endl;
			return false;
		}

		void *(*addr)(void *) = nullptr;

		if (false == module.find("application_call", &addr) )
		{
			std::cout << "Error: function 'application_call' not found in module '" << it_module->second << "';" << std::endl;
			return false;
		}

		std::function<int(server_request *, server_response *)> app_call = reinterpret_cast<int(*)(server_request *, server_response *)>(addr);

		if ( ! app_call)
		{
			std::cout << "Error: invalid function 'application_call' in module '" << it_module->second << "';" << std::endl;
			return false;
		}

		if (false == module.find("application_clear", &addr) )
		{
			std::cout << "Error: function 'application_clear' not found in module '" << it_module->second << "';" << std::endl;
			return false;
		}

		std::function<void(Utils::raw_pair [], const size_t)> app_clear = reinterpret_cast<void(*)(Utils::raw_pair [], const size_t)>(addr);

		std::function<bool()> app_init = std::function<bool()>();

		if (module.find("application_init", &addr) )
		{
			app_init = reinterpret_cast<bool(*)()>(addr);
		}

		std::function<void()> app_final = std::function<void()>();

		if (module.find("application_final", &addr) )
		{
			app_final = reinterpret_cast<void(*)()>(addr);
		}

		bool success = true;

		try
		{
			if (app_init)
			{
				success = app_init();
			}
		}
		catch (...)
		{
			success = false;
		}

		if (false == success)
		{
			std::cout << "Warning: error when initializing application '" << it_module->second << "';" << std::endl;
			return false;
		}

		auto it_temp_dir = app.find("temp_dir");

		const std::string temp_dir = app.cend() != it_temp_dir ? it_temp_dir->second : defaults.temp_dir;

		auto it_request_max_size = app.find("request_max_size");

		const size_t request_max_size = app.cend() != it_request_max_size ? std::strtoull(it_request_max_size->second.c_str(), nullptr, 10) : defaults.request_max_size;

		auto it_module_update = app.find("server_module_update");

		const std::string module_update = app.cend() != it_module_update ? it_module_update->second : "";

		// Calculate module index
		size_t module_index = std::numeric_limits<size_t>::max();

		for (size_t i = 0; i < modules.size(); ++i)
		{
			if (modules[i] == module)
			{
				module_index = i;
				break;
			}
		}

		if (std::numeric_limits<size_t>::max() == module_index)
		{
			module_index = modules.size();
			modules.emplace_back(std::move(module) );
		}

		std::string root_dir = it_root_dir->second;

	#ifdef WIN32
		if ('\\' == root_dir.back() )
		{
			root_dir.pop_back();
		}
	#endif

		// Remove back slash from root_dir
		if ('/' == root_dir.back() )
		{
			root_dir.pop_back();
		}

		// Create application settings struct
		ServerApplicationSettings *sets = new ServerApplicationSettings {
			std::strtol(it_port->second.c_str(), nullptr, 10),
			root_dir,
			temp_dir,
			request_max_size,
			module_index,
			it_module->second,
			module_update,
			app_call,
			app_clear,
			app_init,
			app_final
		};

		// Add application names in tree
		if (names.empty() )
		{
			apps_tree.addApplication(app_name, sets);
		}
		else
		{
			for (size_t i = 0; i < names.size(); ++i)
			{
				apps_tree.addApplication(names[i], sets);
			}
		}

		return true;
	}

	/**
	 * @brief ConfigParser::parseMimes
	 * @param fileName
	 * @param mimes_types
	 * @return bool
	 */
	bool ConfigParser::parseMimes(const std::string &fileName, std::unordered_map<std::string, std::string> &mimes_types)
	{
		std::ifstream file(fileName);

		if ( ! file)
		{
			file.close();

			std::cout << "Error: " << fileName << " - cannot be open;" << std::endl;

			return false;
		}

		file.seekg(0, std::ifstream::end);
		std::streamsize file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		std::streamsize file_size_max = 2048 * 1024;

		if (file_size_max < file_size)
		{
			file.close();

			std::cout << "Error: " << fileName << " - is too large; max file size = " << file_size_max << " bytes;" << std::endl;

			return false;
		}

		std::string whitespace(" \t\v\f\r");

		std::vector<std::string::value_type> buf(file_size);

		file.read(reinterpret_cast<char *>(buf.data() ), file_size);

		std::string str_buf(buf.cbegin(), buf.cend() );

		size_t cur_pos = 0;
		size_t end_pos = str_buf.find('\n', cur_pos);

		while (std::string::npos != end_pos)
		{
			cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
			size_t delimiter = str_buf.find_first_of(whitespace, cur_pos);

			if (delimiter < end_pos)
			{
				std::string mime_type = str_buf.substr(cur_pos, delimiter - cur_pos);

				if ('#' != mime_type.front() )
				{
					delimiter = str_buf.find_first_not_of(whitespace, delimiter);

					if (delimiter < end_pos)
					{
						std::string ext = str_buf.substr(delimiter, end_pos - delimiter);

						delimiter = ext.find_first_of(whitespace);

						if (std::string::npos != delimiter)
						{
							for (size_t ext_pos = 0; std::string::npos != ext_pos; )
							{
								std::string ext_unit = ext.substr(ext_pos, std::string::npos != delimiter ? delimiter - ext_pos : std::string::npos);

								if (false == ext_unit.empty() )
								{
									mimes_types.emplace(std::move(ext_unit), mime_type);
								}

								ext_pos = ext.find_first_not_of(whitespace, delimiter);

								delimiter = ext.find_first_of(whitespace, ext_pos);
							}
						}
						else
						{
							mimes_types.emplace(std::move(ext), std::move(mime_type) );
						}
					}
				}
			}

			cur_pos = end_pos + 1;

			end_pos = str_buf.find('\n', cur_pos);
		}

		return true;
	}

	/**
	 * Config - parse
	 */
	bool ConfigParser::loadConfig(
			const std::string &conf,
			std::unordered_map<std::string, std::string> &settings,
			std::unordered_map<std::string, std::string> &mimes_types,
			std::vector<Module> &modules,
			ServerApplicationsTree &apps_tree
		)
	{
		std::string file_name(conf);
		std::string str_buf;

		if (false == includeConfigFile(file_name, str_buf) )
		{
			return false;
		}

		std::vector<std::unordered_map<std::string, std::string> > applications;

		std::string whitespace(" \t\n\v\f\r");

		size_t cur_pos = 0;
		size_t end_pos = str_buf.find(';', cur_pos);
		size_t block_pos = 0;

		while (std::string::npos != end_pos)
		{
			block_pos = str_buf.find('{', cur_pos);

			if (end_pos < block_pos)
			{
				cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
				size_t delimiter = str_buf.find_first_of(whitespace, cur_pos);

				if (delimiter < end_pos)
				{
					std::string param_name = str_buf.substr(cur_pos, delimiter - cur_pos);

					cur_pos = str_buf.find_first_not_of(whitespace, delimiter + 1);
					delimiter = str_buf.find_last_not_of(whitespace, end_pos);

					std::string param_value = str_buf.substr(cur_pos, delimiter - cur_pos);

					if ("include" == param_name)
					{
						this->includeConfigFile(param_value, str_buf, end_pos + 1);
					}
					else
					{
						settings.emplace(std::move(param_name), std::move(param_value) );
					}
				}

				cur_pos = end_pos + 1;
			}
			else if (std::string::npos != block_pos)
			{
				cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
				size_t delimiter = str_buf.find_first_of(whitespace, cur_pos);

				std::string block_type_name = str_buf.substr(cur_pos, delimiter - cur_pos);

				delimiter = str_buf.find_first_not_of(whitespace, delimiter);

				cur_pos = block_pos + 1;
				size_t block_end = str_buf.find('}', cur_pos);

				if (delimiter == block_pos)
				{
					if ("server" == block_type_name)
					{
						std::unordered_map<std::string, std::string> app;

						end_pos = str_buf.find(';', cur_pos);

						while (block_end > end_pos)
						{
							cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
							delimiter = str_buf.find_first_of(whitespace, cur_pos);

							if (delimiter < end_pos)
							{
								std::string param_name = str_buf.substr(cur_pos, delimiter - cur_pos);

								cur_pos = str_buf.find_first_not_of(whitespace, delimiter + 1);
								delimiter = str_buf.find_last_not_of(whitespace, end_pos);

								std::string param_value = str_buf.substr(cur_pos, delimiter - cur_pos);

								if ("include" == param_name)
								{
									cur_pos = end_pos + 1;
									this->includeConfigFile(param_value, str_buf, cur_pos);
									block_end = str_buf.find('}', cur_pos);
								}
								else
								{
									app.emplace(std::move(param_name), std::move(param_value) );
								}
							}

							cur_pos = end_pos + 1;

							end_pos = str_buf.find(';', cur_pos);
						}

						applications.emplace_back(std::move(app) );
					}
					else
					{
						std::cout << "Warning: " << block_type_name << " - unknown block type;" << std::endl;
					}
				}
				else
				{
					std::cout << "Warning: after " << block_type_name << " expected '{' ;" << std::endl;
				}

				cur_pos = block_end + 1;
			}

			end_pos = str_buf.find(';', cur_pos);
		}

		auto it_mimes = settings.find("mimes");

		if (settings.cend() != it_mimes)
		{
			this->parseMimes(it_mimes->second, mimes_types);
		}
		else
		{
			std::cout << "Warning: mime types file is not set in configuration;" << std::endl;
		}

		if (false == applications.empty() )
		{
			auto it_default_temp_dir = settings.find("default_temp_dir");

			const std::string default_temp_dir = settings.cend() != it_default_temp_dir ? it_default_temp_dir->second : System::getTempDir();

			auto it_default_request_max_size = settings.find("request_max_size");

			const size_t default_request_max_size = settings.cend() != it_default_request_max_size ? std::strtoull(it_default_request_max_size->second.c_str(), nullptr, 10) : 0;

			ServerApplicationDefaultSettings defaults {
				default_temp_dir,
				default_request_max_size
			};

			for (auto &app : applications)
			{
				this->addApplication(app, defaults, modules, apps_tree);
			}
		}

		if (apps_tree.empty() )
		{
			std::cout << "Notice: server does not contain applications;" << std::endl;
		}

		return true;
	}
};