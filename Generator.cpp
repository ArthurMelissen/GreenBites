#include "Generator.h"
#include <QTimer>
#include <iostream>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

Generator::Generator(const QString& jexiaProjectUrl, const QString& jexiaKey, const QString& jexiaSecret)
: _jexiaProjectUrl(jexiaProjectUrl)
, _jexiaKey(jexiaKey)
, _jexiaSecret(jexiaSecret)
{ }

void Generator::run()
{
	QTimer::singleShot(10, [&] { authenticate(); });
	QEventLoop loop;
	loop.exec();
}

void Generator::authenticate()
{
	std::cout << "Authentication 100" << std::endl;

	// Send an authentication request.
	QJsonObject object {
		{"method", "apk"},
		{"key", _jexiaKey},
		{"secret", _jexiaSecret},
	};
	
	QNetworkRequest request(_jexiaProjectUrl + "/auth");
	QNetworkReply* reply = _nam.post(request, QJsonDocument(object).toJson());
	QObject::connect(reply, &QNetworkReply::finished, [reply] () { 
		std::cout << "Authentication finished: " << reply->isFinished() << " running: " << reply->isRunning() << std::endl;
		const QString contents = QString::fromUtf8(reply->readAll());
		std::cout << "Contents:\n" << contents.toStdString();
	});
	
	// The next step
//	queryPartners(); 

	std::cout << "Authentication 999" << std::endl;
}

void Generator::queryPartners()
{
	std::cout << "Querying partners" << std::endl;
	
	QNetworkRequest request(_jexiaProjectUrl + "/ds/partners");
	QNetworkReply* reply = _nam.get(request);
	QObject::connect(reply, &QNetworkReply::finished, [reply] () { 
		std::cout << "Querying partners replied: finished: " << reply->isFinished() << " running: " << reply->isRunning() << std::endl;
		const QString contents = QString::fromUtf8(reply->readAll());
		std::cout << "Contents:\n" << contents.toStdString();
	});
	
	std::cout << "Querying partners finished" << std::endl;
}

void Generator::process()
{
	std::cout << "Processing" << std::endl;
}
