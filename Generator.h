#include <QtGlobal>
#include <QObject>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <vector>

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
	
	struct Partner {
		QString uuid;
		QString name;
	};

	void run();
	void authenticate();
	void queryPartners();
	void parsePartners(const QByteArray& result);
	
public slots:
	void process();
	
private:
	const QString _jexiaProjectUrl;
	const QString _jexiaKey;
	const QString _jexiaSecret;
	
	QString _accessToken;
	QString _refreshToken;
	
	std::vector<Partner> _partners;
	
	QNetworkAccessManager _nam;
	State _state = QueryPartners;
};

