#ifndef CRYPTOENGINE_H
#define CRYPTOENGINE_H

#include <QByteArray>
#include <QString>


class CryptoEngine
{
  private:
    static constexpr int KDF_MASTER_KEY_ITERATIONS = 125328;
    static constexpr int KDF_AAD_ITERATIONS = 13;
    static QByteArray deriveKey(const QByteArray &data, const QByteArray &salt, int iterations, quint64 keyLen);

  public:
    static QByteArray deriveKey(const QString &password, const QByteArray &salt);
    static QByteArray bindContext(const QByteArray& masterKey, const QByteArray &associatedData);
    static void encrypt(QByteArray& data, const QByteArray &key);
    static bool decrypt(QByteArray& data, const QByteArray &key);
};

#endif // CRYPTOENGINE_H
