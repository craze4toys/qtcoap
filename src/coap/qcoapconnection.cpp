/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCoap module.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcoapconnection_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCoapConnection, "qt.coap.connection")

/*!
    \class QCoapConnection
    \inmodule QtCoap

    \brief The QCoapConnection class defines an interface for
    handling transfers of frames to a server.

    It isolates CoAP clients from the transport in use, so that any
    client can be used with any supported transport.
*/

/*!
    \enum QCoapConnection::ConnectionState

    This enum specifies the state of the underlying transport.

    \value Unconnected      The underlying transport is not yet ready for data transmission.

    \value Bound            The underlying transport is ready for data transmission. For example,
                            if QUdpSocket is used for the transport, this corresponds to
                            QAbstractSocket::BoundState.
    \sa state(), bound()
*/

/*!
    \fn void QCoapConnection::error(QAbstractSocket::SocketError error)

    This signal is emitted when a connection error occurs. The \a error
    parameter describes the type of error that occurred.
*/

/*!
    \fn void QCoapConnection::readyRead(const QByteArray &data, const QHostAddress &sender)

    This signal is emitted when a network reply is available. The \a data
    parameter supplies the received data, and the \a sender parameter supplies
    the sender address.
*/

/*!
    \fn void QCoapConnection::bound()

    This signal is emitted when the underlying transport is ready for data transmission.
    Derived implementations must emit this signal whenever they are ready to start
    transferring data frames to and from the server.

    \sa bind()
*/

/*!
    \fn void QCoapConnection::securityConfigurationChanged()

    This signal is emitted when the security configuration is changed.
*/

/*!
    \fn void QCoapConnection::bind(const QString &host, quint16 port)

    Prepares the underlying transport for data transmission to to the given \a host
    address on \a port. Emits the bound() signal when the transport is ready.

    This is a pure virtual method.

    \sa bound()
*/

/*!
    \fn void QCoapConnection::writeData(const QByteArray &data, const QString &host, quint16 port)

    Sends the given \a data frame to the host address \a host at port \a port.

    This is a pure virtual method.
*/

QCoapConnectionPrivate::QCoapConnectionPrivate(QtCoap::SecurityMode security)
    : securityMode(security)
    , state(QCoapConnection::Unconnected)
{}

/*!
    Constructs a new CoAP connection for the given \a securityMode and
    sets \a parent as its parent.
*/
QCoapConnection::QCoapConnection(QtCoap::SecurityMode securityMode, QObject *parent)
    : QCoapConnection(*new QCoapConnectionPrivate(securityMode), parent)
{
}

/*!
    \internal

    Constructs a new new CoAP connection as a child of \a parent, with \a dd
    as its \c d_ptr. This constructor must be used when internally subclassing
    the QCoapConnection class.
*/
QCoapConnection::QCoapConnection(QObjectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    Releases any resources held by QCoapConnection.
*/
QCoapConnection::~QCoapConnection()
{
}

/*!
    \internal

    Prepares the underlying transport for data transmission and sends the given
    \a request frame to the given \a host at the given \a port when the transport
    is ready.

    The preparation of the transport is done by calling the pure virtual bind() method,
    which needs to be implemented by derived classes.
*/
void
QCoapConnectionPrivate::sendRequest(const QByteArray &request, const QString &host, quint16 port)
{
    Q_Q(QCoapConnection);

    CoapFrame frame(request, host, port);
    framesToSend.enqueue(frame);

    if (state == QCoapConnection::ConnectionState::Unconnected) {
        q->connect(q, &QCoapConnection::bound, q,
                [this, q]() {
                    state = QCoapConnection::ConnectionState::Bound;
                    q->startToSendRequest();
                });
        q->bind(host, port);
    } else {
        q->startToSendRequest();
    }
}

/*!
    Returns \c true if security is used, returns \c false otherwise.
*/
bool QCoapConnection::isSecure() const
{
    Q_D(const QCoapConnection);
    return d->securityMode != QtCoap::NoSec;
}

/*!
    Returns the security mode.
*/
QtCoap::SecurityMode QCoapConnection::securityMode() const
{
    Q_D(const QCoapConnection);
    return d->securityMode;
}

/*!
    Returns the connection state.
*/
QCoapConnection::ConnectionState QCoapConnection::state() const
{
    Q_D(const QCoapConnection);
    return d->state;
}

/*!
    \internal

    Sends the last stored frame to the server by calling the pure virtual
    writeData() method.
*/
void QCoapConnection::startToSendRequest()
{
    Q_D(QCoapConnection);

    Q_ASSERT(!d->framesToSend.isEmpty());
    const CoapFrame frame = d->framesToSend.dequeue();
    writeData(frame.currentPdu, frame.host, frame.port);
}

/*!
    Sets the security configuration parameters from the \a configuration.
    The security configuration will be ignored if the QtCoap::NoSec mode is used
    for connection.

    \note This method must be called before the handshake starts.
*/
void QCoapConnection::setSecurityConfiguration(const QCoapSecurityConfiguration &configuration)
{
    Q_D(QCoapConnection);

    if (isSecure()) {
        d->securityConfiguration = configuration;
        emit securityConfigurationChanged();
    } else {
        qCWarning(lcCoapConnection, "Security is disabled, security configuration will be ignored.");
    }
}

/*!
    Returns the security configuration.
*/
QCoapSecurityConfiguration QCoapConnection::securityConfiguration() const
{
    Q_D(const QCoapConnection);
    return d->securityConfiguration;
}

QT_END_NAMESPACE
