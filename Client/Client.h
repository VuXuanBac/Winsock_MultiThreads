#pragma once

#pragma region Header Declarations

#include "CommonHeader.h"

#pragma endregion

#pragma region Constants Definitions

#define OUTPUT_FLAGS "**"
#pragma endregion

#pragma region Function Declarations

/// <summary>
/// Establish a connection to a specified socket address
/// </summary>
/// <param name="socket">The socket want to connect</param>
/// <param name="address">The socket address of a remote process want to connect to</param>
/// <returns>1 if success. 0 otherwise, has errors</returns>
int EstablishConnection(SOCKET socket, ADDRESS address);

/// <summary>
/// Send request to server and Handle response
/// </summary>
/// <param name="socket">The connected socket to server</param>
/// <param name="request">The request want to send</param>
/// <returns>1 if success. 0 if have some errors while sending or receiving. -1 if have errors that the socket should be closed</returns>
int Run(SOCKET socket, MESSAGE request);

/// <summary>
/// Handle the response from remote process: Collect message segmentations, Merge them and Print to console
/// </summary>
/// <param name="socket">The connected socket used to communicate with remote process</param>
/// <returns>1 if success. 0 if cant read response fully. -1 if have errors that the socket should be closed</returns>
int HandleResponse(SOCKET socket);

/// <summary>
/// Extract infomation in Message object and Print the message to console.
/// </summary>
/// <param name="message">The response Message object</param>
/// <param name="title">The title for message, which is shown previous. NULL for not use</param>
void PrintResponse(const MESSAGE message, const char* title = NULL);

/// <summary>
/// Print menu to console. The menu is shown once for each window after the connection established .
/// </summary>
void PrintMenu();

/// <summary>
/// Get user command and create a Message from the result.
/// </summary>
/// <param name="omessage">[Output] The created message</param>
/// <returns>1 if success. 0 if some user input is invalid. -1 if user choose a unsupported function</returns>
int HandleInput(MESSAGE* omessage);

/// <summary>
/// Extract port number and ipv4 string from command-line arguments.
/// If has error, set oport = 0 and oip = NULL.
/// </summary>
/// <param name="argc">Number of Arguments [From main()]</param>
/// <param name="argv">Arguments value [From main()]</param>
/// <param name="oport">[Output] The extracted port number</param>
/// <param name="oip">[Output] The extracted ip address</param>
/// <returns>1 if extract successfully. 0 otherwise, has error</returns>
int ExtractCommand(int argc, char* argv[], int* oport, IP* oip);

/// <summary>
/// Try parse a string to a IPv4 Address
/// </summary>
/// <param name="str">The string want to parse</param>
/// <param name="oip">[Output] The result IPv4 Address</param>
/// <returns>1 if parse successfully. 0 otherwise</returns>
int TryParseIPString(const char* str, IP* oip);

/// <summary>
/// Set receive timeout interval for socket.
/// </summary>
/// <param name="socket">The socket want to set timeout</param>
/// <param name="interval">The timeout interval</param>
/// <returns>1 if set successfully, 0 otherwise</returns>
int SetReceiveTimeout(SOCKET socket, int interval);

/// <summary>
/// Create a Message object that contains a command.
/// The first COMMAND_LENGTH byte of Message is the command text: See CM_ for some command texts
/// </summary>
/// <param name="command">The command text</param>
/// <param name="arguments">The command arguments. Use NULL if have no arguments</param>
/// <returns>A command message. NULL if memory allocation fail</returns>
MESSAGE CreateMessage(const char* command, const char* arguments);

/// <summary>
/// Free memory for Message object
/// </summary>
/// <param name="m">The message want to free</param>
void DestroyMessage(MESSAGE m);
#pragma endregion