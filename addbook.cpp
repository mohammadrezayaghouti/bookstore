#include "addbook.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "QFile"
#include "QFileDialog"
#include <QHttpMultiPart>
#include <QHttpPart>
#include "networkmanager.h"

AddBook::AddBook(QWidget *parent, const QString& token) : QWidget(parent), authToken(token) {
    setObjectName("pageRoot");

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scrollArea);

    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName("pageRoot");
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(24, 24, 24, 24);

    QFrame *card = new QFrame(scrollContent);
    card->setObjectName("authCard");
    card->setMaximumWidth(480);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 28, 28, 28);
    cardLayout->setSpacing(16);

    QLabel *title = new QLabel("Add New Book", card);
    title->setObjectName("pageTitle");
    cardLayout->addWidget(title);

    QFormLayout *form = new QFormLayout();
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft);

    titleEdit = new QLineEdit(card);
    authorEdit = new QLineEdit(card);
    publisherEdit = new QLineEdit(card);
    priceEdit = new QLineEdit(card);
    stockEdit = new QLineEdit(card);
    // QTextEdit's own frame doesn't reliably pick up border/background styling
    // (a QAbstractScrollArea quirk), so it's wrapped in a plain QFrame that
    // carries the input look, with the text edit itself left borderless inside.
    QFrame *descriptionFrame = new QFrame(card);
    descriptionFrame->setObjectName("inputFrame");
    QVBoxLayout *descriptionFrameLayout = new QVBoxLayout(descriptionFrame);
    descriptionFrameLayout->setContentsMargins(1, 1, 1, 1);

    descriptionEdit = new QTextEdit(descriptionFrame);
    descriptionEdit->setFixedHeight(88);
    descriptionEdit->setFrameShape(QFrame::NoFrame);
    descriptionEdit->setStyleSheet("background: transparent; border: none; padding: 7px 9px;");
    descriptionFrameLayout->addWidget(descriptionEdit);

    form->addRow("Title", titleEdit);
    form->addRow("Author", authorEdit);
    form->addRow("Publisher", publisherEdit);
    form->addRow("Price", priceEdit);
    form->addRow("Stock", stockEdit);
    form->addRow("Description", descriptionFrame);

    for (int i = 0; i < form->rowCount(); ++i) {
        QLayoutItem *labelItem = form->itemAt(i, QFormLayout::LabelRole);
        if (labelItem && labelItem->widget()) {
            labelItem->widget()->setObjectName("fieldLabel");
        }
    }

    cardLayout->addLayout(form);

    QHBoxLayout *imageRow = new QHBoxLayout();
    imagePreview = new QLabel("No image selected", card);
    imagePreview->setObjectName("coverLabel");
    imagePreview->setFixedSize(100, 130);
    imagePreview->setAlignment(Qt::AlignCenter);
    imagePreview->setWordWrap(true);

    QPushButton *uploadButton = new QPushButton("Upload Image", card);
    uploadButton->setObjectName("secondaryButton");
    uploadButton->setCursor(Qt::PointingHandCursor);
    connect(uploadButton, &QPushButton::clicked, this, &AddBook::uploadImage);

    imageRow->addWidget(imagePreview);
    imageRow->addWidget(uploadButton);
    imageRow->addStretch();
    cardLayout->addLayout(imageRow);

    QPushButton *saveButton = new QPushButton("Save Book", card);
    saveButton->setObjectName("primaryButton");
    saveButton->setCursor(Qt::PointingHandCursor);
    saveButton->setMinimumHeight(38);
    connect(saveButton, &QPushButton::clicked, this, &AddBook::saveBook);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(saveButton);

    QHBoxLayout *centerRow = new QHBoxLayout();
    centerRow->addStretch();
    centerRow->addWidget(card);
    centerRow->addStretch();

    scrollLayout->addLayout(centerRow);
    scrollLayout->addStretch();
    scrollArea->setWidget(scrollContent);

    setLayout(outer);
}

void AddBook::uploadImage() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Book Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (!filePath.isEmpty()) {
        QImage image(filePath);
        if (!image.isNull()) {
            QPixmap pixmap = QPixmap::fromImage(image);
            imagePreview->setPixmap(pixmap.scaled(imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            imagePath = filePath;
        } else {
            QMessageBox::warning(this, "Error", "Failed to load image.");
        }
    }
}

void AddBook::saveBook() {
    // Validate stock value
    bool ok;
    int stockValue = stockEdit->text().toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Error", "Invalid stock value. Please enter a valid number.");
        return;
    }

    // Prepare multipart form data
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add form fields
    QHttpPart titlePart;
    titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"title\"");
    titlePart.setBody(titleEdit->text().toUtf8());
    multiPart->append(titlePart);

    QHttpPart authorPart;
    authorPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"author\"");
    authorPart.setBody(authorEdit->text().toUtf8());
    multiPart->append(authorPart);

    QHttpPart publisherPart;
    publisherPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"publisher\"");
    publisherPart.setBody(publisherEdit->text().toUtf8());
    multiPart->append(publisherPart);

    QHttpPart pricePart;
    pricePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"price\"");
    pricePart.setBody(priceEdit->text().toUtf8());
    multiPart->append(pricePart);

    QHttpPart stockPart;
    stockPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"stock\"");
    stockPart.setBody(QByteArray::number(stockValue));
    multiPart->append(stockPart);

    QHttpPart descriptionPart;
    descriptionPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"description\"");
    descriptionPart.setBody(descriptionEdit->toPlainText().toUtf8());
    multiPart->append(descriptionPart);

    // Add image if selected
    if (!imagePath.isEmpty()) {
        QFile file(imagePath);
        if (file.open(QIODevice::ReadOnly)) {
            QHttpPart imagePart;
            imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                "form-data; name=\"cover_image\"; filename=\"" + QFileInfo(file).fileName() + "\"");
            imagePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
            imagePart.setBody(file.readAll());
            multiPart->append(imagePart);
            file.close();
        } else {
            QMessageBox::warning(this, "Error", "Failed to read the image file.");
            multiPart->deleteLater();
            return;
        }
    }

    // Send request using NetworkManager
    QUrl url(API::BOOKS_ENDPOINT);
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkAccessManager* nm = new QNetworkAccessManager(this);
    QNetworkReply *reply = nm->post(request, multiPart);

    connect(reply, &QNetworkReply::finished, [reply, multiPart, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "Book added successfully.");
            emit bookAdded();
        } else {
            QByteArray serverResponse = reply->readAll();
            qDebug() << "Error saving book: " << reply->errorString();
            QMessageBox::warning(this, "Error", "Failed to add book.\nServer response: " + QString(serverResponse));
        }

        reply->deleteLater();
        multiPart->deleteLater();
    });
}

AddBook::~AddBook() {
    // destructor code
}

void AddBook::updateProductList() {
    emit productListUpdated();
}
