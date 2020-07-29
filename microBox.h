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

#define MAX_CMD_NUM 20
#define MAX_HISTORY_BUFFER_SIZE 1000

#define MAX_CMD_BUF_SIZE 40
#define MAX_PATH_LEN 10

#define PARTYPE_INT 0x01
#define PARTYPE_DOUBLE 0x02
#define PARTYPE_STRING 0x04
#define PARTYPE_RW 0x10
#define PARTYPE_RO 0x00

#define ESC_STATE_NONE 0
#define ESC_STATE_START 1
#define ESC_STATE_CODE 2

typedef struct
{
    const char* cmdName;
    const char *cmdDesc;
    void (*cmdFunc)(char** param, uint8_t parCnt);
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

class microBox {
public:
    microBox();
    ~microBox();
    void begin(const char* hostName, bool localEcho = true, PARAM_ENTRY* pParams = NULL);
    void cmdParser();
    bool isTimeout(unsigned long* lastTime, unsigned long intervall);
    bool AddCommand(const char* cmdName, void (*cmdFunc)(char** param, uint8_t parCnt), const char* cmdDesc);

private:
    static void showHelp(char** pParam, uint8_t parCnt);
    static void PrintCommands();

private:
    void ShowPrompt();
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
    char currentDir[MAX_PATH_LEN];

    char cmdBuf[MAX_CMD_BUF_SIZE];
    char dirBuf[15];
    char* ParmPtr[10];
    uint8_t bufPos;
    bool csvMode;
    uint8_t escSeq;
    unsigned long watchTimeout;
    const char* machName;
    int historyBufSize;
    int historyWrPos;
    int historyCursorPos;
    bool locEcho;

    static CMD_ENTRY Cmds[MAX_CMD_NUM];
    PARAM_ENTRY* Params;

    static char historyBuf[MAX_HISTORY_BUFFER_SIZE];
};

extern microBox microbox;

#endif
