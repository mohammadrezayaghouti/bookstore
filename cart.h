#ifndef CART_H
#define CART_H

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>

class QVBoxLayout;
class QLabel;
class QPushButton;
class QScrollArea;

class Cart : public QWidget {
    Q_OBJECT

public:
    explicit Cart(QWidget *parent = nullptr);

    void loadCart();

    // Single shared entry point for "Add to Cart" anywhere in the app (Home's
    // book cards, the book-details dialog, etc.) so there is exactly one
    // request/response implementation to trust. Talks to the real backend
    // (POST /api/cart/add_to_cart/) and only reports success once the server
    // has confirmed it. `onDone` is optional; if omitted, a QMessageBox is
    // shown using dialogParent.
    static void addToCart(int bookId, int quantity, QWidget *dialogParent,
                           std::function<void(bool success, const QString &message)> onDone = nullptr);

private slots:
    void onPurchase();

private:
    enum class ViewState { Loading, Empty, Ready, Error };

    void setViewState(ViewState state, const QString &message = QString());
    void clearItemWidgets();
    void displayCartItems(const QJsonArray &items);
    void requestQuantityChange(int cartItemId, int newQuantity);
    void requestRemove(int cartItemId);

    QVBoxLayout *itemsLayout = nullptr;
    QWidget *itemsContainer = nullptr;
    QScrollArea *scrollArea = nullptr;

    QLabel *stateLabel = nullptr;
    QPushButton *retryButton = nullptr;
    QLabel *totalLabel = nullptr;
    QPushButton *purchaseButton = nullptr;
    QPushButton *refreshButton = nullptr;

    bool isLoading = false;
    bool purchaseInFlight = false;
};

#endif // CART_H
