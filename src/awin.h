// Copyright 2015 Takashi Oketani
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifdef INCLED_AWIN_H
    /* nop */
#else

#define INCLED_AWIN_H
        // please adjust to your envirenment or using style .
#define PDATA_MAX 11616


#define ACTION_NONE                     -1
#define ACTION_LIST                      1
#define ACTION_LIST_SPECIFIED            2
#define ACTION_LIST_PID                  3
#define ACTION_LIST_PID_SPECIFIED        4

#define ACTION_ACTIVATE_NONE             5
#define ACTION_ACTIVATE_SWITCH_DESKTOP   6
#define ACTION_ACTIVATE_MOVE_DESKTOP     7

#define ACTION_MOVE_NONE              0
#define ACTION_MOVE_MOVE_ONLY_EXIST   1
#define ACTION_MOVE_MOVE_ONLY_EXECUTE 2
#define ACTION_MOVE_MOVE_BOTH         3


#define TARGET_1ST               1
#define TARGET_FULL              2

#define SORT_NONE                0
#define SORT_BY_PID              1
#define SORT_BY_MACHINE          2
#define SORT_BY_CMDLINE          2
#define SORT_BY_WM_CLASS         3
#define SORT_BY_WINDOWTITLE      4
#define SORT_MAX                 4
#define SORT_ORDER_ASCENDING     1
#define SORT_ORDER_DESCENDING   -1

#define WAIT_AFTER_EXEC_MICROSECOND  200000
#define MAX_RETRY_CNT_WINGET         25


#define MYEXEC_PROC_STORE   1
#define MYEXEC_PROC_CHECK   2
#define MYEXEC_PROC_EXECUTE 3

typedef struct xMyPData{
	pid_t PID;
	int   iLenCmdLine;
	char *CmdLine;
} MyPData;

typedef struct xMyPList
{
	MyPData   pdata[PDATA_MAX];
	int       iCnt;
	int       iMaxPIDDigit;
} MyPList;


typedef struct xMyEachData{
	char *pszData;
	int  iLen;
} MyEachData;

typedef struct xMyListData{
	MyEachData  WindowID;
	MyEachData  DesktopID;
	int         PID;
	MyEachData  WM_CLASS;
	MyEachData  Machine;
	MyEachData  WindowTitle;
} MyListData;


// sort condition
typedef struct xMySortConfig{
	int iOrder;         // SORT_ORDER_xxxx
	int iSortField;     // SORT_BY_xxxx
} MySortConfig;

int  MyCompPList( const void * , const void *);
int  MyCompPListA(const void * , const void *,const int );
int  GetProcessList(MyPList *,regex_t *, regmatch_t *);
void MyExit(Display *,int );
int  MyExec(const int ,const char * ,const unsigned int  );

#endif