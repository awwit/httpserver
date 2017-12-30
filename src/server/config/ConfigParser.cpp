
#include "ConfigParser.h"

#include "../ServerApplicationSettings.h"
#include "../../utils/Utils.h"

#include <iostream>
#include <fstream>

const long FILESIZE = 1024 * 1024;

namespace HttpServer
{
	/**
	 * Config - include file
	 */
	bool ConfigParser::includeConfigFile(const std::string &fileName, std::string &strBuf, const std::size_t offset)
	{
		std:: cout << "config file name: " << fileName << std::endl;
		std::ifstream file(fileName);

		if ( ! file)
		{
			file.close();

			std::cout << "Error: file " << fileName << " cannot be open;" << std::endl;

			return false;
		}

		file.seekg(0, std::ifstream::end);
		std::streamsize file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		constexpr std::streamsize file_size_max = 2 * FILESIZE;

		if (file_size_max < file_size)
		{
			file.close();

			std::cout << "Error: file " << fileName << " is too large; max include file size = " << file_size_max << " bytes;" << std::endl;

			return false;
		}

		if (file_size)
		{
			std::vector<char> buf(file_size);
			file.read(buf.data(), file_size);

			strBuf.insert(strBuf.begin() + offset, buf.cbegin(), buf.cend() );
		}

		file.close();

		return true;
	}

	/**
	 * Config - add application
	 */
	bool ConfigParser::addApplication(
			const std::unordered_multimap<std::string, std::string> &app,
			const ServerApplicationDefaultSettings &defaults,
			std::vector<System::Module> &modules,
			ServerApplicationsTree &apps_tree
		)
	{
		auto const it_name = app.find("server_name");

		if (app.cend() == it_name)
		{
			std::cout << "Error: application parameter 'server_name' has not been specified;" << std::endl;
			return false;
		}

		std::vector<std::string> names;

		const std::string whitespace(" \t\n\v\f\r");

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

		auto const range_port = app.equal_range("listen");

		if (range_port.first == range_port.second)
		{
			std::cout << "Error: application port is not set;" << std::endl;
			return false;
		}

		std::unordered_set<int> ports;
		std::unordered_set<int> tls_ports;

		for (auto it = range_port.first; it != range_port.second; ++it)
		{
			const std::string &lis = it->second;

			const bool is_tls = std::string::npos != lis.find("tls");

			const std::vector<std::string> list = Utils::explode(lis, ' ');

			for (auto const &value : list)
			{
				const int port = std::strtol(value.c_str(), nullptr, 10);

				if (port)
				{
					if (is_tls)
					{
						tls_ports.emplace(port);
					}
					else
					{
						ports.emplace(port);
					}
				}
			}
		}

		std::string cert_file;
		std::string key_file;
		std::string chain_file;
		std::string crl_file;
		std::string stapling_file;
		std::string dh_file;

		if (false == tls_ports.empty() )
		{
			auto const it_ca_file = app.find("tls_certificate_chain");

			if (app.cend() != it_ca_file)
			{
				chain_file = it_ca_file->second;
			}

			auto const it_crl_file = app.find("tls_certificate_crl");

			if (app.cend() != it_crl_file)
			{
				crl_file = it_crl_file->second;
			}

			auto const it_stapling_file = app.find("tls_stapling_file");

			if (app.cend() != it_stapling_file)
			{
				stapling_file = it_stapling_file->second;
			}

			auto const it_dh_params_file = app.find("tls_dh_params_file");

			if (app.cend() != it_dh_params_file)
			{
				dh_file = it_dh_params_file->second;
			}

			auto const it_cert_file = app.find("tls_certificate");

			if (app.cend() == it_cert_file)
			{
				std::cout << "Error: tls certificate file \"CERT\" has not been specified in configuration file;" << std::endl;
				tls_ports.clear();
			}
			else
			{
				cert_file = it_cert_file->second;
			}

			auto const it_key_file = app.find("tls_certificate_key");

			if (app.cend() == it_key_file)
			{
				std::cout << "Error: tls certificate key file \"KEY\" has not been specified in configuration file;" << std::endl;
				tls_ports.clear();
			}
			else
			{
				key_file = it_key_file->second;
			}
		}

		auto const it_root_dir = app.find("root_dir");

		if (app.cend() == it_root_dir || it_root_dir->second.empty() )
		{
			std::cout << "Error: application parameter 'root_dir' has not been specified;" << std::endl;
			return false;
		}

		auto const it_module = app.find("server_module");

		if (app.cend() == it_module)
		{
			std::cout << "Error: application parameter 'server_module' has not been specified;" << std::endl;
			return false;
		}

		// TODO: get module realpath

		System::Module module(it_module->second);

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

		std::function<int(Transfer::app_request *, Transfer::app_response *)> app_call = reinterpret_cast<int(*)(Transfer::app_request *, Transfer::app_response *)>(addr);

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

		std::function<void(void *, size_t)> app_clear = reinterpret_cast<void(*)(void *, size_t)>(addr);

		std::function<bool(const char *)> app_init = std::function<bool(const char *)>();

		if (module.find("application_init", &addr) )
		{
			app_init = reinterpret_cast<bool(*)(const char *)>(addr);
		}

		std::function<void(const char *)> app_final = std::function<void(const char *)>();

		if (module.find("application_final", &addr) )
		{
			app_final = reinterpret_cast<void(*)(const char *)>(addr);
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

		bool success = true;

		try
		{
			if (app_init)
			{
				const std::string root = root_dir;
				success = app_init(root.data() );
			}
		}
		catch (std::exception &exc)
		{
			std::cout << "Warning: an exception was thrown when the application '" << it_module->second << "' was initialized: " << exc.what() << std::endl;

			success = false;
		}

		if (false == success)
		{
			std::cout << "Warning: error when initializing application '" << it_module->second << "';" << std::endl;

			return false;
		}

		auto const it_temp_dir = app.find("temp_dir");

		std::string temp_dir = app.cend() != it_temp_dir ? it_temp_dir->second : defaults.temp_dir;

		auto const it_request_max_size = app.find("request_max_size");

		const size_t request_max_size = app.cend() != it_request_max_size ? std::strtoull(it_request_max_size->second.c_str(), nullptr, 10) : defaults.request_max_size;

		auto const it_module_update = app.find("server_module_update");

		std::string module_update = app.cend() != it_module_update ? it_module_update->second : "";

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

		// Create application settings struct
		ServerApplicationSettings *settings = new ServerApplicationSettings {
			std::move(ports),
			std::move(tls_ports),

			std::move(root_dir),
			std::move(temp_dir),
			request_max_size,

			module_index,
			it_module->second,
			std::move(module_update),

			std::move(cert_file),
			std::move(key_file),
			std::move(chain_file),
			std::move(crl_file),
			std::move(stapling_file),
			std::move(dh_file),

			std::move(app_call),
			std::move(app_clear),
			std::move(app_init),
			std::move(app_final)
		};

		// Add application names in tree
		if (names.empty() )
		{
			apps_tree.addApplication(app_name, settings);
		}
		else
		{
			for (size_t i = 0; i < names.size(); ++i)
			{
				apps_tree.addApplication(names[i], settings);
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

		const std::streamsize file_size_max = 2048 * 1024;

		if (file_size_max < file_size)
		{
			file.close();

			std::cout << "Error: " << fileName << " - is too large; max file size = " << file_size_max << " bytes;" << std::endl;

			return false;
		}

		const std::string whitespace(" \t\v\f\r");

		std::vector<char> buf(file_size);

		file.read(buf.data(), file_size);

		const std::string str_buf(buf.cbegin(), buf.cend() );

		size_t cur_pos = 0;

		ConfigFileDataState dataState = CONFIGFILEDATASNONE;

		while (std::string::npos != cur_pos)
		{
			//std::cout << "parse Mine dataState: " << dataState << std::endl;
			switch(dataState)
			{
				case CONFIGFILEDATASNONE:

				case CONFIGFILEDATASNOTE:
					
					{
						cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
						if (cur_pos == std::string::npos)
						{
							return true;
						}

						//start from next line
						if ('#' == buf[cur_pos])
						{
							//last line
							cur_pos = str_buf.find_first_of("\n", cur_pos);
							if (cur_pos == std::string::npos)
							{
								return true;
							}

							cur_pos += 1;

							dataState = CONFIGFILEDATASNOTE;
						}
						else if ('\n' == buf[cur_pos])
						{
							cur_pos += 1;
							dataState = CONFIGFILEDATASNONE;
						}
						else
						{
							dataState = CONFIGFILEDATASTYPE;
						}
					}
					break;
				case CONFIGFILEDATASTYPE:
					{
						size_t delimiter = str_buf.find_first_of("\n", cur_pos);
						std::string strLine = str_buf.substr(cur_pos, delimiter - cur_pos);
						
						//get mimetypes 
						{
							size_t ext_pos = strLine.find_first_of(whitespace, 0);
							if (ext_pos == std::string::npos)
							{
								return false;
							}
							std::string strExt = strLine.substr(0, ext_pos);
							size_t extUnitPos = strLine.find_first_not_of(whitespace, ext_pos);
							std::string strExtUnit =  strLine.substr(extUnitPos, strLine.length() - extUnitPos);

							mimes_types.emplace(std::move(strExt), std::move(strExtUnit));

							//std::cout << strExt << " " << strExtUnit << std::endl;
						}
						
						//change line 
						cur_pos = str_buf.find_first_of("\n", cur_pos);
						if (cur_pos == std::string::npos)
						{
							return true;
						}

						cur_pos += 1;

						dataState = CONFIGFILEDATASNONE;
						
					}
					break;
				default:
					break;
			}
		}
		
		return true;
	}


	static size_t findBlockEnd(const std::string &str_buf, size_t str_pos)
	{
		size_t pos = str_buf.find('}', str_pos);

		while (std::string::npos != pos)
		{
			size_t begin_line = str_buf.rfind('\n', pos);

			if (std::string::npos == begin_line)
			{
				begin_line = 0;
			}

			begin_line = str_buf.find_first_not_of("\r\n", begin_line);

			if ('#' == str_buf[begin_line])
			{
				str_pos = str_buf.find_first_of("\r\n", pos);
			}
			else
			{
				break;
			}

			pos = str_buf.find('}', str_pos);
		}

		return pos;
	}

	/**
	 * Config - parse
	 */
	bool ConfigParser::loadConfig(const std::string &conf_file_name, ServerSettings &settings, std::vector<System::Module> &modules)
	{
		//std::cout << "enter loadCofig" << std::endl;
		std::string str_buf;

		if (false == includeConfigFile(conf_file_name, str_buf) )
		{
			return false;
		}

		std::unordered_map<std::string, std::string> &global = settings.global;
		std::unordered_map<std::string, std::string> &mimes_types = settings.mimes_types;
		ServerApplicationsTree &apps_tree = settings.apps_tree;

		std::vector<std::unordered_multimap<std::string, std::string> > applications;

		const std::string whitespace(" \t\n\v\f\r");

		size_t cur_pos = 0;

		ConfigFileDataState dataState = CONFIGFILEDATASNONE;
		InBlockDataState inBlockState = INBLOCKNONE;

		std::unordered_multimap<std::string, std::string> app;

		int iStatus = 0;
		while (std::string::npos != cur_pos)
		{
			//std::cout << "parse Mine dataState: " << dataState << std::endl;
			if (iStatus == 1 || iStatus == 2)
			{
				break;
			}

			switch (dataState)
			{
				case CONFIGFILEDATASNONE:

				case CONFIGFILEDATASNOTE:
					{
						cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
						if (cur_pos == std::string::npos)
						{
							iStatus = 1;
						}

						//start from next line
						if ('#' == str_buf[cur_pos])
						{
							//last line
							cur_pos = str_buf.find_first_of("\n", cur_pos);
							if (cur_pos == std::string::npos)
							{
								iStatus = 1;
							}

							cur_pos += 1;

							dataState = CONFIGFILEDATASNOTE;
						}
						else if ('\n' == str_buf[cur_pos])
						{
							cur_pos += 1;
							dataState = CONFIGFILEDATASNONE;
						}
						else
						{
							dataState = CONFIGFILEDATASTYPE;
						}
					}
					break;
				case CONFIGFILEDATASTYPE:
					{
						//in app and main config file "; and \n " has different meaning
						size_t delimiter = str_buf.find_first_of("\n", cur_pos);
						//std:: cout << "str_buf.length()" << str_buf.length() << " delimiter " << delimiter
						//		   << " cur_pos " << cur_pos << std::endl;
						//std::cout << "npos: " << std::string::npos << std::endl;
						if (delimiter == std::string::npos)
						{
							iStatus = 1;
						}

						std::string strLine = str_buf.substr(cur_pos, delimiter - cur_pos);
						//std::cout << "strLine: " << strLine << std::endl;
						do {
							size_t ext_pos = strLine.find_first_of(whitespace, 0);
							if (ext_pos == std::string::npos)
							{
								iStatus = 2;
							}
							std::string param_name = strLine.substr(0, ext_pos);
							size_t extUnitPos = strLine.find_first_not_of(whitespace, ext_pos);

							//mines the last ";"
							std::string param_value;
							if (strLine[strLine.length() - 1] == ';')
							{
								param_value =  strLine.substr(extUnitPos, strLine.length() - extUnitPos - 1);
							}
							else
							{
								param_value =  strLine.substr(extUnitPos, strLine.length() - extUnitPos);
							}

							//std::cout << param_name << " " << param_value << std::endl;
							if (param_name == "server" && param_value == "{")
							{
								dataState = CONFIGFILEDATAINBLOCK;
								inBlockState = INBLOCKNONE;
							}
							else if(param_name == "include")
							{
								this->loadConfig(param_value, settings, modules);
								dataState = CONFIGFILEDATASNONE;
							}
							else
							{
								global.emplace(std::move(param_name), std::move(param_value) );
								dataState = CONFIGFILEDATASNONE;
							}
							break;
						} while (1);

						//change line 
						cur_pos = str_buf.find_first_of("\n", cur_pos);
						if (cur_pos == std::string::npos)
						{
							iStatus = 1;
						}

						cur_pos += 1;
					}
					break;
				case CONFIGFILEDATAINBLOCK:
					{
						switch(inBlockState)
						{
							case INBLOCKNONE:
							case INBLOCKNOTE:
								{
									cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
									if (cur_pos == std::string::npos)
									{
										iStatus = 1;
									}

									//start from next line
									if ('#' == str_buf[cur_pos])
									{
										//last line
										cur_pos = str_buf.find_first_of("\n", cur_pos);
										if (cur_pos == std::string::npos)
										{
											iStatus = 1;
										}

										cur_pos += 1;

										inBlockState = INBLOCKNOTE;
									}
									else if ('\n' == str_buf[cur_pos])
									{
										cur_pos += 1;
										inBlockState = INBLOCKNONE;
									}
									else
									{
										inBlockState = INBLOCKTPYE;
									}
								}
								break;
							case INBLOCKTPYE:
								{
									size_t delimiter = str_buf.find_first_of("\n", cur_pos);
									std::string strLine = str_buf.substr(cur_pos, delimiter - cur_pos);
									
									//std::cout << "inblockLineStr: " << strLine << std::endl;
									
									do {
										if ("}" == strLine)
										{
											dataState = CONFIGFILEDATASNOTE;
											applications.push_back(app);
											app.clear();
											break;
										}
									
										size_t ext_pos = strLine.find_first_of(whitespace, 0);
										if (ext_pos == std::string::npos)
										{
											iStatus = 2;
										}
										std::string strExt = strLine.substr(0, ext_pos);
										size_t extUnitPos = strLine.find_first_not_of(whitespace, ext_pos);
										std::string strExtUnit =  strLine.substr(extUnitPos, strLine.length() - extUnitPos - 1);
										
										//std::cout << "inblockLine " << strExt << " " << strExtUnit << std::endl;
										app.emplace(std::move(strExt), std::move(strExtUnit));
										
										break;

									} while(1);
									
									//change line 
									cur_pos = str_buf.find_first_of("\n", cur_pos);
									if (cur_pos == std::string::npos)
									{
										iStatus = 2;
									}

									cur_pos += 1;

									inBlockState = INBLOCKNONE;
								}
								break;
							default:
								break;
						}
					}
					break;
				default:
					break;
			}
		}

		// std::cout << "app value start" << std::endl;
		// for (size_t i = 0; i < applications.size(); ++i)
		// {
		// 	std::unordered_multimap<std::string, std::string> sa = applications[i];
		// 	for (auto iter = sa.begin(); iter != sa.end(); iter++)
		// 	{
		// 		std::cout << iter->first << "  " << iter->second << std::endl;
		// 	}

		// }
		// std::cout << "app value end" << std::endl;

		auto const it_mimes = global.find("mimes");

		if (global.cend() != it_mimes)
		{
			this->parseMimes(it_mimes->second, mimes_types);
			//std::cout << "parse mimes end" << std::endl;
		}
		else
		{
			std::cout << "Warning: mime types file is not set in configuration;" << std::endl;
		}

		if (!applications.empty())
		{
			auto const it_default_temp_dir = global.find("default_temp_dir");

			const std::string default_temp_dir = global.cend() != it_default_temp_dir ? it_default_temp_dir->second : System::getTempDir();

			auto const it_default_request_max_size = global.find("request_max_size");

			const size_t default_request_max_size = global.cend() != it_default_request_max_size ? std::strtoull(it_default_request_max_size->second.c_str(), nullptr, 10) : 0;

			ServerApplicationDefaultSettings defaults {
				default_temp_dir,
				default_request_max_size
			};

			for (auto const &app : applications)
			{
				this->addApplication(app, defaults, modules, apps_tree);
				std::cout << "addApplication end" << std::endl;
			}
		}

		if (apps_tree.empty() )
		{
			std::cout << "Notice: server does not contain applications;" << std::endl;
		}

		//std::cout << "out loadCofig" << std::endl;
		return iStatus == 1;
	}
};
