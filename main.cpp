#include "monitor.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <coschedula/fs.h>
#include <coschedula/task.h>
#include <iostream>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    MonitorImpl<coschedula::scheduler> mon;

    const auto &&subtask = []() -> coschedula::task<int, coschedula::scheduler> {
        for (std::size_t i = 0; i < 4; ++i) {
            co_await coschedula::suspend{};
        }
        co_return 1;
    };

    std::ifstream stream("/home/borys/datasets/large_text_files/mediumfile0.txt");
    assert(stream.is_open());
    assert(stream.good());
    std::cout << "mediumfile0.txt: "
              << std::string((std::istreambuf_iterator(stream)), std::istreambuf_iterator<char>())
                     .size()
              << std::endl;

    const auto &&task = [&subtask]() -> coschedula::task<int, coschedula::scheduler> {
        co_await coschedula::suspend{};
        const auto st = subtask();

        const auto fstask = coschedula::fs::read<std::string::value_type, coschedula::execution::par>(
            "/home/borys/datasets/large_text_files/mediumfile0.txt");

        for (std::size_t i = 0; i < 4; ++i) {
            co_await coschedula::suspend{};
        }
        co_await st;
        const auto str = co_await fstask;
        std::cout << "str: " << str.size() << std::endl;
        co_return 1;
    };

    engine.setInitialProperties({{"monitor", QVariant::fromValue(&mon)}});

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("coschedula_monitor", "Main");

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&t]() {
        if (!coschedula::scheduler::instance<coschedula::scheduler>.proceed()) {
            t.stop();
        }
    });
    t.start(0);

    QMetaObject::invokeMethod(&app, [&task]() { task(); });

    return app.exec();
}
