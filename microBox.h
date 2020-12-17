#ifndef MICROBOX_H
#define MICROBOX_H

#include <stdint.h>
#include <string.h>
#include <functional>

#define MAX_COMMAND_NUMBER          20
#define MAX_HISTORY_BUFFER_SIZE     1000

#define MAX_COMMAND_BUFFER_SIZE     40

#define ESCAPE_STATE_NONE           0
#define ESCAPE_STATE_START          1
#define ESCAPE_STATE_CODE           2

#define PRINTF_BUFFER_SIZE          256

typedef std::function<void (char** param, uint8_t parCnt)> callback_t;

class PortHandler;

typedef struct
{
    const char* commandName;
    const char* commandDescription;
    callback_t commandFunction;
} COMMAND_ENTRY;

class MicroBox {
public:
    void begin(const char* hostName, PortHandler* portHandler, bool showPrompt = true, bool localEcho = true);
    void commandParser();
    bool addCommand(const char* commandName, callback_t commandFunction, const char* commandDescription);
    void printf(const char* format, ...);
    void showPrompt();

private:
    void showHelp(char** pParam, uint8_t parCnt);
    void printCommands();

private:
    uint8_t parseCommandParameters(char* pParam);
    void errorCommand();
    int8_t getCommandIndex(char* pCmd, int8_t startIdx = 0);
    uint8_t compareParameter(uint8_t idx1, uint8_t idx2);
    void handleTab();
    void historyUp();
    void historyDown();
    void historyPrintHelper();
    void addToHistory(char* buf);
    void executeCommand();
    double parseFloat(char* pBuf);
    bool handleEscapeSequence(unsigned char ch);

private:
    char commandBuffer[MAX_COMMAND_BUFFER_SIZE] =   {0};
    char* parameterPointer[10] =                    {0};
    uint8_t bufferPosition =                        0;
    uint8_t escapeSequence =                        0;
    const char* hostName =                          nullptr;
    int historyBufferSize =                         MAX_HISTORY_BUFFER_SIZE;
    int historyWritePosition =                      0;
    int historyCursorPosition =                     -1;
    bool localEcho =                                false;
    COMMAND_ENTRY commands[MAX_COMMAND_NUMBER] =    {0};
    char historyBuffer[MAX_HISTORY_BUFFER_SIZE] =   {0};
    PortHandler* portHandler =                      nullptr;
};

#endif // MICROBOX_H
