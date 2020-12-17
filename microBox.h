/*
  microBox.h - Library for Linux-Shell like interface for Arduino.
  Created by Sebastian Duell, 06.02.2015.
  More info under http://sebastian-duell.de
  Released under GPLv3.
*/

#ifndef _BASHCMD_H_
#define _BASHCMD_H_

#include <stdint.h>
#include <string.h>
#include <functional>

#define MAX_CMD_NUM 20
#define MAX_HISTORY_BUFFER_SIZE 1000

#define MAX_CMD_BUF_SIZE 40

#define PARTYPE_INT 0x01
#define PARTYPE_DOUBLE 0x02
#define PARTYPE_STRING 0x04
#define PARTYPE_RW 0x10
#define PARTYPE_RO 0x00

#define ESC_STATE_NONE 0
#define ESC_STATE_START 1
#define ESC_STATE_CODE 2

#define PRINTF_BUF 256

typedef std::function<void (char** param, uint8_t parCnt)> callback_t;

class PortHandler;

typedef struct
{
    const char* cmdName;
    const char *cmdDesc;
    callback_t cmdFunc;
} CMD_ENTRY;

typedef struct
{
    const char* paramName;
    void* pParam;
    uint8_t parType;
    uint8_t len;
    void (*setFunc)(uint8_t id);
    void (*getFunc)(uint8_t id);
    uint8_t id;
} PARAM_ENTRY;

class MicroBox {
public:
    MicroBox();
    ~MicroBox();
    void begin(const char* hostName, PortHandler* portHandler, bool showPrompt = true, bool localEcho = true, PARAM_ENTRY* pParams = NULL);
    void cmdParser();
    bool AddCommand(const char* cmdName, callback_t cmdFunc, const char* cmdDesc);
    void printf(const char* format, ...);
    void ShowPrompt();

private:
    void showHelp(char** pParam, uint8_t parCnt);
    void PrintCommands();

private:
    uint8_t ParseCmdParams(char* pParam);
    void ErrorCmd();
    int8_t GetCmdIdx(char* pCmd, int8_t startIdx = 0);
    uint8_t ParCmp(uint8_t idx1, uint8_t idx2, bool cmd = false);
    void HandleTab();
    void HistoryUp();
    void HistoryDown();
    void HistoryPrintHlpr();
    void AddToHistory(char* buf);
    void ExecCommand();
    double parseFloat(char* pBuf);
    bool HandleEscSeq(unsigned char ch);

private:
    char cmdBuf[MAX_CMD_BUF_SIZE];
    char dirBuf[15];
    char* ParmPtr[10];
    uint8_t bufPos;
    uint8_t escSeq;
    unsigned long watchTimeout;
    const char* machName;
    int historyBufSize = {0};
    int historyWrPos;
    int historyCursorPos;
    bool locEcho;

    CMD_ENTRY Cmds[MAX_CMD_NUM] = {0};
    PARAM_ENTRY* Params;

    char historyBuf[MAX_HISTORY_BUFFER_SIZE];

    PortHandler* portHandler = nullptr;
};

#endif
