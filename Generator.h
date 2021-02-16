#include <QtGlobal>
#include <QObject>
#include <QNetworkAccessManager>
#include <QEventLoop>

class Generator : public QObject
{
Q_OBJECT
public:
	Generator(const QString& jexiaProjectUrl, const QString& jexiaKey, const QString& jexiaSecret);
	
	enum State {
		QueryPartners,
		GeneratePartners,
		Finished
	};

	void run();
	void queryPartners();
	
public slots:
	void process();
	
private:
	const QString _jexiaProjectUrl;
	const QString _jexiaKey;
	const QString _jexiaSecret;
	
	QNetworkAccessManager _nam;
	State _state = QueryPartners;
};

