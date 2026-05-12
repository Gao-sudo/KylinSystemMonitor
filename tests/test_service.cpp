#include <QtTest>
#include <QtConcurrent>
#include <QFuture>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QFile>
#include <QThread>

#include "../service/cryptohelper.h"
#include "../service/asyncmonitor.h"

class ServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<AsyncSystemData>("AsyncSystemData");
        AsyncMonitor::instance().stop();
    }

    void cleanup()
    {
        AsyncMonitor::instance().stop();
    }

    void testSm3HashAndString()
    {
        CryptoHelper& helper = CryptoHelper::instance();
        QByteArray data("kylin-monitor");
        QByteArray hash = helper.sm3Hash(data);

        QCOMPARE(hash.size(), 32);
        QCOMPARE(helper.sm3HashToString(data).size(), 64);
        QCOMPARE(helper.sm3HashToString(data), QString::fromLatin1(hash.toHex()));
    }

    void testSm4EncryptDecryptAndBoundary()
    {
        CryptoHelper& helper = CryptoHelper::instance();
        const QByteArray plain("test-plain-text");
        const QByteArray key("0123456789abcdef");

        const QByteArray encrypted = helper.sm4Encrypt(plain, key);
        QVERIFY(encrypted != plain);
        QCOMPARE(helper.sm4Decrypt(encrypted, key), plain);

        const QByteArray encryptedWithEmptyKey = helper.sm4Encrypt(plain, QByteArray());
        QCOMPARE(encryptedWithEmptyKey, plain);
        QCOMPARE(helper.sm4Decrypt(encryptedWithEmptyKey, QByteArray()), plain);
    }

    void testJsonEncryptDecryptAndBoundary()
    {
        CryptoHelper& helper = CryptoHelper::instance();
        const QByteArray key("1234567890abcdef");

        QJsonObject origin;
        origin["host"] = "kylin";
        origin["cpu"] = 20.5;
        origin["ok"] = true;

        QJsonObject encrypted = helper.encryptJsonData(origin, key);
        QVERIFY(encrypted.contains("encrypted"));
        QVERIFY(encrypted.contains("algorithm"));
        QVERIFY(encrypted.contains("timestamp"));
        QCOMPARE(encrypted["algorithm"].toString(), QString("SM4-XOR"));

        QJsonObject decrypted = helper.decryptJsonData(encrypted, key);
        QCOMPARE(decrypted, origin);

        QJsonObject invalid;
        QCOMPARE(helper.decryptJsonData(invalid, key), QJsonObject());
    }

    void testExportEncryptedDataSuccessAndFailure()
    {
        CryptoHelper& helper = CryptoHelper::instance();
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QJsonObject payload;
        payload["a"] = 1;
        payload["b"] = "text";

        const QString outFile = tempDir.path() + "/encrypted.json";
        QVERIFY(helper.exportEncryptedData(outFile, payload));
        QVERIFY(QFileInfo::exists(outFile));
        QVERIFY(QFileInfo::exists(outFile + ".key"));

        QVERIFY(!helper.exportEncryptedData("/path/not/exist/encrypted.json", payload));
    }

    void testCryptoHelperConcurrentIntegration()
    {
        CryptoHelper& helper = CryptoHelper::instance();
        const QByteArray key("fedcba0987654321");
        const int threadCount = 8;
        const int roundsPerThread = 100;

        QVector<QFuture<bool>> futures;
        futures.reserve(threadCount);

        for (int t = 0; t < threadCount; ++t) {
            futures.append(QtConcurrent::run([&helper, key, t, roundsPerThread]() -> bool {
                for (int i = 0; i < roundsPerThread; ++i) {
                    QByteArray plain = QByteArray("thread-") + QByteArray::number(t) +
                                       "-round-" + QByteArray::number(i);
                    QByteArray encrypted = helper.sm4Encrypt(plain, key);
                    QByteArray decrypted = helper.sm4Decrypt(encrypted, key);
                    if (decrypted != plain) {
                        return false;
                    }
                }
                return true;
            }));
        }

        for (auto& f : futures) {
            f.waitForFinished();
            QVERIFY(f.result());
        }
    }

    void testAsyncMonitorStartStopAndBoundary()
    {
        AsyncMonitor& monitor = AsyncMonitor::instance();
        QSignalSpy spy(&monitor, SIGNAL(dataReady(AsyncSystemData)));
        QVERIFY(spy.isValid());

        monitor.start(0);
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 2, 2000);

        QList<QVariant> args = spy.takeFirst();
        QVERIFY(!args.isEmpty());
        AsyncSystemData data = qvariant_cast<AsyncSystemData>(args.at(0));
        QVERIFY(data.timestamp > 0);
        QVERIFY(data.cpuUsage >= 0.0);
        QVERIFY(data.memUsage >= 0.0);

        monitor.stop();
        const int countAfterStop = spy.count();
        QTest::qWait(100);
        QVERIFY(spy.count() <= countAfterStop + 1);
    }

    void testAsyncMonitorMultiThreadIntegration()
    {
        AsyncMonitor& monitor = AsyncMonitor::instance();
        const int workers = 6;
        const int rounds = 20;

        QVector<QFuture<void>> futures;
        futures.reserve(workers);

        for (int i = 0; i < workers; ++i) {
            futures.append(QtConcurrent::run([&monitor, i, rounds]() {
                for (int r = 0; r < rounds; ++r) {
                    if ((i + r) % 2 == 0) {
                        monitor.start(15);
                    } else {
                        monitor.stop();
                    }
                    QThread::msleep(2);
                }
            }));
        }

        for (auto& f : futures) {
            f.waitForFinished();
        }

        QSignalSpy spy(&monitor, SIGNAL(dataReady(AsyncSystemData)));
        monitor.start(20);
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 1500);
        monitor.stop();
    }
};

QTEST_APPLESS_MAIN(ServiceTest)
#include "test_service.moc"
