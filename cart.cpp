#include "cart.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPointer>
#include <QStyle>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QMessageBox>
#include <QDebug>
#include "networkmanager.h"

namespace {

double jsonNumber(const QJsonValue &v) {
    if (v.isString()) {
        return v.toString().toDouble();
    }
    return v.toDouble();
}

QString formatMoney(double value) {
    return QString::number(value, 'f', 2);
}

} // namespace

// ---------------------------------------------------------------------
// Shared "Add to Cart" entry point — every Add to Cart button in the app
// (Home's book cards, the book-details dialog) calls this exact function.
// ---------------------------------------------------------------------
void Cart::addToCart(int bookId, int quantity, QWidget *dialogParent,
                      std::function<void(bool, const QString &)> onDone)
{
    qDebug() << "[Cart] Add requested. bookId=" << bookId << "quantity=" << quantity;

    if (bookId <= 0) {
        qDebug() << "[Cart] Add failed: invalid bookId";
        if (onDone) onDone(false, "Invalid book.");
        else QMessageBox::warning(dialogParent, "Cart", "Invalid book.");
        return;
    }

    QJsonObject body;
    body["book_id"] = bookId;
    body["quantity"] = quantity;

    qDebug() << "[Cart] POST" << API::ADD_TO_CART_ENDPOINT;

    QPointer<QWidget> guardedParent(dialogParent);
    NetworkManager::instance().post(API::ADD_TO_CART_ENDPOINT, body,
        [bookId, guardedParent, onDone](const ApiResponse &response) {
            qDebug() << "[Cart] HTTP status=" << response.httpStatus;
            qDebug() << "[Cart] response=" << response.data.toJson(QJsonDocument::Compact);

            if (response.success) {
                qDebug() << "[Cart] Add succeeded. bookId=" << bookId;
                if (onDone) {
                    onDone(true, "Book added to cart.");
                } else if (guardedParent) {
                    QMessageBox::information(guardedParent, "Cart", "Book added to cart.");
                }
            } else {
                QString serverMessage = response.errorMessage;
                if (response.data.isObject() && response.data.object().contains("error")) {
                    serverMessage = response.data.object().value("error").toString();
                }
                qDebug() << "[Cart] Add failed:" << serverMessage;
                if (onDone) {
                    onDone(false, serverMessage);
                } else if (guardedParent) {
                    QMessageBox::warning(guardedParent, "Cart", "Failed to add book to cart: " + serverMessage);
                }
            }
        });
}

// ---------------------------------------------------------------------
Cart::Cart(QWidget *parent) : QWidget(parent)
{
    setObjectName("pageRoot");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel("Shopping Cart", this);
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
    connect(retryButton, &QPushButton::clicked, this, &Cart::loadCart);
    mainLayout->addWidget(retryButton, 0, Qt::AlignHCenter);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    itemsContainer = new QWidget(scrollArea);
    itemsContainer->setObjectName("pageRoot");
    itemsLayout = new QVBoxLayout(itemsContainer);
    itemsLayout->setContentsMargins(0, 0, 0, 0);
    itemsLayout->setSpacing(12);
    itemsLayout->addStretch();

    scrollArea->setWidget(itemsContainer);
    mainLayout->addWidget(scrollArea, 1);

    QFrame *summary = new QFrame(this);
    summary->setObjectName("header");
    QHBoxLayout *summaryLayout = new QHBoxLayout(summary);
    summaryLayout->setContentsMargins(16, 12, 16, 12);

    totalLabel = new QLabel("Total: $0.00", summary);
    totalLabel->setObjectName("sectionTitle");
    summaryLayout->addWidget(totalLabel);
    summaryLayout->addStretch();

    refreshButton = new QPushButton("Refresh", summary);
    refreshButton->setObjectName("secondaryButton");
    connect(refreshButton, &QPushButton::clicked, this, &Cart::loadCart);
    summaryLayout->addWidget(refreshButton);

    purchaseButton = new QPushButton("Purchase", summary);
    purchaseButton->setObjectName("primaryButton");
    purchaseButton->setEnabled(false);
    connect(purchaseButton, &QPushButton::clicked, this, &Cart::onPurchase);
    summaryLayout->addWidget(purchaseButton);

    mainLayout->addWidget(summary);

    setWindowTitle("Shopping Cart");
    resize(720, 640);
}

void Cart::setViewState(ViewState state, const QString &message)
{
    switch (state) {
    case ViewState::Loading:
        stateLabel->setObjectName("loadingText");
        stateLabel->setText("Loading cart...");
        stateLabel->setVisible(true);
        retryButton->setVisible(false);
        scrollArea->setVisible(false);
        purchaseButton->setEnabled(false);
        break;
    case ViewState::Empty:
        stateLabel->setObjectName("emptyStateText");
        stateLabel->setText("Your cart is empty.");
        stateLabel->setVisible(true);
        retryButton->setVisible(false);
        scrollArea->setVisible(false);
        purchaseButton->setEnabled(false);
        totalLabel->setText("Total: $0.00");
        break;
    case ViewState::Error:
        stateLabel->setObjectName("errorText");
        stateLabel->setText(message.isEmpty() ? "Unable to load cart." : message);
        stateLabel->setVisible(true);
        retryButton->setVisible(true);
        scrollArea->setVisible(false);
        purchaseButton->setEnabled(false);
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

void Cart::clearItemWidgets()
{
    // Only the dynamic item rows are destroyed — the scroll area, its
    // container/layout, and the summary section are permanent and reused.
    while (itemsLayout->count() > 0) {
        QLayoutItem *item = itemsLayout->takeAt(0);
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

void Cart::loadCart()
{
    if (isLoading) {
        return;
    }
    isLoading = true;
    setViewState(ViewState::Loading);

    qDebug() << "[Cart] GET" << API::MY_CART_ENDPOINT;

    NetworkManager::instance().get(API::MY_CART_ENDPOINT, [this](const ApiResponse &response) {
        isLoading = false;
        qDebug() << "[Cart] my_cart HTTP status=" << response.httpStatus;

        if (!response.success) {
            setViewState(ViewState::Error, "Unable to load cart: " + response.errorMessage);
            return;
        }

        if (!response.data.isObject()) {
            setViewState(ViewState::Error, "Unexpected server response.");
            return;
        }

        QJsonObject payload = response.data.object();
        QJsonArray items = payload.value("items").toArray();

        if (items.isEmpty()) {
            clearItemWidgets();
            itemsLayout->addStretch();
            setViewState(ViewState::Empty);
            return;
        }

        displayCartItems(items);
        setViewState(ViewState::Ready);
        purchaseButton->setEnabled(true);

        double total = payload.contains("total") ? jsonNumber(payload.value("total")) : 0.0;
        totalLabel->setText("Total: $" + formatMoney(total));
    });
}

void Cart::displayCartItems(const QJsonArray &items)
{
    clearItemWidgets();

    for (const QJsonValue &itemVal : items) {
        QJsonObject item = itemVal.toObject();
        QJsonObject book = item.value("book").toObject();

        const int cartItemId = item.value("id").toInt();
        const int quantity = item.value("quantity").toInt();
        const int stock = book.value("stock").toInt();
        const double unitPrice = jsonNumber(book.value("price"));
        const double itemTotal = jsonNumber(item.value("total_price"));
        const QString coverUrl = book.value("cover_image").toString();

        QFrame *row = new QFrame(itemsContainer);
        row->setObjectName("bookCard");

        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 12, 12, 12);
        rowLayout->setSpacing(12);

        QLabel *cover = new QLabel(row);
        cover->setObjectName("coverLabel");
        cover->setFixedSize(56, 76);
        cover->setAlignment(Qt::AlignCenter);
        cover->setText("No\nCover");
        if (!coverUrl.isEmpty()) {
            QNetworkAccessManager *imgManager = new QNetworkAccessManager(row);
            QNetworkReply *imgReply = imgManager->get(QNetworkRequest(QUrl(coverUrl)));
            connect(imgReply, &QNetworkReply::finished, cover, [imgReply, cover]() {
                if (imgReply->error() == QNetworkReply::NoError) {
                    QPixmap pixmap;
                    if (pixmap.loadFromData(imgReply->readAll())) {
                        cover->setPixmap(pixmap.scaled(cover->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        cover->setText(QString());
                    }
                }
                imgReply->deleteLater();
            });
        }
        rowLayout->addWidget(cover, 0, Qt::AlignVCenter);

        QVBoxLayout *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(2);
        QLabel *titleLabel = new QLabel(book.value("title").toString(), row);
        titleLabel->setObjectName("bookTitle");
        titleLabel->setWordWrap(true);
        QLabel *authorLabel = new QLabel("By " + book.value("author").toString(), row);
        authorLabel->setObjectName("bookAuthor");
        QLabel *unitPriceLabel = new QLabel("Unit price: $" + formatMoney(unitPrice), row);
        unitPriceLabel->setObjectName("mutedText");
        infoLayout->addWidget(titleLabel);
        infoLayout->addWidget(authorLabel);
        infoLayout->addWidget(unitPriceLabel);
        rowLayout->addLayout(infoLayout, 1);
        rowLayout->setAlignment(infoLayout, Qt::AlignVCenter);

        QVBoxLayout *qtyColumn = new QVBoxLayout();
        qtyColumn->setSpacing(4);
        QLabel *qtyCaption = new QLabel("Quantity", row);
        qtyCaption->setObjectName("mutedText");
        qtyCaption->setAlignment(Qt::AlignHCenter);

        QHBoxLayout *qtyRow = new QHBoxLayout();
        QPushButton *minusBtn = new QPushButton("-", row);
        minusBtn->setObjectName("qtyButton");
        QLabel *qtyValue = new QLabel(QString::number(quantity), row);
        qtyValue->setAlignment(Qt::AlignCenter);
        qtyValue->setFixedWidth(28);
        QPushButton *plusBtn = new QPushButton("+", row);
        plusBtn->setObjectName("qtyButton");

        minusBtn->setEnabled(quantity > 1);
        plusBtn->setEnabled(quantity < stock);

        QPointer<QPushButton> minusPtr(minusBtn);
        QPointer<QPushButton> plusPtr(plusBtn);
        connect(minusBtn, &QPushButton::clicked, this, [this, cartItemId, quantity, minusPtr, plusPtr]() {
            if (minusPtr) minusPtr->setEnabled(false);
            if (plusPtr) plusPtr->setEnabled(false);
            requestQuantityChange(cartItemId, quantity - 1);
        });
        connect(plusBtn, &QPushButton::clicked, this, [this, cartItemId, quantity, minusPtr, plusPtr]() {
            if (minusPtr) minusPtr->setEnabled(false);
            if (plusPtr) plusPtr->setEnabled(false);
            requestQuantityChange(cartItemId, quantity + 1);
        });

        qtyRow->addWidget(minusBtn);
        qtyRow->addWidget(qtyValue);
        qtyRow->addWidget(plusBtn);
        qtyColumn->addWidget(qtyCaption);
        qtyColumn->addLayout(qtyRow);
        rowLayout->addLayout(qtyColumn);
        rowLayout->setAlignment(qtyColumn, Qt::AlignVCenter);

        QVBoxLayout *actionColumn = new QVBoxLayout();
        actionColumn->setSpacing(6);
        QLabel *itemTotalLabel = new QLabel("$" + formatMoney(itemTotal), row);
        itemTotalLabel->setObjectName("bookPrice");
        itemTotalLabel->setAlignment(Qt::AlignRight);

        QPushButton *removeBtn = new QPushButton("Remove", row);
        removeBtn->setObjectName("dangerButton");
        QPointer<QPushButton> removePtr(removeBtn);
        connect(removeBtn, &QPushButton::clicked, this, [this, cartItemId, removePtr]() {
            if (removePtr) removePtr->setEnabled(false);
            requestRemove(cartItemId);
        });

        actionColumn->addWidget(itemTotalLabel);
        actionColumn->addWidget(removeBtn);
        rowLayout->addLayout(actionColumn);
        rowLayout->setAlignment(actionColumn, Qt::AlignVCenter);

        itemsLayout->insertWidget(itemsLayout->count() - 1, row);
    }
}

void Cart::requestQuantityChange(int cartItemId, int newQuantity)
{
    if (newQuantity < 1) {
        loadCart();
        return;
    }

    qDebug() << "[Cart] PATCH cart item" << cartItemId << "quantity=" << newQuantity;

    QJsonObject body;
    body["quantity"] = newQuantity;

    NetworkManager::instance().patch(API::CART_ENDPOINT + QString::number(cartItemId) + "/", body,
        [this](const ApiResponse &response) {
            qDebug() << "[Cart] quantity update HTTP status=" << response.httpStatus;
            if (response.success) {
                loadCart();
            } else {
                QMessageBox::warning(this, "Cart", "Failed to update quantity: " + response.errorMessage);
                loadCart();
            }
        });
}

void Cart::requestRemove(int cartItemId)
{
    qDebug() << "[Cart] DELETE cart item" << cartItemId;

    NetworkManager::instance().deleteResource(API::CART_ENDPOINT + QString::number(cartItemId) + "/",
        [this](const ApiResponse &response) {
            qDebug() << "[Cart] remove HTTP status=" << response.httpStatus;
            if (response.success) {
                loadCart();
            } else {
                QMessageBox::warning(this, "Cart", "Failed to remove item: " + response.errorMessage);
                loadCart();
            }
        });
}

void Cart::onPurchase()
{
    if (purchaseInFlight) {
        return;
    }

    int result = QMessageBox::question(this, "Confirm Purchase",
        "Are you sure you want to purchase these items?",
        QMessageBox::Yes | QMessageBox::No);
    if (result != QMessageBox::Yes) {
        return;
    }

    purchaseInFlight = true;
    purchaseButton->setEnabled(false);

    qDebug() << "[Cart] POST" << API::CART_PURCHASE_ENDPOINT;

    NetworkManager::instance().post(API::CART_PURCHASE_ENDPOINT, QJsonObject(),
        [this](const ApiResponse &response) {
            purchaseInFlight = false;
            qDebug() << "[Cart] purchase HTTP status=" << response.httpStatus;

            if (response.success) {
                QMessageBox::information(this, "Success", "Purchase completed successfully!");
            } else {
                QMessageBox::warning(this, "Error", "Purchase failed: " + response.errorMessage);
            }
            loadCart();
        });
}
