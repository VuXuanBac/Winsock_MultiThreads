#pragma once

#pragma comment(lib, "Ws2_32.lib")

#pragma region Header Declarations

#include <stdio.h>
#include <stdlib.h>

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma endregion

#pragma region Constants Definitions

#define RECEIVE_TIMEOUT_INTERVAL 10000
#define APPLICATION_BUFF_MAX_SIZE 1024
#define USER_INPUT_MAX_SIZE 1023
#define MESSAGE_MAX_SIZE 1023

#define DEFAULT_PORT 5555
#define DEFAULT_IP "127.0.0.1"

#define UDP 0
#define TCP 1

#define CLOSE_NORMAL 0
#define CLOSE_SAFELY 1

#define SEGMENT_HEADER_REMAIN_SIZE 2
#define SEGMENT_HEADER_CURRENT_SIZE 2
#define SEGMENT_HEADER_SIZE 4

#define C_LOGIN 1
#define C_POST 2
#define C_LOGOUT 3

#define CM_LOGIN "USER"
#define CM_POST "POST"
#define CM_LOGOUT "BYE"

#define COMMAND_LENGTH 5
#define STATUS_LENGTH 2
#pragma endregion

#pragma region Type Definitions

#define ADDRESS SOCKADDR_IN
#define IP IN_ADDR
#define MESSAGE char*

#pragma endregion

#pragma region Error Debugging

#define INFO_FLAGS "INF"
#define ERROR_FLAGS "ERR"
#define WARNING_FLAGS "WAR"
#define USER_INPUT_FLAGS ">>"

#define _NOT_SPECIFY_PORT "Port number is not specified. Default port used!"
#define _CONVERT_PORT_FAIL "Fail to convert port number from command-line. Default port used!"
#define _CONVERT_ARGUMENTS_FAIL "Fail to extract port number and ip address from command-line arguments."

#define _ALLOCATE_MEMORY_FAIL "Fail to allocate memory."

#define _INITIALIZE_FAIL "Fail to initialize Winsock 2.2!"
#define _BIND_SOCKET_FAIL "Fail to bind socket with the address."
#define _CREATE_SOCKET_FAIL "Fail to create a socket."
#define _SHUTDOWN_SOCKET_FAIL "Fail to shutdown the socket."
#define _CLOSE_SOCKET_FAIL "Fail to close the socket."
#define _SET_TIMEOUT_FAIL "Fail to set receive timeout for socket."
#define _RECEIVE_FAIL "Fail to receive message from remote process."
#define _SEND_FAIL "Fail to send message to the remote process."
#define _LISTEN_SOCKET_FAIL "Fail to set socket to listen state."
#define _ACCEPT_SOCKET_FAIL "Fail to accept a connection with the socket."

#define _TRANSLATE_DOMAIN_FAIL "Fail to translate the domain name."
#define _TRANSLATE_IP_FAIL "Fail to translate the IP address."

#define _ADDRESS_IN_USE "Address in Use. \"Another process already bound to the address,\""
#define _BOUNDED_SOCKET "Invalid Socket. \"The socket already bound to another address.\""
#define _HOST_UNREACHABLE "Host Unreachable. \"The remote process is unreachable at this time.\""
#define _MESSAGE_TOO_LARGE "Message Too Large. \"The buffer size is not large enough! Some data from remote process lost,\""
#define _MESSAGE_EXTREME_LARGE "Message Too Large. \"The message size is larger than the maximum supported by the underlying transport.\""
#define _SEND_NOT_ALL "Not all bytes was sent."

#define _RECEIVE_UNEXPECTED_MESSAGE "Receive an invalid message."
#define _CONNECTION_DROP "Connection to the remote process has been drop."
#define _TOO_MUCH_BYTES "The number of bytes read is higher than the application buffer size."
#define _NOT_BOUND_SOCKET "Invalid Socket. \"The socket need to be bound to an address.\""
#define _NOT_LISTEN_SOCKET "Invalid Socket. \"The socket need to be set to listen state.\""
#define _REACH_SOCKETS_LIMIT "Too many open sockets"

#define _CONNECTION_REFUSED "Connection refused. \"Remote process refused to establish connection. Try again later.\""
#define _ESTABLISH_CONNECTION_TIMEOUT "Establish connection to remote process timeout. No connection established."
#define _HAS_CONNECTED "There is another connection established before on this socket."
#define _ESTABLISH_CONNECTION_FAIL "Fail to establish connection to the address."
#define _CONNECTION_DROP "Connection to the remote process has been drop."

#define _TOO_MANY_THREADS "Too many threads are running. Can not create one more thread."
#define _INSUFFICIENT_RESOURCES "Insufficient resources for creating one more thread."
#pragma endregion

#pragma region MyRegion

/// <summary>
/// Initialize Winsock 2.2
/// </summary>
/// <returns>1 if initialize successfully, 0 otherwise</returns>
int WSInitialize();

/// <summary>
/// Clean up Winsock 2.2
/// </summary>
/// <returns>1 if close successfully, 0 otherwise</returns>
int WSCleanup();

/// <summary>
/// Create a TCP/UDP Socket.
/// </summary>
/// <param name="protocol">TCP or UDP</param>
/// <returns>
/// Created TCP/UDP Socket.
/// INVALID_SOCKET if protocol is unexpected or have error on Winsock
/// </returns>
SOCKET CreateSocket(int protocol);

/// <summary>
/// Close a created Socket
/// </summary>
/// <param name="socket">The socket want to close</param>
/// <param name="mode">CLOSE_NORMAL or CLOSE_SAFELY (Shutdown before close)</param>
/// <param name="flags">Specify how to close socket safely. Some flags: SD_RECEIVE, SD_SEND, SD_BOTH (Manifest constants for shutdown()). This param is not used with CLOSE_NORMAL </param>
/// <returns>1 if close successfully, 0 otherwise</returns>
int CloseSocket(SOCKET socket, int mode, int flags = 0);

/// <summary>
/// Create a socket address that used IPv4 and Port Number
/// </summary>
/// <param name="ip">The IPv4 Address</param>
/// <param name="port">The Port number</param>
/// <returns>Created socket address</returns>
ADDRESS CreateSocketAddress(IP ip, int port);

/// <summary>
/// Write a byte stream to the connected socket buffer and send
/// </summary>
/// <param name="sender">The connected socket that is used for sending byte stream</param>
/// <param name="bytes">Number of bytes expected to send</param>
/// <param name="byte_stream">The byte stream want to send</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected. -1 if have errors that the socket should be closed</returns>
int Send(SOCKET sender, int bytes, const char* byte_stream);

/// <summary>
/// Segmentation a message into pieces/segment and Send them with a connected socket.
/// Each piece attached with the header consists of SEGMENT_HEADER_CURRENT_SIZE first bytes
/// is the length of message in the piece (not include header size) and SEGMENT_HEADER_REMAIN_SIZE next bytes
/// is the number of bytes on message that has not been sent.
/// </summary>
/// <param name="sender">The connected socket used for sending</param>
/// <param name="message">The message want to segmentation and send</param>
/// <param name="message_len">The length of the message</param>
/// <param name="obyte_sent">[Output] Number of bytes sent successfully</param>
/// <returns>1 if success. 0 if number of bytes sent less than expected. -1 if have errors that the socket should be closed</returns>
int SegmentationSend(SOCKET sender, const char* message, int message_len, int* obyte_sent);

/// <summary>
/// Read a byte stream from a connected socket 
/// </summary>
/// <param name="receiver">The connected socket that is used for receiving bytes stream</param>
/// <param name="bytes">Number of bytes want to read</param>
/// <param name="obyte_stream">[Output] The byte streams read</param>
/// <returns>1 if read successfully. 0 if cant read fully. -1 if have errors that the socket should be closed</returns>
int Receive(SOCKET receiver, int bytes, char** obyte_stream);

/// <summary>
/// Read and Extract one part of/a segment of a message from a connected socket.
/// </summary>
/// <param name="receiver">The connected socket that is used for receiving byte streams</param>
/// <param name="obyte_stream">[Output] The extracted segment, after removing SEGMENT_HEADER_SIZE first bytes from byte stream</param>
/// <param name="ostream_len">[Output] The segment size, in bytes</param>
/// <param name="oremain">[Output] Number of bytes in source message that have not been received</param>
/// <returns>1 if success. 0 if cant read fully. -1 if have errors that the socket should be closed</returns>
int ReceiveSegment(SOCKET receiver, char** obyte_stream, int* ostream_len, int* oremain);

/// <summary>
/// Read segments from a connected socket and Merge them into a complete message.
/// </summary>
/// <param name="socket">The connected socket used to receive segments</param>
/// <param name="omessage">[Output] The merged message</param>
/// <returns>1 if success. 0 if cant read fully. -1 if have errors that the socket should be closed</returns>
int SegmentationReceive(SOCKET socket, char** omessage);

/// <summary>
/// Create a new memory space and Copy [length] bytes from [source] to it.
/// </summary>
/// <param name="source">The source bytes</param>
/// <param name="length">Number of bytes want to copy</param>
/// <param name="start">The first byte in destination will hold the 0th byte of source</param>
/// <returns>New memory space contains content of source. NULL if fail to allocate memory</returns>
char* Clone(const char* source, int length, int start = 0);

#pragma endregion
