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
	
	struct Partner {
		QString uuid;
		QString name;
	};

	struct Product {
		QString uuid;
		QString name;
	};
	
	struct PackageType {
		QString uuid;
		QString name;
		int quantity;
	};
	
	struct Package {
		QString uuid;
		int quantity;
	};
	
	struct Shipment {
		QString uuid;
		QString address;
	};
	
	// General HTTP infra
	void run();
	void get(const QString& path, std::function<void(QNetworkReply*)> func);
	void getArray(const QString& path, std::function<void(QJsonObject)> apply, std::function<void(void)> finally);
	void authenticate();
	
	// Model specific functions
	void getPartners();
	void getProducts();
	void getPackageTypes();
	void getPackages();
	void getShipments();
	
public slots:
	void process();
	
private:
	const QString _jexiaProjectUrl;
	const QString _jexiaKey;
	const QString _jexiaSecret;
	
	QString _accessToken;
	QString _refreshToken;
	
	std::vector<Partner> _partners;
	std::vector<Product> _products;
	std::vector<PackageType> _packageTypes;
	std::vector<Package> _packages;
	std::vector<Shipment> _shipments;
	
	QNetworkAccessManager _nam;
	QEventLoop _loop;
};

