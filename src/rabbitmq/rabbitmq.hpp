#pragma once

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

namespace nutc {
namespace rabbitmq {
bool
connectToRabbitMQ(
    amqp_connection_state_t& conn, const std::string& hostname, int port,
    const std::string& username, const std::string& password
)
{
    conn = amqp_new_connection();
    amqp_socket_t* socket = amqp_tcp_socket_new(conn);

    if (!socket) {
        std::cerr << "Cannot create TCP socket" << std::endl;
        return false;
    }

    int status = amqp_socket_open(socket, hostname.c_str(), port);
    if (status) {
        std::cerr << "Cannot open socket" << std::endl;
        return false;
    }

    amqp_rpc_reply_t reply = amqp_login(
        conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username.c_str(),
        password.c_str()
    );
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Login failed" << std::endl;
        return false;
    }

    return true;
}

bool
publishMessage(
    amqp_connection_state_t& conn, const std::string& queueName,
    const std::string& message
)
{
    amqp_queue_declare(
        conn, 1, amqp_cstring_bytes(queueName.c_str()), 0, 0, 0, 1, amqp_empty_table
    );

    amqp_rpc_reply_t res = amqp_get_rpc_reply(conn);
    if (res.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to declare queue." << std::endl;
        return false;
    }

    amqp_basic_publish(
        conn, 1, amqp_cstring_bytes(""), amqp_cstring_bytes(queueName.c_str()), 0, 0,
        NULL, amqp_cstring_bytes(message.c_str())
    );

    res = amqp_get_rpc_reply(conn);
    if (res.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to publish message." << std::endl;
        return false;
    }

    return true;
}

// Blocking
std::string
consumeMessage(amqp_connection_state_t& conn, const std::string& queueName)
{
    amqp_basic_consume(
        conn, 1, amqp_cstring_bytes(queueName.c_str()), amqp_empty_bytes, 0, 1, 0,
        amqp_empty_table
    );

    amqp_rpc_reply_t res = amqp_get_rpc_reply(conn);
    if (res.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to consume message." << std::endl;
        return "";
    }

    amqp_envelope_t envelope;
    amqp_maybe_release_buffers(conn);
    res = amqp_consume_message(conn, &envelope, NULL, 0);

    if (res.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to retrieve message." << std::endl;
        return "";
    }

    std::string message(
        reinterpret_cast<char*>(envelope.message.body.bytes), envelope.message.body.len
    );
    amqp_destroy_envelope(&envelope);
    return message;
}

bool
initializeConnection(amqp_connection_state_t& conn)
{
    if (!connectToRabbitMQ(conn, "localhost", 5672, "guest", "guest")) {
        return false;
    }
    amqp_channel_open(conn, 1);
    amqp_rpc_reply_t res = amqp_get_rpc_reply(conn);
    if (res.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to open channel." << std::endl;
        return "";
    }
}

void
closeConnection(amqp_connection_state_t& conn)
{
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
}

} // namespace rabbitmq
} // namespace nutc
