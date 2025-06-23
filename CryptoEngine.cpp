#include "CryptoEngine.h"
#include "thirdparty/tiny-AES-c/aes.hpp"
#include <QPasswordDigestor>
#include <QRandomGenerator>


QByteArray CryptoEngine::deriveKey(const QString &password, const QByteArray &salt)
{
    return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, password.toUtf8(), salt, KDF_ITERATIONS, AES_KEYLEN);
}

void CryptoEngine::encrypt(QByteArray &data, const QByteArray &key)
{
    uint8_t pad = AES_BLOCKLEN - (data.size() % AES_BLOCKLEN);
    data.append(pad, pad); // PKCS#7 padding

    QByteArray iv(AES_BLOCKLEN, 0);
    QRandomGenerator::system()->generate(iv.begin(), iv.end());

    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (const uint8_t*)key.constData(), (const uint8_t*)iv.constData());
    AES_CBC_encrypt_buffer(&ctx, (uint8_t*)data.data(), data.size());

    data.append(iv);
}

bool CryptoEngine::decrypt(QByteArray &data, const QByteArray &key)
{
    if(data.isEmpty() || data.size() % AES_BLOCKLEN > 0)
        return false;

    const QByteArray iv = data.right(AES_BLOCKLEN);
    data.chop(AES_BLOCKLEN);

    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (const uint8_t*)key.constData(), (const uint8_t*)iv.constData());
    AES_CBC_decrypt_buffer(&ctx, (uint8_t*)data.data(), data.size());

    uint8_t  pad = data.back();
    if(!data.endsWith(QByteArray(pad, pad)))
        return false;
    data.chop(pad);

    return true;
}

