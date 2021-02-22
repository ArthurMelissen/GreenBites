#include <QtGlobal>
#include <QObject>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <vector>
#include <iostream>
#include <QRandomGenerator>
#include <deque>

class Generator : public QObject
{
Q_OBJECT
public:
	Generator(const QString& jexiaProjectUrl, const QString& jexiaKey, const QString& jexiaSecret);
	
	void uploadFiles();
	
	// General HTTP GET infra
	void run();
private:
	const QString _jexiaProjectUrl;
	const QString _jexiaKey;
	const QString _jexiaSecret;
	
	QString _accessToken;
	QString _refreshToken;
	
	std::deque<std::function<void(void)>> _workQueue;
	
	quint64 _targetProductsSize = 8;
	
	QNetworkAccessManager _nam;
	QEventLoop _loop;
	QRandomGenerator _randomGenerator;
	
	void get(const QString& path, std::function<void(QNetworkReply*)> func);
	void getArray(const QString& path, std::function<void(QJsonObject)> apply, std::function<void(void)> finally, size_t offset = 0);
	void authenticate();
	
	// General HTTP POST infra
	void post(const QString& path, const QByteArray& data, std::function<void(QNetworkReply*)> replyParser);

	void process();
};
