#include "portmidi.h"
#include "porttime.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include <cstdio>
#include <Windows.h>
#include <iostream>
#include <graphics.h>

using namespace std;

#define INPUT_BUFFER_SIZE 100
#define OUTPUT_BUFFER_SIZE 0
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

#define STRING_MAX 80 /* used for console input */

#define DATA_MAX 1000

#define NT_PRESSED 1
#define NT_RELEASED 2
#define NT_NONE 0

HWND desktop;
RECT desktopr;
int wide_graph, high_graph;
float wide_line;

int latency = 0;
int speed = 10;
int timer_max = 6;
int timer = 0;

struct noden {
	float wide;
	int position[89];
};
noden note;

struct nodend {
	int y;
	int status;
};

nodend pressing_note[89];

struct noded {
	int key;
	int starty;
	int endy;
	int status;
};
noded data_n[DATA_MAX];
int cl = 0,op = 0,op1,cl1;

/*
struct nodend {
	int starty = 0;
	int endy = 0;
	int status = NT_NONE;
};
nodend note_data[100];
*/
noden get_note_wide(int x) {
	noden notea;
	notea.wide = (float)x / 89;
	for (int i = 0; i < 88; ++i) {
		notea.position[i] = (int)(notea.wide*(i + 0.5));
	}
	return notea;
}

void add_to_data(int key, int y) {
	cl++;
	cl %= DATA_MAX;
	data_n[cl].key = key;
	data_n[cl].status = NT_RELEASED;
	data_n[cl].starty = pressing_note[key].y;
	data_n[cl].endy = 0;
	pressing_note[key].y = 0;
	//cout << "\nadd to data: " << key;
	return;
}

void clean_data() {
	op1 = op;
	cl1 = cl;
	if (op1 != cl1) {
		op1++;
		if (data_n[op1].status == NT_NONE) {
			op++;
			clean_data();
		}
	}
	//cout << "\nop=" << op << "	cl=" << cl;
	return;
}

void refresh() {
	//cleardevice();
	op1 = op;
	cl1 = cl;
	while (op1 != cl1) {
		op1++;
		op1 %= DATA_MAX;
		if (data_n[op1].status == NT_RELEASED) {
			data_n[op1].endy += speed;
			data_n[op1].starty += speed;
			line(note.position[data_n[op1].key], data_n[op1].starty, note.position[data_n[op1].key], data_n[op1].endy);
			setlinecolor(BLACK);
			line(note.position[data_n[op1].key], data_n[op1].endy, note.position[data_n[op1].key], 0);
			setlinecolor(GREEN);
			//cout << "\nprint: " << data_n[op1].key;
			if (data_n[op1].endy >= high_graph) {
				data_n[op1].status = NT_NONE;
			}
		}
	}
	for (int i = 0; i < 88; ++i) {
		if (pressing_note[i].status == NT_PRESSED) {
			pressing_note[i].y += speed;
			line(note.position[i], pressing_note[i].y, note.position[i], 0);
			//cout << "\nprint(new): " << i;
		}
	}
	clean_data();
	return;
}


/*
void refresh_note_data(nodend notedata[100],int speed) {
	for (int i = 0; i < 88; ++i) {
		switch (notedata[i].status){

		case NT_NONE:
			break;
		case NT_PRESSED:
			notedata[i].starty += speed;
			break;
		case NT_RELEASED:
			notedata[i].starty += speed;
			notedata[i].endy += speed;
			if (notedata[i].endy > wide_graph)
			break;
		default:
			break;
		}
	}
	return;
}
*/
/*
void refresh_graph(nodend notedata[100], noden note) {
	cleardevice();
	for (int i = 0; i < 88; ++i) {

	}
}
*/
/* read a number from console */
int get_number(char *prompt)
{
    char line[STRING_MAX];
    int n = 0, i;
    printf(prompt);
    while (n != 1) {
        n = scanf("%d", &i);
        fgets(line, STRING_MAX, stdin);

    }
    return i;
}



void main_test_input() {
    PmStream * midi;
    PmError status, length;
    PmEvent buffer[1];
    int i = get_number("Type input number: ");
    
    TIME_START;

    /* open input device */
    Pm_OpenInput(&midi, 
                 i,
                 DRIVER_INFO, 
                 INPUT_BUFFER_SIZE, 
                 TIME_PROC, 
                 TIME_INFO);

    printf("Midi Input opened. Reading Midi messages...\n");
    Pm_SetFilter(midi, PM_FILT_ACTIVE | PM_FILT_CLOCK | PM_FILT_SYSEX);
    /* empty the buffer after setting filter, just in case anything
       got through */
    while (Pm_Poll(midi)) {
        Pm_Read(midi, buffer, 1);
    }

	cout << "Launching easyx graphics window\n";
	HWND hwnd_graph;
	hwnd_graph = initgraph(wide_graph, high_graph, EW_SHOWCONSOLE);//打开图形窗口并获取句柄（显示控制台）
	setbkmode(OPAQUE);//设置背景色填充而非透明背景色
	setbkcolor(BLACK);//设置背景色为黑色
	cleardevice();//更新图像以填充背景色
	//setfillstyle(BS_SOLID);//更改填充模式为纯色（固体）填充
	//setfillcolor(GREEN);//更改填充颜色为绿色
	setlinestyle(PS_SOLID | PS_ENDCAP_FLAT, (int)note.wide);//更改画线模式为实线，两端平坦，宽度为预定宽度
	setlinecolor(GREEN);//更改画线颜色为绿色
	int vs;
    /* now start paying attention to messages */
    while (1) {
        status = Pm_Poll(midi);
        if (status == TRUE) {
            length = (PmError)Pm_Read(midi,buffer, 1);
            if (length > 0) {
				/*
                printf("Got message %d: time %ld, %2lx %2lx %2lx\n",
                       i,
                       (long) buffer[0].timestamp,
                       (long) Pm_MessageStatus(buffer[0].message),
                       (long) Pm_MessageData1(buffer[0].message),
                       (long) Pm_MessageData2(buffer[0].message));
                i++;
				*/
				vs = (int)Pm_MessageData1(buffer[0].message);
				vs -= 21;
				if ((int)Pm_MessageData2(buffer[0].message) == 0) {
					//note_data[vs].status = NT_RELEASED;
					pressing_note[vs].status = NT_RELEASED;
					add_to_data(vs, pressing_note[vs].y);
					//cout << "\n" << vs << " released";

				}
				else {
					//note_data[vs].starty = NT_PRESSED;
					pressing_note[vs].status = NT_PRESSED;
					//cout << "\n" << vs << "pressed";
				}
            } else {
                assert(0);
            }
        }
		timer++;
		if (timer >= timer_max) {
			timer %= timer_max;
			refresh();
		}
		Sleep(latency);
    }

    /* close device (this not explicitly needed in most implementations) */
    printf("ready to close...");

    Pm_Close(midi);
    printf("done closing...");
}





int main(){

	cout << "Loading\n";
    int default_in;
    int default_out;
    int i = 0, n = 0;
    char line[STRING_MAX];
	int test_input = 1;
	latency = 3;
	speed = 10;
	

	desktop = GetDesktopWindow();
	GetWindowRect(desktop, &desktopr);
	wide_graph = desktopr.right - desktopr.left;
	high_graph = desktopr.bottom - desktopr.top;
	cout << "Desktop size : " << wide_graph << " x " << high_graph << "\n";

	note = get_note_wide(wide_graph);
	cout << "Note width : " << note.wide << "\n";
	/*
	for (int i = 0; i < 88; ++i) {
		cout << note.position[i] << "	";
	}
	cout << "\n";
	*/
    
    /* list device information */
	cout << "Select your input device :\n";
    default_in = Pm_GetDefaultInputDeviceID();
    default_out = Pm_GetDefaultOutputDeviceID();
    for (i = 0; i < Pm_CountDevices(); i++) {
        char *deflt;
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		printf("%d: %s, %s", i, info->interf, info->name);
		if (info->input) {
			deflt = (i == default_in ? "default " : "");
			printf(" (%sinput)", deflt);
		}
		printf("\n");
    }

	main_test_input();
    
	cout << "Going to exit in 1 second...\n";
	Sleep(1000);
    return 0;
}