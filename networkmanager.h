#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

// API Constants
namespace API {
	const QString BASE_URL = "http://127.0.0.1:8000/api";
	const QString TOKEN_ENDPOINT = BASE_URL + "/token/";
	const QString REFRESH_TOKEN_ENDPOINT = BASE_URL + "/token/refresh/";
	const QString BOOKS_ENDPOINT = BASE_URL + "/books/";
	const QString MY_BOOKS_ENDPOINT = BASE_URL + "/books/my_books/";
	const QString CART_ENDPOINT = BASE_URL + "/cart/";
	const QString MY_CART_ENDPOINT = BASE_URL + "/cart/my_cart/";
	const QString ADD_TO_CART_ENDPOINT = BASE_URL + "/cart/add_to_cart/";
	const QString CART_PURCHASE_ENDPOINT = BASE_URL + "/cart/purchase/";
	// Purchased books (CartItems with purchased=true, collapsed to their Book
	// objects server-side) — distinct from MY_BOOKS_ENDPOINT, which lists
	// books the user owns/added as a seller.
	const QString PURCHASED_BOOKS_ENDPOINT = BASE_URL + "/cart/my_books/";
	const QString USER_DETAIL_ENDPOINT = BASE_URL + "/user-detail/";
	const QString USERS_ENDPOINT = BASE_URL + "/users/";
}

// Response structure for API calls
struct ApiResponse {
	bool success;
	QJsonDocument data;
	QString errorMessage;
	int httpStatus;

	ApiResponse() : success(false), httpStatus(0) {}
};

class NetworkManager : public QObject {
	Q_OBJECT

public:
	// Get singleton instance
	static NetworkManager& instance();

	// Set authentication token
	void setAuthToken(const QString& token);
	QString getAuthToken() const;

	// HTTP methods with automatic Authorization header
	void get(const QString& url, std::function<void(const ApiResponse&)> callback);
	void post(const QString& url, const QJsonObject& data, std::function<void(const ApiResponse&)> callback);
	void patch(const QString& url, const QJsonObject& data, std::function<void(const ApiResponse&)> callback);
	void put(const QString& url, const QJsonObject& data, std::function<void(const ApiResponse&)> callback);
	void deleteResource(const QString& url, std::function<void(const ApiResponse&)> callback);

	// Multipart form data for file uploads
	void postMultipart(const QString& url, QHttpMultiPart* multiPart, std::function<void(const ApiResponse&)> callback);

private:
	// Private constructor for singleton
	explicit NetworkManager(QObject *parent = nullptr);
	~NetworkManager();

	// Delete copy constructor and assignment operator
	NetworkManager(const NetworkManager&) = delete;
	NetworkManager& operator=(const NetworkManager&) = delete;

	QNetworkAccessManager* networkManager;
	QString authToken;

	// Helper method to create authenticated request
	QNetworkRequest createRequest(const QString& url);

	// Helper method to process response
	void processReply(QNetworkReply* reply, std::function<void(const ApiResponse&)> callback);

	// Signal-slot based callback handler
	void onReplyFinished(QNetworkReply* reply, std::function<void(const ApiResponse&)> callback);
};

#endif // NETWORKMANAGER_H
