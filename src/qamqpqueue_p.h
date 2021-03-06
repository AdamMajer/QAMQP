#ifndef QAMQPQUEUE_P_H
#define QAMQPQUEUE_P_H

#include <QQueue>
#include <QStringList>

#include "qamqpchannel_p.h"

namespace QAMQP
{

class QueuePrivate: public ChannelPrivate,
                    public Frame::ContentHandler,
                    public Frame::ContentBodyHandler
{
public:
    enum MethodId {
        METHOD_ID_ENUM(miDeclare, 10),
        METHOD_ID_ENUM(miBind, 20),
        METHOD_ID_ENUM(miUnbind, 50),
        METHOD_ID_ENUM(miPurge, 30),
        METHOD_ID_ENUM(miDelete, 40)
    };

    QueuePrivate(Queue *q);
    ~QueuePrivate();

    void declare();
    virtual bool _q_method(const Frame::Method &frame);

    // AMQP Queue method handlers
    void declareOk(const Frame::Method &frame);
    void deleteOk(const Frame::Method &frame);
    void purgeOk(const Frame::Method &frame);
    void bindOk(const Frame::Method &frame);
    void unbindOk(const Frame::Method &frame);
    void consumeOk(const Frame::Method &frame);

    // AMQP Basic method handlers
    virtual void _q_content(const Frame::Content &frame);
    virtual void _q_body(const Frame::ContentBody &frame);
    void deliver(const Frame::Method &frame);
    void getOk(const Frame::Method &frame);
    void cancelOk(const Frame::Method &frame);

    QString type;
    int options;
    bool delayedDeclare;
    bool declared;
    QQueue<QPair<QString, QString> > delayedBindings;

    QString consumerTag;
    bool recievingMessage;
    Message currentMessage;
    bool consuming;

    Q_DECLARE_PUBLIC(Queue)

};

} // namespace QAMQP

#endif // QAMQPQUEUE_P_H
