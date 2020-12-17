#include "microBox.h"

#include "port_handler.h"
#include <printf/printf.h>
#undef printf

void MicroBox::begin(const char* hostName, PortHandler* portHandler, bool showPrompt, bool localEcho)
{
    this->portHandler = portHandler;
    this->localEcho = localEcho;
    this->hostName = hostName;

    commands[0].commandName = "help";
    commands[0].commandDescription = "Prints help.\n\r";
    commands[0].commandFunction = std::bind(&MicroBox::showHelp, this, std::placeholders::_1, std::placeholders::_2);

    if (showPrompt) {
        this->showPrompt();
    }
}

bool MicroBox::addCommand(const char* commandName, callback_t commandFunction, const char* commandDescription)
{
    uint8_t index = 0;

    while ((commands[index].commandFunction != nullptr) && (index < (MAX_COMMAND_NUMBER - 1))) {
        index++;
    }
    if (index < (MAX_COMMAND_NUMBER - 1)) {
        commands[index].commandName = commandName;
        commands[index].commandDescription = commandDescription;
        commands[index].commandFunction = commandFunction;
        index++;
        commands[index].commandName = nullptr;
        commands[index].commandDescription = nullptr;
        commands[index].commandFunction = nullptr;
        return true;
    }
    return false;
}

// from https://playground.arduino.cc/Main/Printf/
void MicroBox::printf(const char* format, ...)
{
    char buf[PRINTF_BUFFER_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    for (char* p = &buf[0]; *p; p++) // emulate cooked mode for newlines
    {
        if (*p == '\n')
            portHandler->write('\r');
        portHandler->write(*p);
    }
    va_end(ap);
}

void MicroBox::showPrompt()
{
    printf("%s> ", hostName);
}

uint8_t MicroBox::parseCommandParameters(char* pParam)
{
    uint8_t index = 0;

    parameterPointer[index] = pParam;
    if (pParam != nullptr) {
        index++;
        while ((pParam = strchr(pParam, ' ')) != nullptr) {
            pParam[0] = 0;
            pParam++;
            parameterPointer[index++] = pParam;
        }
    }
    return index;
}

void MicroBox::executeCommand()
{
    bool found = false;
    printf("\n\r");
    if (bufferPosition > 0) {
        uint8_t i = 0;
        uint8_t dstlen;
        uint8_t srclen;
        char* pParam;

        commandBuffer[bufferPosition] = 0;
        pParam = strchr(commandBuffer, ' ');
        if (pParam != nullptr) {
            pParam++;
            srclen = pParam - commandBuffer - 1;
        } else
            srclen = bufferPosition;

        addToHistory(commandBuffer);
        historyCursorPosition = -1;

        while (commands[i].commandName != nullptr && found == false) {
            dstlen = strlen(commands[i].commandName);
            if (dstlen == srclen) {
                if (strncmp(commandBuffer, commands[i].commandName, dstlen) == 0) {
                    (commands[i].commandFunction)(parameterPointer, parseCommandParameters(pParam));
                    found = true;
                    bufferPosition = 0;
                    showPrompt();
                }
            }
            i++;
        }
        if (!found) {
            bufferPosition = 0;
            errorCommand();
            showPrompt();
        }
    } else
        showPrompt();
}

void MicroBox::commandParser()
{
    while (portHandler->available()) {
        uint8_t ch;
        ch = portHandler->read();

        if (handleEscapeSequence(ch))
            continue;

        if (ch == 0x7F || ch == 0x08) {
            if (bufferPosition > 0) {
                bufferPosition--;
                commandBuffer[bufferPosition] = 0;
                portHandler->write(ch);
                printf(" \x1B[1D");
            } else {
                printf("\a");
            }
        } else if (ch == '\t') {
            handleTab();
        } else if (ch != '\r' && bufferPosition < (MAX_COMMAND_BUFFER_SIZE - 1)) {
            if (ch != '\n') {
                if (localEcho)
                    portHandler->write(ch);
                commandBuffer[bufferPosition++] = ch;
                commandBuffer[bufferPosition] = 0;
            }
        } else {
            executeCommand();
        }
    }
}

bool MicroBox::handleEscapeSequence(unsigned char ch)
{
    bool ret = false;

    if (ch == 27) {
        escapeSequence = ESCAPE_STATE_START;
        ret = true;
    } else if (escapeSequence == ESCAPE_STATE_START) {
        if (ch == 0x5B) {
            escapeSequence = ESCAPE_STATE_CODE;
            ret = true;
        } else
            escapeSequence = ESCAPE_STATE_NONE;
    } else if (escapeSequence == ESCAPE_STATE_CODE) {
        if (ch == 0x41) // Cursor Up
        {
            historyUp();
        } else if (ch == 0x42) // Cursor Down
        {
            historyDown();
        } else if (ch == 0x43) // Cursor Right
        {
        } else if (ch == 0x44) // Cursor Left
        {
        }
        escapeSequence = ESCAPE_STATE_NONE;
        ret = true;
    }
    return ret;
}

uint8_t MicroBox::compareParameter(uint8_t idx1, uint8_t idx2)
{
    uint8_t i = 0;

    const char* pName1 = commands[idx1].commandName;
    const char* pName2 = commands[idx2].commandName;

    while (pName1[i] != 0 && pName2[i] != 0) {
        if (pName1[i] != pName2[i])
            return i;
        i++;
    }
    return i;
}

int8_t MicroBox::getCommandIndex(char* pCmd, int8_t startIdx)
{
    while (commands[startIdx].commandName != nullptr) {
        if (strncmp(commands[startIdx].commandName, pCmd, strlen(pCmd)) == 0) {
            return startIdx;
        }
        startIdx++;
    }
    return -1;
}

void MicroBox::handleTab()
{
    int8_t idx, idx2;
    char* pParam = nullptr;
    uint8_t i, len = 0;
    uint8_t parlen, matchlen, inlen;

    for (i = 0; i < bufferPosition; i++) {
        if (commandBuffer[i] == ' ')
            pParam = commandBuffer + i;
    }

    if (bufferPosition && pParam == nullptr) {
        pParam = commandBuffer;

        idx = getCommandIndex(pParam);
        if (idx >= 0) {
            parlen = strlen(commands[idx].commandName);
            matchlen = parlen;
            idx2 = idx;
            while ((idx2 = getCommandIndex(pParam, idx2 + 1)) != -1) {
                matchlen = compareParameter(idx, idx2);
                if (matchlen < parlen)
                    parlen = matchlen;
            }
            inlen = strlen(pParam);
            if (matchlen > inlen) {
                len = matchlen - inlen;
                if ((bufferPosition + len) < MAX_COMMAND_BUFFER_SIZE) {
                    strncat(commandBuffer, commands[idx].commandName + inlen, len);
                    bufferPosition += len;
                } else
                    len = 0;
            }
        }
    }
    if (len > 0) {
        printf("%s", pParam + inlen);
    }
}

void MicroBox::historyUp()
{
    if (historyBufferSize == 0 || historyWritePosition == 0)
        return;

    if (historyCursorPosition == -1)
        historyCursorPosition = historyWritePosition - 2;

    while (historyBuffer[historyCursorPosition] != 0 && historyCursorPosition > 0) {
        historyCursorPosition--;
    }
    if (historyCursorPosition > 0)
        historyCursorPosition++;

    strcpy(commandBuffer, historyBuffer + historyCursorPosition);
    historyPrintHelper();
    if (historyCursorPosition > 1)
        historyCursorPosition -= 2;
}

void MicroBox::historyDown()
{
    int pos;
    if (historyCursorPosition != -1 && historyCursorPosition != historyWritePosition - 2) {
        pos = historyCursorPosition + 2;
        pos += strlen(historyBuffer + pos) + 1;

        strcpy(commandBuffer, historyBuffer + pos);
        historyPrintHelper();
        historyCursorPosition = pos - 2;
    }
}

void MicroBox::historyPrintHelper()
{
    uint8_t i;
    uint8_t len;

    len = strlen(commandBuffer);
    for (i = 0; i < bufferPosition; i++)
        printf("\b");
    printf("%s", commandBuffer);
    if (len < bufferPosition) {
        printf("\x1B[K");
    }
    bufferPosition = len;
}

void MicroBox::addToHistory(char* buf)
{
    uint8_t len;
    int blockStart = 0;

    len = strlen(buf);
    if (historyBufferSize > 0) {
        if (historyWritePosition + len + 1 >= historyBufferSize) {
            while (historyWritePosition + len - blockStart >= historyBufferSize) {
                blockStart += strlen(historyBuffer + blockStart) + 1;
            }
            memmove(historyBuffer, historyBuffer + blockStart, historyWritePosition - blockStart);
            historyWritePosition -= blockStart;
        }
        strcpy(historyBuffer + historyWritePosition, buf);
        historyWritePosition += len + 1;
        historyBuffer[historyWritePosition] = 0;
    }
}

void MicroBox::errorCommand()
{
    printf("Command not found. Use \"help\" or \"help <cmd>\" for details.\n\r");
}

// Taken from Stream.cpp
double MicroBox::parseFloat(char* pBuf)
{
    bool isNegative = false;
    bool isFraction = false;
    long value = 0;
    unsigned char c;
    double fraction = 1.0;
    uint8_t idx = 0;

    c = pBuf[idx++];
    // ignore non numeric leading characters
    if (c > 127)
        return 0; // zero returned if timeout

    do {
        if (c == '-')
            isNegative = true;
        else if (c == '.')
            isFraction = true;
        else if (c >= '0' && c <= '9') { // is c a digit?
            value = value * 10 + c - '0';
            if (isFraction)
                fraction *= 0.1;
        }
        c = pBuf[idx++];
    } while ((c >= '0' && c <= '9') || c == '.');

    if (isNegative)
        value = -value;
    if (isFraction)
        return value * fraction;
    else
        return value;
}

void MicroBox::showHelp(char** pParam, uint8_t parCnt)
{
    if (parCnt == 0) {
        printf("List of available commands:\n\r\n\r");
        printCommands();
        printf("\n\rTo get detailed information about <cmd>, type \"help <cmd>\".\n\r");
    } else {
        char* cmdName = pParam[0];
        uint8_t i = 0;
        while (commands[i].commandName != nullptr) {
            if (!strcmp(commands[i].commandName, cmdName)) {
                printf("%s", commands[i].commandDescription);
                return;
            }
            ++i;
        }

        printf("ERROR: Command %s not found.\n\r", cmdName);
    }
}

void MicroBox::printCommands()
{
    uint8_t index = 0;
    while (commands[index].commandName != nullptr) {
        printf("%s\n\r", commands[index].commandName);
        index++;
    }
}