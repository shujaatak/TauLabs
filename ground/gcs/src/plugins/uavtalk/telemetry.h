/**
 ******************************************************************************
 * @file       telemetry.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVTalkPlugin UAVTalk Plugin
 * @{
 * @brief Provide a higher level of telemetry control on top of UAVTalk
 * including setting up transactions and acknowledging their receipt or
 * timeout
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "uavtalk.h"
#include "uavobjectmanager.h"
#include "gcstelemetrystats.h"
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QQueue>
#include <QMap>

class TransactionKey;

class ObjectTransactionInfo: public QObject {
    Q_OBJECT

public:
    ObjectTransactionInfo(QObject * parent);
    ~ObjectTransactionInfo();
    UAVObject* obj;
    bool allInstances;
    bool objRequest;
    qint32 retriesRemaining;
    bool acked;
    QPointer<class Telemetry>telem;
    QTimer* timer;
private slots:
    void timeout();
};

class Telemetry: public QObject
{
    Q_OBJECT

public:
    typedef struct {
        quint32 txBytes;
        quint32 rxBytes;
        quint32 txObjectBytes;
        quint32 rxObjectBytes;
        quint32 rxObjects;
        quint32 txObjects;
        quint32 txErrors;
        quint32 rxErrors;
        quint32 txRetries;
    } TelemetryStats;

    Telemetry(UAVTalk* utalk, UAVObjectManager* objMngr);
    ~Telemetry();
    TelemetryStats getStats();
    void resetStats();
    void transactionTimeout(ObjectTransactionInfo *info);

signals:

private:
    // Constants
    static const int REQ_TIMEOUT_MS = 250;
    static const int MAX_RETRIES = 2;
    static const int MAX_UPDATE_PERIOD_MS = 1000;
    static const int MIN_UPDATE_PERIOD_MS = 1;
    static const int MAX_QUEUE_SIZE = 20;

    // Types
    /**
     * Events generated by objects
     */
    typedef enum {
        EV_NONE = 0x00,             /** No event */
        EV_UNPACKED = 0x01,         /** Object data updated by unpacking */
        EV_UPDATED = 0x02,          /** Object data updated by changing the data structure */
        EV_UPDATED_MANUAL = 0x04,   /** Object update event manually generated */
        EV_UPDATED_PERIODIC = 0x8,  /** Object update event generated by timer */
        EV_UPDATE_REQ = 0x010       /** Request to update object data */
    } EventMask;

    typedef struct {
        UAVObject* obj;
        qint32 updatePeriodMs;      /** Update period in ms or 0 if no periodic updates are needed */
        qint32 timeToNextUpdateMs;  /** Time delay to the next update */
    } ObjectTimeInfo;

    typedef struct {
        UAVObject* obj;
        EventMask event;
        bool allInstances;
    } ObjectQueueInfo;

    // Variables
    UAVObjectManager* objMngr;
    UAVTalk* utalk;
    GCSTelemetryStats* gcsStatsObj;
    QVector<ObjectTimeInfo> objList;
    QQueue<ObjectQueueInfo> objQueue;
    QQueue<ObjectQueueInfo> objPriorityQueue;
    QMap<TransactionKey, ObjectTransactionInfo*>transMap;
    QMutex* mutex;
    QTimer* updateTimer;
    QTimer* statsTimer;
    qint32 timeToNextUpdateMs;
    quint32 txErrors;
    quint32 txRetries;

    // Methods
    void registerObject(UAVObject* obj);
    void addObject(UAVObject* obj);
    void setUpdatePeriod(UAVObject* obj, qint32 periodMs);
    void connectToObjectInstances(UAVObject* obj, quint32 eventMask);
    void updateObject(UAVObject* obj, quint32 eventMask);
    void processObjectUpdates(UAVObject* obj, EventMask event, bool allInstances, bool priority);
    void processObjectTransaction(ObjectTransactionInfo *transInfo);
    void processObjectQueue();
    bool updateTransactionMap(UAVObject* obj, bool request);


private slots:
    void objectUpdatedAuto(UAVObject* obj);
    void objectUpdatedManual(UAVObject* obj);
    void objectUpdatedPeriodic(UAVObject* obj);
    void objectUnpacked(UAVObject* obj);
    void updateRequested(UAVObject* obj);
    void updateAllInstancesRequested(UAVObject* obj);
    void newObject(UAVObject* obj);
    void newInstance(UAVObject* obj);
    void processPeriodicUpdates();
    void transactionSuccess(UAVObject* obj);
    void transactionFailure(UAVObject* obj);
    void transactionRequestCompleted(UAVObject* obj);

};

#endif // TELEMETRY_H
