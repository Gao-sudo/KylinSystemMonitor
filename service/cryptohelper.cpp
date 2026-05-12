#include "cryptohelper.h"
#include <QFile>
#include <QDateTime>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

CryptoHelper::CryptoHelper()
    : QObject(nullptr)
    , m_initialized(false)
{
    m_initialized = true;
}

CryptoHelper::~CryptoHelper()
{
}

CryptoHelper& CryptoHelper::instance()
{
    static CryptoHelper instance;
    return instance;
}

QByteArray CryptoHelper::sm3Hash(const QByteArray& data)
{
    // 使用 SHA256 替代 SM3
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QString CryptoHelper::sm3HashToString(const QByteArray& data)
{
    return sm3Hash(data).toHex();
}

QByteArray CryptoHelper::sm4Encrypt(const QByteArray& plaintext, const QByteArray& key)
{
    QMutexLocker locker(&m_mutex);

    if (key.isEmpty()) {
        qWarning() << "SM4: Empty key";
        return plaintext;
    }

    // 简化 XOR 加密
    QByteArray result = plaintext;
    for (int i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % key.size()];
    }
    return result;
}

QByteArray CryptoHelper::sm4Decrypt(const QByteArray& ciphertext, const QByteArray& key)
{
    // XOR 加密是对称的，解密和加密相同
    return sm4Encrypt(ciphertext, key);
}

QByteArray CryptoHelper::generateSm4Key()
{
    QByteArray key(16, Qt::Uninitialized);
    for (int i = 0; i < 16; ++i) {
        key[i] = static_cast<char>(QRandomGenerator::global()->bounded(0, 256));
    }
    return key;
}

QJsonObject CryptoHelper::encryptJsonData(const QJsonObject& data, const QByteArray& key)
{
    QJsonDocument doc(data);
    QByteArray plaintext = doc.toJson(QJsonDocument::Compact);
    QByteArray ciphertext = sm4Encrypt(plaintext, key);

    QJsonObject result;
    result["encrypted"] = QString::fromLatin1(ciphertext.toBase64());
    result["algorithm"] = "SM4-XOR";
    result["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    return result;
}

QJsonObject CryptoHelper::decryptJsonData(const QJsonObject& encrypted, const QByteArray& key)
{
    QJsonObject result;

    QString encStr = encrypted["encrypted"].toString();
    if (encStr.isEmpty()) {
        return result;
    }

    QByteArray ciphertext = QByteArray::fromBase64(encStr.toLatin1());
    QByteArray plaintext = sm4Decrypt(ciphertext, key);

    QJsonDocument doc = QJsonDocument::fromJson(plaintext);
    if (!doc.isNull()) {
        result = doc.object();
    }

    return result;
}

bool CryptoHelper::exportEncryptedData(const QString& filePath, const QJsonObject& data)
{
    QByteArray key = generateSm4Key();
    QJsonObject encrypted = encryptJsonData(data, key);

    // 保存加密数据
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file:" << filePath;
        return false;
    }

    file.write(QJsonDocument(encrypted).toJson(QJsonDocument::Indented));
    file.close();

    // 保存密钥
    QString keyPath = filePath + ".key";
    QFile keyFile(keyPath);
    if (keyFile.open(QIODevice::WriteOnly)) {
        keyFile.write(key.toBase64());
        keyFile.close();
        qDebug() << "Encrypted data saved to:" << filePath;
        qDebug() << "Key saved to:" << keyPath;
        return true;
    }

    return false;
}
