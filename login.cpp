#include "login.h"
#include "./ui_login.h"
#include "QVBoxLayout"
#include "QHBoxLayout"
#include "QFrame"
#include "QLineEdit"
#include "QPushButton"
#include "QLabel"
#include "QCheckBox"
#include <QMainWindow>
#include "QStackedWidget"
#include "registerwindow.h"
#include "QJsonObject"
#include "QJsonDocument"
#include "home.h"
#include "QMessageBox"
#include "networkmanager.h"

Login::Login(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);

    stackedwidget = new QStackedWidget(this);
    stackedwidget->setObjectName("pageRoot");
    setCentralWidget(stackedwidget);

    loginWidget = new QWidget(this);
    loginWidget->setObjectName("pageRoot");

    QFrame *authCard = new QFrame(loginWidget);
    authCard->setObjectName("authCard");
    authCard->setFixedWidth(360);

    QVBoxLayout *cardLayout = new QVBoxLayout(authCard);
    cardLayout->setContentsMargins(32, 32, 32, 32);
    cardLayout->setSpacing(6);

    QLabel *appTitle = new QLabel("BookStore", authCard);
    appTitle->setObjectName("appTitle");
    appTitle->setAlignment(Qt::AlignHCenter);

    QLabel *subtitle = new QLabel("Welcome back", authCard);
    subtitle->setObjectName("subtitleText");
    subtitle->setAlignment(Qt::AlignHCenter);

    cardLayout->addWidget(appTitle);
    cardLayout->addWidget(subtitle);
    cardLayout->addSpacing(20);

    userlabellog = new QLabel("Username", authCard);
    userlabellog->setObjectName("fieldLabel");
    userlinelog = new QLineEdit(authCard);
    userlinelog->setPlaceholderText("Enter your username");

    passwordlabellog = new QLabel("Password", authCard);
    passwordlabellog->setObjectName("fieldLabel");
    passwordlinelog = new QLineEdit(authCard);
    passwordlinelog->setPlaceholderText("Enter your password");
    passwordlinelog->setEchoMode(QLineEdit::Password);

    cardLayout->addWidget(userlabellog);
    cardLayout->addWidget(userlinelog);
    cardLayout->addSpacing(10);
    cardLayout->addWidget(passwordlabellog);
    cardLayout->addWidget(passwordlinelog);

    showpassword = new QCheckBox("Show password", authCard);
    showpassword->setChecked(false);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(showpassword);

    errorlabel = new QLabel(authCard);
    errorlabel->setObjectName("errorText");
    errorlabel->setWordWrap(true);
    cardLayout->addWidget(errorlabel);

    Loginbtn = new QPushButton("Sign In", authCard);
    Loginbtn->setObjectName("primaryButton");
    Loginbtn->setCursor(Qt::PointingHandCursor);
    Loginbtn->setMinimumHeight(38);
    cardLayout->addSpacing(12);
    cardLayout->addWidget(Loginbtn);

    registerbtn = new QPushButton("Create account", authCard);
    registerbtn->setObjectName("linkButton");
    registerbtn->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(registerbtn, 0, Qt::AlignHCenter);

    QVBoxLayout *loginlayout = new QVBoxLayout(loginWidget);
    QHBoxLayout *centerRow = new QHBoxLayout();
    centerRow->addStretch();
    centerRow->addWidget(authCard);
    centerRow->addStretch();

    loginlayout->addStretch();
    loginlayout->addLayout(centerRow);
    loginlayout->addStretch();

    registerWidget = new RegisterWindow(this);
    homewidget = new Home(this);

    stackedwidget->addWidget(loginWidget);
    stackedwidget->addWidget(registerWidget);
    stackedwidget->addWidget(homewidget);

    connect(Loginbtn, &QPushButton::clicked, this, &Login::on_Loginbtn_clicked);
    connect(registerbtn, &QPushButton::clicked, this, &Login::on_registerbtn_clicked);

    connect(registerWidget, &RegisterWindow::switchToLoginPage, this, [this]() { stackedwidget->setCurrentWidget(loginWidget); });

    connect(homewidget, &Home::logoutRequested, this, [this]() {
        userlinelog->clear();
        passwordlinelog->clear();
        errorlabel->clear();
        stackedwidget->setCurrentWidget(loginWidget);
    });

    connect(showpassword, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            passwordlinelog->setEchoMode(QLineEdit::Password);
        } else {
            passwordlinelog->setEchoMode(QLineEdit::Normal);
        }
    });

    resize(960, 640);
}

Login::~Login()
{
    delete ui;
}

void Login::on_registerbtn_clicked()
{
    stackedwidget->setCurrentWidget(registerWidget);
}

void Login::on_Loginbtn_clicked()
{
    errorlabel->clear();

    // آماده‌سازی داده‌ها
    QJsonObject json;
    json["username"] = userlinelog->text();
    json["password"] = passwordlinelog->text();

    NetworkManager::instance().post(API::TOKEN_ENDPOINT, json, [this](const ApiResponse& response) {
        if (response.success && response.data.isObject()) {
            QJsonObject jsonObj = response.data.object();
            QString accessToken = jsonObj["access"].toString();

            if (!accessToken.isEmpty()) {
                emit onLoginSuccessful(accessToken);
            } else {
                errorlabel->setText("Login failed: No access token in response");
                qDebug() << "Login failed: No access token found";
            }
        } else {
            errorlabel->setText("Login failed: " + response.errorMessage);
            qDebug() << "Login error:" << response.errorMessage;
        }
    });
}

void Login::handleLoginResponse(QNetworkReply *reply) {
    // This method is now unused as we use callbacks in NetworkManager
    if (reply) {
        reply->deleteLater();
    }
}

void Login::onLoginSuccessful(const QString &token)
{
    // Set token in NetworkManager for all subsequent requests
    NetworkManager::instance().setAuthToken(token);

    stackedwidget->setCurrentWidget(homewidget);
    homewidget->setAuthToken(token);
}

void Login::sendRequestWithToken(const QString &token)
{
    // This method is kept for compatibility but uses NetworkManager now
    NetworkManager::instance().setAuthToken(token);
}
