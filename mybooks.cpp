#include "mybooks.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QScrollArea>
#include <QGridLayout>
#include <QPixmap>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QEvent>
#include <QDialog>
#include "networkmanager.h"

MyBooks::MyBooks(QWidget *parent, const QString& token) : QWidget(parent), authToken(token) {
    setObjectName("pageRoot");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(16);

    // عنوان صفحه
    QLabel *titleLabel = new QLabel("My Books", this);
    titleLabel->setObjectName("pageTitle");
    mainLayout->addWidget(titleLabel);

    // ایجاد یک ScrollArea برای نمایش لیست کتاب‌ها
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *contentWidget = new QWidget(scrollArea);
    contentWidget->setObjectName("pageRoot");
    gridLayout = new QGridLayout(contentWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(16);
    gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    contentWidget->setLayout(gridLayout);

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);

    // دریافت لیست کتاب‌ها
    NetworkManager::instance().setAuthToken(authToken);
    fetchBooks();

    setLayout(mainLayout);
    resize(900, 640);
}

void MyBooks::fetchBooks() {
    NetworkManager::instance().get(API::MY_BOOKS_ENDPOINT, [this](const ApiResponse& response) {
        if (response.success && response.data.isArray()) {
            QByteArray responseData = QJsonDocument(response.data).toJson();
            parseBooks(responseData);
        } else {
            qDebug() << "Error fetching books: " << response.errorMessage;
            QMessageBox::warning(this, "Error", "Failed to fetch books.");
        }
    });
}

void MyBooks::parseBooks(const QByteArray &data) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (!jsonDoc.isArray()) {
        qDebug() << "Invalid JSON format.";
        return;
    }

    QJsonArray booksArray = jsonDoc.array();

    if (booksArray.isEmpty()) {
        QLabel *emptyLabel = new QLabel("No books added yet", gridLayout->parentWidget());
        emptyLabel->setObjectName("emptyStateText");
        gridLayout->addWidget(emptyLabel, 0, 0);
        return;
    }

    const int columns = 4;

    for (const QJsonValue &value : booksArray) {
        QJsonObject bookObj = value.toObject();

        // ایجاد کادر برای نمایش هر کتاب
        QFrame *bookFrame = new QFrame(gridLayout->parentWidget());
        bookFrame->setObjectName("bookCard");
        bookFrame->setFixedWidth(196);
        bookFrame->setCursor(Qt::PointingHandCursor);

        QVBoxLayout *bookLayout = new QVBoxLayout(bookFrame);
        bookLayout->setContentsMargins(12, 12, 12, 12);
        bookLayout->setSpacing(6);

        // ایجاد و اضافه کردن تصویر کتاب
        QLabel *imageLabel = new QLabel(bookFrame);
        imageLabel->setObjectName("coverLabel");
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setFixedSize(172, 220);
        imageLabel->setText("No Cover");

        QString imageUrl = bookObj["cover_image"].toString();
        if (!imageUrl.isEmpty()) {
            // Download image from URL
            QUrl url(imageUrl);
            if (url.isValid()) {
                QNetworkAccessManager *imgManager = new QNetworkAccessManager(bookFrame);
                QNetworkRequest imageRequest;
                imageRequest.setUrl(url);
                QNetworkReply *imageReply = imgManager->get(imageRequest);

                connect(imageReply, &QNetworkReply::finished, imageLabel, [imageReply, imageLabel]() {
                    if (imageReply->error() == QNetworkReply::NoError) {
                        QPixmap pixmap;
                        pixmap.loadFromData(imageReply->readAll());
                        if (!pixmap.isNull()) {
                            imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            imageLabel->setText(QString());
                        }
                    }
                    imageReply->deleteLater();
                });
            }
        }
        bookLayout->addWidget(imageLabel);

        QLabel *titleLabel = new QLabel(bookObj["title"].toString(), bookFrame);
        titleLabel->setObjectName("bookTitle");
        titleLabel->setWordWrap(true);
        bookLayout->addWidget(titleLabel);

        bookLayout->addStretch();

        QLabel *priceLabel = new QLabel(bookObj["price"].toString(), bookFrame);
        priceLabel->setObjectName("bookPrice");
        bookLayout->addWidget(priceLabel);

        QPushButton *detailsButton = new QPushButton("View Details", bookFrame);
        detailsButton->setObjectName("secondaryButton");
        detailsButton->setCursor(Qt::PointingHandCursor);
        connect(detailsButton, &QPushButton::clicked, this, [this, bookObj]() {
            showBookDetails(bookObj);
        });
        bookLayout->addWidget(detailsButton);

        bookFrame->setLayout(bookLayout);
        bookFrame->installEventFilter(this);

        int index = gridLayout->count();
        int row = index / columns;
        int col = index % columns;
        gridLayout->addWidget(bookFrame, row, col);
        bookFrames.insert(bookFrame, QVariant::fromValue(bookObj));
    }
}




bool MyBooks::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *frame = qobject_cast<QFrame*>(obj);
        if (frame && bookFrames.contains(frame)) {
            showBookDetails(bookFrames.value(frame).toJsonObject()); // تبدیل به QJsonObject
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}


void MyBooks::showBookDetails(const QJsonObject &bookObj) {
    QDialog *detailsDialog = new QDialog(this);
    detailsDialog->setObjectName("pageRoot");
    detailsDialog->setAttribute(Qt::WA_StyledBackground, true);
    detailsDialog->setAttribute(Qt::WA_DeleteOnClose);
    detailsDialog->setWindowTitle(bookObj["title"].toString());
    detailsDialog->resize(400, 360);

    QVBoxLayout *dialogLayout = new QVBoxLayout(detailsDialog);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    dialogLayout->setSpacing(12);

    // تصویر
    QLabel *imageLabel = new QLabel(detailsDialog);
    imageLabel->setObjectName("coverLabel");
    imageLabel->setFixedSize(150, 200);
    imageLabel->setAlignment(Qt::AlignCenter);
    QString imageUrl = bookObj["cover_image"].toString();

    if (!imageUrl.isEmpty()) {
        QNetworkAccessManager *imgManager = new QNetworkAccessManager(detailsDialog);
        QNetworkRequest imageRequest;
        imageRequest.setUrl(QUrl(imageUrl));
        QNetworkReply *imageReply = imgManager->get(imageRequest);

        connect(imageReply, &QNetworkReply::finished, imageLabel, [imageReply, imageLabel]() {
            if (imageReply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(imageReply->readAll());
                imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                imageLabel->setText("No Image");
            }
            imageReply->deleteLater();
        });
    } else {
        imageLabel->setText("No Image");
    }

    QHBoxLayout *imageRow = new QHBoxLayout();
    imageRow->addStretch();
    imageRow->addWidget(imageLabel);
    imageRow->addStretch();
    dialogLayout->addLayout(imageRow);

    // اطلاعات
    QLabel *titleLabel = new QLabel(bookObj["title"].toString());
    titleLabel->setObjectName("sectionTitle");
    dialogLayout->addWidget(titleLabel);
    dialogLayout->addWidget(new QLabel("By " + bookObj["author"].toString("Unknown")));
    dialogLayout->addWidget(new QLabel("Publisher: " + bookObj["publisher"].toString("Unknown")));
    QLabel *priceLabel = new QLabel(bookObj["price"].toString("N/A"));
    priceLabel->setObjectName("bookPrice");
    dialogLayout->addWidget(priceLabel);
    dialogLayout->addStretch();

    QPushButton *closeButton = new QPushButton("Close", detailsDialog);
    closeButton->setObjectName("secondaryButton");
    connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);
    dialogLayout->addWidget(closeButton);

    detailsDialog->setLayout(dialogLayout);
    detailsDialog->exec();
}
