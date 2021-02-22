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
	QObject::connect(&_nam, &QNetworkAccessManager::authenticationRequired, [] {
		std::cout << "QNetworkAccessManager::authenticationRequired - 100" << std::endl << std::flush;
	});
	
	_workQueue = {
		[&] { authenticate(); }
	};
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
	std::cout << "Authenticating\n" << std::flush;

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
		std::cout << "Authenticated\n" << std::flush;
		process();
	});
}

QString Generator::randomString(size_t length)
{
	static const QString base36 = "0123456789abcdefghijklmnopqrstuvwxyz";
	
	QString result;
	for(size_t i=0; i < length; i++)
		result.append(base36[_randomGenerator.bounded(0, 35)]);
	return result;
}

void Generator::get(const QString& path, std::function<void(QNetworkReply*)> replyParser)
{
	QThread::msleep(200);
	
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

void Generator::uploadFiles(size_t filesize, size_t filecount)
{
	_workQueue.emplace_back([&, filesize, filecount] {
		std::cout << "UploadFilesJob started" << std::endl;
		const QString r = randomString(8);
		const QByteArray data = randomByteArray(filesize);
		for(size_t i=0; i < filecount ;i++) {
			const QString filename = "Generator_" + r + "_" + QString::number(i);
			post("/fs/" + filename, data, [&] (QNetworkReply*) {
				std::cout << "UploadFilesJob: " << i << std::endl;
			});
		}
		std::cout << "UploadFilesJob completed" << std::endl << std::flush;
	});
	std::cout << "Upload files job (" << filesize << ", " << filecount << ") added" << std::endl << std::flush;
}

QByteArray Generator::randomByteArray(size_t size)
{
	return QByteArray(size, size);

	std::mt19937_64 gen(_randomGenerator.generate64());
	std::uniform_int_distribution<uint64_t> dis;
	QByteArray result;
	result.resize(size);
	
	uint64_t p = 0;
	uint64_t temp;
	while((p + 7) < size) {
		temp = dis(gen);
		memcpy(result.data() + p, &temp, 8);
		p += 8;
	}
	if(p < size) {
		temp = dis(gen);
		memcpy(result.data() + p, &temp, size - p);
	}
	return result;
}
