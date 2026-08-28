#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include "QStackedWidget"

class Login;

class RegisterWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    ~RegisterWindow();

signals:
    void switchToLoginPage();


private slots:
    void onRegisterClicked();
    void checkPasswordMatch();
    void onBackToLoginClicked();


private:
    QLineEdit *nameEdit;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *confirmPasswordEdit;
    QLabel *passwordStatusLabel;
    QLineEdit *phoneEdit;
    QDateEdit *birthdateEdit;
    QComboBox *genderComboBox;
    QPushButton *backtologin;
    QStackedWidget *stackedwidget;
};

#endif // REGISTERWINDOW_H
