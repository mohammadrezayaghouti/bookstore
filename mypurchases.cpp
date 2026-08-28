#include "mypurchases.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPointer>
#include <QStyle>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>
#include "networkmanager.h"

MyPurchases::MyPurchases(QWidget *parent) : QWidget(parent)
{
    setObjectName("pageRoot");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel("My Purchases", this);
    titleLabel->setObjectName("pageTitle");
    mainLayout->addWidget(titleLabel);

    stateLabel = new QLabel(this);
    stateLabel->setObjectName("emptyStateText");
    stateLabel->setAlignment(Qt::AlignCenter);
    stateLabel->setVisible(false);
    mainLayout->addWidget(stateLabel);

    retryButton = new QPushButton("Retry", this);
    retryButton->setObjectName("secondaryButton");
    retryButton->setVisible(false);
    connect(retryButton, &QPushButton::clicked, this, &MyPurchases::loadPurchases);
    mainLayout->addWidget(retryButton, 0, Qt::AlignHCenter);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    booksContainer = new QWidget(scrollArea);
    booksContainer->setObjectName("pageRoot");
    gridLayout = new QGridLayout(booksContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(16);
    gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    scrollArea->setWidget(booksContainer);
    mainLayout->addWidget(scrollArea, 1);

    setWindowTitle("My Purchases");
    resize(900, 640);

    loadPurchases();
}

void MyPurchases::setViewState(ViewState state, const QString &message)
{
    switch (state) {
    case ViewState::Loading:
        stateLabel->setObjectName("loadingText");
        stateLabel->setText("Loading purchases...");
        stateLabel->setVisible(true);
        retryButton->setVisible(false);
        scrollArea->setVisible(false);
        break;
    case ViewState::Empty:
        stateLabel->setObjectName("emptyStateText");
        stateLabel->setText("You haven't purchased any books yet.");
        stateLabel->setVisible(true);
        retryButton->setVisible(false);
        scrollArea->setVisible(false);
        break;
    case ViewState::Error:
        stateLabel->setObjectName("errorText");
        stateLabel->setText(message.isEmpty() ? "Unable to load purchases." : message);
        stateLabel->setVisible(true);
        retryButton->setVisible(true);
        scrollArea->setVisible(false);
        break;
    case ViewState::Ready:
        stateLabel->setVisible(false);
        retryButton->setVisible(false);
        scrollArea->setVisible(true);
        break;
    }
    stateLabel->style()->unpolish(stateLabel);
    stateLabel->style()->polish(stateLabel);
}

void MyPurchases::clearBookWidgets()
{
    while (gridLayout->count() > 0) {
        QLayoutItem *item = gridLayout->takeAt(0);
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

void MyPurchases::loadPurchases()
{
    if (isLoading) {
        return;
    }
    isLoading = true;
    setViewState(ViewState::Loading);

    qDebug() << "[MyPurchases] GET" << API::PURCHASED_BOOKS_ENDPOINT;

    NetworkManager::instance().get(API::PURCHASED_BOOKS_ENDPOINT, [this](const ApiResponse &response) {
        isLoading = false;
        qDebug() << "[MyPurchases] HTTP status=" << response.httpStatus;

        if (!response.success) {
            // A failed request must never be presented as "no purchases".
            setViewState(ViewState::Error, "Unable to load purchases: " + response.errorMessage);
            return;
        }

        if (!response.data.isArray()) {
            setViewState(ViewState::Error, "Unexpected server response.");
            return;
        }

        QJsonArray books = response.data.array();
        if (books.isEmpty()) {
            clearBookWidgets();
            setViewState(ViewState::Empty);
            return;
        }

        displayBooks(books);
        setViewState(ViewState::Ready);
    });
}

void MyPurchases::displayBooks(const QJsonArray &books)
{
    clearBookWidgets();

    const int columns = 4;
    int index = 0;

    for (const QJsonValue &bookVal : books) {
        if (!bookVal.isObject()) {
            continue;
        }
        QJsonObject book = bookVal.toObject();

        QFrame *bookFrame = new QFrame(booksContainer);
        bookFrame->setObjectName("bookCard");
        bookFrame->setFixedWidth(196);

        QVBoxLayout *bookLayout = new QVBoxLayout(bookFrame);
        bookLayout->setContentsMargins(12, 12, 12, 12);
        bookLayout->setSpacing(6);

        QLabel *coverLabel = new QLabel(bookFrame);
        coverLabel->setObjectName("coverLabel");
        coverLabel->setFixedSize(172, 220);
        coverLabel->setAlignment(Qt::AlignCenter);
        coverLabel->setText("No Cover");

        QString coverUrl = book.value("cover_image").toString();
        if (!coverUrl.isEmpty()) {
            QNetworkAccessManager *manager = new QNetworkAccessManager(bookFrame);
            QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(coverUrl)));
            connect(reply, &QNetworkReply::finished, coverLabel, [reply, coverLabel]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QPixmap pixmap;
                    if (pixmap.loadFromData(reply->readAll())) {
                        coverLabel->setPixmap(pixmap.scaled(coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        coverLabel->setText(QString());
                    }
                }
                reply->deleteLater();
            });
        }
        bookLayout->addWidget(coverLabel);

        QLabel *titleLabel = new QLabel(book.value("title").toString(), bookFrame);
        titleLabel->setObjectName("bookTitle");
        titleLabel->setWordWrap(true);
        bookLayout->addWidget(titleLabel);

        QLabel *authorLabel = new QLabel("By " + book.value("author").toString(), bookFrame);
        authorLabel->setObjectName("bookAuthor");
        authorLabel->setWordWrap(true);
        bookLayout->addWidget(authorLabel);

        bookLayout->addStretch();

        QLabel *priceLabel = new QLabel(book.value("price").toString(), bookFrame);
        priceLabel->setObjectName("bookPrice");
        bookLayout->addWidget(priceLabel);

        QPushButton *detailsButton = new QPushButton("View Details", bookFrame);
        detailsButton->setObjectName("secondaryButton");
        detailsButton->setCursor(Qt::PointingHandCursor);
        connect(detailsButton, &QPushButton::clicked, this, [this, book]() {
            showBookDetails(book);
        });
        bookLayout->addWidget(detailsButton);

        int row = index / columns;
        int col = index % columns;
        gridLayout->addWidget(bookFrame, row, col);
        ++index;
    }
}

void MyPurchases::showBookDetails(const QJsonObject &book)
{
    QDialog *dialog = new QDialog(this);
    dialog->setObjectName("pageRoot");
    dialog->setAttribute(Qt::WA_StyledBackground, true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(book.value("title").toString());
    dialog->resize(400, 420);

    QVBoxLayout *dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    dialogLayout->setSpacing(12);

    QLabel *imageLabel = new QLabel(dialog);
    imageLabel->setObjectName("coverLabel");
    imageLabel->setFixedSize(150, 200);
    imageLabel->setAlignment(Qt::AlignCenter);
    QString imageUrl = book.value("cover_image").toString();
    if (imageUrl.isEmpty()) {
        imageLabel->setText("No Image");
    } else {
        QNetworkAccessManager *manager = new QNetworkAccessManager(dialog);
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(imageUrl)));
        connect(reply, &QNetworkReply::finished, imageLabel, [reply, imageLabel]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                if (pixmap.loadFromData(reply->readAll())) {
                    imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    imageLabel->setText("No Image");
                }
            } else {
                imageLabel->setText("No Image");
            }
            reply->deleteLater();
        });
    }
    QHBoxLayout *imageRow = new QHBoxLayout();
    imageRow->addStretch();
    imageRow->addWidget(imageLabel);
    imageRow->addStretch();
    dialogLayout->addLayout(imageRow);

    // Only fields the backend actually returns for a purchased book are
    // shown here — no invented quantity, price-paid, or purchase date.
    QFormLayout *infoLayout = new QFormLayout;
    infoLayout->setSpacing(8);
    infoLayout->addRow("Title:", new QLabel(book.value("title").toString()));
    infoLayout->addRow("Author:", new QLabel(book.value("author").toString("Unknown")));
    infoLayout->addRow("Publisher:", new QLabel(book.value("publisher").toString("Unknown")));
    QLabel *priceValueLabel = new QLabel(book.value("price").toString("N/A"));
    priceValueLabel->setObjectName("bookPrice");
    infoLayout->addRow("Price:", priceValueLabel);

    QLabel *descriptionLabel = new QLabel(book.value("description").toString("No Description"));
    descriptionLabel->setWordWrap(true);
    infoLayout->addRow("Description:", descriptionLabel);

    dialogLayout->addLayout(infoLayout);
    dialogLayout->addStretch();

    QPushButton *closeButton = new QPushButton("Close", dialog);
    closeButton->setObjectName("secondaryButton");
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    dialogLayout->addWidget(closeButton);

    dialog->exec();
}
