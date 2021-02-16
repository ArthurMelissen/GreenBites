#include "Generator.h"
#include <QTimer>
#include <iostream>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QAuthenticator>
#include <QJsonArray>

Generator::Generator(const QString& jexiaProjectUrl, const QString& jexiaKey, const QString& jexiaSecret)
: _jexiaProjectUrl(jexiaProjectUrl)
, _jexiaKey(jexiaKey)
, _jexiaSecret(jexiaSecret)
{ 
//	void QNetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)

	QObject::connect(&_nam, &QNetworkAccessManager::authenticationRequired, [] {
		std::cout << "QNetworkAccessManager::authenticationRequired - 100" << std::endl;
	});
	
}

void Generator::run()
{
	QTimer::singleShot(10, [&] { authenticate(); });
	QEventLoop loop;
	loop.exec();
}

void Generator::authenticate()
{
	// Send an authentication request.
	QJsonObject object {
		{"method", "apk"},
		{"key", _jexiaKey},
		{"secret", _jexiaSecret},
	};
	
	QNetworkRequest request(_jexiaProjectUrl + "/auth");
	request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
	QNetworkReply* reply = _nam.post(request, QJsonDocument(object).toJson());
	QObject::connect(reply, &QNetworkReply::finished, [this, reply] () { 
		auto doc = QJsonDocument::fromJson(reply->readAll());
		if(!doc.isObject())
			throw std::runtime_error("Authentication reply is not a JSON object");
		const auto object = doc.object();
		if(!object.contains("access_token"))
			throw std::runtime_error("Authentication JSON object must contain access_token");
		if(!object.contains("refresh_token"))
			throw std::runtime_error("Authentication JSON object must contain refresh_token");
		_accessToken = object.value("access_token").toString();
		_refreshToken = object.value("refresh_token").toString();
		if(_accessToken.isEmpty() || _refreshToken.isEmpty())
			throw std::runtime_error("One of the tokens is empty");
		
		// The next step
		queryPartners();
	});
}

void Generator::queryPartners()
{
	std::cout << "Query partners - 100" << std::endl;
	
	QNetworkRequest request(_jexiaProjectUrl + "/ds/partners");
	request.setRawHeader("Authorization", _accessToken.toUtf8());
	
	QNetworkReply* reply = _nam.get(request);
	QObject::connect(reply, &QNetworkReply::finished, [&] () { 
		std::cout << "Querying partners replied: finished: " << reply->isFinished() << " running: " << reply->isRunning() << std::endl;
		const QByteArray result = reply->readAll();
		const QString contents = QString::fromUtf8(result);
		std::cout << "Contents:\n" << contents.toStdString();
		parsePartners(result);
	});
	
	std::cout << "Query partners - 999" << std::endl;
}

void Generator::parsePartners(const QByteArray& result)
{
	const auto doc = QJsonDocument::fromJson(result);
	if(!doc.isArray())
		throw std::runtime_error("Partners is not a json array");
	const auto array = doc.array();
	for(const QJsonValue& element: array) {
		if(!element.isObject())
			throw std::runtime_error("Element is not an object");
		const auto object = element.toObject();
		if(!object.contains("name"))
			throw std::runtime_error("Authentication JSON object must contain access_token");
		_partners.emplace_back(Partner{ object.value("name").toString() });
	}
}

void Generator::process()
{
	std::cout << "Processing" << std::endl;
}
