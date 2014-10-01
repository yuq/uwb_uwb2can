#include "uwb2can.h"
#include <QDebug>
#include <QTcpSocket>
#include <QUuid>
#include <cmath>

void get_position(double *p1, double *p2, double *p3,
                  double *p4, double *p5,
                  double d1, double d2, double d3, double d4,
                  double d5, double *r);

UWB2CAN::UWB2CAN(QObject *parent) :
    QObject(parent), mCanNode("can0", false),
    mUWBClientCount(0), mCurrentTimestamp(0),
    mPoints{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}, {1, 1, 1}},
    mUpdated{0}, mLogOn(false)
{
    if (mLogOn) {
        mLogFile.setFileName("uwb.log." + QUuid::createUuid().toString());
        if (mLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            mLogStream.setDevice(&mLogFile);
            mLogTime.start();
        }
        else {
            qCritical() << "log file open fail";
            mLogOn = false;
        }
    }

    if (!mCanNode.open_can()) {
        qCritical() << "CAN open fail";
        return;
    }

    mCANReadNotifier = new QSocketNotifier(mCanNode.get_fd(), QSocketNotifier::Read, this);
    connect(mCANReadNotifier, SIGNAL(activated(int)), SLOT(onRecvCanData(int)));

    mCANWriteNotifier = new QSocketNotifier(mCanNode.get_fd(), QSocketNotifier::Write, this);
    mCANWriteNotifier->setEnabled(false);
    connect(mCANWriteNotifier, SIGNAL(activated(int)), SLOT(onSendCanData(int)));

    // UWB server for target point phone to connect
    mUWBServer = new QTcpServer(this);
    connect(mUWBServer, SIGNAL(newConnection()), SLOT(onNewUWBConnection()));
    if (!mUWBServer->listen(QHostAddress::Any, 8123)) {
        qCritical() << "UWB server listen fail";
        return;
    }

    // Monitor server for monitor phone to connect
    mMonServer = new QTcpServer(this);
    connect(mMonServer, SIGNAL(newConnection()), SLOT(onNewMonConnection()));
    if (!mMonServer->listen(QHostAddress::Any, 8456)) {
        qCritical() << "Mon server listen fail";
        return;
    }

}

void UWB2CAN::onNewUWBConnection()
{
    qDebug() << "new UWB client connected";

    QTcpSocket *socket;
    while ((socket = mUWBServer->nextPendingConnection()) != NULL) {
        mUWBClientCount++;
        connect(socket, SIGNAL(disconnected()), SLOT(onUWBClientDisconnect()));
        connect(socket, SIGNAL(readyRead()), SLOT(onRecvUWBData()));
    }

    if (mUWBClientCount)  {
        foreach (QTcpSocket *client, mMonClients) {
            client->write("[connect] true\n");
        }
    }
}

void UWB2CAN::onUWBClientDisconnect()
{
    mUWBClientCount--;
    if (!mUWBClientCount) {
        foreach (QTcpSocket *client, mMonClients) {
            client->write("[connect] false\n");
        }
    }
    sender()->deleteLater();
}

void UWB2CAN::onNewMonConnection()
{
    qDebug() << "new MON client connected";

    QTcpSocket *socket;
    while ((socket = mMonServer->nextPendingConnection()) != NULL) {
        mMonClients.insert(socket);
        if (mUWBClientCount)
            socket->write("[connect] true\n");
        else
            socket->write("[connect] false\n");
        connect(socket, SIGNAL(disconnected()), SLOT(onMonClientDisconnect()));
    }
}

void UWB2CAN::onMonClientDisconnect()
{
    QTcpSocket *socket = (QTcpSocket *)sender();
    mMonClients.remove(socket);
    socket->deleteLater();
}

void UWB2CAN::onRecvUWBData()
{
    qDebug() << "UWB data here";

    QTcpSocket *socket = (QTcpSocket *)sender();
    while (socket->canReadLine()) {
        QByteArray data = socket->readLine(128);
        qDebug() << "UWB receive " << data;

        QStringList list = QString(data).split(' ');
        if (list.count() != 3) {
            qDebug() << "UWB data wrong format";
            return;
        }

        bool ok;
        int id = list[0].toInt(&ok);
        if (!ok) {
            qDebug() << "UWB id to int fail";
            return;
        }

        qlonglong time = list[1].toLongLong(&ok);
        if (!ok) {
            qDebug() << "UWB time to long long fail";
            return;
        }

        float dist = list[2].toFloat(&ok);
        if (!ok) {
            qDebug() << "UWB dist to float fail";
            return;
        }

        if (id < 0 || id > 4) {
            qDebug() << "UWB id out of range";
            return;
        }

        if (mLogOn) {
            mLogStream << '[' << mLogTime.elapsed() << "][sencor] P" << id + 1
                       << " time=" << time << " dist=" << dist << endl;
        }

        // new round
        if (mCurrentTimestamp < time) {
            mCurrentTimestamp = time;
            // if enough nodes present last round, compute position
            // P5 must be present
            if (mUpdated[4] && updatedNum() >= 4)
                computePosition();
            memset(mUpdated, 0, sizeof(mUpdated));
        }

        if (mCurrentTimestamp == time) {
            mDistance[id] = dist;
            mUpdated[id] = 1;
            if (updatedNum() >= 5) {
                computePosition();
                memset(mUpdated, 0, sizeof(mUpdated));
            }
        }
    }
}

void UWB2CAN::computePosition()
{
    // get distance of this round
    // set to very big number when not collected so that be dropped by algorithm
    for (int i = 0; i < 5; i++) {
        if (mUpdated[i] == 0)
            mDistance[i] = 1000000000;
    }

    get_position(mPoints[0], mPoints[1], mPoints[2], mPoints[3], mPoints[4],
            mDistance[0], mDistance[1], mDistance[2], mDistance[3], mDistance[4],
            mCurrentPos);

    qCritical() << "compute pos: x=" << mCurrentPos[0]
                << " y=" << mCurrentPos[1] << " z=" << mCurrentPos[2];

    double x = mCurrentPos[0] * 100;
    double y = mCurrentPos[1] * 100;
    double z = mCurrentPos[2] * 100;

    if (mLogOn) {
        mLogStream << '[' << mLogTime.elapsed() << "][compute] x=" << x
                   << " y=" << y << " z=" << z << endl;
    }


    QString mesg;
    mesg.sprintf("[compute] x=%d y=%d z=%d\n", (int)x, (int)y, (int)z);
    foreach (QTcpSocket *client, mMonClients) {
        client->write(mesg.toLatin1());
    }

    struct can_frame *frame = new can_frame;
    XCMGProtocol::ComputePosition *pack = (XCMGProtocol::ComputePosition *)frame->data;
    frame->can_id = XCMGProtocol::CANID_COMPUTE_POSITION;
    frame->can_dlc = sizeof(XCMGProtocol::ComputePosition);
    pack->x = x;
    pack->y = y;
    pack->z = z;
    pack->d = sqrt(x * x + y * y + z * z);
    mCanSendQueue.enqueue(frame);

    // begin to send can frames
    mCANWriteNotifier->setEnabled(true);
}

int UWB2CAN::updatedNum()
{
    int ret = 0;
    for (int i = 0; i < 5; i++)
        ret += mUpdated[i];
    return ret;
}

void UWB2CAN::onRecvCanData(int fd)
{
    Q_UNUSED(fd);
    struct can_frame frame;

    qDebug() << "CAN receive data";

    while (mCanNode.read_can(&frame)) {
        qDebug() << "can_id:" << frame.can_id << "can_dlc:"
                 << frame.can_dlc << "data:" << frame.data[0]
                 << frame.data[1] << frame.data[2] << frame.data[3]
                 << frame.data[4] << frame.data[5] << frame.data[6]
                 << frame.data[7];

        // error frame
        if (frame.can_id & CAN_ERR_FLAG)
            continue;

        int id = frame.can_id & CAN_SFF_MASK;
        switch (id) {
        case XCMGProtocol::CANID_SAVE_CONFIG:
            if (frame.can_dlc != sizeof(XCMGProtocol::SaveConfig)) {
                qCritical() << "CANID_SAVE_CONFIG dlc missmatch";
                break;
            }
            canUpdate((XCMGProtocol::SaveConfig *)frame.data);
            break;
        case XCMGProtocol::CANID_SET_POSITION_P1:
        case XCMGProtocol::CANID_SET_POSITION_P2:
        case XCMGProtocol::CANID_SET_POSITION_P3:
        case XCMGProtocol::CANID_SET_POSITION_P4:
        case XCMGProtocol::CANID_SET_POSITION_P5:
            if (frame.can_dlc != sizeof(XCMGProtocol::SetPosition)) {
                qCritical() << "CANID_SET_POSITION dlc missmatch";
                break;
            }
            canUpdate(id - XCMGProtocol::CANID_SET_POSITION_P1,
                      (XCMGProtocol::SetPosition *)frame.data);
            break;
        default:
            break;
        }
    }
}

void UWB2CAN::canUpdate(XCMGProtocol::SaveConfig *pack)
{
    (void)pack;
    qCritical() << "CAN save config has not been impelemented";
}

void UWB2CAN::canUpdate(int i, XCMGProtocol::SetPosition *pack)
{
    qCritical() << "CAN set position: P" << i + 1 << " x="
                << pack->x << " y=" << pack->y << " z=" << pack->z;

    mPoints[i][0] = (double)pack->x / 100;
    mPoints[i][1] = (double)pack->y / 100;
    mPoints[i][2] = (double)pack->z / 100;

    if (mLogOn) {
        mLogStream << '[' << mLogTime.elapsed() << "][update] P" << i + 1
                   << " x=" << pack->x << " y=" << pack->y << " z=" << pack->z << endl;
    }
}

void UWB2CAN::onSendCanData(int fd)
{
    Q_UNUSED(fd);

    qDebug() << "CAN send data";

    while (!mCanSendQueue.isEmpty()) {
        const can_frame *frame = mCanSendQueue.head();
        if (mCanNode.write_can(frame)) {
            qDebug() << "CAN send frame successfully";
            delete mCanSendQueue.dequeue();
        }
        else {
            qDebug() << "CAN send frame fail";
            break;
        }
    }

    if (mCanSendQueue.isEmpty())
        mCANWriteNotifier->setEnabled(false);
}
