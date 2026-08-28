#include "registerwindow.h"
#include "QPushButton"
#include "QVBoxLayout"
#include "QHBoxLayout"
#include "QFrame"
#include "QLocale"
#include "QJsonObject"
#include "QJsonDocument"
#include "QMessageBox"
#include "QStackedWidget"
#include "networkmanager.h"

RegisterWindow::RegisterWindow(QWidget *Parent) : QWidget(Parent) {
    setObjectName("pageRoot");

    QFrame *authCard = new QFrame(this);
    authCard->setObjectName("authCard");
    authCard->setFixedWidth(400);

    QVBoxLayout *cardLayout = new QVBoxLayout(authCard);
    cardLayout->setContentsMargins(32, 28, 32, 28);
    cardLayout->setSpacing(6);

    QHBoxLayout *topRow = new QHBoxLayout();
    backtologin = new QPushButton("< Back", authCard);
    backtologin->setObjectName("linkButton");
    backtologin->setCursor(Qt::PointingHandCursor);
    topRow->addWidget(backtologin);
    topRow->addStretch();
    cardLayout->addLayout(topRow);

    QLabel *appTitle = new QLabel("Create account", authCard);
    appTitle->setObjectName("appTitle");
    cardLayout->addWidget(appTitle);

    QLabel *subtitle = new QLabel("Join BookStore to start shopping", authCard);
    subtitle->setObjectName("subtitleText");
    cardLayout->addWidget(subtitle);
    cardLayout->addSpacing(16);

    auto addField = [&](const QString &labelText, QLineEdit *&edit) {
        QLabel *label = new QLabel(labelText, authCard);
        label->setObjectName("fieldLabel");
        edit = new QLineEdit(authCard);
        cardLayout->addWidget(label);
        cardLayout->addWidget(edit);
        cardLayout->addSpacing(8);
    };

    addField("Full Name", nameEdit);
    addField("Username", usernameEdit);
    addField("Password", passwordEdit);
    passwordEdit->setEchoMode(QLineEdit::Password);
    addField("Confirm Password", confirmPasswordEdit);
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    passwordStatusLabel = new QLabel(authCard);
    passwordStatusLabel->setObjectName("mutedText");
    cardLayout->addWidget(passwordStatusLabel);
    cardLayout->addSpacing(4);

    addField("Phone Number", phoneEdit);

    QLabel *birthdateLabel = new QLabel("Birthday", authCard);
    birthdateLabel->setObjectName("fieldLabel");
    birthdateEdit = new QDateEdit(authCard);
    birthdateEdit->setCalendarPopup(true);
    cardLayout->addWidget(birthdateLabel);
    cardLayout->addWidget(birthdateEdit);
    cardLayout->addSpacing(8);

    QLabel *genderLabel = new QLabel("Gender", authCard);
    genderLabel->setObjectName("fieldLabel");
    genderComboBox = new QComboBox(authCard);
    genderComboBox->addItem("Male");
    genderComboBox->addItem("Female");
    cardLayout->addWidget(genderLabel);
    cardLayout->addWidget(genderComboBox);
    cardLayout->addSpacing(16);

    QPushButton *registerbtn = new QPushButton("Create account", authCard);
    registerbtn->setObjectName("primaryButton");
    registerbtn->setCursor(Qt::PointingHandCursor);
    registerbtn->setMinimumHeight(38);
    cardLayout->addWidget(registerbtn);

    QVBoxLayout *outer = new QVBoxLayout(this);
    QHBoxLayout *centerRow = new QHBoxLayout();
    centerRow->addStretch();
    centerRow->addWidget(authCard);
    centerRow->addStretch();
    outer->addStretch();
    outer->addLayout(centerRow);
    outer->addStretch();
    setLayout(outer);

    connect(registerbtn, &QPushButton::clicked, this, &RegisterWindow::onRegisterClicked);
    connect(confirmPasswordEdit, &QLineEdit::textChanged, this, &RegisterWindow::checkPasswordMatch);
    connect(backtologin, &QPushButton::clicked, this, &RegisterWindow::onBackToLoginClicked);
}



RegisterWindow::~RegisterWindow() {}

void RegisterWindow::onRegisterClicked()
{
    if (passwordEdit->text() != confirmPasswordEdit->text()) {
        passwordStatusLabel->setText("Passwords do not match.");
        passwordStatusLabel->setObjectName("errorText");
        style()->unpolish(passwordStatusLabel);
        style()->polish(passwordStatusLabel);
        return;
    }

    // ساختار JSON براساس مشخصات سرور
    QJsonObject json;
    json["username"] = usernameEdit->text();
    json["password"] = passwordEdit->text();
    json["full_name"] = nameEdit->text();
    json["phone_number"] = phoneEdit->text();
    json["birth_date"] = birthdateEdit->date().toString(Qt::ISODate); // فرمت تاریخ به ISO
    json["gender"] = genderComboBox->currentText();

    // ارسال درخواست به سرور
    NetworkManager::instance().post(API::USERS_ENDPOINT, json, [this](const ApiResponse& response) {
        if (response.success) {
            QMessageBox messageBox(this);
            messageBox.setIcon(QMessageBox::Information);
            messageBox.setWindowTitle("Success");
            messageBox.setText("Registration successful!");
            messageBox.exec();
            emit switchToLoginPage();
        } else {
            QMessageBox messageBox(this);
            messageBox.setIcon(QMessageBox::Warning);
            messageBox.setWindowTitle("Registration Failed");
            messageBox.setText("Registration failed: " + response.errorMessage);
            messageBox.exec();
        }
    });
}


void RegisterWindow::checkPasswordMatch()
{
    if (passwordEdit->text() == confirmPasswordEdit->text()) {
        passwordStatusLabel->setText("Passwords match.");
        passwordStatusLabel->setObjectName("successText");
    } else {
        passwordStatusLabel->setText("Passwords do not match.");
        passwordStatusLabel->setObjectName("errorText");
    }
    style()->unpolish(passwordStatusLabel);
    style()->polish(passwordStatusLabel);
}

void RegisterWindow::onBackToLoginClicked()
{
    emit switchToLoginPage();
}
