Simplified and generalized version of the [**microBox** by wastel7.](https://github.com/wastel7/microBox)

## Description

microBox is an library that provides a command line interface with Linux Shell like look and feel.

## Features

* Linux Shell look and feel
* Command history
* Autocompletion(Tab)
* User commands
* Int, Double and String datatypes supported for parameters
* Could be ported to any platform (you need to add the description for your platform to the `platform` folder)

## How to use

1. Add platform definition.

The library needs to know which interface to use and how to control it. To add new platform definition, you need to implement all the functions described in [microBox_platform.h](https://github.com/AntonEvmenenko/microBox/blob/develop/microBox_platform.h). Some examples [are available](https://github.com/AntonEvmenenko/microBox/tree/develop/platforms).

2. Initialize microBox object.

```cpp
char hostname[] = "myhostname";`
microbox.begin(hostname);
```

3. Add your CLI commands.

```cpp
microbox.AddCommand("sum", [](char** param, uint8_t parCnt){
    if (parCnt == 2) {
        int a = atoi(param[0]);
        int b = atoi(param[1]);
        int c = a + b;
        SerialPrint(c);
    } else {
        SerialPrint("ERROR: check \"help <cmd>\" for the detailed information\n\r");
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

4. Сall `microbox.cmdParser()` periodically.
