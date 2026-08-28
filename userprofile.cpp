#include "userprofile.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>
#include "networkmanager.h"

UserProfile::UserProfile(QWidget *parent, const QString &token)
    : QWidget(parent), authToken(token) {
    setObjectName("pageRoot");

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);

    QFrame *card = new QFrame(this);
    card->setObjectName("authCard");
    card->setMaximumWidth(440);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 28, 28, 28);
    cardLayout->setSpacing(16);

    QLabel *title = new QLabel("Profile", card);
    title->setObjectName("pageTitle");
    cardLayout->addWidget(title);

    QFormLayout *form = new QFormLayout();
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft);

    idEdit = new QLineEdit(card);
    idEdit->setReadOnly(true);
    usernameEdit = new QLineEdit(card);
    fullNameEdit = new QLineEdit(card);
    phoneNumberEdit = new QLineEdit(card);
    birthDateEdit = new QLineEdit(card);
    genderEdit = new QLineEdit(card);

    form->addRow("ID", idEdit);
    form->addRow("Username", usernameEdit);
    form->addRow("Full Name", fullNameEdit);
    form->addRow("Phone Number", phoneNumberEdit);
    form->addRow("Birth Date", birthDateEdit);
    form->addRow("Gender", genderEdit);

    for (int i = 0; i < form->rowCount(); ++i) {
        QLayoutItem *labelItem = form->itemAt(i, QFormLayout::LabelRole);
        if (labelItem && labelItem->widget()) {
            labelItem->widget()->setObjectName("fieldLabel");
        }
    }

    cardLayout->addLayout(form);

    QHBoxLayout *buttonRow = new QHBoxLayout();
    fetchButton = new QPushButton("Fetch Profile", card);
    fetchButton->setObjectName("secondaryButton");
    updateButton = new QPushButton("Save Changes", card);
    updateButton->setObjectName("primaryButton");
    buttonRow->addWidget(fetchButton);
    buttonRow->addStretch();
    buttonRow->addWidget(updateButton);
    cardLayout->addLayout(buttonRow);

    QFrame *divider = new QFrame(card);
    divider->setObjectName("hline");
    cardLayout->addWidget(divider);

    logoutButton = new QPushButton("Logout", card);
    logoutButton->setObjectName("dangerButton");
    cardLayout->addWidget(logoutButton);

    outer->addWidget(card);
    outer->addStretch();

    connect(fetchButton, &QPushButton::clicked, this, &UserProfile::fetchUserProfile);
    connect(updateButton, &QPushButton::clicked, this, &UserProfile::updateUserProfile);
    connect(logoutButton, &QPushButton::clicked, this, &UserProfile::logoutRequested);

    // Use centralized NetworkManager
    NetworkManager::instance().setAuthToken(token);
}

void UserProfile::fetchUserProfile() {
    NetworkManager::instance().get(API::USER_DETAIL_ENDPOINT, [this](const ApiResponse& response) {
        if (response.success && response.data.isObject()) {
            QJsonObject jsonObject = response.data.object();

            // Set form fields from response
            idEdit->setText(QString::number(jsonObject["id"].toInt()));
            usernameEdit->setText(jsonObject["username"].toString());
            fullNameEdit->setText(jsonObject["full_name"].toString());
            phoneNumberEdit->setText(jsonObject["phone_number"].toString());
            birthDateEdit->setText(jsonObject["birth_date"].toString());
            genderEdit->setText(jsonObject["gender"].toString());
        } else {
            qDebug() << "Error fetching profile: " << response.errorMessage;
            QMessageBox::warning(this, "Error", "Failed to fetch profile: " + response.errorMessage);
        }
    });
}


void UserProfile::updateUserProfile() {
    QJsonObject jsonObject;
    jsonObject["id"] = idEdit->text().toInt();
    jsonObject["username"] = usernameEdit->text();
    jsonObject["full_name"] = fullNameEdit->text();
    jsonObject["phone_number"] = phoneNumberEdit->text();
    jsonObject["birth_date"] = birthDateEdit->text();
    jsonObject["gender"] = genderEdit->text();

    QString userId = idEdit->text();
    QString updateUrl = API::USERS_ENDPOINT + userId + "/";

    NetworkManager::instance().put(updateUrl, jsonObject, [this](const ApiResponse& response) {
        if (response.success) {
            QMessageBox::information(this, "Success", "Profile updated successfully.");
        } else {
            qDebug() << "Error updating profile: " << response.errorMessage;
            QMessageBox::warning(this, "Error", "Failed to update profile: " + response.errorMessage);
        }
    });
}

void UserProfile::setAuthToken(const QString &token) {
    authToken = token;
}

UserProfile::~UserProfile() {}
