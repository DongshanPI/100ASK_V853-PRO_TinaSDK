/*
 * \file        tptester.c
 * \brief
 *
 * \version     1.0.0
 * \date        2012年06月24日
 * \author      James Deng <csjamesdeng@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */

/* Different platform tp_name
 *
 * r16_evb             "gt82x"
 * r40_m2ultraservers "ft5x"
 * r40_m2ultra        "ft5x"
 * r11_dh                "gt911_m785q6"
 * nuclear_dev           "gt82x"
 * r16_parrot          "gt82x"
 * r11_v3s               "gt911_m785q6"
 * r18_m64             "ft5x_ts"
 * r11_perf1             "gt911_m785q6"
 * r18_d1              "gsl_a86"
 * r18_pine64          "gt911_DB"
 * r16_spk             "gt82x"
 * r58_sch           "gsl1688_r58h720"
 */

#include <linux/input.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mjson.h"

#undef LOG_TAG
#define LOG_TAG "TpTester"

#ifndef EV_SYN
#define EV_SYN 0
#endif

char *events[EV_MAX + 1] = {
    [0 ... EV_MAX] = NULL,
    [EV_SYN] = "Sync",                          [EV_KEY] = "Key",
    [EV_REL] = "Relative",                      [EV_ABS] = "Absolute",
    [EV_MSC] = "Misc",                          [EV_LED] = "LED",
    [EV_SND] = "Sound",                         [EV_REP] = "Repeat",
    [EV_FF] = "ForceFeedback",                  [EV_PWR] = "Power",
    [EV_FF_STATUS] = "ForceFeedbackStatus",
};

char *keys[KEY_MAX + 1] = {
    [0 ... KEY_MAX] = NULL,
    [KEY_RESERVED] = "Reserved",                [KEY_ESC] = "Esc",
    [KEY_1] = "1",                              [KEY_2] = "2",
    [KEY_3] = "3",                              [KEY_4] = "4",
    [KEY_5] = "5",                              [KEY_6] = "6",
    [KEY_7] = "7",                              [KEY_8] = "8",
    [KEY_9] = "9",                              [KEY_0] = "0",
    [KEY_MINUS] = "Minus",                      [KEY_EQUAL] = "Equal",
    [KEY_BACKSPACE] = "Backspace",              [KEY_TAB] = "Tab",
    [KEY_Q] = "Q",                              [KEY_W] = "W",
    [KEY_E] = "E",                              [KEY_R] = "R",
    [KEY_T] = "T",                              [KEY_Y] = "Y",
    [KEY_U] = "U",                              [KEY_I] = "I",
    [KEY_O] = "O",                              [KEY_P] = "P",
    [KEY_LEFTBRACE] = "LeftBrace",              [KEY_RIGHTBRACE] = "RightBrace",
    [KEY_ENTER] = "Enter",                      [KEY_LEFTCTRL] = "LeftControl",
    [KEY_A] = "A",                              [KEY_S] = "S",
    [KEY_D] = "D",                              [KEY_F] = "F",
    [KEY_G] = "G",                              [KEY_H] = "H",
    [KEY_J] = "J",                              [KEY_K] = "K",
    [KEY_L] = "L",                              [KEY_SEMICOLON] = "Semicolon",
    [KEY_APOSTROPHE] = "Apostrophe",            [KEY_GRAVE] = "Grave",
    [KEY_LEFTSHIFT] = "LeftShift",              [KEY_BACKSLASH] = "BackSlash",
    [KEY_Z] = "Z",                              [KEY_X] = "X",
    [KEY_C] = "C",                              [KEY_V] = "V",
    [KEY_B] = "B",                              [KEY_N] = "N",
    [KEY_M] = "M",                              [KEY_COMMA] = "Comma",
    [KEY_DOT] = "Dot",                          [KEY_SLASH] = "Slash",
    [KEY_RIGHTSHIFT] = "RightShift",            [KEY_KPASTERISK] = "KPAsterisk",
    [KEY_LEFTALT] = "LeftAlt",                  [KEY_SPACE] = "Space",
    [KEY_CAPSLOCK] = "CapsLock",                [KEY_F1] = "F1",
    [KEY_F2] = "F2",                            [KEY_F3] = "F3",
    [KEY_F4] = "F4",                            [KEY_F5] = "F5",
    [KEY_F6] = "F6",                            [KEY_F7] = "F7",
    [KEY_F8] = "F8",                            [KEY_F9] = "F9",
    [KEY_F10] = "F10",                          [KEY_NUMLOCK] = "NumLock",
    [KEY_SCROLLLOCK] = "ScrollLock",            [KEY_KP7] = "KP7",
    [KEY_KP8] = "KP8",                          [KEY_KP9] = "KP9",
    [KEY_KPMINUS] = "KPMinus",                  [KEY_KP4] = "KP4",
    [KEY_KP5] = "KP5",                          [KEY_KP6] = "KP6",
    [KEY_KPPLUS] = "KPPlus",                    [KEY_KP1] = "KP1",
    [KEY_KP2] = "KP2",                          [KEY_KP3] = "KP3",
    [KEY_KP0] = "KP0",                          [KEY_KPDOT] = "KPDot",
    [KEY_ZENKAKUHANKAKU] = "Zenkaku/Hankaku",   [KEY_102ND] = "102nd",
    [KEY_F11] = "F11",                          [KEY_F12] = "F12",
    [KEY_RO] = "RO",                            [KEY_KATAKANA] = "Katakana",
    [KEY_HIRAGANA] = "HIRAGANA",                [KEY_HENKAN] = "Henkan",
    [KEY_KATAKANAHIRAGANA] = "Katakana/Hiragana",[KEY_MUHENKAN] = "Muhenkan",
    [KEY_KPJPCOMMA] = "KPJpComma",              [KEY_KPENTER] = "KPEnter",
    [KEY_RIGHTCTRL] = "RightCtrl",              [KEY_KPSLASH] = "KPSlash",
    [KEY_SYSRQ] = "SysRq",                      [KEY_RIGHTALT] = "RightAlt",
    [KEY_LINEFEED] = "LineFeed",                [KEY_HOME] = "Home",
    [KEY_UP] = "Up",                            [KEY_PAGEUP] = "PageUp",
    [KEY_LEFT] = "Left",                        [KEY_RIGHT] = "Right",
    [KEY_END] = "End",                          [KEY_DOWN] = "Down",
    [KEY_PAGEDOWN] = "PageDown",                [KEY_INSERT] = "Insert",
    [KEY_DELETE] = "Delete",                    [KEY_MACRO] = "Macro",
    [KEY_MUTE] = "Mute",                        [KEY_VOLUMEDOWN] = "VolumeDown",
    [KEY_VOLUMEUP] = "VolumeUp",                [KEY_POWER] = "Power",
    [KEY_KPEQUAL] = "KPEqual",                  [KEY_KPPLUSMINUS] = "KPPlusMinus",
    [KEY_PAUSE] = "Pause",                      [KEY_KPCOMMA] = "KPComma",
    [KEY_HANGUEL] = "Hanguel",                  [KEY_HANJA] = "Hanja",
    [KEY_YEN] = "Yen",                          [KEY_LEFTMETA] = "LeftMeta",
    [KEY_RIGHTMETA] = "RightMeta",              [KEY_COMPOSE] = "Compose",
    [KEY_STOP] = "Stop",                        [KEY_AGAIN] = "Again",
    [KEY_PROPS] = "Props",                      [KEY_UNDO] = "Undo",
    [KEY_FRONT] = "Front",                      [KEY_COPY] = "Copy",
    [KEY_OPEN] = "Open",                        [KEY_PASTE] = "Paste",
    [KEY_FIND] = "Find",                        [KEY_CUT] = "Cut",
    [KEY_HELP] = "Help",                        [KEY_MENU] = "Menu",
    [KEY_CALC] = "Calc",                        [KEY_SETUP] = "Setup",
    [KEY_SLEEP] = "Sleep",                      [KEY_WAKEUP] = "WakeUp",
    [KEY_FILE] = "File",                        [KEY_SENDFILE] = "SendFile",
    [KEY_DELETEFILE] = "DeleteFile",            [KEY_XFER] = "X-fer",
    [KEY_PROG1] = "Prog1",                      [KEY_PROG2] = "Prog2",
    [KEY_WWW] = "WWW",                          [KEY_MSDOS] = "MSDOS",
    [KEY_COFFEE] = "Coffee",                    [KEY_DIRECTION] = "Direction",
    [KEY_CYCLEWINDOWS] = "CycleWindows",        [KEY_MAIL] = "Mail",
    [KEY_BOOKMARKS] = "Bookmarks",              [KEY_COMPUTER] = "Computer",
    [KEY_BACK] = "Back",                        [KEY_FORWARD] = "Forward",
    [KEY_CLOSECD] = "CloseCD",                  [KEY_EJECTCD] = "EjectCD",
    [KEY_EJECTCLOSECD] = "EjectCloseCD",        [KEY_NEXTSONG] = "NextSong",
    [KEY_PLAYPAUSE] = "PlayPause",              [KEY_PREVIOUSSONG] = "PreviousSong",
    [KEY_STOPCD] = "StopCD",                    [KEY_RECORD] = "Record",
    [KEY_REWIND] = "Rewind",                    [KEY_PHONE] = "Phone",
    [KEY_ISO] = "ISOKey",                       [KEY_CONFIG] = "Config",
    [KEY_HOMEPAGE] = "HomePage",                [KEY_REFRESH] = "Refresh",
    [KEY_EXIT] = "Exit",                        [KEY_MOVE] = "Move",
    [KEY_EDIT] = "Edit",                        [KEY_SCROLLUP] = "ScrollUp",
    [KEY_SCROLLDOWN] = "ScrollDown",            [KEY_KPLEFTPAREN] = "KPLeftParenthesis",
    [KEY_KPRIGHTPAREN] = "KPRightParenthesis",  [KEY_F13] = "F13",
    [KEY_F14] = "F14",                          [KEY_F15] = "F15",
    [KEY_F16] = "F16",                          [KEY_F17] = "F17",
    [KEY_F18] = "F18",                          [KEY_F19] = "F19",
    [KEY_F20] = "F20",                          [KEY_F21] = "F21",
    [KEY_F22] = "F22",                          [KEY_F23] = "F23",
    [KEY_F24] = "F24",                          [KEY_PLAYCD] = "PlayCD",
    [KEY_PAUSECD] = "PauseCD",                  [KEY_PROG3] = "Prog3",
    [KEY_PROG4] = "Prog4",                      [KEY_SUSPEND] = "Suspend",
    [KEY_CLOSE] = "Close",                      [KEY_PLAY] = "Play",
    [KEY_FASTFORWARD] = "Fast Forward",         [KEY_BASSBOOST] = "Bass Boost",
    [KEY_PRINT] = "Print",                      [KEY_HP] = "HP",
    [KEY_CAMERA] = "Camera",                    [KEY_SOUND] = "Sound",
    [KEY_QUESTION] = "Question",                [KEY_EMAIL] = "Email",
    [KEY_CHAT] = "Chat",                        [KEY_SEARCH] = "Search",
    [KEY_CONNECT] = "Connect",                  [KEY_FINANCE] = "Finance",
    [KEY_SPORT] = "Sport",                      [KEY_SHOP] = "Shop",
    [KEY_ALTERASE] = "Alternate Erase",         [KEY_CANCEL] = "Cancel",
    [KEY_BRIGHTNESSDOWN] = "Brightness down",   [KEY_BRIGHTNESSUP] = "Brightness up",
    [KEY_MEDIA] = "Media",                      [KEY_UNKNOWN] = "Unknown",
    [BTN_0] = "Btn0",                           [BTN_1] = "Btn1",
    [BTN_2] = "Btn2",                           [BTN_3] = "Btn3",
    [BTN_4] = "Btn4",                           [BTN_5] = "Btn5",
    [BTN_6] = "Btn6",                           [BTN_7] = "Btn7",
    [BTN_8] = "Btn8",                           [BTN_9] = "Btn9",
    [BTN_LEFT] = "LeftBtn",                     [BTN_RIGHT] = "RightBtn",
    [BTN_MIDDLE] = "MiddleBtn",                 [BTN_SIDE] = "SideBtn",
    [BTN_EXTRA] = "ExtraBtn",                   [BTN_FORWARD] = "ForwardBtn",
    [BTN_BACK] = "BackBtn",                     [BTN_TASK] = "TaskBtn",
    [BTN_TRIGGER] = "Trigger",                  [BTN_THUMB] = "ThumbBtn",
    [BTN_THUMB2] = "ThumbBtn2",                 [BTN_TOP] = "TopBtn",
    [BTN_TOP2] = "TopBtn2",                     [BTN_PINKIE] = "PinkieBtn",
    [BTN_BASE] = "BaseBtn",                     [BTN_BASE2] = "BaseBtn2",
    [BTN_BASE3] = "BaseBtn3",                   [BTN_BASE4] = "BaseBtn4",
    [BTN_BASE5] = "BaseBtn5",                   [BTN_BASE6] = "BaseBtn6",
    [BTN_DEAD] = "BtnDead",                     [BTN_A] = "BtnA",
    [BTN_B] = "BtnB",                           [BTN_C] = "BtnC",
    [BTN_X] = "BtnX",                           [BTN_Y] = "BtnY",
    [BTN_Z] = "BtnZ",                           [BTN_TL] = "BtnTL",
    [BTN_TR] = "BtnTR",                         [BTN_TL2] = "BtnTL2",
    [BTN_TR2] = "BtnTR2",                       [BTN_SELECT] = "BtnSelect",
    [BTN_START] = "BtnStart",                   [BTN_MODE] = "BtnMode",
    [BTN_THUMBL] = "BtnThumbL",                 [BTN_THUMBR] = "BtnThumbR",
    [BTN_TOOL_PEN] = "ToolPen",                 [BTN_TOOL_RUBBER] = "ToolRubber",
    [BTN_TOOL_BRUSH] = "ToolBrush",             [BTN_TOOL_PENCIL] = "ToolPencil",
    [BTN_TOOL_AIRBRUSH] = "ToolAirbrush",       [BTN_TOOL_FINGER] = "ToolFinger",
    [BTN_TOOL_MOUSE] = "ToolMouse",             [BTN_TOOL_LENS] = "ToolLens",
    [BTN_TOUCH] = "Touch",                      [BTN_STYLUS] = "Stylus",
    [BTN_STYLUS2] = "Stylus2",                  [BTN_TOOL_DOUBLETAP] = "Tool Doubletap",
    [BTN_TOOL_TRIPLETAP] = "Tool Tripletap",    [BTN_GEAR_DOWN] = "WheelBtn",
    [BTN_GEAR_UP] = "Gear up",                  [KEY_OK] = "Ok",
    [KEY_SELECT] = "Select",                    [KEY_GOTO] = "Goto",
    [KEY_CLEAR] = "Clear",                      [KEY_POWER2] = "Power2",
    [KEY_OPTION] = "Option",                    [KEY_INFO] = "Info",
    [KEY_TIME] = "Time",                        [KEY_VENDOR] = "Vendor",
    [KEY_ARCHIVE] = "Archive",                  [KEY_PROGRAM] = "Program",
    [KEY_CHANNEL] = "Channel",                  [KEY_FAVORITES] = "Favorites",
    [KEY_EPG] = "EPG",                          [KEY_PVR] = "PVR",
    [KEY_MHP] = "MHP",                          [KEY_LANGUAGE] = "Language",
    [KEY_TITLE] = "Title",                      [KEY_SUBTITLE] = "Subtitle",
    [KEY_ANGLE] = "Angle",                      [KEY_ZOOM] = "Zoom",
    [KEY_MODE] = "Mode",                        [KEY_KEYBOARD] = "Keyboard",
    [KEY_SCREEN] = "Screen",                    [KEY_PC] = "PC",
    [KEY_TV] = "TV",                            [KEY_TV2] = "TV2",
    [KEY_VCR] = "VCR",                          [KEY_VCR2] = "VCR2",
    [KEY_SAT] = "Sat",                          [KEY_SAT2] = "Sat2",
    [KEY_CD] = "CD",                            [KEY_TAPE] = "Tape",
    [KEY_RADIO] = "Radio",                      [KEY_TUNER] = "Tuner",
    [KEY_PLAYER] = "Player",                    [KEY_TEXT] = "Text",
    [KEY_DVD] = "DVD",                          [KEY_AUX] = "Aux",
    [KEY_MP3] = "MP3",                          [KEY_AUDIO] = "Audio",
    [KEY_VIDEO] = "Video",                      [KEY_DIRECTORY] = "Directory",
    [KEY_LIST] = "List",                        [KEY_MEMO] = "Memo",
    [KEY_CALENDAR] = "Calendar",                [KEY_RED] = "Red",
    [KEY_GREEN] = "Green",                      [KEY_YELLOW] = "Yellow",
    [KEY_BLUE] = "Blue",                        [KEY_CHANNELUP] = "ChannelUp",
    [KEY_CHANNELDOWN] = "ChannelDown",          [KEY_FIRST] = "First",
    [KEY_LAST] = "Last",                        [KEY_AB] = "AB",
    [KEY_NEXT] = "Next",                        [KEY_RESTART] = "Restart",
    [KEY_SLOW] = "Slow",                        [KEY_SHUFFLE] = "Shuffle",
    [KEY_BREAK] = "Break",                      [KEY_PREVIOUS] = "Previous",
    [KEY_DIGITS] = "Digits",                    [KEY_TEEN] = "TEEN",
    [KEY_TWEN] = "TWEN",                        [KEY_DEL_EOL] = "Delete EOL",
    [KEY_DEL_EOS] = "Delete EOS",               [KEY_INS_LINE] = "Insert line",
    [KEY_DEL_LINE] = "Delete line",
};

char *absval[5] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat " };

char *relatives[REL_MAX + 1] = {
    [0 ... REL_MAX] = NULL,
    [REL_X] = "X",                              [REL_Y] = "Y",
    [REL_Z] = "Z",                              [REL_HWHEEL] = "HWheel",
    [REL_DIAL] = "Dial",                        [REL_WHEEL] = "Wheel",
    [REL_MISC] = "Misc",
};

char *absolutes[ABS_MAX + 1] = {
    [0 ... ABS_MAX] = NULL,
    [ABS_X] = "X",                              [ABS_Y] = "Y",
    [ABS_Z] = "Z",                              [ABS_RX] = "Rx",
    [ABS_RY] = "Ry",                            [ABS_RZ] = "Rz",
    [ABS_THROTTLE] = "Throttle",                [ABS_RUDDER] = "Rudder",
    [ABS_WHEEL] = "Wheel",                      [ABS_GAS] = "Gas",
    [ABS_BRAKE] = "Brake",                      [ABS_HAT0X] = "Hat0X",
    [ABS_HAT0Y] = "Hat0Y",                      [ABS_HAT1X] = "Hat1X",
    [ABS_HAT1Y] = "Hat1Y",                      [ABS_HAT2X] = "Hat2X",
    [ABS_HAT2Y] = "Hat2Y",                      [ABS_HAT3X] = "Hat3X",
    [ABS_HAT3Y] = "Hat3Y",                      [ABS_PRESSURE] = "Pressure",
    [ABS_DISTANCE] = "Distance",                [ABS_TILT_X] = "XTilt",
    [ABS_TILT_Y] = "YTilt",                     [ABS_TOOL_WIDTH] = "Tool Width",
    [ABS_VOLUME] = "Volume",                    [ABS_MISC] = "Misc",
    [ABS_MT_SLOT] = "Mt Slot",                  [ABS_MT_TOUCH_MAJOR] = "Mt Touch Major",
    [ABS_MT_TOUCH_MINOR] = "Mt Touch Minor",    [ABS_MT_WIDTH_MAJOR] = "Mt Width Major",
    [ABS_MT_WIDTH_MINOR] = "Mt Width Minor",    [ABS_MT_ORIENTATION] = "Mt Orientation",
    [ABS_MT_POSITION_X] = "Mt Position X",      [ABS_MT_POSITION_Y] = "Mt Position Y",
    [ABS_MT_TOOL_TYPE] = "Mt Tool Type",        [ABS_MT_BLOB_ID] = "Mt Blob Id",
    [ABS_MT_TRACKING_ID] = "Mt Tracking Id",    [ABS_MT_PRESSURE] = "Mt Pressure",
    [ABS_MT_DISTANCE] = "Mt Distance",          [ABS_MT_TOOL_X] = "Mt Tool X",
    [ABS_MT_TOOL_Y] = "Mt Tool Y",
};

char *misc[MSC_MAX + 1] = {
    [ 0 ... MSC_MAX] = NULL,
    [MSC_SERIAL] = "Serial",                    [MSC_PULSELED] = "Pulseled",
    [MSC_GESTURE] = "Gesture",                  [MSC_RAW] = "RawData",
    [MSC_SCAN] = "ScanCode",
};

char *leds[LED_MAX + 1] = {
    [0 ... LED_MAX] = NULL,
    [LED_NUML] = "NumLock",                     [LED_CAPSL] = "CapsLock",
    [LED_SCROLLL] = "ScrollLock",               [LED_COMPOSE] = "Compose",
    [LED_KANA] = "Kana",                        [LED_SLEEP] = "Sleep",
    [LED_SUSPEND] = "Suspend",                  [LED_MUTE] = "Mute",
    [LED_MISC] = "Misc",
};

char *repeats[REP_MAX + 1] = {
    [0 ... REP_MAX] = NULL,
    [REP_DELAY] = "Delay",                      [REP_PERIOD] = "Period"
};

char *sounds[SND_MAX + 1] = {
    [0 ... SND_MAX] = NULL,
    [SND_CLICK] = "Click",                      [SND_BELL] = "Bell",
    [SND_TONE] = "Tone"
};

char **names[EV_MAX + 1] = {
    [0 ... EV_MAX] = NULL,
    [EV_SYN] = events,                          [EV_KEY] = keys,
    [EV_REL] = relatives,                       [EV_ABS] = absolutes,
    [EV_MSC] = misc,                            [EV_LED] = leds,
    [EV_SND] = sounds,                          [EV_REP] = repeats,
};

#define BITS_PER_LONG                   (sizeof(long) * 8)
#define NBITS(x)                        ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)                          ((x)%BITS_PER_LONG)
#define BIT(x)                          (1UL<<OFF(x))
#define LONG(x)                         ((x)/BITS_PER_LONG)
#define test_bit(bit, array)            ((array[LONG(bit)] >> OFF(bit)) & 1)

static int match_keyboard(char *dev_name, char *tp_name)
{
    return strncmp(dev_name, tp_name, strlen(tp_name));
}

int main(int argc, char *argv[])
{
    char dev_path[64];
    char evt_path[32];
    int fd;
    int version;
    unsigned short id[4];
    char name[32];
    unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
    int i, j, k;
    int abs[5];
    struct input_event evt[64];
    int rd;
    char pre_key[64];
    int key_enable;
    FILE *volume_pipe = NULL;
    char volume_key[16];
    char *tp_name;

    int touch_times;
    int start_times=0;
	int event_number=0;


    if((tp_name = mjson_fetch_string("/base/production/tptester/tp_name")) != NULL)
    {
        printf("\ttp_name = %s\n",tp_name);
    }
    else
    {
        printf("\tmjson_fetch_string return NULL\n");
        return 1;
    }

    if((touch_times = mjson_fetch_int("/base/production/tptester/touch_times")) != -1)
    {
        printf("\ttouch_times = %d\n",touch_times);
    }
    else
    {
        printf("\tmjson_fetch_int return %d\n",touch_times);
        return 1;
    }

	while(1) {
		snprintf(dev_path, sizeof(dev_path),
				"/sys/class/input/input%d/name", event_number);
		if (access(dev_path, F_OK) != 0)
			return -1;
		fd = open(dev_path, O_RDONLY);
		if (fd < 0) {
		    printf("can't open %s(%s)\n", dev_path, strerror(errno));
		    return errno;
		}
		read(fd, evt_path, sizeof(evt_path));
		if (match_keyboard(evt_path, tp_name) == 0) {
		    close(fd);
		    break;
		}
		close(fd);
		event_number++;
	}


    snprintf(evt_path, sizeof(evt_path), "/dev/input/event%d", event_number);
    if ((fd = open(evt_path, O_RDONLY)) < 0) {
        printf("can't open %s(%s)\n", evt_path, strerror(errno));
        return errno;
    }

    if (ioctl(fd, EVIOCGVERSION, &version)) {
        printf("can't get version\n");
        return 1;
    }
    printf("Input driver version is %d.%d.%d\n",version >> 16, (version >> 8) & 0xff, version & 0xff);

    ioctl(fd, EVIOCGID, id);
    printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n", id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    printf("Input device name: \"%s\"\n", name);

    memset(bit, 0, sizeof(bit));

    ioctl(fd, EVIOCGBIT(0, sizeof(bit[0])), bit[0]);
    printf("Supported events:\n");
    for (i = 0; i < EV_MAX; i++) {
        if (test_bit(i, bit[0])) {
            printf("    Event type %d (%s)\n", i, events[i] ? events[i] : "?");
            if (!i)
                continue;

            ioctl(fd, EVIOCGBIT(i, sizeof(bit[0])), bit[i]);

            for (j = 0; j < KEY_MAX; j++) {
                if (test_bit(j, bit[i])) {
                    printf("        Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
                    if (i == EV_ABS) {
                        ioctl(fd, EVIOCGABS(j), abs);
                        for (k = 0; k < 5; k++) {
                            if ((k < 3) || abs[k])
                                printf("%s %6d\n", absval[k], abs[k]);
                        }
                    }
                }
            }
        }
    }

    while ( start_times < touch_times*2 ) {
        rd = read(fd, evt, sizeof(struct input_event) * 64);

        if (rd < (int)sizeof(struct input_event)) {
            printf("error in reading\n");
            return 1;
        }

        for (i = 0; i < rd / sizeof(struct input_event); i++) {
            if (evt[i].type == EV_SYN) {
                ;
            }
            else if (evt[i].type == EV_MSC && (evt[i].code == MSC_RAW || evt[i].code == MSC_SCAN)) {
                     printf("%s, %s(%d, %d)\n",
                     events[evt[i].type] ? events[evt[i].type] : "?",
                     names[evt[i].type] ? (names[evt[i].type][evt[i].code] ? names[evt[i].type][evt[i].code] : "?") : "?",
                     evt[i].code,
                     evt[i].value);

                     if (evt[i].value)
                     continue;
                 }
                 else {
                     printf("%s, %s(%d, %d)\n",
                     events[evt[i].type] ? events[evt[i].type] : "?",
                     names[evt[i].type] ? (names[evt[i].type][evt[i].code] ? names[evt[i].type][evt[i].code] : "?") : "?",
                     evt[i].code,
                     evt[i].value);
                 }
        }
        printf("*********************************\n");
        start_times++;
    }
    return 0;
}
