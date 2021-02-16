#include "Generator.h"
#include <QTimer>
#include <iostream>

Generator::Generator(const QString& jexiaProjectUrl, const QString& jexiaKey, const QString& jexiaSecret)
: _jexiaProjectUrl(jexiaProjectUrl)
, _jexiaKey(jexiaKey)
, _jexiaSecret(jexiaSecret)
{ }

void Generator::run()
{
	QTimer::singleShot(10, [&] { queryPartners(); });
	QEventLoop loop;
	loop.exec();
}

void Generator::queryPartners()
{
	std::cout << "Querying partners" << std::endl;
	
	QNetworkRequest request;
	_nam.get(request);
	
	std::cout << "Querying partners finished" << std::endl;
}

void Generator::process()
{
	std::cout << "Processing" << std::endl;
}
