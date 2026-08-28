#ifndef HOME_H
#define HOME_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QNetworkReply>
#include <QMap>
#include <QVBoxLayout>
#include "cart.h"

class Home : public QMainWindow {
    Q_OBJECT
public:
    explicit Home(QWidget *parent = nullptr);
    ~Home();

    void setAuthToken(const QString &token);   // تنظیم توکن

signals:
    void bookSelected(const QString &bookId);  // انتخاب یک کتاب (سیگنال)
    void logoutRequested();                    // درخواست خروج از حساب کاربری

private slots:
    void onCartClicked();                      // کلیک روی دکمه سبد خرید
    void onAddProductClicked();                // کلیک روی دکمه افزودن محصول
    void onSearchTextChanged(const QString &text); // تغییر متن جستجو
    void onProfileClicked();                   // کلیک روی پروفایل
    void onMyBooksClicked();
    void onMyPurchasesClicked();
    void performLogout();                      // خروج از حساب کاربری

    void displayBooks(const QJsonArray &booksArray);
    void showBookDetails(const QJsonObject &book);
    void onBooksFetched(QNetworkReply *reply);
    void onBookClicked(const QJsonObject &bookObj);        // کلیک روی کتاب‌ها

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QString getUserIdFromToken(const QString& token); // گرفتن شناسه کاربر از توکن

    // ویجت‌ها
    QLineEdit *searchEdit;
    QPushButton *cartButton;
    QPushButton *addProductButton;
    QPushButton *homeNavButton;
    QPushButton *myBooksNavButton;
    QPushButton *myPurchasesNavButton;
    QPushButton *profileNavButton;
    QPushButton *logoutNavButton;

    // اطلاعات کاربر
    QString authToken;

    QNetworkAccessManager *networkManager;
    Cart *cartWindow;

    QMap<QWidget*, QJsonObject> bookData;
    QVBoxLayout *mainLayout;
    QWidget *booksContainer = nullptr;
};


#endif // HOME_H
