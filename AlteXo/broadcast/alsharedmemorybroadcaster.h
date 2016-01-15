#ifndef ALSHAREDMEMORYBROADCASTER_H
#define ALSHAREDMEMORYBROADCASTER_H

#include <QObject>
#include <QImage>
#include <QSharedMemory>

class AlSharedMemoryBroadcaster : public QObject
{
    Q_OBJECT
public:
    explicit AlSharedMemoryBroadcaster(QObject *parent = 0);

signals:

public slots:
    void slotNewFrame(QImage img);
private:
    QSharedMemory sharedMemory;
};

#endif // ALSHAREDMEMORYBROADCASTER_H