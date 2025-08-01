/*
 * cli-binding.c
 *
 * This file is part of the TULIP4041 project.
 * Copyright (C) 2024 Meindert Kuipers
 *
 * Distributed under the MIT license
 * This is free software: you are free to change and redistribute it.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Version info
 * 00.01.01 initial public release
 *          support for limited function set:
 *            system - status/welcome/pio/ident
 *            romlist
 *            emulation on/off
 *            tracer on/off
 *            ilscope on/off
 *            ilprinter on/off
 *            tracefilter inlcude range
 *            tracefilter exclude range
 *            tracer sysrom on/off
 */

// file contains only the bindings for the embedded-cli
// implementation of the functions is in userinterface.c

#include <stdio.h>
#include "cli-binding.h"


// Expand cli implementation here (must be in one file only)
#define EMBEDDED_CLI_IMPL
#include "embedded_cli.h"

#define CLI_BUFFER_SIZE     1500
#define CLI_RX_BUFFER_SIZE  64
#define CLI_CMD_BUFFER_SIZE 128
#define CLI_HISTORY_SIZE    64
#define CLI_BINDING_COUNT   32


EmbeddedCli *cli;

CLI_UINT cliBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];

EmbeddedCliConfig *config;

bool firstconnect = false;          // to detect the first connect to a CDC host
                                    // to display a welcome message

// other requirements for the CLI implementatio:


// create an instance in the CLI
//      EmbeddedCli *cli = embeddedCliNew(config);

// or with the default config
//      EmbeddedCli *cli = embeddedCliNewDefault();

// Provide a function that will be used to send chars to the other end:
//      void writeChar(EmbeddedCli *embeddedCli, char c);
//      ...
//      cli->writeChar = writeChar;

// After creation, provide desired bindings to CLI (can be provided at any point in runtime):
//      embeddedCliAddBinding(cli, {
//         "get-led",          // command name (spaces are not allowed)
//         "Get led status",   // Optional help for a command (NULL for no help)
//         false,              // flag whether to tokenize arguments (see below)
//         nullptr,            // optional pointer to any application context
//         onLed               // binding function 
//      });
//      embeddedCliAddBinding(cli, {
//         "get-adc",
//         "Read adc value",
//         true,
//         nullptr,
//         onAdc
//      });

//Don't forget to create binding functions as well:
//  void onLed(EmbeddedCli *cli, char *args, void *context) {
//     // use args as raw null-terminated string of all arguments
//  }
//  void onAdc(EmbeddedCli *cli, char *args, void *context) {
//     // use args as list of tokens
//  }



EmbeddedCli *getCliPointer() {
    return cli;
}


// Function to encapsulate the 'embeddedCliPrint()' call with print formatting arguments (act like printf(), but keeps cursor at correct location).
// The 'embeddedCliPrint()' function does already add a linebreak ('\r\n') to the end of the print statement, so no need to add it yourself.
int cli_printf(const char *format, ...) {
    // Create a buffer to store the formatted string
    char buffer[CLI_PRINT_BUFFER_SIZE];

    // Format the string using snprintf
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Check if string fitted in buffer else print error to stderr
    if (length < 0) {
        fprintf(stderr, "Error formatting the string\r\n");
        return EOF;
    }

    // Call embeddedCliPrint with the formatted string
    tud_task();  // must keep the USB port updated
    embeddedCliPrint(getCliPointer(), buffer);
    return 0;
}

// Function to encapsulate the 'embeddedCliPrint()' call with print formatting arguments (act like printf(), but keeps cursor at correct location).
// The 'embeddedCliPrintN()' function does NOT add a linebreak ('\r\n') to the end of the print statement, so you need to add it yourself.
int cli_printfn(const char *format, ...) {
    // Create a buffer to store the formatted string
    char buffer[CLI_PRINT_BUFFER_SIZE];

    // Format the string using snprintf
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Check if string fitted in buffer else print error to stderr
    if (length < 0) {
        fprintf(stderr, "Error formatting the string\r\n");
        return EOF;
    }

    // Call embeddedCliPrint with the formatted string
    tud_task();  // must keep the USB port updated
    embeddedCliPrintN(getCliPointer(), buffer);
    return 0;
}


const char* on_off[] =
// lfor generic argument testing
{
    "on"            // feature on
    "off",          // feature off
};

const char* __in_flash()system_cmds[] =
// list of arguments for the system command
{
    "status",
    "pio",
    "cdc",
    "cdcident",
    "REBOOT",
    "BOOTSEL",
    "poweron",
    "calcreset",
    "configinit",
    "configlist",
};

void onSystemCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);
    int cmd = -1;
    int num_cmds = sizeof(system_cmds) / sizeof(char *);

    if (arg1 == NULL) {
        // no argument given, just show the system status
        // for testing just use the welcome message        
        cli_printf("type help for more info");
        uif_status();      // calling the welcome in userinterface.cpp        
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
      cmd = strcmp(arg1, system_cmds[i]);
      i++;
    }

    if (cmd != 0) 
    {
      i = -1;
    }

    // argument found, now execute
    switch (i) {
      case 1 : uif_status();            // status
            break;
      case 2 : uif_pio_status();        // pio
            break;
      case 3 : uif_cdc_status();        // cdc
            break;
      case 4 : uif_cdc_ident();         // cdcident
            break;
      case 5 : uif_reboot();            // REBOOT
            break;
      case 6 : uif_bootsel();           // BOOTSEL
            break;
      case 7 : uif_poweron();           // poweron
            break;
      case 8 : uif_calcreset();         // calcreset
            break;
      case 9 : uif_configinit();        // re-initialize persistent settings
            break;
      case 10 : uif_configlist();        // re-initialize persistent settings
            break;            
      default:
          cli_printf("system: unkown command %s\n", arg1);    // unknown command
    }
}


// dir command, show directopry listing of SD card
void onDirCLI(EmbeddedCli *cli, char *args, void *context) 
{
    const char *arg1 = embeddedCliGetToken(args, 1);
    if (arg1 == NULL) {
        // no argument given, list the root directory 
        uif_dir(".");      
        return;
    }
    else
    {
        // arg1 was not empty
        uif_dir(arg1);
    }
}

const char* __in_flash()sdcard_cmds[] =
// list of arguments for the system command
{
    "status",
    "mount",
    "unmount",
    "mounted",
    "connect",
    "eject",
};

void onSDCardCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);
    int cmd = -1;
    int num_cmds = sizeof(sdcard_cmds) / sizeof(char *);

    if (arg1 == NULL) {
        // no argument given, just show the system status
        // for testing just use the welcome message        
        cli_printf("type help for more info");
        uif_sdcard_status();      // calling the welcome in userinterface.cpp        
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while ((cmd != 0) && (i < num_cmds)) {
      cmd = strcmp(arg1, sdcard_cmds[i]);
      i++;
    }

    if (cmd != 0) 
    {
      i = -1;
    }

    // argument found, now execute
    switch (i) {
      case 1 : uif_sdcard_status();     // status
          break;
      case 2 : uif_sdcard_mount();      // mount
          break;
      case 3 : uif_sdcard_unmount();    // unmount
          break;
      case 4 : uif_sdcard_mounted();    // mounted
          break;          
      case 5 : uif_sdcard_connect();    // unmount
          break;
      case 6 : uif_sdcard_eject();      // mounted
          break; 

      default:
          cli_printf("sdcard: unkown command %s\n", arg1);    // unknown command
    }
}

const char* __in_flash()import_cmds[] =
// list of arguments for the import command
// import [filename]
{
    "ALL",      // import all files in the directory
    // "UPDATE",   // update files in the file sysytem
    // "compare",  // compare files in the file system prior to an update
    // "FRAM", 
};

// the following commans line options are possible:
// import [filename] <FRAM>
// import [filename] <UPDATE> <FRAM>
// import [filename] <COMPARE> <FRAM>
// import [directory] [ALL]
// import [directory] [ALL] <COMPARE>
// import [directory] [ALL] <UPDATE>
void onImportCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);        // filename or directory
    const char *arg2 = embeddedCliGetToken(args, 2);        // ALL, UPDATE or FRAM
    const char *arg3 = embeddedCliGetToken(args, 3);        // ALL, UPDATE or FRAM

    int cmd = -1;
    int num_cmds = sizeof(import_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given, this does nothing       
        cli_printf("arguments not supported, use: import [filename] <ALL> <UPDATE/COMPARE>");    
        return;
    }

    if (arg2 == NULL && arg3 == NULL) {
        // no second or third argument given, import a simgle file, name in arg1
        // this is a single file import, so pass the filename only
        uif_import((char*)arg1, 0, 0);     // pass the filename only
        return;
    }

    // scan arg2 for something known, in this case only the ALL is supported
    int a2 = 0;
    while (cmd != 0 && a2 < num_cmds) {
        cmd = strcmp(arg2, import_cmds[a2]);
        a2++;
    }

    if (cmd != 0) {
        a2 = -1;
    }

    // scan arg3 for something known, in this case only the ALL is supported
    int a3 = 0;
    while (cmd != 0 && a3 < num_cmds) {
        cmd = strcmp(arg3, import_cmds[a3]);
        a3++;
    }

    // a1 and a2 are passed with the following values:
    // a2/a3 = 4      only filename, regular single file import, already handled
    // a2/a3 = 1      ALL option
    // a2/a3 = 2      UPDATE option
    // a2/a3 = 3      compare option
    // a2/a3 = 4      FRAM option


    // first filter out unsupported options
    if (a2 == a3) {
        // both arguments are the same, this is not supported
        cli_printf("argument combination not supported, use: import [filename] <ALL/FRAM> <UPDATE/COMPARE>");
        return;
    }

    if ((a2 == 1) && (a3 == 4)) {
        // ALL and FRAM is not supported
        cli_printf("cannot use ALL with FRAM");
        return;
    }

    if ((a2 == 4) || (a3 == 4)) {
        // importing to FRAM is not yet supported
        cli_printf("import to FRAM is not yet supported");
        return;
    }

    // pass the filename and detected arguments
    uif_import((char*)arg1, a2, a3);     // pass the arguments
}


void onDeleteCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);        // only one argument now
    const char *arg2 = embeddedCliGetToken(args, 2);        // 
    int cmd = -1;
    // int num_cmds = sizeof(delete_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given
        cli_printf("no arguments given, use: delete [filename], see help");   
        return;
    }

    uif_delete((char*)arg1);       // pass the filename
}



const char* __in_flash()plug_cmds[] =
// list of arguments for the plug command
// plug [FLASH/FRAM] [filename]        
{
    "hpil",        // plug the embedded HP-IL ROM in Page 7 and enables emulation
    "ilprinter",   // plug the embedded HP-IL Printer ROM in Page 6
    "printer",     // plug the embedded HP82143A Printer ROM in Page 6 and enables emulation
    // "module",   for later use, to plug a physical module
};

        #define plug_hpil       1
        #define plug_ilprinter  2
        #define plug_printer    3
        #define plug_module     4
        #define plug_file       5

const char* __in_flash()plug_module_args[] =
// list of arguments for the plug module command
{
    "cx",
    "printer",
    "hpil",
    "hpil-dis",	
    "clear",
};
/*
 define PLUG_HELP_TXT "plug functions\r\n\
        [no argument] shows the current plugged ROMs\r\n\
        status        shows the current plugged ROMs\r\n\
        module        to inform about plugged physical modules\r\n\
           pX [name]  X is the Page number in hex, name is descriptive name of the ROM\r\n\
           cx         resreve Page 3 and 5 if you have an HP41CX\r\n\
           printer    reserve Page 6 for the printer or IR module\r\n\
           hpil       reserve Page 6+7 for the HP-IL module and printer\r\n\
           hpil-dis   reserve Page 7 for the HP-IL module with printer disabled\r\n\
           clear      clear all reserved physical modules\r\n\
        [filename]    plug the ROM in the first suitable Page\r\n\
        [filename] PX plug the ROM in Page X\r\n\
        [filename] PX bN plug the ROM in Page X and bank N (1..4)\r\n\
          [filename] is the name of the file in FLASH or FRAM, maybe without extension\r\n\"


        uif_plug() takes the following arguments:
        uif_plug(int cmd, int p, int b, char *name)
            cmd     according to table above
            p       page number
            b       bank number
            name    filename of ROM/MOD to be plugged, only if cmd = 7
*/  


// plug command, plug a ROM in the system
// this version only supports plugging a named file and the Page number in hex (must be provided!)
void onPlugCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);        // file name
    const char *arg2 = embeddedCliGetToken(args, 2);        // Page number in hex
    const char *arg3 = embeddedCliGetToken(args, 3);        // not used for now
    int cmd = -1;
    int num_cmds = sizeof(plug_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no arguments given, show the plugged ROM as status
        cli_printf("no arguments given, use: plug [filename] Page (in hex)");   
        cli_printf("use the cat command to show the plugged ROMs");
        // uif_plug(plug_status, 0, 0, NULL); 
        return;
    }

    // check arg1 for known arguments
        // scan arg2 for something known, in this case only the ALL is supported
    int a1 = 0;
    while (cmd != 0 && a1 < num_cmds) {
        cmd = strcmp(arg1, plug_cmds[a1]);
        a1++;
    }

    if (cmd != 0) {
        a1 = -1;
    }

    if (a1 >= 0) {
        // arg1 is a known command, so execute it
        switch (a1) {
            case plug_hpil : 
                uif_plug(plug_hpil, 7, 1, NULL);   // plug the HP-IL ROM in Page 7
                return;
            case plug_ilprinter : 
                uif_plug(plug_ilprinter, 6, 1, NULL);   // plug the HP-IL Printer ROM in Page 6
                return;
            case plug_printer : 
                uif_plug(plug_printer, 6, 1, NULL);   // plug the HP82143A Printer ROM in Page 6
                return;
            case plug_module : 
                // this is not yet implemented
                cli_printf("plug module not yet implemented");
                return;
            default:
                // proceed with the file name
        }
    }

    if (arg2 == NULL) {
        // no Page number given, report and show all plugged rOMs
        cli_printf("no Page number given, use: plug [filename] Page (in hex)");   
        // uif_plug(plug_status, 0, 0, NULL); 
        return;
    }

    // We can now pass the filename and Page number

    // check for Px with the Page number in hex
    int p = 0;
    int res = sscanf(arg2, "%X", &p);            // if one decimal is found the res = 1
    if ((res != 1) | (p > 15) | (p < 4)) {
        // no valid result and no valid command, so get out
        cli_printf("invalid Page number, must be >4 and <F (hex)", arg2);    // unknown command
        return;
    }
    // p now contains a valid page number, pass this with the ROM file name
    // file name checking is done in the uif_plug function
    // Bank is not used here now
    uif_plug(plug_file, p, 1, arg1);
}


void onUnPlugCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);        // only one argument now
    const char *arg2 = embeddedCliGetToken(args, 2);        // 
    int cmd = -1;
    int num_cmds = sizeof(plug_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given, show the plugged ROM  
        cli_printf("no arguments given, use: unplug Page (in hex), see help");
        // uif_unplug(plug_status);  
        return;
    }

        // check for Px with the Page number in hex
    int p = 0;
    int res = sscanf(arg1, "%X", &p);            // if one decimal is found then res = 1
    if ((res != 1) | (p > 15) | (p < 4)) {
        // no valid result and no valid command, so get out
        cli_printf("invalid Page number, must be >=4 and <=F (hex)", arg2);    // unknown command
        return;
    }
    // p now contains a valid page number, pass this with the ROM file name
    // file name checking is done in the uif_plug function
    // Bank is not used here now
    uif_unplug(p);

}

void onCatCLI(EmbeddedCli *cli, char *args, void *context)
{
    // check for a hex argument in the range 4..F
    const char *arg1 = embeddedCliGetToken(args, 1);        // only one argument now
    const char *arg2 = embeddedCliGetToken(args, 2);        //
    int cmd = -1;
    int num_cmds = sizeof(plug_cmds) / sizeof(char *);
    if ((arg1 == NULL)) {
        // no argument given, show the plugged ROM  
        cli_printf("no arguments given, use: cat Page (in hex), see help");   
        uif_cat(0);           // show a summary of the plugged ROMs
        return;
    }
    // check for Px with the Page number in hex
    int p = 0;
    int res = sscanf(arg1, "%X", &p);            // if one decimal is found then res = 1
    if ((res != 1) | (p > 15) | (p < 4)) {
        // no valid result and no valid command, so get out
        cli_printf("invalid Page number, must be >=4 and <=F (hex)", arg1);    // unknown command
        return;
    }
    // p now contains a valid page number, pass this with the ROM file name
    uif_cat(p);   
}


const char* __in_flash()printer_cmds[] =
// list of arguments for the HP82143 printer command
// 
{
    "status",           // get status
    "power",            // toggle power
    "trace",            // printer mode trace
    "norm",             // printer mode normal
    "man",              // printer mode manual
    "paper",            // toggle Out Of Paper status
    "print",            // push PRINT button
    "adv",              // push ADV button
    "irtest",           // test the infrared LED
};

void onPrinterCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);        // only one argument now
    const char *arg2 = embeddedCliGetToken(args, 2);        // 
    int cmd = -1;
    int num_cmds = sizeof(printer_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given, show the plugged ROM  
        cli_printf("no arguments given, use: printer [command], see help");   
        uif_printer(1); 
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, printer_cmds[i]);
        i++;
    }

    if (cmd != 0) {
        i = -1;
    }

    if (i >= 0) {
        uif_printer(i);
        
    } else {
        cli_printf("invalid argument %s: use: printer [command], see help", arg1);    // unknown command
    }
}


const char* __in_flash()xmem_cmds[] =
// list of arguments for the HP82143 printer command
// 
{
    "status",           // get status
    "dump",             // dump xmem contents
    "PATTERN",          // program test pattern in FRAM
    "ERASE",            // erase all Extended Memory
};

void onXMEMCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);        // only one argument now
    const char *arg2 = embeddedCliGetToken(args, 2);        // 
    int xmem = 3;
    int cmd = -1;
    int num_cmds = sizeof(xmem_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given, show the plugged ROM  
        cli_printf("no arguments given, see help");   
        uif_xmem(1);   // just show the status
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, xmem_cmds[i]);
        i++;
    }

    if (cmd != 0) {
        // no valid command found, check for a valid number
        i = -1;
        int res = sscanf(arg1, "%d", &xmem);            // if one decimal is found the res = 1

        if ((res != 1) | (xmem > 2) | (xmem < 0)) {
            // no valid result and no valid command, so get out
            cli_printf("invalid argument %s: see help", arg1);    // unknown command
            return;
        }
    }

    // we get here if there is a valid command (i >= 0 or a value 0..2)
    if (i > 0) {
        // status, dump or pattern command found, pass 1..3
        uif_xmem(i);    
    } else {
        // no command found but a valid number 0..2
        // pass a value 10..12
        uif_xmem(xmem + 10);
    }
}

/*
#define TRACER_HELP_TXT "tracer functions\r\n\
        [no argument] shows the tracer status\r\n\
        status        shows the tracer status\r\n\
        trace         toggle tracer enable/disable\r\n\
        sysrom        toggle system rom tracing (Page 0, 1, 2, 3, 5)\r\n\
        ilrom         toggle tracing of Page 6 + 7\r\n\
        sysloop       toggle tracing of system loops\r\n\
        block [a1] [a2] block tracing of range between a1 and a2 (hex addresses 0000-FFFF)\r\n\
        block [n]     toggle tracing of designated block entry, n=0..15\r\n\
        block [Pn]    block Page n (n= hex 0..F)\r\n\
        block del [n] delete entry [n]
        block [no arg] show block entries\r\n\
        pass [a1] [a2] pass only tracing of range between a1 and a2 (hex addresses 0000-FFFF)\r\n\
        pass [n]      toggle tracing of designated pass entry, n=0..15\r\n\
        pass [Pn]     pass only tracing of Page n (n= hex 0..F)\r\n\
        pass [no arg] show pass entries\r\n\
        hpil          toggle HP-IL tracing to ILSCOPE USB port\r\n\
        pilbox        toggle PILBox serial tracing to ILSCOPE serial port\r\n"
*/

const char* __in_flash()tracer_cmds[] =
// list of arguments for the tracer command
// 
{
    "status",           // get status
    "trace",            // toggle tracer enable/disable
    "sysloop",          // toggle tracing of system loops
    "sysrom",           // toggle system rom tracing (Page 0, 1, 2, 3, 5)
    "ilrom",            // toggle tracing of Page 6 + 7
    "hpil",             // toggle HP-IL tracing
    "pilbox",           // toggle PILBox serial tracing
    "ilregs",           // toggle tracing of HP-IL registers
    "save",             // save tracer settings
};




void onTracerCLI(EmbeddedCli *cli, char *args, void *context)
{
    const char *arg1 = embeddedCliGetToken(args, 1);        // command
    // const char *arg2 = embeddedCliGetToken(args, 2);        // start address
    // const char *arg3 = embeddedCliGetToken(args, 3);        // end address
    int cmd = -1;
    int num_cmds = sizeof(tracer_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given, show the plugged ROM  
        cli_printf("no arguments given, use: tracer [command], see help");   
        uif_tracer(1); 
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, tracer_cmds[i]);
        i++;
    }

    if (cmd != 0) {
        i = -1;
    }

    if (i >= 0) {
        // cli_printf("argument %s, %d", arg1, i);
        uif_tracer(i);
    }
    else {
        cli_printf("invalid argument %s: use: tracer [command], see help", arg1);    // unknown command
    }

}

void onClearCLI(EmbeddedCli *cli, char *args, void *context) {
    cli_printf("\33[2J");           // clears the screen
    // cli_printf("\x1b[3J");       // clear buffer for TeraTerm
    // cli_printf("\x07");
    // cli_printf("\33[1,1H");          // move the cursor to the top of the screen (TeraTerm)
    
}

void onLed(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);

    int blinks = 5;
    int cmd = -1;
    int num_cmds = sizeof(system_cmds) / sizeof(char *);

    if (arg1 == NULL) {
        // no argument given, default blink is 5*
        // for testing just use the welcome message  
        uif_blink(blinks);
    }
    else
    {
        // check argument for a valid number
        int res = sscanf(arg1, "%d", &blinks);

        if ((res != 1) | (blinks > 9) | (blinks < 0))
        {
            // no valid result
            cli_printf("no valid input, input a number 1..9");
        }
        else
        {
            uif_blink(blinks);
        }
    } 
}

const char* __in_flash()flash_cmds[] =
// list of arguments for the flash command
// 
{
    "status",           // get status
    "dump",             // dump FLASH contents
    "INIT",             // initialize FLASH file system
    "NUKEALL",          // erase all FLASH pages
};

void onFlashCLI(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);

    int cmd = -1;
    uint32_t addr = 0;
    int num_cmds = sizeof(flash_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given, show status
        cli_printf("no arguments given, use: flash [command], see help");   
        uif_flash(1,0); 
        return;
    }

    if (arg2 != NULL) {
        // there is an arg2, used for a hex address in dump
        int res = sscanf(arg2, "%x", &addr);
        if (res != 1) {
            cli_printf("invalid address %s: address defaults to 0", arg2);    // invalid input
            addr = 0;
        }
    }

    if (arg2 == NULL) {
        addr = 0x40414243;      
    }   

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, flash_cmds[i]);
        i++;
    }

    if (cmd != 0) {
        i = -1;
    }

    if (i >= 0) {
        uif_flash(i, addr);
    }
    else {
        cli_printf("invalid argument %s: use: flash [command], see help", arg1);    // unknown command
    }
}


const char* __in_flash()fram_cmds[] =
// list of arguments for the flash command
// 
{
    "status",           // get status
    "dump",             // dump FLASH contents
    "INIT",             // initialize FLASH file system
    "NUKEALL",          // erase all FLASH pages
};

void onFramCLI(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);

    int cmd = -1;
    uint32_t addr = 0;
    int num_cmds = sizeof(flash_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no argument given, show status
        cli_printf("no arguments given, use: fram [command], see help");   
        uif_fram(1,0); 
        return;
    }

    if (arg2 != NULL) {
        // there is an arg2, used for a hex address in dump
        int res = sscanf(arg2, "%x", &addr);
        if (res != 1) {
            cli_printf("invalid address %s: address defaults to 0", arg2);    // invalid input
            addr = 0;
        }
    }

    if (arg2 == NULL) {
        addr = 0x40414243;      
    }   

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, fram_cmds[i]);
        i++;
    }

    if (cmd != 0) {
        i = -1;
    }

    if (i >= 0) {
        uif_fram(i, addr);
    }
    else {
        cli_printf("invalid argument %s: use: fram [command], see help", arg1);    // unknown command
    }
}


const char* __in_flash()list_cmds[] =
// list of arguments for the list command, but none used
// 
// the following command line options are possible:
// list [filename]      shows details of one file, or the first file starting with the filename
// list <no argument>   shows all files in the file system
// list <flash>/<fram>  shows all files in either FRAM or FLASH only
// list <ext>           extended listing with more details per file
// list <all>           list all files, including erased and dummy files

{
    "all",             // list all files, including erased and dummy files
    "ext",             // extended listing with more details per file
    "flash",           // list all files in FLASH
    "fram",            // list all files in FRAM
};

void onListCLI(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);

    int cmd = -1;
    uint32_t addr = 0;
    int num_cmds = sizeof(list_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no arguments given, show all files
        uif_list(-1, NULL); 
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, list_cmds[i]);
        i++;
    }
    
    if (cmd != 0) {                 // no valid argument found
        i = -1;
    }

    if (i >= 0) {
        uif_list(i, arg2);          // valid argument, arg2 may contain a string
    }

    if ((i == -1) && (arg2 == NULL)) {
        // no valid argument, assume arg1 is a filename
        uif_list(-1, arg1);         // no valid argument, assume it is a filename
    }

    if ((i == -1) && (arg2 != NULL)) {
        // no valid argument, and a filename is given
        // but we never get here, an invalid argument is seen as a filename to list
        cli_printf("invalid argument %s: use: list [all/ext/flash/fram] [filename]", arg1);    // unknown command	
    }

}

/*
#define RTC_HELP_TXT "RTC test functions\r\n\
        [no argument] shows the RTC status\r\n\
        status        shows the RTC status\r\n\
        set           set the RTC to the given date and time\r\n\
        get           get the current date and time from the RTC\r\n\
        test          test the RTC functions\r\n\
        dump          dump the RTC registers\r\n"

#define rtc_status      1
#define rtc_set         2
#define rtc_get         3    
#define rtc_reset       4
#define rtc_dump        5
#define rtc_display     6
*/


const char* __in_flash()rtc_cmds[] =
// list of arguments for the RTC command
// 
{
    "status",           // get status
    "set",              // set RTC date and time
    "get",              // get RTC date and time
    "reset",            // test RTC functions
    "dump",             // dump RTC registers
    "display",          // test the SSD1315 display
};  

void onRTCCLI(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);

    int cmd = -1;
    int num_cmds = sizeof(rtc_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no arguments given, show status
        cli_printf("no arguments given, use: rtc [command], see help");
        uif_rtc(rtc_status, NULL); 
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, rtc_cmds[i]);
        i++;
    }
    
    if (cmd != 0) {                 // no valid argument found
        i = -1;
    }

    if (i >= 0) {
        uif_rtc(i, arg2);          // valid argument, arg2 may contain a string for the set functions
                                    // just pass the string and let the uif_rtc function do the rest
        return;
    }

    if (i == -1) {
        // no valid argument
        cli_printf("invalid argument %s, see help", arg1);    // unknown command	
    }
}

/*
#define emulate_status    1
#define emulate_hpil      2
#define emulate_printer   3
#define emulate_xmem      4
#define emulate_blinky    5
#define emulate_timer     6
*/

const char* __in_flash()emulate_cmds[] =
// list of arguments for the RTC command
// 
{
    "status",        
    "hpil",          
    "printer",               
};  

void onEmulateCLI(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1);
    const char *arg2 = embeddedCliGetToken(args, 2);

    int cmd = -1;
    int num_cmds = sizeof(emulate_cmds) / sizeof(char *);

    if ((arg1 == NULL)) {
        // no arguments given, show status
        cli_printf("no arguments given, use: emulate [device], see help");
        uif_emulate(emulate_status); 
        return;
    }

    // scan the list of arguments for something known
    int i = 0;
    while (cmd != 0 && i < num_cmds) {
        cmd = strcmp(arg1, emulate_cmds[i]);
        i++;
    }
    
    if (cmd != 0) {                 // no valid argument found
        i = -1;
    }

    if (i >= 0) {
        uif_emulate(i);          // valid argument
        return;
    }

    if (i == -1) {
        // no valid argument
        cli_printf("invalid argument %s, see help", arg1);    // unknown command	
    }
}



// this routine receives one character from the CLI
void receiveCLIchar()
{
    if(cdc_available(ITF_CONSOLE))
    {
        // data is available in the console so read it
        // gpio_put(ONBOARD_LED, true); 
        int c = cdc_read_char(ITF_CONSOLE);
        tud_task();  // must keep the USB port updated
        embeddedCliReceiveChar(cli, c);
        // gpio_put(ONBOARD_LED, false);
    }
}

// send one character to the CLI CDC port
void writeCharToCLI(EmbeddedCli *embeddedCli, char c) 
{
    cdc_send_char(ITF_CONSOLE, c);
    cdc_flush(ITF_CONSOLE);
    tud_task();  // must keep the USB port updated
}

void runCLI()
{
    if ((cli != NULL) && (cdc_connected(ITF_CONSOLE)))
    {
        // only if the CLI is initialized and CDC interface is connected
        receiveCLIchar();
        embeddedCliProcess(cli);
        if (!firstconnect)
        {
            // firstconnect was false, so this is now a new CDC connection
            // display the welcome/status message
            firstconnect = true;
            uif_status();
        }
    }
    else
    // CLI CDC not connected
    {
        firstconnect = false;
    }
}

void initCliBinding() {
    // Define bindings as local variables, so we don't waste static memory

    EmbeddedCliConfig *config = embeddedCliDefaultConfig();
    config->cliBuffer = cliBuffer;
    config->cliBufferSize = CLI_BUFFER_SIZE;
    config->rxBufferSize = CLI_RX_BUFFER_SIZE;
    config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
    config->maxBindingCount = CLI_BINDING_COUNT;
    config->invitation = "TULIP> ";
    cli = embeddedCliNew(config);

    if (cli == NULL) {
        printf("\nCli was not created. Check sizes!\n");
        uint16_t size = embeddedCliRequiredSize(config);
        printf(" expected size = %d\n", size);
        // stdio_flush();
        sleep_ms(1000);
        return;
    }

    // binding for the system command
    CliCommandBinding system_binding = {
            .name = "system",
            .help = SYSTEM_HELP_TXT,
            // .help = "system help",
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onSystemCLI
    };    

    // Command binding for the clear command
    CliCommandBinding clear_binding = {
            .name = "clear",
            .help = "clears the console\n",
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onClearCLI
    };

    // Command binding for the led command
    CliCommandBinding led_binding = {
            .name = "blink",
            .help = "blink [b], blink the LED b times, just for testing and fun, 0 toggles the LED status\n",
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onLed
    };

    // Command binding for the dir command
    CliCommandBinding dir_binding = {
            .name = "dir",
            .help = "dir [subdir], shows uSD card directory\n",
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onDirCLI
    };

    // Command binding for the sdcard command
    CliCommandBinding sdcard_binding = {
            .name = "sdcard",
            .help = SDCARD_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onSDCardCLI
    };
    
    // Command binding for the plug command
    CliCommandBinding plug_binding = {
            .name = "plug",
            .help = PLUG_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onPlugCLI
    };

    // Command binding for the unplug command
    CliCommandBinding unplug_binding = {
            .name = "unplug",
            .help = UNPLUG_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onUnPlugCLI
    };

    // Command binding for the printer command
    CliCommandBinding printer_binding = {
            .name = "printer",
            .help = PRINTER_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onPrinterCLI
    };

        // Command binding for the printer command
    CliCommandBinding xmem_binding = {
            .name = "xmem",
            .help = XMEM_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onXMEMCLI
    };

    // Command binding for the tracer command
    CliCommandBinding tracer_binding = {
            .name = "tracer",
            .help = TRACER_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onTracerCLI
    };

    // Command binding for the flash command
    CliCommandBinding flash_binding = {
            .name = "flash",
            .help = FLASH_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onFlashCLI
    };

    // Command binding for the fram command
    CliCommandBinding fram_binding = {
            .name = "fram",
            .help = FRAM_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onFramCLI
    };
    // Command binding for the import command
    CliCommandBinding import_binding = {
            .name = "import",
            .help = IMPORT_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onImportCLI
    };

    // Command binding for the list command
    CliCommandBinding list_binding = {
            .name = "list",
            .help = LIST_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onListCLI
    };

    // Command binding for the rtc command
    CliCommandBinding rtc_binding = {
            .name = "rtc",
            .help = RTC_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onRTCCLI
    };

    // Command binding for the cat command
    CliCommandBinding cat_binding = {
            .name = "cat",
            .help = CAT_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onCatCLI
    };

    // Command binding for the emulate command
    CliCommandBinding emulate_binding = {
            .name = "emulate",
            .help = EMULATE_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onEmulateCLI
    };

    // Command binding for the delete command
    CliCommandBinding delete_binding = {
            .name = "delete",
            .help = DELETE_HELP_TXT,
            .tokenizeArgs = true,
            .context = NULL,
            .binding = onDeleteCLI
    };

    // Assign character write function
    cli->writeChar = writeCharToCLI;

    // EmbeddedCli *cli = getCliPointer();
    embeddedCliAddBinding(cli, system_binding);
    embeddedCliAddBinding(cli, sdcard_binding);   

    #if (TULIP_HARDWARE == T_MODULE)
        // RTC support only on the TULIP module version
        embeddedCliAddBinding(cli, rtc_binding);
    #endif  

    embeddedCliAddBinding(cli, clear_binding);
    embeddedCliAddBinding(cli, led_binding);
    embeddedCliAddBinding(cli, printer_binding);
    embeddedCliAddBinding(cli, tracer_binding);    
    embeddedCliAddBinding(cli, xmem_binding);    
    embeddedCliAddBinding(cli, flash_binding);    
    embeddedCliAddBinding(cli, fram_binding);    

    embeddedCliAddBinding(cli, dir_binding);
    embeddedCliAddBinding(cli, list_binding);
    embeddedCliAddBinding(cli, delete_binding);  
    embeddedCliAddBinding(cli, import_binding);

    embeddedCliAddBinding(cli, plug_binding);
    embeddedCliAddBinding(cli, unplug_binding);

    embeddedCliAddBinding(cli, cat_binding);
    embeddedCliAddBinding(cli, emulate_binding);

}