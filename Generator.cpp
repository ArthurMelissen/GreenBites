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
, _randomGenerator(QRandomGenerator::securelySeeded())
{ 
//	void QNetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)

	QObject::connect(&_nam, &QNetworkAccessManager::authenticationRequired, [] {
		std::cout << "QNetworkAccessManager::authenticationRequired - 100" << std::endl;
	});
	
}

void Generator::run()
{
	QTimer::singleShot(10, [&] { authenticate(); });
	_loop.exec();
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

void Generator::get(const QString& path, std::function<void(QNetworkReply*)> replyParser)
{
	QNetworkRequest request(_jexiaProjectUrl + path);
	request.setRawHeader("Authorization", "Bearer " + _accessToken.toUtf8());
	
	QNetworkReply* reply = _nam.get(request);
	QObject::connect(reply, &QNetworkReply::finished, [replyParser, reply] {
		if(!reply->isFinished())
			throw std::runtime_error("HTTP Reply is not finished");
		if(reply->isRunning())
			throw std::runtime_error("HTTP Reply is still running");
		if(reply->error() != QNetworkReply::NoError) {
			std::cout << "HTTP Request failed:\n" << std::endl;
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl;
			const QString errorString = reply->errorString();
			const auto s = "HTTP Request failed: " + QString::number(reply->error()) + errorString;
			throw std::runtime_error(s.toStdString());
		}
		replyParser(reply);
	});
}

void Generator::getArray(const QString& path, std::function<void(QJsonObject)> apply, std::function<void(void)> finally)
{
	get(path, [apply, finally] (QNetworkReply* reply) {
		const QByteArray result = reply->readAll();
		const auto doc = QJsonDocument::fromJson(result);
		if(!doc.isArray())
			throw std::runtime_error("Document is not a json array");
		const auto array = doc.array();
		for(const QJsonValue& element: array) {
			if(!element.isObject())
				throw std::runtime_error("Element is not an object");
			apply(element.toObject());
		}
		finally();
	});
}

void Generator::getPartners()
{
	getArray("/ds/partners", [&] (QJsonObject object) { 
		if(!object.contains("id") || !object.contains("name"))
			throw std::runtime_error("Partner JSON object is invalid");
		const QString uuid = object.value("id").toString();
		const QString name = object.value("name").toString();
		_partners.emplace_back(Partner{ uuid, name });
	}, [&] {
		std::cout << "========= Parsed partners ========= " << _partners.size() << std::endl;
		for(auto& p: _partners)
			p.print();
		getProducts();
	});
}

void Generator::getProducts()
{
	getArray("/ds/products", [&] (QJsonObject object) {
		if(!object.contains("id") || !object.contains("name"))
			throw std::runtime_error("Product JSON object is invalid");
		const QString uuid = object.value("id").toString();
		const QString name = object.value("name").toString();
		_products.emplace_back(Product{ uuid, name });
	}, [&] {
		std::cout << "========= Parsed products ========= " << _products.size() << std::endl;
		for(auto& p: _products)
			p.print();
		deleteProducts();
	});
}

void Generator::deleteProducts()
{
	// A condition is required
	QNetworkRequest request(_jexiaProjectUrl + "/ds/products?1=1");
	request.setRawHeader("Authorization", "Bearer " + _accessToken.toUtf8());
//	request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
	QJsonArray array;
	for(auto& p: _products)
		array.append(QJsonObject{{"id", p.uuid}});
	const QByteArray body = QJsonDocument(array).toJson();
	auto reply = _nam.sendCustomRequest(request, "DELETE", body);

	QObject::connect(reply, &QNetworkReply::finished, [&, reply] {
		std::cout << "Delete reply finished\n";
		if(!reply->isFinished())
			throw std::runtime_error("HTTP Reply is not finished");
		if(reply->isRunning())
			throw std::runtime_error("HTTP Reply is still running");
		if(reply->error() != QNetworkReply::NoError) {
			std::cout << "HTTP Request failed:\n" << std::endl;
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl;
			const QString errorString = reply->errorString();
			const auto s = "HTTP Request failed: " + QString::number(reply->error()) + errorString;
			throw std::runtime_error(s.toStdString());
		}
		getPackageTypes();
	});
}

void Generator::getPackageTypes()
{
	getArray("/ds/package_types", [&] (QJsonObject object) {
		if(!object.contains("id") || !object.contains("name") || !object.contains("quantity"))
			throw std::runtime_error("Package type JSON object is invalid");
		const QString uuid = object.value("id").toString();
		const QString name = object.value("name").toString();
		const int quantity = object.value("quantity").toInt();
		_packageTypes.emplace_back(PackageType{ uuid, name, quantity });
	}, [&] {
		std::cout << "========= Parsed package types ========= " << _packageTypes.size() << std::endl;
		for(auto& p: _packageTypes)
			p.print();
		getPackages();
	});
}

void Generator::getPackages()
{
	getArray("/ds/packages", [&] (QJsonObject object) {
		if(!object.contains("id") || !object.contains("quantity"))
			throw std::runtime_error("Package type JSON object is invalid");
		const QString uuid = object.value("id").toString();
		const int quantity = object.value("quantity").toInt();
		_packages.emplace_back(Package{ uuid, quantity });
	}, [&] {
		std::cout << "========= Parsed packages ========= " << _packages.size() << std::endl;
		for(auto& p: _packages)
			p.print();
		getShipments();
	});
}

void Generator::getShipments()
{
	getArray("/ds/shipments", [&] (QJsonObject object) {
		if(!object.contains("id") || !object.contains("address"))
			throw std::runtime_error("Shipment JSON object is invalid");
		const QString uuid = object.value("id").toString();
		const QString address = object.value("address").toString();
		_shipments.emplace_back(Shipment{ uuid, address });
	}, [&] {
		std::cout << "========= Parsed shipments ========= " << _shipments.size() << std::endl;
		for(auto& s: _shipments)
			s.print();
		postPartners();
	});
}

void Generator::post(const QString& path, const QByteArray& data, std::function<void(QNetworkReply*)> replyParser)
{
	QNetworkRequest request(_jexiaProjectUrl + path);
	request.setRawHeader("Authorization", "Bearer " + _accessToken.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
	
	QNetworkReply* reply = _nam.post(request, data);
	QObject::connect(reply, &QNetworkReply::finished, [replyParser, reply] {
		if(!reply->isFinished())
			throw std::runtime_error("HTTP Reply is not finished");
		if(reply->isRunning())
			throw std::runtime_error("HTTP Reply is still running");
		if(reply->error() != QNetworkReply::NoError) {
			std::cout << "HTTP Request failed:\n" << std::endl;
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl;
			const QString errorString = reply->errorString();
			const auto s = "HTTP Request failed: " + QString::number(reply->error()) + errorString;
			throw std::runtime_error(s.toStdString());
		}
		replyParser(reply);
	});
}

void Generator::postPartners()
{
	const auto google = std::find_if(_partners.begin(), _partners.end(), [] (const auto& p) { return p.name == "Google"; });
	if(google == _partners.end()) {
		QJsonObject o {{"name", "Google"}};
		const QByteArray data = QJsonDocument(o).toJson();
		std::cout << QString::fromUtf8(data).toStdString() << "\n";
		post("/ds/partners", data, [&] (QNetworkReply* reply) {
			std::cout << "__________ Finished  ______________" << std::endl;
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl;
			postProducts();
		});
	} else {
		postProducts();
	}
}

void Generator::postProducts()
{
	static const QString base36 = "0123456789abcdefghijklmnopqrstuvwxyz";
	
	const size_t batchSize = 200;
	
	if(_products.size() < _targetProductsSize) {
		const auto generateProduct = [&] {
			QString name;
			for(size_t i=0; i < 20; i++)
				name.append(base36[_randomGenerator.bounded(0, 35)]);
			return QJsonObject {{"name", name}};
		};
	
		QByteArray data;
		if(batchSize == 1) {
			data = QJsonDocument(generateProduct()).toJson();
		} else {
			QJsonArray array;
			for(size_t i=0; i < batchSize; i++)
				array.append(generateProduct());
			data = QJsonDocument(array).toJson();
		}
		
		std::cout << QString::fromUtf8(data).toStdString() << "\n";
		post("/ds/products", data, [&] (QNetworkReply* reply) {
			std::cout << "__________ Finished  ______________" << std::endl;
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl;
			_loop.quit();
		});
	} else {
		_loop.quit();
	}
}

void Generator::process()
{
	std::cout << "Processing" << std::endl;
}
