#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include "registerwindow.h"
#include "home.h"

namespace Ui {
class Login;
}

class Login : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_Loginbtn_clicked();
    void on_registerbtn_clicked();
    void handleLoginResponse(QNetworkReply *reply);
    void onLoginSuccessful(const QString& token);
    void sendRequestWithToken(const QString& token);

private:
    Ui::Login *ui;
    QStackedWidget *stackedwidget;
    QWidget *loginWidget;
    RegisterWindow *registerWidget;
    Home *homewidget;

    QLineEdit *userlinelog;
    QLineEdit *passwordlinelog;
    QLabel *userlabellog;
    QLabel *passwordlabellog;
    QLabel *errorlabel;
    QCheckBox *showpassword;
    QPushButton *Loginbtn;
    QPushButton *registerbtn;
};

#endif // LOGIN_H
