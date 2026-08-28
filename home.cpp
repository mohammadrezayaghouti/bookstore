#include "home.h"
#include <QEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "userprofile.h"
#include "addbook.h"
#include <QJsonArray>
#include <QMessageBox>
#include <QFrame>
#include <QScrollArea>
#include <QPixmap>
#include <QPointer>
#include <QDebug>
#include <QGridLayout>
#include <QDialog>
#include "mybooks.h"
#include "mypurchases.h"
#include "networkmanager.h"
#include "QFormLayout"

namespace {
QPushButton *makeNavButton(const QString &text, QWidget *parent) {
    QPushButton *btn = new QPushButton(text, parent);
    btn->setObjectName("navButton");
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}
}

Home::Home(QWidget *parent)
    : QMainWindow(parent),
    networkManager(new QNetworkAccessManager(this)),
    cartWindow(nullptr) {
    setObjectName("pageRoot");

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setObjectName("pageRoot");
    setCentralWidget(centralWidget);

    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---------------- Header / top navigation ----------------
    QFrame *header = new QFrame(centralWidget);
    header->setObjectName("header");
    header->setFixedHeight(60);

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 0, 24, 0);
    headerLayout->setSpacing(4);

    QLabel *appTitle = new QLabel("BookStore", header);
    appTitle->setObjectName("appTitle");

    homeNavButton = makeNavButton("Home", header);
    homeNavButton->setProperty("active", true);
    myBooksNavButton = makeNavButton("My Books", header);
    myPurchasesNavButton = makeNavButton("My Purchases", header);
    addProductButton = makeNavButton("Add Book", header);
    cartButton = makeNavButton("Cart", header);
    profileNavButton = makeNavButton("Profile", header);
    logoutNavButton = makeNavButton("Logout", header);
    logoutNavButton->setProperty("role", "danger");

    headerLayout->addWidget(appTitle);
    headerLayout->addSpacing(32);
    headerLayout->addWidget(homeNavButton);
    headerLayout->addWidget(myBooksNavButton);
    headerLayout->addWidget(myPurchasesNavButton);
    headerLayout->addWidget(addProductButton);
    headerLayout->addStretch();
    headerLayout->addWidget(cartButton);
    headerLayout->addWidget(profileNavButton);
    headerLayout->addWidget(logoutNavButton);

    mainLayout->addWidget(header);

    // ---------------- Content area ----------------
    QWidget *content = new QWidget(centralWidget);
    content->setObjectName("pageRoot");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 20, 24, 24);
    contentLayout->setSpacing(16);

    QLabel *pageTitle = new QLabel("Home", content);
    pageTitle->setObjectName("pageTitle");
    contentLayout->addWidget(pageTitle);

    QHBoxLayout *searchRow = new QHBoxLayout();
    searchEdit = new QLineEdit(content);
    searchEdit->setObjectName("searchInput");
    searchEdit->setPlaceholderText("Search books...");
    searchEdit->setMaximumWidth(320);
    searchEdit->setMinimumHeight(34);
    searchRow->addWidget(searchEdit);
    searchRow->addStretch();
    contentLayout->addLayout(searchRow);

    QScrollArea *scrollArea = new QScrollArea(content);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    booksContainer = new QWidget(scrollArea);
    booksContainer->setObjectName("pageRoot");
    QGridLayout *gridLayout = new QGridLayout(booksContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(16);
    gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scrollArea->setWidget(booksContainer);
    contentLayout->addWidget(scrollArea, 1);

    mainLayout->addWidget(content, 1);

    // ---------------- Networking ----------------
    QNetworkRequest request{QUrl(API::BOOKS_ENDPOINT)};
    QNetworkReply *booksReply = networkManager->get(request);
    connect(booksReply, &QNetworkReply::finished, this, [this, booksReply]() {
        onBooksFetched(booksReply);
    });

    // ---------------- Connections ----------------
    connect(cartButton, &QPushButton::clicked, this, &Home::onCartClicked);
    connect(addProductButton, &QPushButton::clicked, this, &Home::onAddProductClicked);
    connect(searchEdit, &QLineEdit::textChanged, this, &Home::onSearchTextChanged);
    connect(myBooksNavButton, &QPushButton::clicked, this, &Home::onMyBooksClicked);
    connect(myPurchasesNavButton, &QPushButton::clicked, this, &Home::onMyPurchasesClicked);
    connect(profileNavButton, &QPushButton::clicked, this, &Home::onProfileClicked);
    connect(logoutNavButton, &QPushButton::clicked, this, &Home::performLogout);

    resize(1024, 720);
}

Home::~Home() {}

void Home::onBooksFetched(QNetworkReply *reply) {
    if (!reply) {
        qDebug() << "Network reply is null.";
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network Error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON Parse Error:" << parseError.errorString();
        reply->deleteLater();
        return;
    }

    if (!jsonDoc.isArray()) {
        qDebug() << "Invalid JSON format: Expected an array.";
        reply->deleteLater();
        return;
    }

    QJsonArray booksArray = jsonDoc.array();

    // ارسال آرایه کتاب‌ها برای نمایش
    displayBooks(booksArray);

    reply->deleteLater();
}

void Home::displayBooks(const QJsonArray &booksArray) {
    QGridLayout *gridLayout = qobject_cast<QGridLayout *>(booksContainer->layout());
    if (!gridLayout) {
        return;
    }

    if (booksArray.isEmpty()) {
        QLabel *emptyLabel = new QLabel("No books found", booksContainer);
        emptyLabel->setObjectName("emptyStateText");
        gridLayout->addWidget(emptyLabel, 0, 0);
        return;
    }

    const int columns = 4;

    for (const QJsonValue &bookValue : booksArray) {
        if (!bookValue.isObject()) {
            continue;
        }

        QJsonObject bookObject = bookValue.toObject();
        QString title = bookObject.value("title").toString();
        QString author = bookObject.value("author").toString();
        QString price = bookObject.value("price").toString();
        QString coverImageUrl = bookObject.value("cover_image").toString();
        const int bookId = bookObject.value("id").toInt();
        qDebug() << "[Home] Book card created. title=" << title << "bookId=" << bookId;

        // ایجاد یک کارت جدید برای هر کتاب
        QFrame *bookFrame = new QFrame(booksContainer);
        bookFrame->setObjectName("bookCard");
        bookFrame->setFixedWidth(196);
        bookFrame->setCursor(Qt::PointingHandCursor);

        QVBoxLayout *bookLayout = new QVBoxLayout(bookFrame);
        bookLayout->setContentsMargins(12, 12, 12, 12);
        bookLayout->setSpacing(6);

        // نمایش تصویر کتاب
        QLabel *coverLabel = new QLabel(bookFrame);
        coverLabel->setObjectName("coverLabel");
        coverLabel->setFixedSize(172, 220);
        coverLabel->setAlignment(Qt::AlignCenter);
        coverLabel->setText("No Cover");

        if (!coverImageUrl.isEmpty()) {
            QNetworkAccessManager *manager = new QNetworkAccessManager(bookFrame);
            QNetworkRequest coverRequest{QUrl(coverImageUrl)};
            QNetworkReply *reply = manager->get(coverRequest);
            connect(reply, &QNetworkReply::finished, coverLabel, [reply, coverLabel]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QPixmap pixmap;
                    pixmap.loadFromData(reply->readAll());
                    if (!pixmap.isNull()) {
                        coverLabel->setPixmap(pixmap.scaled(coverLabel->size(),
                                                             Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation));
                        coverLabel->setText(QString());
                    }
                }
                reply->deleteLater();
            });
        }

        bookLayout->addWidget(coverLabel);

        // نمایش عنوان کتاب
        QLabel *titleLabel = new QLabel(title, bookFrame);
        titleLabel->setObjectName("bookTitle");
        titleLabel->setWordWrap(true);
        bookLayout->addWidget(titleLabel);

        // نمایش نام نویسنده
        QLabel *authorLabel = new QLabel("By " + author, bookFrame);
        authorLabel->setObjectName("bookAuthor");
        authorLabel->setWordWrap(true);
        bookLayout->addWidget(authorLabel);

        bookLayout->addStretch();

        // نمایش قیمت کتاب
        QLabel *priceLabel = new QLabel(price, bookFrame);
        priceLabel->setObjectName("bookPrice");
        bookLayout->addWidget(priceLabel);

        // دکمه مشاهده جزئیات
        QPushButton *detailsButton = new QPushButton("View Details", bookFrame);
        detailsButton->setObjectName("secondaryButton");
        detailsButton->setCursor(Qt::PointingHandCursor);
        connect(detailsButton, &QPushButton::clicked, this, [this, bookObject]() {
            onBookClicked(bookObject);
        });
        bookLayout->addWidget(detailsButton);

        // دکمه افزودن به سبد خرید
        QPushButton *addToCartButton = new QPushButton("Add to Cart", bookFrame);
        addToCartButton->setObjectName("primaryButton");
        addToCartButton->setCursor(Qt::PointingHandCursor);
        QPointer<QPushButton> addToCartButtonPtr(addToCartButton);
        connect(addToCartButton, &QPushButton::clicked, this, [this, bookId, title, addToCartButtonPtr]() {
            qDebug() << "[Home] Add to Cart clicked. book title=" << title << "clicked bookId=" << bookId;
            if (addToCartButtonPtr) addToCartButtonPtr->setEnabled(false);
            Cart::addToCart(bookId, 1, this, [this, addToCartButtonPtr](bool success, const QString &message) {
                if (addToCartButtonPtr) addToCartButtonPtr->setEnabled(true);
                if (success) {
                    QMessageBox::information(this, "Cart", message);
                    if (cartWindow && cartWindow->isVisible()) {
                        cartWindow->loadCart();
                    }
                } else {
                    QMessageBox::warning(this, "Cart", "Failed to add book to cart: " + message);
                }
            });
        });
        bookLayout->addWidget(addToCartButton);

        // افزودن کلیک به هر فریم کتاب برای نمایش جزئیات
        bookFrame->installEventFilter(this); // نصب event filter برای کلیک
        bookData[bookFrame] = bookObject;  // ذخیره اطلاعات کتاب برای استفاده بعدی

        int index = gridLayout->count();
        int row = index / columns;
        int col = index % columns;
        gridLayout->addWidget(bookFrame, row, col);
    }
}

void Home::showBookDetails(const QJsonObject &book) {
    QMessageBox::information(this, "Book Details",
                             "Title: " + book["title"].toString() + "\n"
                                                                    "Author: " + book["author"].toString() + "\n"
                                                               "Price: " + book["price"].toString() + "\n"
                                                              "Description: " + book["description"].toString());
}

bool Home::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *clickedFrame = qobject_cast<QFrame *>(watched);
        if (clickedFrame && bookData.contains(clickedFrame)) {
            QJsonObject bookObj = bookData[clickedFrame];
            onBookClicked(bookObj); // فراخوانی تابع برای نمایش جزئیات
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


void Home::onCartClicked() {
    if (!cartWindow) { // بررسی اگر پنجره هنوز ساخته نشده است
        cartWindow = new Cart(nullptr);
    }
    cartWindow->loadCart(); // بارگذاری/به‌روزرسانی آیتم‌های سبد خرید
    cartWindow->show(); // نمایش پنجره سبد خرید
    cartWindow->raise();
    cartWindow->activateWindow();
}

void Home::onSearchTextChanged(const QString &text) {
    // Example: Iterate through the grid layout that contains your book cards
    // Assuming booksContainer is the holding widget and it has the QGridLayout
    if (!booksContainer || !booksContainer->layout()) return;

    QLayout* layout = booksContainer->layout();
    for (int i = 0; i < layout->count(); ++i) {
        QWidget* bookWidget = layout->itemAt(i)->widget();
        if (bookWidget) {
            // Find the QLabel containing the title within the bookWidget
            // and determine if its text contains the search string.
            // If it matches, show it, otherwise hide the bookWidget:
            //
            // bool matches = titleLabel->text().contains(text, Qt::CaseInsensitive);
            // bookWidget->setHidden(!matches);
        }
    }
}


void Home::onProfileClicked() {
    QString userAuthToken = authToken; // استفاده از توکن ذخیره شده
    UserProfile *userProfile = new UserProfile(nullptr, userAuthToken);

    // اطمینان از حذف پنجره پس از بسته شدن
    userProfile->setAttribute(Qt::WA_DeleteOnClose, true);

    connect(userProfile, &UserProfile::logoutRequested, this, &Home::performLogout);

    // نمایش پنجره
    userProfile->setWindowTitle("Profile");
    userProfile->resize(420, 480); // تنظیم اندازه پنجره
    userProfile->show();
}

void Home::setAuthToken(const QString &token) {
    authToken = token;  // ذخیره توکن
}

void Home::onAddProductClicked() {
    AddBook *addBookWindow = new AddBook(nullptr, authToken);
    addBookWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    addBookWindow->setWindowTitle("Add New Book");
    addBookWindow->resize(520, 640);
    addBookWindow->show();
}


QString Home::getUserIdFromToken(const QString &token) {
    QStringList parts = token.split('.');
    if (parts.size() != 3) {
        qWarning() << "Invalid JWT token format.";
        return QString();
    }

    QByteArray payload = QByteArray::fromBase64(parts[1].toUtf8());
    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload);

    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        QJsonObject payloadObject = jsonDoc.object();

        if (payloadObject.contains("user_id")) { // بررسی user_id
            return QString::number(payloadObject["user_id"].toInt());
        } else if (payloadObject.contains("userId")) { // بررسی userId
            return payloadObject["userId"].toString();
        } else if (payloadObject.contains("sub")) { // بررسی sub
            return payloadObject["sub"].toString();
        }
    }

    qWarning() << "Could not extract userId from token.";
    return QString();
}

void Home::performLogout() {
    NetworkManager::instance().setAuthToken(QString());
    authToken.clear();
    if (cartWindow) {
        cartWindow->hide();
    }
    emit logoutRequested();
}

void Home::onBookClicked(const QJsonObject &bookObj) {
    if (bookObj.isEmpty()) {
        return;
    }

    QDialog *detailsDialog = new QDialog(this);
    detailsDialog->setObjectName("pageRoot");
    detailsDialog->setAttribute(Qt::WA_StyledBackground, true);
    detailsDialog->setWindowTitle(bookObj["title"].toString());
    detailsDialog->resize(400, 440);

    QVBoxLayout *dialogLayout = new QVBoxLayout(detailsDialog);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    dialogLayout->setSpacing(12);

    // تصویر کتاب
    QLabel *imageLabel = new QLabel(detailsDialog);
    imageLabel->setObjectName("coverLabel");
    imageLabel->setFixedSize(150, 200);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setText("Loading...");
    QString imageUrl = bookObj["cover_image"].toString();

    if (imageUrl.isEmpty()) {
        imageLabel->setText("No Image");
    } else {
        QNetworkRequest imageRequest{QUrl(imageUrl)};
        QNetworkReply *imageReply = networkManager->get(imageRequest);

        connect(imageReply, &QNetworkReply::finished, imageLabel, [imageReply, imageLabel]() {
            if (imageReply->error() == QNetworkReply::NoError) {
                QByteArray imageData = imageReply->readAll();
                QPixmap pixmap;
                if (!imageData.isEmpty() && pixmap.loadFromData(imageData)) {
                    imageLabel->setPixmap(pixmap.scaled(imageLabel->size(),
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation));
                    imageLabel->setText(QString());
                } else {
                    imageLabel->setText("No Image");
                }
            } else {
                imageLabel->setText("No Image");
            }
            imageReply->deleteLater();
        });
    }

    QHBoxLayout *imageRow = new QHBoxLayout();
    imageRow->addStretch();
    imageRow->addWidget(imageLabel);
    imageRow->addStretch();
    dialogLayout->addLayout(imageRow);

    // افزودن سایر اطلاعات کتاب
    QFormLayout *infoLayout = new QFormLayout;
    infoLayout->setSpacing(8);
    infoLayout->addRow("Title:", new QLabel(bookObj["title"].toString()));
    infoLayout->addRow("Author:", new QLabel(bookObj["author"].toString("Unknown")));
    infoLayout->addRow("Publisher:", new QLabel(bookObj["publisher"].toString("Unknown")));
    QLabel *priceValueLabel = new QLabel(bookObj["price"].toString("N/A"));
    priceValueLabel->setObjectName("bookPrice");
    infoLayout->addRow("Price:", priceValueLabel);
    infoLayout->addRow("Stock:", new QLabel(QString::number(bookObj["stock"].toInt())));

    QLabel *descriptionLabel = new QLabel(bookObj["description"].toString("No Description"));
    descriptionLabel->setWordWrap(true);
    infoLayout->addRow("Description:", descriptionLabel);

    dialogLayout->addLayout(infoLayout);
    dialogLayout->addStretch();

    QHBoxLayout *actionRow = new QHBoxLayout();
    QPushButton *closeButton = new QPushButton("Close", detailsDialog);
    closeButton->setObjectName("secondaryButton");
    connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);

    QPushButton *addToCartButton = new QPushButton("Add to Cart", detailsDialog);
    addToCartButton->setObjectName("primaryButton");
    const int detailBookId = bookObj.value("id").toInt();
    connect(addToCartButton, &QPushButton::clicked, this, [this, detailBookId, bookObj, detailsDialog]() {
        qDebug() << "[Home] Add to Cart (details dialog) clicked. book title="
                  << bookObj.value("title").toString() << "clicked bookId=" << detailBookId;
        detailsDialog->accept();
        Cart::addToCart(detailBookId, 1, this, [this](bool success, const QString &message) {
            if (success) {
                QMessageBox::information(this, "Cart", message);
                if (cartWindow && cartWindow->isVisible()) {
                    cartWindow->loadCart();
                }
            } else {
                QMessageBox::warning(this, "Cart", "Failed to add book to cart: " + message);
            }
        });
    });

    actionRow->addWidget(closeButton);
    actionRow->addStretch();
    actionRow->addWidget(addToCartButton);
    dialogLayout->addLayout(actionRow);

    detailsDialog->setAttribute(Qt::WA_DeleteOnClose);
    detailsDialog->exec();
}

void Home::onMyBooksClicked() {
    MyBooks *myBooksWindow = new MyBooks(nullptr, authToken);
    myBooksWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    myBooksWindow->setWindowTitle("My Books");
    myBooksWindow->resize(900, 640); // تنظیم اندازه پیش‌فرض
    myBooksWindow->show();
}

void Home::onMyPurchasesClicked() {
    MyPurchases *myPurchasesWindow = new MyPurchases(nullptr);
    myPurchasesWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    myPurchasesWindow->show();
}
