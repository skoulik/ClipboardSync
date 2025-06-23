#ifndef CRYPTOENGINE_H
#define CRYPTOENGINE_H

#include <QByteArray>
#include <QString>


class CryptoEngine
{
  private:
    static constexpr int KDF_ITERATIONS = 125328;

  public:
    static QByteArray deriveKey(const QString &password, const QByteArray &salt);
    static void encrypt(QByteArray& data, const QByteArray &key);
    static bool decrypt(QByteArray& data, const QByteArray &key);
};

#endif // CRYPTOENGINE_H
