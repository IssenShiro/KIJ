#include "connection.h"

Connection::Connection(QObject *parent) :
    QObject(parent)
{
    srand(time(NULL));
    this->socket = new QTcpSocket(this);
    this->connect_slots();
    this->initPrivateKey();
}

//connect slots
void Connection::connect_slots()
{
    connect(this->socket, SIGNAL(readyRead()), this, SLOT(emitSignalReady()));
}

//get function
QString Connection::getServerAdress()
{
    return this->serverAddress;
}

int Connection::getPort()
{
    return this->port;
}

//set function
void Connection::setServerAddress(QString address)
{
    this->serverAddress = address;
}

void Connection::setPort(int port)
{
    this->port = port;
}

//system function
void Connection::sendKey()
{
    //get key
    QString key;
    char temp[MAX_STRING];
    sprintf(temp, "%llu", getPublicKeyG());
    key.push_back(temp);
    qDebug("client public key -> %s", key.toStdString().c_str());
    qDebug("client private key -> %llu", this->privateKey);


    //send key
    this->socket->write(key.toUtf8().trimmed());
}

void Connection::readKey()
{
    //testing
    QString key = QString::fromUtf8(this->socket->read(MAX_BUFFER).trimmed());
    qDebug("key server -> %s", key.toStdString().c_str());

    QRegExp checker("-");
    QStringList temp = key.split(checker);

    uint64_t varP = temp[0].toInt(0, 10);
    uint64_t varQ = temp[1].toInt(0, 10);
    uint64_t varG = temp[2].toInt(0, 10);

    setPublicKey(varP, varQ, varG);
}

int Connection::readConfirmation()
{
    //testing
    QString confirmation = QString::fromUtf8(this->socket->read(MAX_BUFFER).trimmed());
    qDebug("confirmation from server -> %s", confirmation.toStdString().c_str());

    if(confirmation == "KEY_EXCHANGE_SUCCESS")
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void Connection::sendData(QString state, QString flag, QString receiver, QString sender, QString type, QString content)
{
    //encrypt
    uint64_t seed = getSharedKey(this->serverKey);
    qDebug("seed -> %llu", seed);
    QString encrypted_message = AES_Encrypt(this->getFormatMessage(state, flag, receiver, sender, type, content), seed);
    qDebug("message -> %s", this->getFormatMessage(state, flag, receiver, sender, type, content).toStdString().c_str());
    qDebug("encrypted message -> %s", encrypted_message.toStdString().c_str());

    //send
    this->socket->write(encrypted_message.toUtf8().trimmed());
}

QString Connection::readData()
{
    //testing
    QString temp = QString::fromUtf8(this->socket->read(MAX_BUFFER).trimmed());
    qDebug("encrypted server message -> %s", temp.toStdString().c_str());

    //decrypt here
    uint64_t seed = getSharedKey(this->serverKey);
    qDebug("seed -> %llu", seed);
    QString decrypted_message = AES_Decrpyt(temp, seed);
    qDebug("decrypted server message -> %s", decrypted_message.toStdString().c_str());
    return decrypted_message;
}

void Connection::ConnectingToServer()
{
    this->socket->connectToHost(this->serverAddress, this->port);
}

void Connection::disconnecting()
{
    this->socket->disconnectFromHost();
}

bool Connection::isConnectedToServer()
{
    return this->socket->waitForConnected(WAIT_TIME);
}

void Connection::waiting()
{
    this->socket->waitForReadyRead(WAIT_TIME);
}

void Connection::emitSignalReady()
{
    emit incomingData();
}

//Private function
QString Connection::getFormatMessage(QString state, QString flag, QString receiver, QString sender, QString type, QString content)
{
    QString output;
    output += state + "\r\n" + flag + "\r\n" + receiver + "\r\n" + sender + "\r\n" + type + "\r\n" + content;
    return output;
}

//Diffie-helman function
void Connection::initPrivateKey()
{
    this->privateKey = rand() % LIMIT_KEY;
}

void Connection::setPublicKey(uint64_t P, uint64_t Q, uint64_t G)
{
    this->publicKeyP = P;
    this->publicKeyQ = Q;
    this->serverKey = G;
}

uint64_t Connection::getPublicKeyG()
{
    uint64_t power = pow((double)this->publicKeyP, (double)this->privateKey);
    uint64_t result = (uint64_t)(power % this->publicKeyQ);
    return result;
}

uint64_t Connection::getSharedKey(uint64_t publicKeyG)
{
    uint64_t power = pow((double)publicKeyG, (double)this->privateKey);
    uint64_t result = (uint64_t)(power % this->publicKeyQ);
    return result;
}

//Diffie-helman end

//AES function
QString Connection::AES_Encrypt(QString message, uint64_t seed)
{
    char convert[MAX_BUFFER];
    char result[MAX_BUFFER][33];
    int b, i, blocksIV, blocksAscii;

    //AES mode (128, 192, 256) :
    this->AES_Mode = 128;

    //assign Nk dan Nr
    this->Nk = this->AES_Mode / 32;
    this->Nr = this->Nk + 6;

    //Initialization Vector 16 byte
    GenerateIV(seed, this->IV);

    //Key Enkripsi
    unsigned char tempkey[32] = {0x00  ,0x01  ,0x02  ,0x03  ,0x04  ,0x05  ,0x06  ,0x07  ,0x08  ,0x09  ,0x0a  ,0x0b  ,0x0c  ,0x0d  ,0x0e  ,0x0f};

    for(i=0;i<this->Nk*4;i++)
    {
        this->Key[i]=tempkey[i];
    }

    blocksAscii = ((message.length() - 1) / 16);

    //dipisah 16 byte
    char partAscii[MAX_DIVIDE][17];
    QByteArray array = message.toLocal8Bit();
    char* buffer = array.data();
    divideAscii(partAscii, buffer);

    char hasilxorencrypt[MAX_BUFF_DIVIDE][33];

    //Mulai enkripsi
    for (int ba = 0; ba <= blocksAscii; ba++)
    {
        if(ba == 0)
        {
            blocksIV = ((strlen(this->IV) - 1) / 16);
            //printf("jumlah : %d\n", blocksIV);
        }
        else
        {
            blocksIV = ((strlen(convert) - 1) / 16);
        }

        for(b = 0; b <= blocksIV; b++)
        {
            for(i = 0; i < Nb*4; i++)
            {
                if(ba == 0)
                {
                    if((i + (b * 16) < (int)strlen(this->IV)))
                    {
                        this->in[i] = (unsigned char)this->IV[ (i + (b * 16) ) ];
                    }
                    else
                    {
                        this->in[i] = 0x00;
                    }
                }
                else
                {
                    if((i + (b * 16) < (int)strlen(convert)))
                    {
                        this->in[i] = (unsigned char)convert[ (i + (b * 16) ) ];
                    }
                    else
                    {
                        this->in[i] = 0x00;
                    }
                }
            }

            // Fungsi KeyExpansion untuk ekspan key
            KeyExpansion();

            // Fungsi Cipher
            Cipher();

            char tempconvert[16];
            uncharToChar(tempconvert, this->out, sizeof(this->out));

            memset(&convert[0], 0, sizeof(convert));
            strcat(convert, tempconvert);
            memset(&tempconvert[0], 0, sizeof(tempconvert));

            memset(&this->in[0], 0, sizeof(this->in));
            memset(&this->out[0], 0, sizeof(this->out));
        }

        char plainInHex[33];
        char xorResult[MAX_BUFFER];
        char convSelected[MAX_BUFFER];

        memset(&convSelected[0], 0, sizeof(convSelected));
        strncpy(convSelected, convert, strlen(partAscii[ba])*2);
        memset(&plainInHex[0], 0, sizeof(plainInHex));
        char2hex(partAscii[ba], plainInHex);
        xor_str(plainInHex, convSelected, xorResult);

        strcpy(hasilxorencrypt[ba], xorResult);
        qDebug("PartAscii dengan panjang %lu -> %s\n", strlen(partAscii[ba]), partAscii[ba]);
        qDebug("plainInHex dengan panjang %lu -> %s\n", strlen(plainInHex), plainInHex);
        qDebug("panjang convertSelected %lu -> %s\n", strlen(convSelected), convSelected);
        qDebug("hasil xor ke %d -> %s\n", ba, hasilxorencrypt[ba]);
        strcpy(result[ba], hasilxorencrypt[ba]);
        memset(&hasilxorencrypt[ba][0], 0, sizeof(hasilxorencrypt[ba]));

        char tempConvResult[MAX_BUFFER];
        memset(&tempConvResult[0], 0, sizeof(tempConvResult));
        convertToReal(tempConvResult, convSelected);
        memset(&convert[0], 0, sizeof(convert));
        strcpy(convert, tempConvResult);
        memset(&xorResult[0], 0, sizeof(xorResult));
    }

    QString encrypted_message;
    encrypted_message.clear();
    for(i = 0; i <= blocksAscii; i++)
    {
        encrypted_message.push_back(result[i]);
        qDebug("hasil xor di luar ke %d -> %s\n", i, result[i]);

        memset(&result[i][0], 0, sizeof(result[i]));
    }

    for(i = 0; i < 4; i++)
    {
        memset(&this->state[i][0], 0, sizeof(this->state[i]));
    }

    return encrypted_message;
}

QString Connection::AES_Decrpyt(QString message, uint64_t seed)
{
    char convert[MAX_BUFFER];
    char coba[MAX_DIVIDE][17];
    int b, i, blocksIV, blocksHexa;

    //AES mode (128, 192, 256) :
    this->AES_Mode = 128;

    //assign Nk dan Nr
    this->Nk = this->AES_Mode / 32;
    this->Nr = this->Nk + 6;

    //Initialization Vector 16 byte
    GenerateIV(seed, this->IV);

    //Key Enkripsi
    unsigned char tempkey[32] = {0x00  ,0x01  ,0x02  ,0x03  ,0x04  ,0x05  ,0x06  ,0x07  ,0x08  ,0x09  ,0x0a  ,0x0b  ,0x0c  ,0x0d  ,0x0e  ,0x0f};

    for(i=0;i<Nk*4;i++)
    {
        this->Key[i]=tempkey[i];
    }

    blocksHexa = ((message.length() - 1) / 32);

    //dipisah 16 byte
    char partHexa[MAX_BUFF_DIVIDE][33];
    QByteArray array = message.toLocal8Bit();
    char* buffer = array.data();
    divideHexa(partHexa, buffer);

    char hasilxordecrypt[MAX_BUFF_DIVIDE][33];

    //Mulai enkripsi
    for (int ba = 0; ba <= blocksHexa; ba++)
    {
        if(ba == 0)
        {
            blocksIV = ((strlen(this->IV) - 1) / 16);
            //printf("jumlah : %d\n", blocksIV);
        }
        else
        {
            blocksIV = ((strlen(convert) - 1) / 16);
        }

        for(b = 0; b <= blocksIV; b++)
        {
            for(i=0;i<Nb*4;i++)
            {
                if(ba == 0)
                {
                    if((i + (b * 16) < (int)strlen(this->IV)))
                    {
                        this->in[i] = (unsigned char)this->IV[ (i + (b * 16) ) ];
                    }
                    else
                    {
                        this->in[i] = 0x00;
                    }
                }
                else
                {
                    if((i + (b * 16) < (int)strlen(convert)))
                    {
                        this->in[i] = (unsigned char)convert[ (i + (b * 16) ) ];
                    }
                    else
                    {
                        this->in[i] = 0x00;
                    }
                }
            }

            // Fungsi KeyExpansion untuk ekspan key
            KeyExpansion();

            // Fungsi Cipher
            Cipher();

            char tempconvert[33];
            uncharToChar(tempconvert, this->out, sizeof(this->out));

            memset(&convert[0], 0, sizeof(convert));
            strcat(convert, tempconvert);

            memset(&this->in[0], 0, sizeof(this->in));
            memset(&this->out[0], 0, sizeof(this->out));            
        }

        char tempRes[MAX_BUFFER];
        char convSelected2[MAX_BUFFER];

        memset(&convSelected2[0], 0, sizeof(convSelected2));
        strncpy(convSelected2, convert, strlen(partHexa[ba]));
        xor_str(partHexa[ba], convSelected2, tempRes);

        strcpy(hasilxordecrypt[ba], tempRes);
        qDebug("Encrypted message dengan panjang %lu -> %s\n", strlen(partHexa[ba]), partHexa[ba]);
        qDebug("panjang convertSelected2 %lu -> %s\n", strlen(convSelected2), convSelected2);
        qDebug("hasil xor ke %d -> %s\n", ba, hasilxordecrypt[ba]);

        char tempConvResult[MAX_BUFFER];
        memset(&tempConvResult[0], 0, sizeof(tempConvResult));
        convertToReal(tempConvResult, convSelected2);
        memset(&convert[0], 0, sizeof(convert));
        strcpy(convert, tempConvResult);
        memset(&tempRes[0], 0, sizeof(tempRes));

        memset(&coba[ba][0], 0, sizeof(coba[ba]));
        convertToReal(coba[ba], hasilxordecrypt[ba]);
    }

    QString decrypted_message;
    decrypted_message.clear();
    for(i = 0; i <= blocksHexa; i++)
    {
        decrypted_message.push_back(QString::fromUtf8(coba[i]));
        memset(&coba[i][0], 0, sizeof(coba[i]));
    }

    for(i = 0; i < 4; i++)
    {
        memset(&this->state[i][0], 0, sizeof(this->state[i]));
    }

    return decrypted_message;
}

int Connection::getSBoxValue(int num)
{
    int sbox[256] =   {
        //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, //0
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //1
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, //2
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //3
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, //4
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //5
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, //6
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //7
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, //8
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //9
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, //A
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //B
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, //C
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //D
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, //E
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 }; //F
    return sbox[num];
}

int Connection::getRconValue(int num)
{
    // The round constant word array, Rcon[i], contains the values given by
    // x to th e power (i-1) being powers of x (x is denoted as {02}) in the field GF(28)
    // Note that i starts at 1, not 0).
    int Rcon[255] = {
        0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
        0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39,
        0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a,
        0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8,
        0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef,
        0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc,
        0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b,
        0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3,
        0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94,
        0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
        0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35,
        0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f,
        0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04,
        0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63,
        0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd,
        0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb  };
    return Rcon[num];
}

void Connection::KeyExpansion()
{
    int i,j;
    unsigned char temp[4],k;

    // round key pertama.
    for(i=0;i<Nk;i++)
    {
        RoundKey[i*4]=Key[i*4];
        RoundKey[i*4+1]=Key[i*4+1];
        RoundKey[i*4+2]=Key[i*4+2];
        RoundKey[i*4+3]=Key[i*4+3];
    }

    // round key untuk round lainnya.
    while (i < (Nb * (Nr+1)))
    {
        for(j=0;j<4;j++)
        {
            temp[j]=RoundKey[(i-1) * 4 + j];
        }
        if (i % Nk == 0)
        {
            // Fungsi rotasi 4 byte ke kiri.
            // [a0,a1,a2,a3] jadi [a1,a2,a3,a0]

            // Fungsi RotWord()
            {
                k = temp[0];
                temp[0] = temp[1];
                temp[1] = temp[2];
                temp[2] = temp[3];
                temp[3] = k;
            }

            // SubWord() fungsi yang mengambil 4 byte word
            // dan dilakukan subtitusi terhadap S-Box.

            // Fungsi Subword()
            {
                temp[0]=getSBoxValue(temp[0]);
                temp[1]=getSBoxValue(temp[1]);
                temp[2]=getSBoxValue(temp[2]);
                temp[3]=getSBoxValue(temp[3]);
            }

            temp[0] =  temp[0] ^ getRconValue(i/Nk);
        }
        else if (Nk > 6 && i % Nk == 4)
        {
            // Fungsi Subword()
            {
                temp[0]=getSBoxValue(temp[0]);
                temp[1]=getSBoxValue(temp[1]);
                temp[2]=getSBoxValue(temp[2]);
                temp[3]=getSBoxValue(temp[3]);
            }
        }
        RoundKey[i*4+0] = RoundKey[(i-Nk)*4+0] ^ temp[0];
        RoundKey[i*4+1] = RoundKey[(i-Nk)*4+1] ^ temp[1];
        RoundKey[i*4+2] = RoundKey[(i-Nk)*4+2] ^ temp[2];
        RoundKey[i*4+3] = RoundKey[(i-Nk)*4+3] ^ temp[3];
        i++;
    }
}

void Connection::AddRoundKey(int round)
{
    int i,j;
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            state[j][i] ^= RoundKey[round * Nb * 4 + i * Nb + j];
        }
    }
}

void Connection::SubBytes()
{
    int i,j;
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            state[i][j] = this->getSBoxValue(state[i][j]);

        }
    }
}

void Connection::ShiftRows()
{
    unsigned char temp;

    // Rotasi row pertama 1 kolom ke kiri
    temp=state[1][0];
    state[1][0]=state[1][1];
    state[1][1]=state[1][2];
    state[1][2]=state[1][3];
    state[1][3]=temp;

    // Rotasi row kedua 2 kolom ke kiri
    temp=state[2][0];
    state[2][0]=state[2][2];
    state[2][2]=temp;

    temp=state[2][1];
    state[2][1]=state[2][3];
    state[2][3]=temp;

    // Rotasi row ketiga 3 kolom ke kiri
    temp=state[3][0];
    state[3][0]=state[3][3];
    state[3][3]=state[3][2];
    state[3][2]=state[3][1];
    state[3][1]=temp;
}

void Connection::MixColumns()
{
    int i;
    unsigned char Tmp,Tm,t;
    for(i=0;i<4;i++)
    {
        t=state[0][i];
        Tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i] ;
        Tm = state[0][i] ^ state[1][i] ; Tm = xtime(Tm); state[0][i] ^= Tm ^ Tmp ;
        Tm = state[1][i] ^ state[2][i] ; Tm = xtime(Tm); state[1][i] ^= Tm ^ Tmp ;
        Tm = state[2][i] ^ state[3][i] ; Tm = xtime(Tm); state[2][i] ^= Tm ^ Tmp ;
        Tm = state[3][i] ^ t ; Tm = xtime(Tm); state[3][i] ^= Tm ^ Tmp ;
    }
}

void Connection::Cipher()
{
    int i,j,round=0;

    //Copy input PlainText ke state array.
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            state[j][i] = in[i*4 + j];
        }
    }

    // Tambahkan round key pertama ke state sebelum memulai rounds.
    AddRoundKey(0);

    // Akan ada rounds sebanyak Nr.
    // Nr-1 rounds pertama identik.
    // Nr-1 rounds dijalankan dengan loop dibawah.
    for(round=1;round<Nr;round++)
    {
        SubBytes();
        ShiftRows();
        MixColumns();
        AddRoundKey(round);
    }

    // Round terakhir.
    // Fungsi MixColumns tidak dilakukan di round ini.
    SubBytes();
    ShiftRows();
    this->AddRoundKey(Nr);

    // Proses enkripsi berakhir.
    // Copy state array ke output array.
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            out[i*4+j]=state[j][i];
        }
    }
}
//AES Procedure end

char Connection::hexToAscii(char first, char second)
{
    char hex[5], *stop;
    hex[0] = '0';
    hex[1] = 'x';
    hex[2] = first;
    hex[3] = second;
    hex[4] = 0;
    return strtol(hex, &stop, 16);
}

void Connection::uncharToChar(char *dst, unsigned char *src,size_t src_len)
{
        while (src_len--)
            dst += sprintf(dst,"%02x",*src++);
        *dst = '\0';
}

void Connection::convertToReal(char *dest, char *source)
{
    int i;
    int count = 0;

    memset(&dest[0], 0, sizeof(dest));

    for(i = 0; i < (int)strlen(source)/2; i++)
    {
        if(source[count] == '0' && source[count+1] == '0')
        {
            dest[i] = 0;
        }
        else
        {
            dest[i] = hexToAscii(source[count], source[count+1]);
        }

        count += 2;
    }
}


void Connection::containHex(char dest[MAX_BUFFER][1], char *source)
{
    int i;
    int count = 0;
    for(i = 0; i < (int)strlen(source)/2; i++)
    {
        dest[i][0] = hexToAscii(source[count], source[count+1]);
        count += 2;
    }
}

// Fungsi char -> hex versi 2
void Connection::char2hex(char* A, char *Hex) {
    int i=0;
    int j=0;
    int t;
    char temp[32];

    while(A[i] != '\0') {

        //printf("%d %lu\n", i, strlen(A));

        t = A[i] >> 4;
        if (t < 10) {
            temp[j] = '0' + t;
        }
        else {
            temp[j] = 'a' + t - 10;
        }
        j++;
        t = A[i] % 16;
        if (t < 10) {
            temp[j] = '0' + t;
        }
        else {
            temp[j] = 'a' + t - 10;
        }
        j++;
        i++;
    }

    for(i = 0; i < (int)strlen(A)*2; i++)
    {
        Hex[i] = temp[i];
    }
}

// Fungsi xor
void Connection::xor_str(char *A, char *B, char *res) {
    int i = 0;
    int a, b, x;

    while (A[i] != 0) {//kalau belum null
        if (A[i] >= '0' && A[i] <= '9') {
            a = A[i] - '0';
        }
        else if (A[i] >= 'a' && A[i] <= 'z'){
            a = A[i] - 'a' + 10;
        }
        if (B[i] >= '0' && B[i] <= '9') {
            b = B[i] - '0';
        }
        else if (B[i] >= 'a' && B[i] <= 'z'){
            b = B[i] - 'a' + 10;
        }
        x = a ^ b;
        if (x < 10) {
            res[i] = '0' + x;
        }
        else {
            res[i] = 'a' + x - 10;
        }
        i++;
    }
    res[i] = 0;
}

void Connection::divideAscii(char dest[MAX_STRING][17], char *source)
{
    int i, j, part;
    part = (strlen(source) - 1) / 16;
    for(i = 0; i <= part; i++)
    {
        memset(&dest[i][0], 0, sizeof(dest[i]));
        for(j = i * 16; j < ((i+1) * 16); j++)
        {
            if(j >= (int)strlen(source))
            {
                dest[i][j] = '\0';
                break;
            }
            dest[i][j%16] = source[j];
        }
    }
}

void Connection::divideHexa(char dest[MAX_BUFFER][33], char *source)
{
    int i, j, part;
    part = (strlen(source) - 1) / 32;
    for(i = 0; i <= part; i++)
    {
        memset(&dest[i][0], 0, sizeof(dest[i]));
        for(j = i * 32; j < ((i+1) * 32); j++)
        {
            if(j >= (int)strlen(source))
            {
                dest[i][j] = '\0';
                break;
            }
            dest[i][j%32] = source[j];
        }
    }
}

void Connection::GenerateIV(long long int seed, char *out)
{
    char result[17];
    char randomletter[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    memset(&result[0], 0, sizeof(result));

    for(int i = 0; i < 16; i++)
    {
        switch(i)
        {
            case 0:
                result[i] = randomletter[(seed) % 26];
                break;

            case 1:
                result[i] = randomletter[(i+(seed*2)) % 26];
                break;

            case 2:
                result[i] = randomletter[(seed*i) % 26];
                break;

            case 3:
                result[i] = randomletter[((seed*5)/i) % 26];
                break;

            case 4:
                result[i] = randomletter[((seed*seed)-i) % 26];
                break;

            case 5:
                result[i] = randomletter[((seed*i)+seed) % 26];
                break;

            case 6:
                result[i] = randomletter[((seed*i)+(seed/i)) % 26];
                break;

            case 7:
                result[i] = randomletter[((seed*i)-(seed/i)) % 26];
                break;

            case 8:
                result[i] = randomletter[((seed+i)+(seed-i)) % 26];
                break;

            case 9:
                result[i] = randomletter[((seed+i)*(seed)) % 26];
                break;

            case 10:
                result[i] = randomletter[((seed*i)+(seed*i)) % 26];
                break;

            case 11:
                result[i] = randomletter[((seed*i)*(seed*i)) % 26];
                break;

            case 12:
                result[i] = randomletter[((seed+i)+(seed*i)) % 26];
                break;

            case 13:
                result[i] = randomletter[((seed*i)/(seed+i)) % 26];
                break;

            case 14:
                result[i] = randomletter[((seed/i)+(seed)) % 26];
                break;

            case 15:
                result[i] = randomletter[((seed/i)*(seed+i)) % 26];
                break;
        }
    }
    memset(&out[0], 0, sizeof(out));
    strcpy(out, result);
}


