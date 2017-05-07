# httpserver

Http server is written on C++14 language.

Dynamic libraries act as applications for the server
(\*.so - linux, \*.dll - windows).
Library connection takes place in the configuration file
[samples/apps.conf](samples/apps.conf)
by using the parameter `server_module`.

Sample application code: https://github.com/awwit/httpserverapp

## Features

This http server support:

* HTTP v1.1
* HTTPS (TLS)
* HTTP v2 (*need optimize cleanup of streams*)
* Keep-Alive
* WebSocket
* X-Sendfile (header)
* Get-Parted requests

## Dependencies

Common:

* [gnutls](https://www.gnutls.org/)

Linux: `dl`, `pthread`, `rt`, `gnutls`

Windows: `ws2_32.lib`, `libgnutls.dll.a`

## Build

Linux:

```sh
git clone https://github.com/awwit/httpserver.git
cd httpserver
make
```

or

```sh
git clone https://github.com/awwit/httpserver.git
cd httpserver
mkdir build
cd build
qbs build -f ./../projects/qt-creator/httpserver.qbs release
```

Windows:

```sh
git clone https://github.com/awwit/httpserver.git
cd httpserver
mkdir build
cd build
devenv ./../projects/msvs/httpserver.sln /build
```

## Server start

```sh
./httpserver --start
```

Configuration files must be located in the working (current) directory.
Or input a parameter `--config-path=<path>` to set the directory with configuration files.

Use the parameter `--server-name=<name>` to define the name of web-server's instance.
Instances can be used to run web-servers with different settings.

## Server configuration

Server (and its applications) setting is made using config-files.
Examples of settings are located in the folder [samples](samples/).

# License

The source codes are licensed under the
[AGPL](http://www.gnu.org/licenses/agpl.html),
the full text of the license is located in the [LICENSE](LICENSE) file.
