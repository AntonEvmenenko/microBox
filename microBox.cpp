/*
  microBox.cpp - Library for Linux-Shell like interface for Arduino.
  Created by Sebastian Duell, 06.02.2015.
  More info under http://sebastian-duell.de
  Released under GPLv3.
*/

#include <microBox.h>
#include <microBox_platform.h>

microBox microbox;

CMD_ENTRY microBox::Cmds[] = {
    { "help", "Prints help.\n\r", microBox::showHelp},
    { NULL, NULL, NULL }
};

char microBox::historyBuf[MAX_HISTORY_BUFFER_SIZE];

microBox::microBox()
{
    bufPos = 0;
    csvMode = false;
    locEcho = false;
    watchTimeout = 0;
    escSeq = 0;
    historyWrPos = 0;
    historyBufSize = 0;
    historyCursorPos = -1;
}

microBox::~microBox()
{
}

void microBox::begin(const char* hostName, bool localEcho, PARAM_ENTRY* pParams)
{
    historyBufSize = MAX_HISTORY_BUFFER_SIZE;
    historyBuf[0] = 0;
    historyBuf[1] = 0;

    locEcho = localEcho;
    Params = pParams;
    machName = hostName;
    ParmPtr[0] = NULL;
    strcpy(currentDir, "/");
    ShowPrompt();
}

bool microBox::AddCommand(const char* cmdName, void (*cmdFunc)(char** param, uint8_t parCnt), const char* cmdDesc)
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

void microBox::ShowPrompt()
{
    SerialPrint("root@");
    SerialPrint(machName);
    SerialPrint(":");
    SerialPrint(currentDir);
    SerialPrint(">");
}

uint8_t microBox::ParseCmdParams(char* pParam)
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

void microBox::ExecCommand()
{
    bool found = false;
    SerialPrint("\n\r");
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
                    (*Cmds[i].cmdFunc)(ParmPtr, ParseCmdParams(pParam));
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

void microBox::cmdParser()
{
    while (SerialAvailable()) {
        uint8_t ch;
        ch = SerialRead();

        if (HandleEscSeq(ch))
            continue;

        if (ch == 0x7F || ch == 0x08) {
            if (bufPos > 0) {
                bufPos--;
                cmdBuf[bufPos] = 0;
                SerialWrite(ch);
                SerialPrint(" \x1B[1D");
            } else {
                SerialPrint("\a");
            }
        } else if (ch == '\t') {
            HandleTab();
        } else if (ch != '\r' && bufPos < (MAX_CMD_BUF_SIZE - 1)) {
            if (ch != '\n') {
                if (locEcho)
                    SerialWrite(ch);
                cmdBuf[bufPos++] = ch;
                cmdBuf[bufPos] = 0;
            }
        } else {
            ExecCommand();
        }
    }
}

bool microBox::HandleEscSeq(unsigned char ch)
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

uint8_t microBox::ParCmp(uint8_t idx1, uint8_t idx2, bool cmd)
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

int8_t microBox::GetCmdIdx(char* pCmd, int8_t startIdx)
{
    while (Cmds[startIdx].cmdName != NULL) {
        if (strncmp(Cmds[startIdx].cmdName, pCmd, strlen(pCmd)) == 0) {
            return startIdx;
        }
        startIdx++;
    }
    return -1;
}

void microBox::HandleTab()
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
        SerialPrint(pParam + inlen);
    }
}

void microBox::HistoryUp()
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

void microBox::HistoryDown()
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

void microBox::HistoryPrintHlpr()
{
    uint8_t i;
    uint8_t len;

    len = strlen(cmdBuf);
    for (i = 0; i < bufPos; i++)
        SerialPrint("\b");
    SerialPrint(cmdBuf);
    if (len < bufPos) {
        SerialPrint("\x1B[K");
    }
    bufPos = len;
}

void microBox::AddToHistory(char* buf)
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

void microBox::ErrorCmd()
{
    SerialPrint("Command not found. Use \"help\" or \"help <cmd>\" for details.\n\r");
}

// Taken from Stream.cpp
double microBox::parseFloat(char* pBuf)
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

void microBox::showHelp(char** pParam, uint8_t parCnt)
{
    if (parCnt == 0) {
        SerialPrint("List of available commands:\n\r");
        SerialPrint("\n\r");
        PrintCommands();
        SerialPrint("\n\r");
        SerialPrint("To get detailed information about <cmd>, type \"help <cmd>\".\n\r");
    } else {
        char* cmdName = pParam[0];
        uint8_t i = 0;
        while (Cmds[i].cmdName != NULL) {
            if (!strcmp(Cmds[i].cmdName, cmdName)) {
                SerialPrint(Cmds[i].cmdDesc);
                return;
            }
            ++i;
        }

        SerialPrint("ERROR: Command ");
        SerialPrint(cmdName);
        SerialPrint(" not found.\n\r");
    }
}

void microBox::PrintCommands()
{
    uint8_t index = 0;
    while (Cmds[index].cmdName != NULL) {
        SerialPrint(Cmds[index].cmdName);
        SerialPrint("\n\r");
        index++;
    }
}