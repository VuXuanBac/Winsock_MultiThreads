#include "Client.h"

int main(int argc, char* argv[])
{
    int server_port;
    IP server_ip;
    int is_ok = 1;
    // Handle command line
    if (ExtractCommand(argc, argv, &server_port, &server_ip) == 0) {
        printf("[%s] %s\n", WARNING_FLAGS, _CONVERT_ARGUMENTS_FAIL);
        printf("[%s] Do you want to use default address? (y/n): ", USER_INPUT_FLAGS);
        char c;
        scanf_s("%c", &c, 1);
        if (c == 'y' || c == 'Y') {
            server_port = DEFAULT_PORT;
            is_ok = TryParseIPString(DEFAULT_IP, &server_ip);
        }
        else
            is_ok = 0;
        scanf_s("%c", &c, 1); // consume '\n'
    }

    if (is_ok && WSInitialize()) {
        SOCKET socket = CreateSocket(TCP);
        if (socket != INVALID_SOCKET) {
            SetReceiveTimeout(socket, RECEIVE_TIMEOUT_INTERVAL);

            ADDRESS server = CreateSocketAddress(server_ip, server_port);

            int try_establish = 0;
            do {
                if (EstablishConnection(socket, server)) {
                    try_establish = 0;
                    printf("[%s] Ready to communicate...\n", INFO_FLAGS);
                    PrintMenu();
                    MESSAGE request;
                    int status = 1;
                    while (status != -1) {
                        status = HandleInput(&request);
                        if (status == -1)
                            break;
                        status = Run(socket, request);
                        DestroyMessage(request);
                    }
                }
                // Handle establish fail
                else {
                    // Let user wait and try again
                    printf("[%s] Do you want to try establish the connection again? (y) or you can exit program (n). (y/n): ", USER_INPUT_FLAGS);
                    char c;
                    scanf_s("%c", &c, 1);
                    try_establish = (c == 'y' || c == 'Y');
                    scanf_s("%c", &c, 1); // consume '\n'
                }
            } while (try_establish);

        }
        CloseSocket(socket, CLOSE_SAFELY, SD_BOTH);
        WSCleanup();
    }
    printf("[%s] Stopping...\n", INFO_FLAGS);
    return 0;
}

#pragma region Socket Common

int WSInitialize()
{
    WORD version = MAKEWORD(2, 2);
    WSADATA wsa_data;
    if (WSAStartup(version, &wsa_data)) {
        printf("[%s] %s\n", ERROR_FLAGS, _INITIALIZE_FAIL);
        WSACleanup();
        return 0;
    }
    return 1;
}

int WSCleanup()
{
    return WSACleanup();
}

ADDRESS CreateSocketAddress(IP ip, int port)
{
    ADDRESS addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = ip;
    return addr;
}

SOCKET CreateSocket(int protocol)
{
    SOCKET s = INVALID_SOCKET;
    if (protocol == UDP) {
        s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    else if (protocol == TCP) {
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    if (s == INVALID_SOCKET) {
        printf("[%s] %s\n", ERROR_FLAGS, _CREATE_SOCKET_FAIL);
    }
    return s;
}

int CloseSocket(SOCKET socket, int mode, int flags)
{
    if (socket == INVALID_SOCKET)
        return 1;
    int is_ok = 1;
    if (mode == CLOSE_SAFELY) {
        if (shutdown(socket, flags) == SOCKET_ERROR) {
            is_ok = 0;
            printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SHUTDOWN_SOCKET_FAIL);
        }
    }
    if (closesocket(socket) == SOCKET_ERROR) {
        is_ok = 0;
        printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _CLOSE_SOCKET_FAIL);
    }
    return is_ok;
}

int EstablishConnection(SOCKET socket, ADDRESS address)
{
    int ret = connect(socket, (SOCKADDR*)&address, sizeof(address));
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAECONNREFUSED) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _CONNECTION_REFUSED);
        }
        else if (err == WSAEHOSTUNREACH) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _HOST_UNREACHABLE);
        }
        else if (err == WSAETIMEDOUT) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _ESTABLISH_CONNECTION_TIMEOUT);
        }
        else if (err == WSAEISCONN) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _HAS_CONNECTED);
        }
        else {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _ESTABLISH_CONNECTION_FAIL);
        }
        return 0;
    }
    return 1;
}
#pragma endregion

#pragma region Send and Receive

int Send(SOCKET sender, int bytes, const char* byte_stream)
{
    int ret = send(sender, byte_stream, bytes, 0);
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEHOSTUNREACH) {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _HOST_UNREACHABLE);
        }
        else if (err == WSAECONNABORTED || err == WSAECONNRESET) {
            printf("[%s:%d] %s\n", ERROR_FLAGS, err, _CONNECTION_DROP);
        }
        else {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _SEND_FAIL);
        }
        return -1;
    }
    else if (ret < bytes) {
        printf("[%s] %s\n", WARNING_FLAGS, _SEND_NOT_ALL);
        return 0;
    }
    return 1;
}

int SegmentationSend(SOCKET sender, const char* message, int message_len, int* obyte_sent)
{
    int start_byte = 0; // start byte in message.
    unsigned short bsend = 0; // number of bytes will send, not include header size.
    unsigned short bremain = 0; // number of bytes remain.

    char content[APPLICATION_BUFF_MAX_SIZE];
    while (start_byte < message_len) {
        // Prepare content for sending: header (number of bytes send | number of bytes remain) + body (a part of message)
        bsend = message_len - start_byte;
        if (bsend + SEGMENT_HEADER_SIZE > APPLICATION_BUFF_MAX_SIZE) {
            bsend = APPLICATION_BUFF_MAX_SIZE - SEGMENT_HEADER_SIZE;
        }
        bremain = message_len - start_byte - bsend;

        int bsend_bigendian = htons(bsend); // uniform with many architectures.
        int bremain_bigendian = htons(bremain);

        memcpy_s(content, SEGMENT_HEADER_CURRENT_SIZE, &bsend_bigendian, SEGMENT_HEADER_CURRENT_SIZE);
        memcpy_s(content + SEGMENT_HEADER_CURRENT_SIZE, SEGMENT_HEADER_REMAIN_SIZE, &bremain_bigendian, SEGMENT_HEADER_REMAIN_SIZE);
        memcpy_s(content + SEGMENT_HEADER_SIZE, bsend, message + start_byte, bsend);
        // Send
        int ret = Send(sender, bsend + SEGMENT_HEADER_SIZE, content);
        if (ret == 1) {
            start_byte += bsend;
        }
        else {
            if (obyte_sent != NULL)
                *obyte_sent = start_byte;
            return ret;
        }
    }
    if (obyte_sent != NULL)
        *obyte_sent = start_byte;
    return 1;
}

int Receive(SOCKET receiver, int length, char** obyte_stream)
{
    if (length > APPLICATION_BUFF_MAX_SIZE)
    {
        printf("[%s] %s\n", WARNING_FLAGS, _TOO_MUCH_BYTES);
        length = APPLICATION_BUFF_MAX_SIZE;
    }
    char buffer[APPLICATION_BUFF_MAX_SIZE];

    int ret = recv(receiver, buffer, length, 0);
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAECONNABORTED || err == WSAECONNRESET) {
            printf("[%s:%d] %s\n", ERROR_FLAGS, err, _CONNECTION_DROP);
        }
        else {
            printf("[%s:%d] %s\n", WARNING_FLAGS, err, _RECEIVE_FAIL);
        }
        return -1;
    }
    else if (ret == 0) {
        return -1;
    }
    else if (ret < length) {
        printf("[%s] %s\n", WARNING_FLAGS, _RECEIVE_UNEXPECTED_MESSAGE);
        *obyte_stream = NULL;
        return 0;
    }
    else {
        *obyte_stream = Clone(buffer, length);
    }
    return 1;
}

int ReceiveSegment(SOCKET receiver, char** obyte_stream, int* ostream_len, int* oremain)
{
    *obyte_stream = NULL;
    char* read;
    int remain;
    int current;
    // read number of bytes remain | number of bytes current
    int ret = Receive(receiver, SEGMENT_HEADER_SIZE, &read);
    if (ret == 1) {
        current = ntohs(*(unsigned short*)read);
        remain = ntohs(*(unsigned short*)(read + SEGMENT_HEADER_CURRENT_SIZE));

        *ostream_len = current;
        *oremain = remain;
        free(read);
    }
    else {
        *ostream_len = 0;
        *oremain = 0;
        return ret;
    }

    // read message content
    ret = Receive(receiver, current, &read);
    if (ret == 1) {
        *obyte_stream = Clone(read, current);
        free(read);
    }
    else {
        *ostream_len = 0;
        *oremain = 0;
        return ret;
    }
    return 1;
}

int SegmentationReceive(SOCKET socket, char** omessage)
{
    char* _message;
    int mlen, remain, start_byte = 0;
    int status = 1;
    *omessage = NULL;
    while (1) {
        status = ReceiveSegment(socket, &_message, &mlen, &remain);
        if (status != 1) {
            free(_message);
            return status;
        }
        if (*omessage == NULL) {
            *omessage = (char*)malloc((size_t)mlen + remain);
            if (*omessage == NULL) {
                printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
                free(_message);
                return 0;
            }
        }
        memcpy_s(*omessage + start_byte, mlen, _message, mlen);
        start_byte += mlen;
        free(_message);
        if (remain <= 0)
            break;
    }
    return status;
}

#pragma endregion

#pragma region Handle Response

int Run(SOCKET socket, MESSAGE message)
{
    int status = 0;
    if (message != NULL) {
        status = SegmentationSend(socket, message, (int)strlen(message) + 1, NULL);
        if (status == 1) {
            status = HandleResponse(socket);
        }
    }
    return status;
}

int HandleResponse(SOCKET socket)
{
    MESSAGE response = NULL;
    int status = SegmentationReceive(socket, &response);
    if(status == 1){
        PrintResponse(response, NULL);
    }
    DestroyMessage(response);
    return status;
}

void PrintResponse(const MESSAGE message, const char* title)
{
    if(title != NULL)
        printf("%s\n", title);
    if (message != NULL) {
        int len = (int)strlen(message);
        if (len >= STATUS_LENGTH) {
            int status_code = (message[0] - '0') * 10 + (message[1] - '0');
            printf("[%d]\t%s\n", status_code, message + STATUS_LENGTH);
        }
    }
}

#pragma endregion

#pragma region Utilities

void PrintMenu()
{
    printf("\t############# COMMANDS #############\n");
    printf("\t#     1. Log in with user name     #\n");
    printf("\t#     2. Post status               #\n");
    printf("\t#     3. Log out                   #\n");
    printf("\t#     4. Custom request            #\n");
    printf("\t# Other. Exit program              #\n");
    printf("\t####################################\n");
}

int HandleInput(MESSAGE* omessage)
{
    *omessage = NULL;
    char c;
    char request[USER_INPUT_MAX_SIZE];
    int status = 1;
    printf("[%s] Enter your choice: ", USER_INPUT_FLAGS);
    scanf_s("%c", &c, 1);
    if (c == '1') {
        printf("[%s] [Login] Enter your user name: ", USER_INPUT_FLAGS);
        scanf_s("%c", &c, 1); //consume \n
        gets_s(request, USER_INPUT_MAX_SIZE);
        int request_len = (int)strlen(request);
        if (request_len > 0) {
            *omessage = CreateMessage(CM_LOGIN, request);
        }
        else {
            printf("[%s] The user name can not be null.\n", WARNING_FLAGS);
            status = 0;
        }
    }
    else if (c == '2') {
        printf("[%s] [Post] Enter your article: ", USER_INPUT_FLAGS);
        scanf_s("%c", &c, 1); //consume \n
        gets_s(request, USER_INPUT_MAX_SIZE);
        *omessage = CreateMessage(CM_POST, request);
    }
    else if (c == '3') {
        scanf_s("%c", &c, 1); //consume \n
        *omessage = CreateMessage(CM_LOGOUT, NULL);
    }
    else if (c == '4') {
        printf("[%s] [Custom] Enter your request: ", USER_INPUT_FLAGS);
        scanf_s("%c", &c, 1); //consume \n
        gets_s(request, USER_INPUT_MAX_SIZE);
        *omessage = Clone(request, (int)strlen(request) + 1);
    }
    else {
        status = -1;
    }
    return status;
}

int ExtractCommand(int argc, char* argv[], int* oport, IP* oip)
{
    int is_ok = 1;
    if (argc < 3) {
        *oport = 0;
        oip = NULL;
        is_ok = 0;
    }
    else {
        is_ok = TryParseIPString(argv[1], oip);
        *oport = atoi(argv[2]);
        is_ok &= (*oport != 0);
    }
    return is_ok;
}

int SetReceiveTimeout(SOCKET socket, int interval)
{
    int _interval = interval;
    int ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&_interval, sizeof(_interval));
    if (ret == SOCKET_ERROR) {
        printf("[%s:%d] %s\n", WARNING_FLAGS, WSAGetLastError(), _SET_TIMEOUT_FAIL);
        return 0;
    }
    return 1;
}

int TryParseIPString(const char* str, IP* oip)
{
    return inet_pton(AF_INET, str, oip) == 1;
}

char* Clone(const char* source, int length, int start)
{
    char* _clone = (char*)malloc((size_t)length + start);

    if (_clone == NULL)
        printf("[%s] %s\n", WARNING_FLAGS, _ALLOCATE_MEMORY_FAIL);
    else
        memcpy_s(_clone + start, length, source, length);
    return _clone;
}

MESSAGE CreateMessage(const char* command, const char* arguments)
{
    if (command == NULL)
        return NULL;
    int command_len = (int)strlen(command) + 1;
    if (command_len == 1)
        return NULL;
    MESSAGE m;
    if (arguments == NULL) {
        m = Clone("", 1, command_len);
    }
    else {
        m = Clone(arguments, (int)strlen(arguments) + 1, command_len);
    }
    if (m != NULL) {
        memcpy_s(m, command_len, command, command_len);
        m[command_len - 1] = ' ';
    }
    return m;
}

void DestroyMessage(MESSAGE m)
{
    free(m);
}
#pragma endregion

