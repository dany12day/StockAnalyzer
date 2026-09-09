#pragma once

#include <QObject>
#include <QString>

#include <QtQmlIntegration>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)

enum class ParseError
{
    MalformedJson,
    UnexpectedShape,
    EmptyResult,
    MissingField
};

struct Quote
{
    QString symbol;
    QString currency;
    double price;
    qint64 regularMarketTime;
};

class StockData : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString resultText READ resultText NOTIFY resultTextChanged)

public:
    explicit StockData(QObject *parent = nullptr);

    Q_INVOKABLE void fetch(const QString &symbol);

    QString resultText() const { return m_resultText; }

private:
    QNetworkAccessManager *m_manager;
    QString m_resultText;

signals:
    void resultTextChanged();
};