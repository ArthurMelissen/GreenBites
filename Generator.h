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
	
	void createPartners(size_t count);
	void createProducts(size_t count);
	void deleteProducts();
	
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
	
	// General HTTP GET infra
	void run();
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
	
	std::deque<std::function<void(void)>> _workQueue;
	
	quint64 _targetProductsSize = 8;
	
	QNetworkAccessManager _nam;
	QEventLoop _loop;
	QRandomGenerator _randomGenerator;
	
	void get(const QString& path, std::function<void(QNetworkReply*)> func);
	void getArray(const QString& path, std::function<void(QJsonObject)> apply, std::function<void(void)> finally);
	void authenticate();
	
	// Model specific getters
	void getPartners();
	void getProducts();
	void deleteProductsJob();
	void getPackageTypes();
	void getPackages();
	void getShipments();
	
	// General HTTP POST infra
	void post(const QString& path, const QByteArray& data, std::function<void(QNetworkReply*)> replyParser);

	// Model specific creaters
	void createPartnersJob(size_t count);
	void createProductsJob(size_t count);
	
	void process();
};
