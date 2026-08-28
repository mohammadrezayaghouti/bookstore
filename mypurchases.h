#ifndef MYPURCHASES_H
#define MYPURCHASES_H

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>

class QGridLayout;
class QLabel;
class QPushButton;
class QScrollArea;

// Read-only purchase history: shows books the current user has actually
// bought, sourced live from GET /api/cart/my_books/ (backend-authoritative —
// nothing here is cached only in local Qt state).
class MyPurchases : public QWidget {
    Q_OBJECT

public:
    explicit MyPurchases(QWidget *parent = nullptr);

    void loadPurchases();

private:
    enum class ViewState { Loading, Empty, Ready, Error };

    void setViewState(ViewState state, const QString &message = QString());
    void clearBookWidgets();
    void displayBooks(const QJsonArray &books);
    void showBookDetails(const QJsonObject &book);

    QWidget *booksContainer = nullptr;
    QGridLayout *gridLayout = nullptr;
    QScrollArea *scrollArea = nullptr;
    QLabel *stateLabel = nullptr;
    QPushButton *retryButton = nullptr;

    bool isLoading = false;
};

#endif // MYPURCHASES_H
