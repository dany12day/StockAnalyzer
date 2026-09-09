#include <infrastructure/StockData.hpp>

#include <QByteArray>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariant>
#include <expected>

StockData::StockData(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))

{
    m_manager->setTransferTimeout(5000); // Set timeout to 5 seconds
}

[[nodiscard("parse failures must be handled")]]
static std::expected<Quote, ParseError> parseQuote(const QByteArray &data)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        return std::unexpected(ParseError::MalformedJson);
    }

    const QJsonValue chartValue = doc["chart"];
    if (!chartValue.isObject())
    {
        return std::unexpected(ParseError::UnexpectedShape);
    }

    const QJsonValue resultValue = chartValue["result"];
    if (!resultValue.isArray())
    {
        return std::unexpected(ParseError::UnexpectedShape);
    }

    const QJsonArray resultArray = resultValue.toArray();
    if (resultArray.isEmpty())
    {
        return std::unexpected(ParseError::EmptyResult);
    }

    const QJsonValue metaValue = resultArray.first()["meta"];
    if (!metaValue.isObject())
    {
        return std::unexpected(ParseError::UnexpectedShape);
    }

    const QJsonObject metaObj = metaValue.toObject();

    const QJsonValue currencyVal = metaObj.value("currency");
    if (!currencyVal.isString())
    {
        return std::unexpected(ParseError::MissingField);
    }
    const QString currency = currencyVal.toString();

    const QJsonValue symbolVal = metaObj.value("symbol");
    if (!symbolVal.isString())
    {
        return std::unexpected(ParseError::MissingField);
    }
    const QString symbol = symbolVal.toString();

    const QJsonValue priceVal = metaObj.value("regularMarketPrice");
    if (!priceVal.isDouble())
    {
        return std::unexpected(ParseError::MissingField);
    }
    const double price = priceVal.toDouble();

    const QJsonValue regularMarketTimeVal = metaObj.value("regularMarketTime");
    if (!regularMarketTimeVal.isDouble())
    {
        return std::unexpected(ParseError::MissingField);
    }
    const qint64 regularMarketTime = regularMarketTimeVal.toInteger();

    return Quote{symbol, currency, price, regularMarketTime}; // Return the extracted quote if successful
}

void StockData::fetch(const QString &symbol)
{
    this->m_resultText = QStringLiteral("Fetching stock data for %1...").arg(symbol);
    emit resultTextChanged();

    const QString url =
        QStringLiteral("https://query2.finance.yahoo.com/v8/finance/chart/%1?range=1d&interval=1d").arg(symbol);

    QNetworkReply *reply = m_manager->get(QNetworkRequest(QUrl(url)));

    connect(
        reply, &QNetworkReply::finished, this,
        [this, reply, symbol]() {
            QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

            if (reply->error() == QNetworkReply::NoError)
            {
                const QByteArray responseData = reply->readAll();

                auto result = parseQuote(responseData);

                if (result)
                {
                    const Quote &quote = *result;
                    qDebug() << "Fetched stock data for" << quote.symbol << ":" << quote.price << quote.currency
                             << quote.regularMarketTime;

                    this->m_resultText = QStringLiteral("The price is %1 %2 at %3")
                                             .arg(quote.price)
                                             .arg(quote.currency)
                                             .arg(QDateTime::fromSecsSinceEpoch(quote.regularMarketTime).toString());
                }
                else
                {
                    ParseError error = result.error();
                    switch (error)
                    {
                    case ParseError::MalformedJson:
                        qDebug() << "Error: Malformed JSON response for stock data request for" << symbol;
                        break;
                    case ParseError::UnexpectedShape:
                        qDebug() << "Error: Unexpected JSON shape for stock data request for" << symbol;
                        break;
                    case ParseError::EmptyResult:
                        qDebug() << "Error: Empty result in JSON response for stock data request for" << symbol;
                        break;
                    case ParseError::MissingField:
                        qDebug() << "Error: Missing expected field in JSON response for stock data request for"
                                 << symbol;
                        break;
                    }

                    this->m_resultText =
                        QStringLiteral("Error fetching stock data for %1: %2").arg(symbol).arg(static_cast<int>(error));
                }
            }
            else if (statusCode.isValid())
            {
                qDebug() << "HTTP error fetching stock data for" << symbol << ":" << statusCode.toInt();
                this->m_resultText =
                    QStringLiteral("HTTP error fetching stock data for %1: %2").arg(symbol).arg(statusCode.toInt());
            }
            else
            {
                qDebug() << "No response received for stock data request for" << symbol << ":" << reply->errorString();
                this->m_resultText = QStringLiteral("No response received for stock data request for %1: %2")
                                         .arg(symbol)
                                         .arg(reply->errorString());
            }

            emit resultTextChanged();

            reply->deleteLater();
        }

    );
}
