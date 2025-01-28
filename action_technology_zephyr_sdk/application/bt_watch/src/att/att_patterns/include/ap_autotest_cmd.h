#ifndef _AP_AUTOTEST_CMD_H
#define _AP_AUTOTEST_CMD_H

//ATT工具命令字
typedef enum
{
    STUB_CMD_ATT_START                      = 0x0400,
    STUB_CMD_ATT_STOP                       = 0x0401,
    STUB_CMD_ATT_PAUSE                      = 0x0402,
    STUB_CMD_ATT_RESUME                     = 0x0403,
    STUB_CMD_ATT_GET_TEST_ID                = 0x0404,
    STUB_CMD_ATT_GET_TEST_ARG               = 0x0405,
    STUB_CMD_ATT_REPORT_TRESULT             = 0x0406,
    STUB_CMD_ATT_PRINT_LOG                  = 0x0407,
    STUB_CMD_ATT_FOPEN                      = 0x0408,
    STUB_CMD_ATT_FREAD                      = 0x0409,

    STUB_CMD_ATT_REPORT_PARA                = 0x040C,
    STUB_CMD_ATT_GET_TEST_RESULT            = 0x040D,
    STUB_CMD_ATT_INFORM_STATE               = 0x040E,
}ATT_COMMAND_E;


#endif

