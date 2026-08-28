#ifndef ADDBOOK_H
#define ADDBOOK_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QJsonObject>
#include "QLabel"


class AddBook : public QWidget
{
    Q_OBJECT

public:
    explicit AddBook(QWidget *parent = nullptr, const QString &token = QString());
    ~AddBook();


private slots:
    void saveBook();
    void uploadImage();

public slots:
    void updateProductList();


signals:
    void bookAdded();
    void productListUpdated();


private:
    QString authToken;
    QString imagePath;

    QLineEdit *titleEdit;
    QLineEdit *authorEdit;
    QLineEdit *publisherEdit;
    QLineEdit *priceEdit;
    QLineEdit *stockEdit;
    QTextEdit *descriptionEdit;
    QLabel *imagePreview;
    QString userId;
};

#endif // ADDBOOK_H
