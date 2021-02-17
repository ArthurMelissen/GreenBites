#include <iostream>
#include <QDateTime>
#include <QCoreApplication>
#include <QProcessEnvironment>
#include "Generator.h"
#include <stdexcept>
#include <QCommandLineParser>
//
// The GreenBites dataset generator
//
int main(int argc, char**argv)
{
	QCoreApplication qapp(argc, argv);
	QCommandLineParser clParser;
	clParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
	
	QCommandLineOption createPartnersArg(
				"createpartners", "Create more partners!");
	clParser.addOption(createPartnersArg);

	QCommandLineOption createProductsArg(
				"createproducts", "Create more products!");
	clParser.addOption(createProductsArg);

	QCommandLineOption deleteProductsArg(
				"deleteproducts", "Delete all products from the dataset");
	clParser.addOption(deleteProductsArg);

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

	clParser.process(qapp);
	Generator g(jexiaProjectUrl, jexiaKey, jexiaSecret);
	
	if(clParser.isSet(createPartnersArg)) {
		std::cout << "Create partners job added\n";
		g.createPartners(10);
	}
	if(clParser.isSet(createProductsArg)) {
		std::cout << "Create products job added\n";
		g.createProducts(10);
	}
	if(clParser.isSet(deleteProductsArg)) {
		std::cout << "Delete products job added\n";
		g.deleteProducts();
	}
	
	g.run();

	std::cout << "Finished on " << startTime.toString().toStdString() << "\n";
	return 0;
}
