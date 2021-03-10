#include "Generator.h"
#include <QTimer>
#include <iostream>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QAuthenticator>
#include <QJsonArray>
#include <QThread>
#include <QScopedPointer>

Generator::Generator(const QString& jexiaProjectUrl, const QString& jexiaKey, const QString& jexiaSecret)
: _jexiaProjectUrl(jexiaProjectUrl)
, _jexiaKey(jexiaKey)
, _jexiaSecret(jexiaSecret)
, _randomGenerator(QRandomGenerator::securelySeeded())
{
//	void QNetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)

	QObject::connect(&_nam, &QNetworkAccessManager::authenticationRequired, [] {
		std::cout << "QNetworkAccessManager::authenticationRequired - 100" << std::endl << std::flush;
	});

	_workQueue = {
		[&] { authenticate(); },
		[&] { getPartners(); },
//		[&] { getProducts(); },
		[&] { getPackageTypes(); },
		[&] { getPackages(); },
		[&] { getShipments(); }
	};
}

void Generator::setRepetitions(size_t count)
{
	_repetitions = count;
}

void Generator::getProducts()
{
	_workQueue.emplace_back([&] { getProductsJob(); });
}

void Generator::getProductsCount()
{
	_workQueue.emplace_back([&] { getProductsCountJob(); });
}

void Generator::createPartners(size_t count)
{
	_workQueue.emplace_back([&] { createPartnersJob(count); });
}

void Generator::createProducts(size_t count)
{
	for(size_t i = 0; i < _repetitions; i++)
		_workQueue.emplace_back([&, count] { createProductsJob(count); });
}

void Generator::deleteAllProducts()
{
	// These must be enqueued as separate tasks
	// because the lambda finishes before the responses are parsed.
	_workQueue.emplace_back([&] { deleteAllProductsJob(); });
}

void Generator::run()
{
	QTimer::singleShot(10, [&] { process(); });
	_loop.exec();
}

void Generator::process()
{
	if(_workQueue.empty()) {
		_loop.quit();
		return;
	}
	
	auto f = _workQueue.front();
	_workQueue.pop_front();
	f();
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
		QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> r(reply);
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
		process();
	});
}

void Generator::get(const QString& path, std::function<void(QNetworkReply*)> replyParser)
{
//	QThread::msleep(200);
	
	QNetworkRequest request(_jexiaProjectUrl + path);
	request.setRawHeader("Authorization", "Bearer " + _accessToken.toUtf8());
	
	QNetworkReply* reply = _nam.get(request);
	QObject::connect(reply, &QNetworkReply::finished, [replyParser, reply] {
		QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> r(reply);
		if(!reply->isFinished())
			throw std::runtime_error("HTTP Reply is not finished");
		if(reply->isRunning())
			throw std::runtime_error("HTTP Reply is still running");
		if(reply->error() != QNetworkReply::NoError) {
			std::cout << "HTTP GET Request failed: \n" << std::endl;
			std::cout << "Url: " << reply->request().url().toString().toStdString() << "\n";
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl << std::flush;
			const QString errorString = reply->errorString();
			const auto s = "HTTP GET Request failed (" + QString::number(reply->error()) + "): " + errorString;
			std::cout << "Status code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<"\n";
			throw std::runtime_error(s.toStdString());
		}
		replyParser(reply);
	});
}

// range={"limit": 5, "offset": 3}
void Generator::getArray(const QString& path, std::function<void(QJsonObject)> apply, std::function<void(void)> finally, size_t offset)
{
	std::cout << "getArray (" << path.toStdString() << ", " << offset << ")" << std::endl << std::flush;
	
	QString paginatePath;
	if(offset == 0) {
		paginatePath = path;
	} else {
		const QString r = "{\"limit\": 1000, \"offset\": " + QString::number(offset) + "}";
		paginatePath = path + "?range=" + QUrl::toPercentEncoding(r);
	}
	
	get(paginatePath, [&, path, apply, finally, offset] (QNetworkReply* reply) {
		const QByteArray result = reply->readAll();
		const auto doc = QJsonDocument::fromJson(result);
		if(!doc.isArray())
			throw std::runtime_error("Document is not a json array");
		const auto array = doc.array();
		if(!array.isEmpty()) {
			for(const QJsonValue& element: array) {
				if(!element.isObject())
					throw std::runtime_error("Element is not an object");
				apply(element.toObject());
			}
			// Continue with next offset
			getArray(path, apply, finally, offset + array.size());
		} else {
			// If we get an empty array, we are done.
			finally();
		}
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
		std::cout << "========= Parsed partners ========= " << _partners.size() << std::endl << std::flush;
		for(auto& p: _partners)
			p.print();
		process();
	});
}

void Generator::getProductsJob()
{
	getArray("/ds/products", [&] (QJsonObject object) {
		if(!object.contains("id") || !object.contains("name"))
			throw std::runtime_error("Product JSON object is invalid");
		const QString uuid = object.value("id").toString();
		const QString name = object.value("name").toString();
		_products.emplace_back(Product{ uuid, name });
	}, [&] {
		std::cout << "========= Parsed products ========= " << _products.size() << std::endl << std::flush;
		for(auto& p: _products)
			p.print();
		process();
	});
}

void Generator::getProductsCountJob()
{
	const QString parameters = QString::fromUtf8(QByteArray("[{\"elementcount\": \"count(id)\"}]").toPercentEncoding());

	get("/ds/products?outputs=" + parameters, [&] (QNetworkReply* reply) {
		const QString r = QString::fromUtf8(reply->readAll());
		std::cout << "Get products count response: " << r.toStdString() << "END response\n";
		process();
	});
}

// From Rein
// https://af84f7fd-3376-486d-884f-6839d7de4de9.nl00.app.jexia.com/ds/test?
// cond=%5B%7B%22field%22%3A%22id%22%7D%2C%22%3D%22%2C%222a51593d-e99f-4025-b20b-159e226fc47d%22%5D


void Generator::deleteAllProductsJob()
{
	std::cout << "Deleting from products " << _products.size() << " items\n" << std::flush;
	if(_products.empty()) {
		process();
		return;
	}
	
	// This is how you can delete one product.
// 	const QString productUuid = _products.front().uuid;
// 	std::cout << ("Deleting product " + productUuid + "\n").toStdString();
// 	const QString condition = "[{\"field\":\"id\"},\"=\",\"" + productUuid + "\"]";
	
	const QString condition = "[1,\"=\",1]";
	QNetworkRequest request(_jexiaProjectUrl + "/ds/products?cond=" + QUrl::toPercentEncoding(condition));
	request.setRawHeader("Authorization", "Bearer " + _accessToken.toUtf8());
 	auto reply = _nam.deleteResource(request);
	
	QObject::connect(reply, &QNetworkReply::finished, [&, reply] {
		QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> r(reply);
		std::cout << "Delete reply finished\n" << std::flush;
		if(!reply->isFinished())
			throw std::runtime_error("HTTP Reply is not finished");
		if(reply->isRunning())
			throw std::runtime_error("HTTP Reply is still running");
		if(reply->error() != QNetworkReply::NoError) {
			std::cout << "Delete all products HTTP Request failed:\n" << std::endl;
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl << std::flush;
			const QString errorString = reply->errorString();
			const auto s = "Delete all products HTTP Request failed: " + QString::number(reply->error()) + errorString;
			throw std::runtime_error(s.toStdString());
		}
		process();
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
		std::cout << "========= Parsed package types ========= " << _packageTypes.size() << std::endl << std::flush;
		for(auto& p: _packageTypes)
			p.print();
		process();
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
		std::cout << "========= Parsed packages ========= " << _packages.size() << std::endl << std::flush;
		for(auto& p: _packages)
			p.print();
		process();
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
		std::cout << "========= Parsed shipments ========= " << _shipments.size() << std::endl << std::flush;
		for(auto& s: _shipments)
			s.print();
		process();
	});
}

void Generator::post(const QString& path, const QByteArray& data, std::function<void(QNetworkReply*)> replyParser)
{
	QNetworkRequest request(_jexiaProjectUrl + path);
	request.setRawHeader("Authorization", "Bearer " + _accessToken.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
	
	QNetworkReply* reply = _nam.post(request, data);
	QObject::connect(reply, &QNetworkReply::finished, [replyParser, reply] {
		QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> r(reply);
		if(!reply->isFinished())
			throw std::runtime_error("HTTP Reply is not finished");
		if(reply->isRunning())
			throw std::runtime_error("HTTP Reply is still running");
		if(reply->error() != QNetworkReply::NoError) {
			std::cout << "HTTP POST Request failed: \n" << std::endl;
			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl << std::flush;
			const QString errorString = reply->errorString();
			const auto s = "HTTP POST Request failed (" + QString::number(reply->error()) + "): " + errorString;
			throw std::runtime_error(s.toStdString());
		}
		replyParser(reply);
	});
}

void Generator::createPartnersJob(size_t count)
{
	const auto google = std::find_if(_partners.begin(), _partners.end(), [] (const auto& p) { return p.name == "Google"; });
	if(google == _partners.end()) {
		QJsonObject o {{"name", "Google"}};
		const QByteArray data = QJsonDocument(o).toJson();
		std::cout << QString::fromUtf8(data).toStdString() << "\n" << std::flush;
		post("/ds/partners", data, [&] (QNetworkReply* reply) {
			std::cout << "__________ Finished  ______________" << std::endl;
//			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl << std::flush;
			process();
		});
	} else {
		process();
	}
}

void Generator::createProductsJob(size_t count)
{
	static const QString base36 = "0123456789abcdefghijklmnopqrstuvwxyz";
	
	std::cout << "Creating " << count << " new products\n" << std::flush;

	if(_products.size() < _targetProductsSize) {
		const auto generateProduct = [&] {
			QString name;
			for(size_t i=0; i < 20; i++)
				name.append(base36[_randomGenerator.bounded(0, 35)]);
			return QJsonObject {{"name", name}};
		};

		QByteArray data;
		if(count == 1) {
			data = QJsonDocument(generateProduct()).toJson();
		} else {
			QJsonArray array;
			for(size_t i=0; i < count; i++)
				array.append(generateProduct());
			data = QJsonDocument(array).toJson();
		}

//		std::cout << QString::fromUtf8(data).toStdString() << "\n" << std::flush;
		post("/ds/products", data, [&] (QNetworkReply* reply) {
			std::cout << "__________ Finished  ______________" << std::endl;
//			std::cout << QString::fromUtf8(reply->readAll()).toStdString() << std::endl << std::flush;
			process();
		});
	} else {
		process();
	}
}
