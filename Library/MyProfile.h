#pragma once

#include "Windows.h"

#define PROFILE_ARRAY_SIZE 50
#define PROFILE_NAME_SIZE 32
#define DEFAULT_MIN 0x7fff
#define DEFAULT_MAX 0

#define PROFILE_BEGIN(Name) profileBegin(Name);
#define PROFILE_END(Name) profileEnd(Name);


void profileBegin(const char *Name);
void profileEnd(const char *Name);
void printProfile(const char *Name);

struct Profile {
	bool flag;
	char name[PROFILE_NAME_SIZE];
	__int64 total;
	__int64 origin;
	__int64 min[2];
	__int64 max[2];
	__int64 call;
};

LARGE_INTEGER g_frequency;
Profile g_profileArray[PROFILE_ARRAY_SIZE];
double g_milliseconds = 1000.0f;
double g_microsecond = 1000000.0f;

class Init {
public:
	Init() {
		QueryPerformanceFrequency(&g_frequency);

		for (int icnt = 0; icnt < PROFILE_ARRAY_SIZE; icnt++) {
			g_profileArray[icnt].flag = false;
			g_profileArray[icnt].total = 0;
			g_profileArray[icnt].origin = 0;
			g_profileArray[icnt].min[0] = DEFAULT_MIN;
			g_profileArray[icnt].max[0] = DEFAULT_MAX;
			g_profileArray[icnt].call = 0;
		}
	}
	~Init() {

	}

};

Init profile;

//시작
void profileBegin(const char *Name) {
	LARGE_INTEGER time;
	Profile *pro = NULL;
	//배열 순회
	for (int i = 0; i < PROFILE_ARRAY_SIZE; i++) {
		if (g_profileArray[i].flag) {	//비어있지않으면 이름체크
			if (!strcmp(g_profileArray[i].name, Name)) {
				pro = &g_profileArray[i];
				break;
			}
			else {	//빈 배열이 없음
				if (i >= PROFILE_ARRAY_SIZE - 1) {
					throw;
				}
			}
		}
		else {	//비어있으면 바로
			pro = &g_profileArray[i];
			break;
		}
	}
	//중복으로 호출됬으면 에러
	if (pro->origin != 0) {
		throw;
	}
	//값 셋
	pro->flag = true;
	strcpy_s(pro->name, PROFILE_NAME_SIZE, Name);
	QueryPerformanceCounter(&time);
	pro->origin = time.QuadPart;
}

//끝
void profileEnd(const char *Name) {
	LARGE_INTEGER time;
	//바로 시간 체크
	QueryPerformanceCounter(&time);

	Profile *pro = NULL;
	//배열 순회
	for (int i = 0; i < PROFILE_ARRAY_SIZE; i++) {
		if (g_profileArray[i].flag) {	//비어있지않으면 이름체크
			if (!strcmp(g_profileArray[i].name, Name)) {
				pro = &g_profileArray[i];
				break;
			}
			else {	//빈 배열이 없음
				if (i >= PROFILE_ARRAY_SIZE - 1) {
					throw;
				}
			}
		}
		else {	//이름이 없으면 에러
			throw;
		}
	}
	//중복으로 호출됬으면 에러
	if (pro->origin == 0) {
		throw;
	}
	//값 셋
	pro->origin = time.QuadPart - pro->origin;
	pro->total = pro->total + pro->origin;
	//최소값
	if (pro->origin < pro->min[0]) {
		pro->min[1] = pro->min[0];
		pro->min[0] = pro->origin;
	}
	//최대값
	if (pro->origin > pro->max[0]) {
		pro->max[1] = pro->max[0];
		pro->max[0] = pro->origin;
	}
	pro->call = pro->call + 1;
	//printf("%11.4f \n", (pro->origin / (double)g_frequency.QuadPart) * g_microsecond);
	pro->origin = 0;
}

//출력
void printProfile(const char *Name) {
	FILE *fp = NULL;

	fopen_s(&fp, Name, "wt");
	if (fp == NULL) {
		throw;
	}
	fprintf_s(fp, "           Name  |     Average  |        Min   |        Max   |      Call  \n");
	fprintf_s(fp, "---------------------------------------------------------------------------\n");
	for (int i = 0; i < PROFILE_ARRAY_SIZE; i++) {
		if (g_profileArray[i].flag) {
			if (g_profileArray[i].min[1] == DEFAULT_MIN) {
				g_profileArray[i].min[1] = g_profileArray[i].min[0];
			}
			else {
				g_profileArray[i].total = g_profileArray[i].total - g_profileArray[i].min[0];
				g_profileArray[i].call = g_profileArray[i].call - 1;
			}
			if (g_profileArray[i].max[1] == DEFAULT_MAX) {
				g_profileArray[i].max[1] = g_profileArray[i].max[0];
			}
			else {
				g_profileArray[i].total = g_profileArray[i].total - g_profileArray[i].max[0];
				g_profileArray[i].call = g_profileArray[i].call - 1;
			}
			//           Name  | Average |  Min   |  Max   | Call  
			fprintf_s(fp, "%-16s |%11.4f㎲ |%11.4f㎲ |%11.4f㎲ |%10d \n",
				g_profileArray[i].name,
				((g_profileArray[i].total / (double)g_frequency.QuadPart) * g_microsecond) / g_profileArray[i].call,
				(g_profileArray[i].min[1] / (double)g_frequency.QuadPart) * g_microsecond,
				(g_profileArray[i].max[1] / (double)g_frequency.QuadPart) * g_microsecond,
				g_profileArray[i].call);
		}
		else {
			break;
		}
	}
}