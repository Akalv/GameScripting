// GameScriptingLearning-chapter4.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <conio.h>
#include <string.h>
#include <iostream>
#include <map>
#include <iterator>

using namespace std;

#define COMMAND_PLAYERMOVE	"PlayerMove"
#define COMMAND_LOOKAT "LookAt"
#define COMMAND_PAUSE	"Pause"
#define COMMAND_DEFCONST	"DefConst"
#define COMMAND_IF "If"
#define COMMAND_BLOCK "Block"

const int MAX_SOURCE_LINE_SIZE = 192;
const int MAX_COMMAND_SIZE = 64;
const int MAX_PARAM_SIZE = 1024;
const int MAX_PLAYER_SIZE = 100;

const int UP = 1;
const int DOWN = 2;
const int LEFT = 3;
const int RIGHT = 4;

//代码块数据结构和代码块列表
typedef struct Block {
	int iStartNum;
	int iEndNum;
};

map<string, Block> mBlockMap;

//游戏标记表
map<string, int> mGameMap;

//常量表
map<string, int> mConstMap;

int g_iScriptSize = 0;
int g_iCurrScriptLineChar = 0;
int g_iCurrScriptLine = 0;
char ** g_ppstrScript;

//玩家结构体
typedef struct Player {
	int iPlayerID;
	int iPlayerLocX;
	int iPlayerLocY;
	int iPlayerMoveX;
	int iPlayerMoveY;
	float fPauseTime;
	int iPlayerLookAt;

	bool bActive;
};

Player players[MAX_PLAYER_SIZE];

void RunScript(int iPlayerID, int istartLine, int iendLine);

void LoadScript(const char * pstrFilename) {
	FILE * pScriptFile;
	//找出这个脚本存放了多少行代码
	if (!(pScriptFile = fopen(pstrFilename, "rb"))) {	//以二进制形式打开
		printf("文件打开失败!\n");
		exit(0);
	}
	while (!feof(pScriptFile)) {
		if (fgetc(pScriptFile) == '\n') {
			++g_iScriptSize;
		}
	}
	++g_iScriptSize;
	fclose(pScriptFile);
	//装载脚本
	if (!(pScriptFile = fopen(pstrFilename, "r"))) {
		printf("文件打开失败!\n");
		exit(0);
	}
	g_ppstrScript = (char **)malloc(g_iScriptSize * sizeof(char *));
	for (int iCurrLineIndex = 0; iCurrLineIndex < g_iScriptSize; ++iCurrLineIndex) {
		g_ppstrScript[iCurrLineIndex] = (char*)malloc(MAX_SOURCE_LINE_SIZE + 1);
		fgets(g_ppstrScript[iCurrLineIndex], MAX_SOURCE_LINE_SIZE, pScriptFile);
	}
	fclose(pScriptFile);
}

void UnloadScript() {
	//如果脚本已经被释放了，那就直接返回
	if (!g_ppstrScript)
	{
		return;
	}
	for (int iCurrLineIndex = 0; iCurrLineIndex < g_iScriptSize; ++iCurrLineIndex)
	{
		free(g_ppstrScript[iCurrLineIndex]);
	}
	free(g_ppstrScript);
	g_iCurrScriptLine = 0;
	g_iCurrScriptLineChar = 0;
	g_iScriptSize = 0;
}

//辅助函数
void GetCommand(char * command) {
	//记录命令的长度
	int iCommandSize = 0;
	char cCurrChar;
	//循环到空格或换行符
	while (g_iCurrScriptLineChar < (int)strlen(g_ppstrScript[g_iCurrScriptLine])) {
		cCurrChar = g_ppstrScript[g_iCurrScriptLine][g_iCurrScriptLineChar];
		if (cCurrChar == ' ' || cCurrChar == '\n') {
			break;
		}
		command[iCommandSize] = cCurrChar;
		iCommandSize++;
		++g_iCurrScriptLineChar;
	}
	++g_iCurrScriptLineChar;
	//添加null终结符
	command[iCommandSize] = '\0';
	//大写化
	strupr(command);
}
void GetStringParam(char * stringParam) {
	//记录字符串参数长度
	int iStringSize = 0;
	char cCurrChar;
	++g_iCurrScriptLineChar;
	//循环到引号或换行，把引号去掉
	while (g_iCurrScriptLineChar < (int)strlen(g_ppstrScript[g_iCurrScriptLine])) {
		cCurrChar = g_ppstrScript[g_iCurrScriptLine][g_iCurrScriptLineChar];
		if (cCurrChar == '"' || cCurrChar == '\n') {
			break;
		}
		stringParam[iStringSize] = cCurrChar;
		++iStringSize;
		++g_iCurrScriptLineChar;
	}
	//跳过引号和终结符
	g_iCurrScriptLineChar += 2;
	//添加null终结符
	stringParam[iStringSize] = '\0';
}
int GetIntParam() {
	//创建暂存整数字符串的空间
	char pstrString[MAX_PARAM_SIZE];
	int iParamSize = 0;
	char cCurrChar;
	//循环到空格或换行
	while (g_iCurrScriptLineChar < (int)strlen(g_ppstrScript[g_iCurrScriptLine])) {
		cCurrChar = g_ppstrScript[g_iCurrScriptLine][g_iCurrScriptLineChar];
		if (cCurrChar == ' ' || cCurrChar == '\n') {
			break;
		}
		pstrString[iParamSize] = cCurrChar;
		++iParamSize;
		++g_iCurrScriptLineChar;
	}
	//跳过空格
	++g_iCurrScriptLineChar;
	//添加null终结符
	pstrString[iParamSize] = '\0';
	//查找常量表，是否有与常量相同的标识符
	map<string, int>::iterator iter = mConstMap.find(pstrString);
	if (iter != mConstMap.end())
	{
		printf("找到常量\n");
		return iter->second;
	}
	//转成整型，返回
	int iIntValue = atoi(pstrString);
	return iIntValue;
}
float GetFloatParam() {
	//创建暂存浮点数字符串的空间
	char pstrString[MAX_PARAM_SIZE];
	int iParamSize = 0;
	char cCurrChar;
	//循环到空格或换行
	while (g_iCurrScriptLineChar < (int)strlen(g_ppstrScript[g_iCurrScriptLine])) {
		cCurrChar = g_ppstrScript[g_iCurrScriptLine][g_iCurrScriptLineChar];
		if (cCurrChar == ' ' || cCurrChar == '\n') {
			break;
		}
		pstrString[iParamSize] = cCurrChar;
		++iParamSize;
		++g_iCurrScriptLineChar;
	}
	//跳过空格
	++g_iCurrScriptLineChar;
	//添加null终结符
	pstrString[iParamSize] = '\0';
	//转成浮点数，返回
	float fFloatValue = atof(pstrString);
	return fFloatValue;
}

void RunBlock(int id){
	char pstrStringParam[MAX_PARAM_SIZE];
	GetCommand(pstrStringParam);
	map<string, Block>::iterator iter = mBlockMap.find(pstrStringParam);
	if (iter!=mBlockMap.end())
	{
		RunScript(id, iter->second.iStartNum, iter->second.iEndNum);
	}
	else {
		printf("代码块未定义！\n");
		exit(0);
	}
}

Block GetBlock() {
	char pstrStringParam[MAX_PARAM_SIZE];
	int iStartNum = 0;
	int iEndNum = 0;
	GetCommand(pstrStringParam);
	map<string, Block>::iterator iter = mBlockMap.find(pstrStringParam);
	if (iter != mBlockMap.end())
	{
		printf("重复定义！\n");
		exit(0);
	}
	g_iCurrScriptLine += 2;
	g_iCurrScriptLineChar = 0;
	iStartNum = g_iCurrScriptLine;
	while (g_ppstrScript[g_iCurrScriptLine][g_iCurrScriptLineChar] != '}') {
		g_iCurrScriptLine++;
	}
	iEndNum = g_iCurrScriptLine - 1;
	g_iCurrScriptLine++;
	Block block = { iStartNum,iEndNum };
	mBlockMap[pstrStringParam] = block;
	return block;
}

void RunScript(int iPlayerID,int istartLine,int iendLine) {
	//分配字符串用以存放源中的子字符串
	char pstrCommand[MAX_COMMAND_SIZE];
	char pstrStringParam[MAX_PARAM_SIZE];

	for (g_iCurrScriptLine = istartLine; g_iCurrScriptLine < iendLine; ++g_iCurrScriptLine) {
		g_iCurrScriptLineChar = 0;
		char cCurrChar = g_ppstrScript[g_iCurrScriptLine][g_iCurrScriptLineChar];
		//注释和换行过滤
		if ((cCurrChar == '/' && g_ppstrScript[g_iCurrScriptLine][g_iCurrScriptLineChar + 1] == '/') ||
			cCurrChar == '\n' || cCurrChar == ' ') {
			continue;
		}

		//获取命令
		GetCommand(pstrCommand);

		if (stricmp(pstrCommand, COMMAND_PLAYERMOVE) == 0)	//stricmp返回一个整型值
		{
			//玩家移动
			int iXMove = GetIntParam();
			int iYMove = GetIntParam();
			printf("玩家[%d]从位置[%d,%d]移动到位置[%d,%d]\n",iPlayerID,players[iPlayerID].iPlayerLocX, 
				players[iPlayerID].iPlayerLocY, players[iPlayerID].iPlayerLocX+iXMove, players[iPlayerID].iPlayerLocY+iYMove );
		}
		else if (stricmp(pstrCommand, COMMAND_LOOKAT) == 0)
		{
			//改变玩家朝向
			int iLookAt = GetIntParam();
			printf("玩家[%d]看向", iPlayerID);
			switch (iLookAt)
			{
				case 2:
					printf("上方\n"); break;
				case 3:
					printf("下方\n"); break;
				case 4:
					printf("左方\n"); break;
				case 5:
					printf("右方\n"); break;
				default:
					printf("输入错误\n"); exit(0);
			}
		}
		else if (stricmp(pstrCommand, COMMAND_PAUSE) == 0)
		{
			//暂停
			float fPauseTime = GetFloatParam();
			players[iPlayerID].fPauseTime = fPauseTime;
			printf("玩家[%d]睡了\n",iPlayerID);
			_sleep(fPauseTime);
			printf("玩家[%d]睡醒了！\n",iPlayerID);
		}
		else if (stricmp(pstrCommand, COMMAND_DEFCONST) == 0)
		{
			//定义常量
			//获取常量标识符
			GetCommand(pstrStringParam);
			int iConstValue = GetIntParam();
			map<string, int>::iterator iter = mConstMap.find(pstrStringParam);
			if (iter!=mConstMap.end())
			{
				//如果重复定义
				printf("重复定义！\n");
				break;
			}
			mConstMap[pstrStringParam] = iConstValue;
			printf("定义常量:%s，对应值为%d\n",pstrStringParam,iConstValue);
		}
		else if (stricmp(pstrCommand,COMMAND_IF)==0)
		{
			GetCommand(pstrStringParam);
			if (mGameMap[pstrStringParam]==1)
			{
				RunBlock(iPlayerID);
			}
			else {
				while (g_iCurrScriptLineChar != ' ') g_iCurrScriptLineChar++;
				RunBlock(iPlayerID);
			}
		}
		else if (stricmp(pstrCommand, COMMAND_BLOCK) == 0) {
			Block tmp = GetBlock();
			printf("代码块从[%d]到[%d]", tmp.iStartNum, tmp.iEndNum);
		}
		else {
			printf("非法输入！,%s\n",pstrCommand);
			break;
		}
	}

}

void PreLoad(const char * pstrFilename, int id) {
	LoadScript(pstrFilename);
	RunScript(id,0,g_iScriptSize);
	UnloadScript();
}

int main()
{
	Player p1 = { 0,0,0,0,0,0,1,false };

	//将游戏标记加入到游戏标记表中
	mGameMap["WolfDead"] = 0;
	mGameMap["MonsterDead"] = 0;
	
	//预处理
	const char * preFile = "CONSTTABLE.txt";
	PreLoad(preFile, p1.iPlayerID);


	const char *  fileName = "script.txt";
	LoadScript(fileName);
	RunScript(p1.iPlayerID, 0, g_iScriptSize);
	UnloadScript();
	system("pause");
	return 0;
}

