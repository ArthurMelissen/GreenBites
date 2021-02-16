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
		getPartners();
	});
}

void Generator::get(const QString& path, std::function<void(QNetworkReply*)> func)
{
	QNetworkRequest request(_jexiaProjectUrl + path);
	request.setRawHeader("Authorization", "Bearer " + _accessToken.toUtf8());
	
	QNetworkReply* reply = _nam.get(request);
	QObject::connect(reply, &QNetworkReply::finished, [func, reply] {
		if(!reply->isFinished())
			throw std::runtime_error("HTTP Reply is not finished");
		if(reply->isRunning())
			throw std::runtime_error("HTTP Reply is still running");
		if(reply->error() != QNetworkReply::NoError)
			throw std::runtime_error("HTTP Request failed");
		func(reply);
	});
}

void Generator::getPartners()
{
	get("/ds/partners", [&] (QNetworkReply* reply) { 
		const QByteArray result = reply->readAll();
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
			const QString uuid = object.value("id").toString();
			const QString name = object.value("name").toString();
			_partners.emplace_back(Partner{ uuid, name });
			
			getProducts();
		}
		std::cout << "Parsed partners: " << _partners.size() << std::endl;
	});
}

void Generator::getProducts()
{
	get("/ds/products", [&] (QNetworkReply* reply) {
		const QByteArray result = reply->readAll();
		std::cout << "parseProducts:\n" << QString::fromUtf8(result).toStdString() + "\n\n";
	});
}

void Generator::process()
{
	std::cout << "Processing" << std::endl;
}
