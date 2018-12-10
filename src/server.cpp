#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocketImpl.h>
#include <handlers/factory.hpp>
#include <server.hpp>
#include "Poco/Data/Session.h"
#include "Poco/Data/SQLite/Connector.h"
#include <Poco/Data/SQLite/Utility.h>
#include <data_session_factory.hpp>
#include <logger.hpp>


int Server::main(const std::vector<std::string> & args) {
	using namespace Poco::Data::Keywords;

	meeting::GetLogger().information("Start Server");

	Poco::Data::SQLite::Connector::registerConnector();
	Poco::Data::SQLite::Utility::setThreadMode(Poco::Data::SQLite::Utility::THREAD_MODE_SINGLE);
	auto session = DataSessionFactory::getInstance();

	if (std::find(args.begin(), args.end(), "init-db") != args.end()) {

		session << "DROP TABLE IF EXISTS meeting;", now;
		
	}
	session << R"(CREATE TABLE IF NOT EXISTS `meeting` (
					`id`	INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
					`name`	TEXT,
					`description`	TEXT,
					`address`	TEXT,
					`published`	INTEGER
				))",
				now;
	session.close();

	auto *parameters = new Poco::Net::HTTPServerParams();
	parameters->setTimeout(10000);
	parameters->setMaxQueued(100);
	parameters->setMaxThreads(2);

	Poco::Net::SocketAddress socket_address("127.0.0.1:8080");
	Poco::Net::ServerSocket socket;
	socket.bind(socket_address, true, false);
	socket.listen(100);

	Poco::Net::HTTPServer server(new handlers::Factory(), socket, parameters);

	server.start();
	waitForTerminationRequest();
	server.stopAll();
	socket.close();
	return 0;
}
