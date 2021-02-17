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

	QCommandLineOption getProductsArg(
				"getproducts", "List the products!");
	clParser.addOption(getProductsArg);

	QCommandLineOption createProductsArg(
				"createproducts", "Create more products!", "count");
	clParser.addOption(createProductsArg);

	QCommandLineOption deleteAllProductsArg(
				"deleteAllProducts", "Delete all products from the dataset");
	clParser.addOption(deleteAllProductsArg);

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
	
	if(clParser.isSet(createPartnersArg)) {
		std::cout << "Create partners job added\n";
		g.createPartners(10);
	}
	if(clParser.isSet(getProductsArg)) {
		std::cout << "Get products job added\n";
		g.getProducts();
	}
	if(clParser.isSet(createProductsArg)) {
		bool ok = true;
		const int count = clParser.value(createProductsArg).toInt(&ok);
		if(!ok)
			throw std::runtime_error("Could not parse count");
		std::cout << "Create products job added (" << count << ")\n";
		g.createProducts(count);
	}
	if(clParser.isSet(deleteAllProductsArg)) {
		std::cout << "Delete all products job added\n";
		g.deleteAllProducts();
	}
	
	g.run();

	std::cout << "Finished on " << startTime.toString().toStdString() << std::endl << std::flush;
	return 0;
}
