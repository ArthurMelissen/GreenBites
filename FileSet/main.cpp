#include <iostream>
#include <QDateTime>
#include <QCoreApplication>
#include <QProcessEnvironment>
#include "Generator.h"
#include <stdexcept>
#include <QCommandLineParser>
//
// The GreenBites fileset generator
//
int main(int argc, char**argv)
{
	QCoreApplication qapp(argc, argv);
	QCommandLineParser clParser;
	clParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
	
	QCommandLineOption uploadFilesArg(
				"uploadfiles", "Upload some files", "count");
	clParser.addOption(uploadFilesArg);

	const QDateTime startTime = QDateTime::currentDateTimeUtc();
	std::cout << "Started on " << startTime.toString().toStdString() << std::endl << std::flush;
	auto env = QProcessEnvironment::systemEnvironment();

	const auto getEnv = [&] (const QString& variable) {
		if(!env.contains(variable)) {
			std::cout << "Need to set " + variable.toStdString() + " in environment\n";
			throw std::runtime_error("Environment not configured");
		}
		return env.value(variable);
	};
	const QString jexiaProjectUrl = getEnv("JEXIA_PROJECT_URL");
	const QString jexiaKey = getEnv("JEXIA_KEY");
	const QString jexiaSecret = getEnv("JEXIA_SECRET");

	clParser.process(qapp);
	Generator g(jexiaProjectUrl, jexiaKey, jexiaSecret);
	
	if(clParser.isSet(uploadFilesArg)) {
		const QString arg = clParser.value(uploadFilesArg);
		bool ok = true;
		size_t fileCount = 1;
		size_t fileSize = 1048576;
		if(arg.contains(",")) {
			// If there is a comma, parse the file count and size
			const QStringList args = arg.split(",");
			fileSize = args[0].toInt(&ok);
			if(!ok)
				throw std::runtime_error("Could not parse file size");
			fileSize = args[1].toInt(&ok);
			if(!ok)
				throw std::runtime_error("Could not parse file count");
		} else {
			// Just parse the file size
			fileSize = arg.toInt(&ok);
			if(!ok)
				throw std::runtime_error("Could not parse file size");
		}
		std::cout << "Upload files job added\n";
		g.uploadFiles(10);
	}
	
	g.run();

	std::cout << "Finished on " << startTime.toString().toStdString() << std::endl << std::flush;
	return 0;
}
