Simplified and generalized version of the [**microBox** by wastel7.](https://github.com/wastel7/microBox)

## Description

![pic](https://github.com/AntonEvmenenko/microBox/blob/develop/screenshot.png)

microBox is an library that provides a command line interface with Linux Shell like look and feel.

## Features

* Linux Shell look and feel
* Command history
* Autocompletion(Tab)
* User commands
* Int, Double and String datatypes supported for parameters
* Any duplex communication interface could be used (you need to add the description of your interface to the `port_handlers` folder)

## How to use

1. Add port handler.

The library needs to know which port to use and how to control it. To add new port handler, you need to create a class derived from `PortHandler` ([port_handler.h](https://github.com/AntonEvmenenko/microBox/blob/develop/port_handler.h)). Some examples [are available](https://github.com/AntonEvmenenko/microBox/tree/develop/port_handlers).

2. Initialize your port. Create microBox object, initialize it too.

```cpp

    MicroBox microbox;
    ...
    portHandler.begin(115200);
    microbox.begin("hostname", &portHandler);
```

3. Add your CLI commands.

```cpp
microbox.addCommand("sum", [](char** param, uint8_t parCnt){
    if (parCnt == 2) {
        int a = atoi(param[0]);
        int b = atoi(param[1]);
        int c = a + b;
        microbox.printf("%d", c);
    } else {
        microbox.printf("ERROR: check \"help <cmd>\" for the detailed information\n\r");
    }
}, 
"DESCRIPTION:\n\r"
"    Use this command to print the sum of two integers.\n\r" 
"USAGE:\n\r"
"    sum a b\n\r"
"PARAMETERS:\n\r"
"    a -- first summand\n\r"
"    b -- second summand\n\r"
);
```

4. Сall `microbox.commandParser()` periodically.
