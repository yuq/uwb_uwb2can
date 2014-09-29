#ifndef UWB2CAN_H
#define UWB2CAN_H

#include <QObject>
#include <QSocketNotifier>
#include <QTcpServer>
#include <QQueue>
#include <QFile>
#include <QTime>
#include <QTextStream>
#include "../can.h"
#include "protocol.h"

class UWB2CAN : public QObject
{
    Q_OBJECT
public:
    explicit UWB2CAN(QObject *parent = 0);

signals:

private slots:
    void onRecvCanData(int fd);
    void onSendCanData(int fd);
    void onRecvUWBData();
    void onNewUWBConnection();
    void onUWBClientDisconnect();
    void onNewMonConnection();
    void onRecvMonData();
    void onMonClientDisconnect();

private:
    Can mCanNode;
    QSocketNotifier *mCANReadNotifier;
    QSocketNotifier *mCANWriteNotifier;
    int mUWBClientCount;
    QTcpServer *mUWBServer;
    QTcpServer *mMonServer;

    QQueue<can_frame *> mCanSendQueue;

    qlonglong mCurrentTimestamp;
    double mPoints[5][3];
    double mDistance[5];
    double mCurrentPos[3];
    int mUpdated[5];

    bool mLogOn;
    QTime mLogTime;
    QFile mLogFile;
    QTextStream mLogStream;

    void computePosition();
    int updatedNum();

    void canUpdate(XCMGProtocol::SaveConfig *pack);
    void canUpdate(int i, XCMGProtocol::SetPosition *pack);
};

#endif // UWB2CAN_H
