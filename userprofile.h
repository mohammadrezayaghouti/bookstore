#ifndef USERPROFILE_H
#define USERPROFILE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class UserProfile : public QWidget
{
    Q_OBJECT

public:
    explicit UserProfile(QWidget *parent = nullptr, const QString &token = QString());

    ~UserProfile();

signals:
    void logoutRequested();

private slots:
    void setAuthToken(const QString &token);
    void fetchUserProfile();
    void updateUserProfile();

private:
    QLineEdit *usernameEdit;
    QLineEdit *fullNameEdit;
    QLineEdit *phoneNumberEdit;
    QLineEdit *birthDateEdit;
    QLineEdit *genderEdit;
    QPushButton *fetchButton;
    QPushButton *updateButton;
    QPushButton *logoutButton;
    QString authToken;
    QLineEdit *idEdit;
};

#endif // USERPROFILE_H
