/*
  microBox.cpp - Library for Linux-Shell like interface for Arduino.
  Created by Sebastian Duell, 06.02.2015.
  More info under http://sebastian-duell.de
  Released under GPLv3.
*/

#include "microBox.h"

#include "port_handler.h"
#include <printf/printf.h>
#undef printf

MicroBox::MicroBox()
{
    bufPos = 0;
    locEcho = false;
    watchTimeout = 0;
    escSeq = 0;
    historyWrPos = 0;
    historyBufSize = 0;
    historyCursorPos = -1;
}

MicroBox::~MicroBox()
{
}

void MicroBox::begin(const char* hostName, PortHandler* portHandler, bool showPrompt, bool localEcho, PARAM_ENTRY* pParams)
{
    this->portHandler = portHandler;

    Cmds[0].cmdName = "help";
    Cmds[0].cmdDesc = "Prints help.\n\r";
    Cmds[0].cmdFunc = std::bind(&MicroBox::showHelp, this, std::placeholders::_1, std::placeholders::_2);

    historyBufSize = MAX_HISTORY_BUFFER_SIZE;

    locEcho = localEcho;
    Params = pParams;
    machName = hostName;
    ParmPtr[0] = NULL;

    if (showPrompt) {
        ShowPrompt();
    }
}

bool MicroBox::AddCommand(const char* cmdName, callback_t cmdFunc, const char* cmdDesc)
{
    uint8_t idx = 0;

    while ((Cmds[idx].cmdFunc != NULL) && (idx < (MAX_CMD_NUM - 1))) {
        idx++;
    }
    if (idx < (MAX_CMD_NUM - 1)) {
        Cmds[idx].cmdName = cmdName;
        Cmds[idx].cmdDesc = cmdDesc;
        Cmds[idx].cmdFunc = cmdFunc;
        idx++;
        Cmds[idx].cmdName = NULL;
        Cmds[idx].cmdDesc = NULL;
        Cmds[idx].cmdFunc = NULL;
        return true;
    }
    return false;
}

// from https://playground.arduino.cc/Main/Printf/
void MicroBox::printf(const char* format, ...)
{
    char buf[PRINTF_BUF];
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

void MicroBox::ShowPrompt()
{
    printf("%s>", machName);
}

uint8_t MicroBox::ParseCmdParams(char* pParam)
{
    uint8_t idx = 0;

    ParmPtr[idx] = pParam;
    if (pParam != NULL) {
        idx++;
        while ((pParam = strchr(pParam, ' ')) != NULL) {
            pParam[0] = 0;
            pParam++;
            ParmPtr[idx++] = pParam;
        }
    }
    return idx;
}

void MicroBox::ExecCommand()
{
    bool found = false;
    printf("\n\r");
    if (bufPos > 0) {
        uint8_t i = 0;
        uint8_t dstlen;
        uint8_t srclen;
        char* pParam;

        cmdBuf[bufPos] = 0;
        pParam = strchr(cmdBuf, ' ');
        if (pParam != NULL) {
            pParam++;
            srclen = pParam - cmdBuf - 1;
        } else
            srclen = bufPos;

        AddToHistory(cmdBuf);
        historyCursorPos = -1;

        while (Cmds[i].cmdName != NULL && found == false) {
            dstlen = strlen(Cmds[i].cmdName);
            if (dstlen == srclen) {
                if (strncmp(cmdBuf, Cmds[i].cmdName, dstlen) == 0) {
                    (Cmds[i].cmdFunc)(ParmPtr, ParseCmdParams(pParam));
                    found = true;
                    bufPos = 0;
                    ShowPrompt();
                }
            }
            i++;
        }
        if (!found) {
            bufPos = 0;
            ErrorCmd();
            ShowPrompt();
        }
    } else
        ShowPrompt();
}

void MicroBox::cmdParser()
{
    while (portHandler->available()) {
        uint8_t ch;
        ch = portHandler->read();

        if (HandleEscSeq(ch))
            continue;

        if (ch == 0x7F || ch == 0x08) {
            if (bufPos > 0) {
                bufPos--;
                cmdBuf[bufPos] = 0;
                portHandler->write(ch);
                printf(" \x1B[1D");
            } else {
                printf("\a");
            }
        } else if (ch == '\t') {
            HandleTab();
        } else if (ch != '\r' && bufPos < (MAX_CMD_BUF_SIZE - 1)) {
            if (ch != '\n') {
                if (locEcho)
                    portHandler->write(ch);
                cmdBuf[bufPos++] = ch;
                cmdBuf[bufPos] = 0;
            }
        } else {
            ExecCommand();
        }
    }
}

bool MicroBox::HandleEscSeq(unsigned char ch)
{
    bool ret = false;

    if (ch == 27) {
        escSeq = ESC_STATE_START;
        ret = true;
    } else if (escSeq == ESC_STATE_START) {
        if (ch == 0x5B) {
            escSeq = ESC_STATE_CODE;
            ret = true;
        } else
            escSeq = ESC_STATE_NONE;
    } else if (escSeq == ESC_STATE_CODE) {
        if (ch == 0x41) // Cursor Up
        {
            HistoryUp();
        } else if (ch == 0x42) // Cursor Down
        {
            HistoryDown();
        } else if (ch == 0x43) // Cursor Right
        {
        } else if (ch == 0x44) // Cursor Left
        {
        }
        escSeq = ESC_STATE_NONE;
        ret = true;
    }
    return ret;
}

uint8_t MicroBox::ParCmp(uint8_t idx1, uint8_t idx2, bool cmd)
{
    uint8_t i = 0;

    const char* pName1;
    const char* pName2;

    if (cmd) {
        pName1 = Cmds[idx1].cmdName;
        pName2 = Cmds[idx2].cmdName;
    } else {
        pName1 = Params[idx1].paramName;
        pName2 = Params[idx2].paramName;
    }

    while (pName1[i] != 0 && pName2[i] != 0) {
        if (pName1[i] != pName2[i])
            return i;
        i++;
    }
    return i;
}

int8_t MicroBox::GetCmdIdx(char* pCmd, int8_t startIdx)
{
    while (Cmds[startIdx].cmdName != NULL) {
        if (strncmp(Cmds[startIdx].cmdName, pCmd, strlen(pCmd)) == 0) {
            return startIdx;
        }
        startIdx++;
    }
    return -1;
}

void MicroBox::HandleTab()
{
    int8_t idx, idx2;
    char* pParam = NULL;
    uint8_t i, len = 0;
    uint8_t parlen, matchlen, inlen;

    for (i = 0; i < bufPos; i++) {
        if (cmdBuf[i] == ' ')
            pParam = cmdBuf + i;
    }

    if (bufPos && pParam == NULL) {
        pParam = cmdBuf;

        idx = GetCmdIdx(pParam);
        if (idx >= 0) {
            parlen = strlen(Cmds[idx].cmdName);
            matchlen = parlen;
            idx2 = idx;
            while ((idx2 = GetCmdIdx(pParam, idx2 + 1)) != -1) {
                matchlen = ParCmp(idx, idx2, true);
                if (matchlen < parlen)
                    parlen = matchlen;
            }
            inlen = strlen(pParam);
            if (matchlen > inlen) {
                len = matchlen - inlen;
                if ((bufPos + len) < MAX_CMD_BUF_SIZE) {
                    strncat(cmdBuf, Cmds[idx].cmdName + inlen, len);
                    bufPos += len;
                } else
                    len = 0;
            }
        }
    }
    if (len > 0) {
        printf("%s", pParam + inlen);
    }
}

void MicroBox::HistoryUp()
{
    if (historyBufSize == 0 || historyWrPos == 0)
        return;

    if (historyCursorPos == -1)
        historyCursorPos = historyWrPos - 2;

    while (historyBuf[historyCursorPos] != 0 && historyCursorPos > 0) {
        historyCursorPos--;
    }
    if (historyCursorPos > 0)
        historyCursorPos++;

    strcpy(cmdBuf, historyBuf + historyCursorPos);
    HistoryPrintHlpr();
    if (historyCursorPos > 1)
        historyCursorPos -= 2;
}

void MicroBox::HistoryDown()
{
    int pos;
    if (historyCursorPos != -1 && historyCursorPos != historyWrPos - 2) {
        pos = historyCursorPos + 2;
        pos += strlen(historyBuf + pos) + 1;

        strcpy(cmdBuf, historyBuf + pos);
        HistoryPrintHlpr();
        historyCursorPos = pos - 2;
    }
}

void MicroBox::HistoryPrintHlpr()
{
    uint8_t i;
    uint8_t len;

    len = strlen(cmdBuf);
    for (i = 0; i < bufPos; i++)
        printf("\b");
    printf("%s", cmdBuf);
    if (len < bufPos) {
        printf("\x1B[K");
    }
    bufPos = len;
}

void MicroBox::AddToHistory(char* buf)
{
    uint8_t len;
    int blockStart = 0;

    len = strlen(buf);
    if (historyBufSize > 0) {
        if (historyWrPos + len + 1 >= historyBufSize) {
            while (historyWrPos + len - blockStart >= historyBufSize) {
                blockStart += strlen(historyBuf + blockStart) + 1;
            }
            memmove(historyBuf, historyBuf + blockStart, historyWrPos - blockStart);
            historyWrPos -= blockStart;
        }
        strcpy(historyBuf + historyWrPos, buf);
        historyWrPos += len + 1;
        historyBuf[historyWrPos] = 0;
    }
}

void MicroBox::ErrorCmd()
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
        PrintCommands();
        printf("\n\rTo get detailed information about <cmd>, type \"help <cmd>\".\n\r");
    } else {
        char* cmdName = pParam[0];
        uint8_t i = 0;
        while (Cmds[i].cmdName != NULL) {
            if (!strcmp(Cmds[i].cmdName, cmdName)) {
                printf("%s", Cmds[i].cmdDesc);
                return;
            }
            ++i;
        }

        printf("ERROR: Command %s not found.\n\r", cmdName);
    }
}

void MicroBox::PrintCommands()
{
    uint8_t index = 0;
    while (Cmds[index].cmdName != NULL) {
        printf("%s\n\r", Cmds[index].cmdName);
        index++;
    }
}