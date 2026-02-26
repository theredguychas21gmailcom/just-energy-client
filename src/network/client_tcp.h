/**
 * @file client_tcp.h
 * @brief TCP client interface for network communication
 *
 * This header provides a simple and portable TCP client implementation with
 * support for connection timeouts, reliable data transmission, and proper
 * resource management. The implementation supports both IPv4 and IPv6.
 */
#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H

/**
 * @struct ClientTCP
 * @brief TCP client connection structure
 *
 * Opaque structure that holds the TCP client connection state.
 * Users should not directly access the fields of this structure.
 */
#include <stddef.h>
#include <sys/types.h>

typedef enum {
    TCP_STATE_DISCONNECTED,
    TCP_STATE_CONNECTING,
    TCP_STATE_CONNECTED,
    TCP_STATE_ERROR
} TCPState;

typedef struct {
    int fd; /**< Socket file descriptor (-1 when not connected) */
    TCPState state; 
    char* write_buf;
    size_t write_len;
    char* read_buf;
    size_t read_len;
} ClientTCP;

/**
 * @brief Creates a new TCP client instance
 *
 * Allocates and initializes a new ClientTCP structure with default values.
 * The socket file descriptor is initialized to -1 (not connected).
 * The returned client must be destroyed with client_tcp_destroy() when
 * no longer needed.
 *
 * @return Pointer to the newly created ClientTCP structure, or NULL if
 *         memory allocation fails
 *
 * @see client_tcp_destroy()
 *
 * @par Example:
 * @code
 * ClientTCP *client = client_tcp_create();
 * if (!client) {
 *     fprintf(stderr, "Failed to create TCP client\n");
 *     return -1;
 * }
 * @endcode
 */
ClientTCP* client_tcp_create();

/**
 * @brief Destroys a TCP client instance and frees all resources
 *
 * Closes the connection if it's still open and frees the memory allocated
 * for the ClientTCP structure. Safe to call with NULL pointer. After calling
 * this function, the pointer should not be used anymore.
 *
 * @param tcp Pointer to the ClientTCP structure to destroy (can be NULL)
 *
 * @see client_tcp_create()
 *
 * @par Example:
 * @code
 * ClientTCP *client = client_tcp_create();
 * // ... use client ...
 * client_tcp_destroy(client);
 * @endcode
 */
void client_tcp_destroy(ClientTCP* tcp);
/**
 * @brief Establishes a TCP connection to a remote host
 *
 * Connects to the specified host and port with a configurable timeout.
 * The function uses non-blocking connection with select() to implement
 * the timeout mechanism, then switches back to blocking mode after
 * successful connection. Supports both IPv4 and IPv6 addresses.
 *
 * The function attempts to resolve the hostname and tries each resolved
 * address until one succeeds or all fail.
 *
 * @param tcp Pointer to the ClientTCP structure
 * @param host The hostname or IP address to connect to (e.g., "example.com"
 *             or "192.168.1.1")
 * @param port The port number to connect to (1-65535)
 * @param timeout_ms Connection timeout in milliseconds
 *
 * @return 0 on success, -1 on failure
 * @retval 0 Connection established successfully
 * @retval -1 Failed to connect (check errno for details)
 *
 * @note The function will fail if:
 *       - Invalid parameters are provided (tcp is NULL, host is NULL)
 *       - The client is already connected (tcp->fd >= 0)
 *       - DNS resolution fails
 *       - Connection times out or is refused
 *
 * @see client_tcp_close()
 *
 * @par Example:
 * @code
 * ClientTCP *client = client_tcp_create();
 * if (client_tcp_connect(client, "example.com", 80, 5000) == 0) {
 *     printf("Connected successfully\n");
 * } else {
 *     fprintf(stderr, "Connection failed\n");
 * }
 * @endcode
 */
int client_tcp_connect_async(ClientTCP* tcp, const char* host, int port,
                       int timeout_ms);

/**
 * @brief Sends data over the TCP connection
 *
 * Sends the specified data buffer over the established TCP connection.
 * The function ensures all data is sent by looping until the entire buffer
 * is transmitted. It handles EINTR (interrupted system call) gracefully
 * by retrying the operation.
 *
 * @param tcp Pointer to the ClientTCP structure
 * @param data Pointer to the data buffer to send
 * @param len Length of the data buffer in bytes
 *
 * @return 0 on success, -1 on failure
 * @retval 0 All data sent successfully
 * @retval -1 Failed to send data (check errno for details)
 *
 * @note The function will fail if:
 *       - Invalid parameters are provided (tcp is NULL, data is NULL)
 *       - The client is not connected (tcp->fd < 0)
 *       - A network error occurs during transmission
 *
 * @warning This function blocks until all data is sent or an error occurs.
 *
 * @see client_tcp_recv()
 *
 * @par Example:
 * @code
 * const char *message = "GET / HTTP/1.1\r\n\r\n";
 * if (client_tcp_send(client, message, strlen(message)) == 0) {
 *     printf("Data sent successfully\n");
 * } else {
 *     fprintf(stderr, "Failed to send data\n");
 * }
 * @endcode
 */
ssize_t client_tcp_send_async(ClientTCP* tcp, const void* data, size_t len);

/**
 * @brief Receives data from the TCP connection with timeout
 *
 * Receives data from the established TCP connection into the provided buffer.
 * Uses select() to implement a timeout mechanism. The function will block
 * until data is available, the timeout expires, or an error occurs.
 *
 * @param tcp Pointer to the ClientTCP structure
 * @param buffer Pointer to the buffer where received data will be stored
 * @param len Maximum number of bytes to receive
 * @param timeout_ms Receive timeout in milliseconds
 *
 * @return Number of bytes received on success, -1 on failure
 * @retval >0 Number of bytes received (can be less than len)
 * @retval -1 Failed to receive data (check errno for details)
 *
 * @note The function will fail if:
 *       - Invalid parameters are provided (tcp is NULL, buffer is NULL)
 *       - The client is not connected (tcp->fd < 0)
 *       - The timeout expires (errno set to ETIMEDOUT)
 *       - A network error occurs
 *
 * @note The number of bytes received may be less than the requested length.
 *       This is normal TCP behavior and the caller should handle partial reads.
 *
 * @warning This function blocks until data is received or timeout occurs.
 *
 * @see client_tcp_send()
 *
 * @par Example:
 * @code
 * char buffer[1024];
 * int received = client_tcp_recv(client, buffer, sizeof(buffer), 5000);
 * if (received > 0) {
 *     printf("Received %d bytes\n", received);
 * } else if (errno == ETIMEDOUT) {
 *     fprintf(stderr, "Timeout\n");
 * } else {
 *     fprintf(stderr, "Receive error\n");
 * }
 * @endcode
 */
int client_tcp_recv(ClientTCP* tcp, void* buffer, size_t len, int timeout_ms);

/**
 * @brief Closes the TCP connection
 *
 * Closes the socket file descriptor and resets it to -1. Safe to call
 * multiple times or with NULL pointer. Does not free the ClientTCP
 * structure itself - use client_tcp_destroy() for complete cleanup.
 *
 * @param tcp Pointer to the ClientTCP structure (can be NULL)
 *
 * @note After calling this function, the client can be reconnected using
 *       client_tcp_connect().
 *
 * @see client_tcp_connect(), client_tcp_destroy()
 *
 * @par Example:
 * @code
 * client_tcp_close(client);
 * // Client can be reconnected now
 * client_tcp_connect(client, "example.com", 80, 5000);
 * @endcode
 */
void client_tcp_close(ClientTCP* tcp);

#endif
