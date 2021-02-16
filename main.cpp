#include <iostream>
#include <QDateTime>
#include <QCoreApplication>
#include <QProcessEnvironment>
#include "Generator.h"
#include <stdexcept>
//
// The GreenBites dataset generator
//
int main(int argc, char**argv)
{
	QCoreApplication app(argc, argv);
	const QDateTime startTime = QDateTime::currentDateTimeUtc();
	std::cout << "Started on " << startTime.toString().toStdString() << "\n";
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

	// url + "/ds/partners"
	
	Generator g(jexiaProjectUrl, jexiaKey, jexiaSecret);
	
	g.run();

	std::cout << "Finished on " << startTime.toString().toStdString() << "\n";
	return 0;
}
