#ifndef CRYPTOHELPER_H
#define CRYPTOHELPER_H

#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QMutex>

class CryptoHelper : public QObject
{
    Q_OBJECT
public:
    static CryptoHelper& instance();

    // SM3 哈希
    QByteArray sm3Hash(const QByteArray& data);
    QString sm3HashToString(const QByteArray& data);

    // SM4 对称加密
    QByteArray sm4Encrypt(const QByteArray& plaintext, const QByteArray& key);
    QByteArray sm4Decrypt(const QByteArray& ciphertext, const QByteArray& key);

    // 生成密钥
    QByteArray generateSm4Key();

    // 加密/解密 JSON 数据
    QJsonObject encryptJsonData(const QJsonObject& data, const QByteArray& key);
    QJsonObject decryptJsonData(const QJsonObject& encrypted, const QByteArray& key);

    // 导出加密数据
    bool exportEncryptedData(const QString& filePath, const QJsonObject& data);

private:
    CryptoHelper();
    ~CryptoHelper();
    CryptoHelper(const CryptoHelper&) = delete;
    CryptoHelper& operator=(const CryptoHelper&) = delete;

    QByteArray xorEncrypt(const QByteArray& data, const QByteArray& key);

    QMutex m_mutex;
    bool m_initialized;
};

#endif
