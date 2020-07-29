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
#include <easyx.h>
#include <time.h>

using namespace std;

#define INPUT_BUFFER_SIZE 100
#define OUTPUT_BUFFER_SIZE 0
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

#define STRING_MAX 80 /* used for console input */

#define DATA_MAX 500
#define PARTICLE_MAX 10000

#define NT_PRESSED 1
#define NT_RELEASED 2
#define NT_NONE 0

HWND desktop;
RECT desktopr;
int wide_graph, high_graph;
float wide_line;

int latency = 5;
int speed = 3;
int timer_max = 1;
int timer = 0;

bool glow_on = true;
int glow_size;
int glow_color_minus;

bool particle_on = true;
int num_particle_tick = 1;

IMAGE icon;

struct PARTICLE {
	int size;//pixels
	int speed;// 10 pixels per tick
	int original_speed;//10 pixels per tick
	int opacity;//[0,255]
	int original_opacity;//[0,255]
	int life;//tick
	int life_max;//tick
	int birth_position;//[-pixel,pixel]
	int x;
	int y;
	bool status;
};
PARTICLE data_p[PARTICLE_MAX];
int op_p, cl_p,op_p1,cl_p1;
int life_max = 150; // better no more than 200
int particle_size = 2;
int particle_opacity = 200;
int particle_speed = 50;//10 pixels
float vs_particle_opacity;

struct nodec {
	int r;
	int g;
	int b;
};

struct nodes {
	char name[STRING_MAX];
	bool up;
	nodec fcolor;
	nodec bcolor;

};

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
		op1 %= DATA_MAX;
		if (data_n[op1].status == NT_NONE) {
			op++;
			op %= DATA_MAX;
			clean_data();
		}
	}
	//cout << "\nop=" << op << "	cl=" << cl;
	return;
}

void new_particle(int key) {

	cl_p++;
	cl_p %= PARTICLE_MAX;

	data_p[cl_p].birth_position = rand() % glow_size*(rand() % 2 * 2 - 1);
	data_p[cl_p].x = note.position[key] + data_p[cl_p].birth_position;
	data_p[cl_p].y = rand()%(glow_size/2+1);
	data_p[cl_p].life_max = rand() % (life_max / 8 + 1) * (rand() % 2 * 2 - 1) + life_max;
	data_p[cl_p].life = 0;
	data_p[cl_p].size = rand() % (particle_size / 5 + 1)*(rand() % 2 * 2 - 1) + particle_size;
	data_p[cl_p].original_speed = rand() % (particle_speed / 4 + 1)*(rand() % 2 * 2 - 1) + particle_speed;
	data_p[cl_p].speed = data_p[cl_p].original_speed;
	data_p[cl_p].original_opacity = (rand() % (particle_opacity / 5 + 1)*(rand() % 2 * 2 - 1) + particle_opacity)%256;
	data_p[cl_p].opacity = data_p[cl_p].original_opacity;
	data_p[cl_p].status = true;
	return;
}

void clean_particle() {
	op_p1 = op_p;
	cl_p1 = cl_p;

	if (op_p1 != cl_p1) {
		op_p1++;
		op_p1 %= PARTICLE_MAX;
		if (data_p[op_p1].life >= data_p[op_p1].life_max || data_p[op_p1].status == false) {
			data_p[op_p1].status = false;
			op_p++;
			op_p %= PARTICLE_MAX;
			clean_particle();
		}
	}
	return;
}

void refresh_particle() {
	
	cl_p1 = cl_p;
	op_p1 = op_p;

	while (op_p1 != cl_p1) {

		op_p1++;
		op_p1 %= PARTICLE_MAX;

		if (data_p[op_p1].status == true && data_p[op_p1].life <= data_p[op_p1].life_max) {
			data_p[op_p1].life++;
			
			data_p[op_p1].speed = (int)(((float)1-(float)data_p[op_p1].life / data_p[op_p1].life_max) * data_p[op_p1].original_speed);
			data_p[op_p1].y += (int)((float)data_p[op_p1].speed/10*((float)1-(float)abs(data_p[op_p1].birth_position)/note.wide/2))+1;
			data_p[op_p1].x += (int)((float)data_p[op_p1].speed / 10 * ((float)data_p[op_p1].birth_position / note.wide)*(float)(data_p[op_p1].life+100)/(data_p[op_p1].life_max+100)*3);
			data_p[op_p1].opacity = abs((int)(data_p[op_p1].original_opacity - vs_particle_opacity*data_p[op_p1].life*data_p[op_p1].life));
			setfillcolor(RGB(data_p[op_p1].opacity/4, data_p[op_p1].opacity, data_p[op_p1].opacity/4));
			setlinecolor(RGB(data_p[op_p1].opacity/4, data_p[op_p1].opacity, data_p[op_p1].opacity/4));
			setlinestyle(PS_SOLID | PS_ENDCAP_ROUND, 0);
			fillcircle(data_p[op_p1].x, data_p[op_p1].y, data_p[op_p1].size);
		}
	}

	setlinestyle(PS_SOLID | PS_ENDCAP_ROUND, (int)note.wide);
	setlinecolor(RGB(0, 255, 0));
	
	return;
}

void refresh() {
	cleardevice();
	
	//particle refresh
	if (particle_on == true) {
		clean_particle();
		refresh_particle();
	}

	//notes
	for (int i = 0; i < 88; ++i) {
		if (pressing_note[i].status == NT_PRESSED) {
			//glow
			if (glow_on == true) {
				for (int j = glow_size; j >= 0; --j) {
					setfillcolor(RGB((glow_size - j)*glow_color_minus/10, (glow_size - j)*glow_color_minus, (glow_size - j)*glow_color_minus / 10));
					setlinecolor(RGB((glow_size - j)*glow_color_minus / 10, (glow_size - j)*glow_color_minus, (glow_size - j)*glow_color_minus / 10));
					fillellipse(note.position[i] - j, -glow_size / 3 - j, note.position[i] + j, glow_size / 3 + j);
				}
				setlinecolor(RGB(0, 255, 0));
			}
			//add particle
			if (particle_on == true) {
				for (int j = 0; j < num_particle_tick; ++j) {
					new_particle(i);
				}
			}

			//draw notes
			pressing_note[i].y += speed;
			line(note.position[i], pressing_note[i].y, note.position[i], 0);
			//cout << "\nprint(new): " << i;
		}
	}
	op1 = op;
	cl1 = cl;
	while (op1 != cl1) {
		op1++;
		op1 %= DATA_MAX;
		if (data_n[op1].status == NT_RELEASED) {
			data_n[op1].endy += speed;
			data_n[op1].starty += speed;
			line(note.position[data_n[op1].key], data_n[op1].starty, note.position[data_n[op1].key], data_n[op1].endy);
			//setlinecolor(BLACK);
			//line(note.position[data_n[op1].key], data_n[op1].endy, note.position[data_n[op1].key], 0);
			//setlinecolor(RGB(0,255,0));
			//cout << "\nprint: " << data_n[op1].key;
			if (data_n[op1].endy >= high_graph) {
				data_n[op1].status = NT_NONE;
			}
		}
	}
	
	
	clean_data();
	FlushBatchDraw();
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

void show_start() {
	loadimage(&icon, "res/icon_big.jpg");
	RECT r = { 350,high_graph / 2 - 100,wide_graph,high_graph / 2 + 100 };
	LOGFONT font;
	gettextstyle(&font);
	font.lfHeight = 150;
	_tcscpy_s(font.lfFaceName, _T("Edwardian Script ITC"));
	font.lfQuality = ANTIALIASED_QUALITY;
	settextstyle(&font);
	/*
	BeginBatchDraw();
	for (int i = 0; i < wide/2-200; i+=3) {
	cleardevice();
	putimage(wide/2 - 200 - i, high / 2 - 200, 400, 400, &icon, 0, 0);
	Sleep(1);
	FlushBatchDraw();
	}
	EndBatchDraw();
	*/
	putimage(50, high_graph / 2 - 150, 300, 300, &icon, 0, 0);
	drawtext(_T("Visual Notes"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	//putimage(0, high / 2 - 200, 200, 200, &icon, 0, 0);
	FlushBatchDraw();
	Sleep(2000);
	cleardevice();
	
	return;
}


void main_function() {
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
	setfillstyle(BS_SOLID);//更改填充模式为纯色（固体）填充
	//setfillcolor(RGB(0,255,0));//更改填充颜色为绿色
	setlinestyle(PS_SOLID | PS_ENDCAP_ROUND, (int)note.wide);//更改画线模式为实线，两端平坦，宽度为预定宽度
	setlinecolor(RGB(0,255,0));//更改画线颜色为绿色
	BeginBatchDraw();//开始批量绘制，以防止出现闪烁

	srand(time(NULL));
	vs_particle_opacity = (float)particle_opacity / (life_max*life_max);

	//hide caption
	long WindowStyle = GetWindowLong(hwnd_graph, GWL_STYLE);
	WindowStyle &= ~WS_CAPTION;
	SetWindowLong(hwnd_graph, GWL_STYLE, WindowStyle);
	//move to right place
	MoveWindow(hwnd_graph, 0, 0, wide_graph, high_graph, false);

	show_start();

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

				vs = (int)Pm_MessageStatus(buffer[0].message);
				if (vs == 128 || vs == 144) {
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
				}
            } else {
                assert(0);
            }
        }
		/*
		timer++;
		if (timer >= timer_max) {
			timer %= timer_max;
			refresh();
		}
		*/
		refresh();
		Sleep(latency);
    }

    /* close device (this not explicitly needed in most implementations) */
    printf("ready to close...");

    Pm_Close(midi);
    printf("done closing...");
}

void read_set() {
	char rubbish[STRING_MAX];
	freopen("setting.vndat", "r", stdin);
	scanf("%d", &glow_on);
	scanf("%d", &particle_on);
	scanf("%d", &speed);
	scanf("%d", &latency);

	freopen("CON", "r", stdin);
}





int main(){

    int default_in;
    int i = 0, n = 0;
    char line[STRING_MAX];
	int test_input = 1;
	
	desktop = GetDesktopWindow();
	GetWindowRect(desktop, &desktopr);
	wide_graph = desktopr.right - desktopr.left;
	high_graph = desktopr.bottom - desktopr.top;
	cout << "Desktop size : " << wide_graph << " x " << high_graph << "\n";

	note = get_note_wide(wide_graph);
	glow_size = (int)(note.wide / 2);
	glow_color_minus = 255 / glow_size + 1;
	cout << "Note width : " << note.wide << "\n";
    
    /* list device information */
	cout << "Select your input device :\n";
    default_in = Pm_GetDefaultInputDeviceID();
    //default_out = Pm_GetDefaultOutputDeviceID();
    for (i = 0; i < Pm_CountDevices(); i++) {
        char *deflt;
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		
		if (info->input) {
			printf("%d: %s, %s", i, info->interf, info->name);
			deflt = (i == default_in ? "default " : "");
			printf(" (%sinput)", deflt);
		}
		printf("\n");
    }

	main_function();
    
	cout << "Going to exit in 1 second...\n";
	Sleep(1000);
    return 0;
}