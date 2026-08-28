#ifndef MYBOOKS_H
#define MYBOOKS_H

#include <QWidget>
#include <QGridLayout>
#include "QFrame"
#include <QMap>

class MyBooks : public QWidget {
    Q_OBJECT

public:
    explicit MyBooks(QWidget *parent = nullptr, const QString &token = "");
    ~MyBooks() override = default;

private:
    QString authToken;
    QGridLayout *gridLayout;

    // Methods
    void fetchBooks();
    void parseBooks(const QByteArray &data);
    void showBookDetails(const QJsonObject &bookObj);
    QMap<QFrame*, QVariant> bookFrames;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // MYBOOKS_H
