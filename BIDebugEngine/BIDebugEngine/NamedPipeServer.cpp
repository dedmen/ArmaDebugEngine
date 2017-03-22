#include "NamedPipeServer.h"
#include <windows.h>

NamedPipeServer::NamedPipeServer() {

    SECURITY_DESCRIPTOR SD;
    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SD, TRUE, NULL, FALSE);
    SECURITY_ATTRIBUTES SA;
    SA.nLength = sizeof(SA);
    SA.lpSecurityDescriptor = &SD;
    SA.bInheritHandle = TRUE;
    waitForDataEvent = CreateEvent(
        &SA,    // default security attribute 
        TRUE,    // manual-reset event 
        TRUE,    // initial state = signaled 
        NULL);   // unnamed event object 

    waitForWriteEvent = CreateEvent(
        &SA,    // default security attribute 
        TRUE,    // manual-reset event 
        TRUE,    // initial state = signaled 
        NULL);   // unnamed event object 

}


NamedPipeServer::~NamedPipeServer() {
    CloseHandle(waitForDataEvent);
    waitForDataEvent = nullptr;
}

void NamedPipeServer::open() {
    openPipe();
}

void NamedPipeServer::close() {
    closePipe();
}

void NamedPipeServer::writeMessage(std::string message) {
    DWORD cbWritten;
    OVERLAPPED pipeOverlap{ 0 };
    pipeOverlap.hEvent = waitForWriteEvent;
    auto fSuccess = WriteFile(
        pipe,                  // pipe handle 
        message.c_str(),             // message 
        static_cast<DWORD>(message.length()),              // message length 
        &cbWritten,             // bytes written 
        &pipeOverlap);                  // not overlapped 
    if (!fSuccess) {
        auto errorCode = GetLastError();
        if (errorCode == ERROR_IO_PENDING)//Handle overlapped datatransfer
        {
            DWORD waitResult = WaitForSingleObject(waitForWriteEvent, 3000);
            errorCode = GetLastError();
            if (waitResult == WAIT_TIMEOUT) {
                errorCode = WAIT_TIMEOUT;
            } else {
                return;	 //successful write. We dont need to handle any errors
            }
        }
        if (errorCode == ERROR_BROKEN_PIPE) {
            messageReadFailed();
            openPipe();
        }
    }
}

std::string NamedPipeServer::readMessageBlocking() {
    DWORD cbRead;
    OVERLAPPED pipeOverlap{ 0 };
    pipeOverlap.hEvent = waitForDataEvent;
    auto fSuccess = ReadFile(
        pipe,    // pipe handle 
        recvBuffer.data(),    // buffer to receive reply 
        static_cast<DWORD>(recvBuffer.size() * sizeof(char)),  // size of buffer 
        &cbRead,  // number of bytes read 
        &pipeOverlap);    // not overlapped 
    if (!fSuccess) {
        auto errorCode = GetLastError();
        if (errorCode == ERROR_IO_PENDING)//Handle overlapped datatransfer
        {
            DWORD waitResult = WaitForSingleObject(waitForDataEvent, 3000);
            errorCode = GetLastError();
            if (waitResult == WAIT_TIMEOUT) {
                errorCode = WAIT_TIMEOUT;
            } else {
                GetOverlappedResult(
                    pipe, // handle to pipe 
                    &pipeOverlap, // OVERLAPPED structure 
                    &cbRead,            // bytes transferred 
                    FALSE);            // do not wait
                errorCode = GetLastError();

                if (cbRead == 0)
                    return "";

                std::string output = std::string(recvBuffer.data(), cbRead);
                if (errorCode == ERROR_MORE_DATA) {

                    do {
                        fSuccess = ReadFile(
                            pipe,    // pipe handle 
                            recvBuffer.data(),    // buffer to receive reply 
                            static_cast<DWORD>(recvBuffer.size() * sizeof(char)),  // size of buffer 
                            &cbRead,  // number of bytes read 
                            &pipeOverlap);    // not overlapped
                        if (cbRead != 0)
                            output += std::string(recvBuffer.data(), cbRead);
                    } while (!fSuccess || cbRead == 0);

                }

                return output;	 //successful read. We dont need to handle any errors
            }
        }
        if (errorCode == ERROR_BROKEN_PIPE) {
            messageReadFailed();
            openPipe();
        }
    }
    return "";

}

void NamedPipeServer::transactMessage(char* output, int outputSize, const char* input) {
    //if !pipe call openPipe. If still not open throw error and tell user what failed

    DWORD written = 0;
    OVERLAPPED pipeOverlap{ 0 };
    pipeOverlap.hEvent = waitForDataEvent;
    DWORD errorCode = ERROR_SUCCESS;
    if (!TransactNamedPipe(pipe, (void*) input, static_cast<DWORD>(strlen(input)), output, outputSize, &written, &pipeOverlap)) {
        errorCode = GetLastError();
        if (errorCode == ERROR_IO_PENDING)//Handle overlapped datatransfer
        {
            DWORD waitResult = WaitForSingleObject(waitForDataEvent, 3000);
            errorCode = GetLastError();
            if (waitResult == WAIT_TIMEOUT) {
                errorCode = WAIT_TIMEOUT;
            } else {
                GetOverlappedResult(
                    pipe, // handle to pipe 
                    &pipeOverlap, // OVERLAPPED structure 
                    &written,            // bytes transferred 
                    FALSE);            // do not wait
                errorCode = GetLastError();
                if (written == 0)
                    errorCode = ERROR_NO_DATA;
                else
                    return;	 //successful read. We dont need to handle any errors
            }
        }

        //When WAIT_TIMEOUT happens teamspeak is likely unresponsive or crashed so we still reopen the pipe
        if (errorCode != ERROR_PIPE_LISTENING) openPipe();

        switch (errorCode) {
            case ERROR_NO_DATA:
            case WAIT_TIMEOUT:
                strncpy_s(output, outputSize, "Pipe was closed by TS", _TRUNCATE);
                break;
            case ERROR_PIPE_LISTENING:
                strncpy_s(output, outputSize, "Pipe not opened from TS plugin", _TRUNCATE);
                break;
            case ERROR_BAD_PIPE:
                strncpy_s(output, outputSize, "Not connected to TeamSpeak", _TRUNCATE);
                break;
            default:
                char unknownError[16];
                sprintf_s(unknownError, 16, "Pipe error %i", errorCode);
                strncpy_s(output, outputSize, unknownError, _TRUNCATE);
                break;
        }
    }

}

void NamedPipeServer::openPipe() {
    closePipe();

    SECURITY_DESCRIPTOR SD;
    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SD, TRUE, NULL, FALSE);
    SECURITY_ATTRIBUTES SA;
    SA.nLength = sizeof(SA);
    SA.lpSecurityDescriptor = &SD;
    SA.bInheritHandle = TRUE;

    pipe = CreateNamedPipe(
        L"\\\\.\\pipe\\ArmaDebugEnginePipeIface", // name of the pipe
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // send data as message
        1, // only allow 1 instance of this pipe
        0, // no outbound buffer
        0, // no inbound buffer
        0, // use default wait time
        &SA // use default security attributes
    );
}

void NamedPipeServer::closePipe() {
    if (!pipe) return;
    CloseHandle(pipe);
    pipe = nullptr;
}

void NamedPipeServer::queueRead() {
    struct overlappedCarrier {
        OVERLAPPED pipeOverlap{ 0 };
        NamedPipeServer* _this;
    };
    overlappedCarrier overlapCarry;
    //overlapCarry.pipeOverlap.hEvent = waitForDataEvent;
    overlapCarry._this = this;

    auto fSuccess = ReadFileEx(
        pipe,
        recvBuffer.data(),
        static_cast<DWORD>(recvBuffer.size() * sizeof(recvBuffer)),
        &overlapCarry.pipeOverlap,
        [](DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
        auto ob = reinterpret_cast<overlappedCarrier *>(lpOverlapped);
        ob->_this;
    });
    if (!fSuccess) {
        auto errorCode = GetLastError();
        if (errorCode == ERROR_BROKEN_PIPE) {
            openPipe();
        }
    }
    SleepEx(5000, true);


}
