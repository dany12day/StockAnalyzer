#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "app/StockDataAPI/StockData.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("ValueApp", "Main");

    // StockData stockData;
    // stockData.fetch("AAPL"); // Fetch stock data for Apple Inc.
    // stockData.fetch("FLOW.AS"); // Fetch stock data for Flow Traders

    return app.exec();
}
