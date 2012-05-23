#include "amqp_network.h"

#include <QDebug>

QAMQP::Network::Network( QObject * parent /*= 0*/ ):QObject(parent)
{
	qRegisterMetaType<QAMQP::Frame::Method>("QAMQP::Frame::Method");

	
	buffer_ = new QBuffer(this);
	offsetBuf = 0;
	leftSize = 0;

	buffer_->open(QIODevice::ReadWrite);

	initSocket(false);
}

QAMQP::Network::~Network()
{
	disconnect();
}

void QAMQP::Network::connectTo( const QString & host, quint32 port )
{
	if (isSsl())
	{
		static_cast<QSslSocket *>(socket_.data())->connectToHostEncrypted(host, port);
	} else {
		socket_->connectToHost(host, port);
	}
	
}

void QAMQP::Network::disconnect()
{
	if(socket_)
		socket_->abort();
}

void QAMQP::Network::connected()
{
	if(isSsl() && !static_cast<QSslSocket *>(socket_.data())->isEncrypted() )
	{	
		qDebug() << "[SSL] start encrypt";
		static_cast<QSslSocket *>(socket_.data())->startClientEncryption();
	} else {
		conectionReady();
	}
	
}

void QAMQP::Network::disconnected()
{

}

void QAMQP::Network::error( QAbstractSocket::SocketError socketError )
{
	Q_UNUSED(socketError);
}

void QAMQP::Network::readyRead()
{
	QDataStream streamA(socket_);
	QDataStream streamB(buffer_);
	
	/*
	�������� ���������, ��������� � �����
	�������� ���� �����, ���� ����� ������� �� ������ �� ������ ���
	*/
	while(!socket_->atEnd())
	{
		if(leftSize == 0) // ���� ����� �������� ��� ���� �����, �� ������ ��������� ������
		{
			lastType_  = 0;
			qint16 channel_  = 0;
			leftSize  = 0;
			offsetBuf = 0;

			streamA >> lastType_;
			streamB << lastType_;
			streamA >> channel_;
			streamB << channel_;
			streamA >> leftSize;
			streamB << leftSize;
			leftSize++; // �������� ������ �� 1, ��� ������� ����� ������
		}

		QByteArray data_;
		data_.resize(leftSize);
		offsetBuf = streamA.readRawData(data_.data(), data_.size());
		leftSize -= offsetBuf;
		streamB.writeRawData(data_.data(), offsetBuf);
		if(leftSize == 0)
		{		
			buffer_->reset();
			switch(QAMQP::Frame::Type(lastType_))
			{
			case QAMQP::Frame::ftMethod:
				{
					QAMQP::Frame::Method frame(streamB);
					emit method(frame);
				}
				break;
			case QAMQP::Frame::ftHeader:
				{
					QAMQP::Frame::Content frame(streamB);
					emit content(frame);
				}
				break;
			case QAMQP::Frame::ftBody:
				{
					QAMQP::Frame::ContentBody frame(streamB);
					emit body(frame.channel(), frame.body());
				}
				break;
			default:
				qWarning("Unknown frame type");
			}
			buffer_->reset();
		}
	}
}

void QAMQP::Network::sendFrame( const QAMQP::Frame::Base & frame )
{
	QDataStream stream(socket_);
	frame.toStream(stream);
}

bool QAMQP::Network::isSsl() const
{
	return QString(socket_->metaObject()->className()).compare( "QSslSocket", Qt::CaseInsensitive) == 0;
}

void QAMQP::Network::setSsl( bool value )
{
	initSocket(value);
}

void QAMQP::Network::initSocket( bool ssl /*= false*/ )
{
	if(socket_)
		delete socket_;

	if(ssl)
	{		
		socket_ = new QSslSocket(this);
		QSslSocket * ssl_= static_cast<QSslSocket*> (socket_.data());
		ssl_->setProtocol(QSsl::AnyProtocol);
		connect(socket_, SIGNAL(sslErrors(const QList<QSslError> &)),
			this, SLOT(sslErrors(const QList<QSslError> &)));	

		//connect(socket_, SIGNAL(encrypted()), this, SLOT(conectionReady()));
		connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
	} else {
		socket_ = new QTcpSocket(this);		
		connect(socket_, SIGNAL(connected()), this, SLOT(conectionReady()));
	}
	
	connect(socket_, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket_, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));	
}

void QAMQP::Network::sslErrors( const QList<QSslError> & errors )
{
	static_cast<QSslSocket*>(socket_.data())->ignoreSslErrors();
}

void QAMQP::Network::conectionReady()
{
	char header_[8] = {'A', 'M', 'Q', 'P', 0,0,9,1};
	socket_->write(header_, 8);
}