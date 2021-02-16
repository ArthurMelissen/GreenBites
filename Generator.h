#include <QtGlobal>
#include <QObject>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <vector>
#include <iostream>

class Generator : public QObject
{
Q_OBJECT
public:
	Generator(const QString& jexiaProjectUrl, const QString& jexiaKey, const QString& jexiaSecret);
	
	struct Partner {
		QString uuid;
		QString name;
		
		void print() {
			std::cout << uuid.toStdString() << ": " << name.toStdString() << "\n";
		}
	};

	struct Product {
		QString uuid;
		QString name;
		
		void print() {
			std::cout << uuid.toStdString() << ": " << name.toStdString() << "\n";
		}
	};
	
	struct PackageType {
		QString uuid;
		QString name;
		int quantity;
		
		void print() {
			std::cout << uuid.toStdString() << ": " << name.toStdString() << "(" << quantity << ")\n";
		}
	};
	
	struct Package {
		QString uuid;
		int quantity;
		
		void print() {
			std::cout << uuid.toStdString() << ": " << quantity << "\n";
		}
	};
	
	struct Shipment {
		QString uuid;
		QString address;
		
		void print() {
			std::cout << uuid.toStdString() << ": " << address.toStdString() << "\n";
		}
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

