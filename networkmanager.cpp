#include "networkmanager.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QHttpMultiPart>

// Static instance
static NetworkManager* g_instance = nullptr;

NetworkManager::NetworkManager(QObject *parent)
	: QObject(parent), networkManager(new QNetworkAccessManager(this))
{
}

NetworkManager::~NetworkManager()
{
}

NetworkManager& NetworkManager::instance()
{
	if (!g_instance) {
		g_instance = new NetworkManager();
	}
	return *g_instance;
}

void NetworkManager::setAuthToken(const QString& token)
{
	authToken = token;
	qDebug() << "NetworkManager: Auth token set";
}

QString NetworkManager::getAuthToken() const
{
	return authToken;
}

QNetworkRequest NetworkManager::createRequest(const QString& url)
{
	QUrl qurl(url);
	QNetworkRequest request;
	request.setUrl(qurl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	if (!authToken.isEmpty()) {
		request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());
	}

	return request;
}

void NetworkManager::get(const QString& url, std::function<void(const ApiResponse&)> callback)
{
	QNetworkRequest request = createRequest(url);
	QNetworkReply* reply = networkManager->get(request);

	onReplyFinished(reply, callback);
}

void NetworkManager::post(const QString& url, const QJsonObject& data, std::function<void(const ApiResponse&)> callback)
{
	QNetworkRequest request = createRequest(url);
	QByteArray jsonData = QJsonDocument(data).toJson();
	QNetworkReply* reply = networkManager->post(request, jsonData);

	onReplyFinished(reply, callback);
}

void NetworkManager::patch(const QString& url, const QJsonObject& data, std::function<void(const ApiResponse&)> callback)
{
	QNetworkRequest request = createRequest(url);
	QByteArray jsonData = QJsonDocument(data).toJson();
	QNetworkReply* reply = networkManager->sendCustomRequest(request, "PATCH", jsonData);

	onReplyFinished(reply, callback);
}

void NetworkManager::put(const QString& url, const QJsonObject& data, std::function<void(const ApiResponse&)> callback)
{
	QNetworkRequest request = createRequest(url);
	QByteArray jsonData = QJsonDocument(data).toJson();
	QNetworkReply* reply = networkManager->put(request, jsonData);

	onReplyFinished(reply, callback);
}

void NetworkManager::deleteResource(const QString& url, std::function<void(const ApiResponse&)> callback)
{
	QNetworkRequest request = createRequest(url);
	QNetworkReply* reply = networkManager->deleteResource(request);

	onReplyFinished(reply, callback);
}

void NetworkManager::postMultipart(const QString& url, QHttpMultiPart* multiPart, std::function<void(const ApiResponse&)> callback)
{
	QNetworkRequest request = createRequest(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, ""); // Clear content type, let Qt set it
	QNetworkReply* reply = networkManager->post(request, multiPart);

	onReplyFinished(reply, callback);
}

void NetworkManager::onReplyFinished(QNetworkReply* reply, std::function<void(const ApiResponse&)> callback)
{
	connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
		processReply(reply, callback);
	});
}

void NetworkManager::processReply(QNetworkReply* reply, std::function<void(const ApiResponse&)> callback)
{
	ApiResponse response;
	response.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (reply->error() == QNetworkReply::NoError) {
		QByteArray responseData = reply->readAll();

		if (responseData.isEmpty()) {
			// e.g. HTTP 204 No Content from a DELETE — a genuinely successful
			// response with nothing to parse, not a JSON error.
			response.success = true;
			qDebug() << "NetworkManager: Request successful (empty body), status:" << response.httpStatus;
		} else {
			QJsonParseError parseError;
			response.data = QJsonDocument::fromJson(responseData, &parseError);

			if (parseError.error != QJsonParseError::NoError) {
				response.success = false;
				response.errorMessage = "JSON parse error: " + parseError.errorString();
				qDebug() << "NetworkManager: JSON parse error:" << response.errorMessage;
			} else {
				response.success = true;
				qDebug() << "NetworkManager: Request successful, status:" << response.httpStatus;
			}
		}
	} else {
		response.success = false;
		response.errorMessage = reply->errorString();
		QByteArray responseData = reply->readAll();

		// Try to parse error message from response
		if (!responseData.isEmpty()) {
			QJsonParseError parseError;
			QJsonDocument errorDoc = QJsonDocument::fromJson(responseData, &parseError);
			if (parseError.error == QJsonParseError::NoError && errorDoc.isObject()) {
				response.data = errorDoc;
			}
		}

		qDebug() << "NetworkManager: Network error:" << response.errorMessage
				 << "Status:" << response.httpStatus
				 << "Response:" << responseData;
	}

	callback(response);
	reply->deleteLater();
}
