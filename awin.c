// awin.c
//
// This is a tool based on wmctrl to raise the window(s) top or to lounch application.
//
// Copyright 2015 Takataka
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
//

//#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <regex.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "mymem.h"
#include "mywin.h"
#include "awin.h"

#define VERSION "2.0"
#define HELP "\nawin " VERSION "\n\n" \
" Usage: awin [OPTION]...\n" \
"\n" \
" OPTION: \n" \
"   -l             list all window managed by the window manager.\n" \
"   -L <SEARCH>    list window only having window title specified <SEARCH>. \n" \
"   -p             list all processes. PID(Process ID) and cmmand line \n" \
"   -P <SEARCH>    list processes having command line specified <SEARCH>. \n" \
"   -a             activate window ,1st listed (switch desktop if defferent). \n"   \
"   -A             activate window ,1st listed (move to current desktop if defferent).\n"   \
"   -g             activate target is all listed window. please use with -a or -A.\n"  \
"   -s             display <SEARCH> if specified\n" \
"   -w             check WM_CLASS instead of window title if specified <SEARCH>.\n" \
"   -m             check machine (client) instead of window title if specified <SEARCH>.\n" \
"   -c <COMMAND>   execute command if not listed.\n" \
"   -o <sort condition> \n" \
"                  specify sort condition \n" \
"                     filed,order[,filed,order[,filed,order[,filed,order]]]\n" \
"                       filed : \n" \
"                          1 : PID (Process ID)      3 : class\n" \
"                          2 : machine (client)      4 : title\n" \
"                       order : \n" \
"                          a : ascending             d : descending \n" \
"                    in case of set -p or -P , then you can set only field:1 (PID) or 2 (cmdline)\n " \
"\nNotion  : <SEARCH>  you can use regular expression.\n" \
"          if no option passed then same  'awin -l'.\n\n" \
" example:  awin -L lxterm -w -o 1,d,2,a \n" \
"\n Copyright 2015 Takataka.\n" \
" Released under the GNU General Public License. \n"

#define BUF_LEN		4096

/* global variable */
MySortConfig SortConfig[SORT_MAX];


int IsNumber( char *pText )
{
	int	i;

	for( i = 0 ; pText[i] ; i++ ){
	if( pText[i] < '0' || '9' < pText[i] )
		return 0;		// containing not only 0-9 ,then return 0
	}

	return i;    // only 0-9 return length (digit)
}

// compare functio for qsort
int MyCompPList( const void * a , const void * b){
	return MyCompPListA(a,b,0);
}

// compare func for PID LIST
int MyCompPListA(const void *a,const void *b,const int iCompTimes){
	int  		iRc;

	if (iCompTimes > 1) return 0;

	switch(SortConfig[iCompTimes].iSortField){
		case SORT_NONE:
			return  0;
			break;
		case SORT_BY_PID:
			iRc = (((MyPData *)a)->PID) - (((MyPData *)b)->PID);
			break;
		case SORT_BY_CMDLINE:
			iRc = strcmp(((MyPData *)a)->CmdLine,((MyPData *)b)->CmdLine);
			break;
		default:
			iRc = 0;
	}
	if( iRc == 0 ){
		if ( iCompTimes < 1 ){
			iRc = MyCompPListA( a,b,iCompTimes+1);
		}
	}else{
		iRc=iRc*(SortConfig[iCompTimes].iOrder);
	}
	return iRc;
}




int GetProcessList(MyPList *plist,regex_t *ppreg, regmatch_t* pmatch){
	DIR  *dir;
	struct dirent  *dp;
	char   path_cmdline[100];
	int    iLenPID;
	pid_t  myPID;
	int    iPID;
	char   szReadLine[1024];
	int    fd;


	if((dir=opendir("/proc"))==NULL){
		perror("Err:opendir /proc ");
		exit(EXIT_FAILURE);
	}

	if (plist->iCnt == 0) plist->iMaxPIDDigit = 0;

	myPID=getpid();

	for(dp=readdir(dir);dp!=NULL;dp=readdir(dir)){
		switch(dp->d_type)
		{
			case DT_DIR:      // This is a directory.
				iLenPID=IsNumber(dp->d_name);
				if( iLenPID > 0 ){ // all is number -> pid !
					iPID=atoi(dp->d_name);
					if( (iPID == (int)myPID ) ||	//  me!
						(iPID == 1))				// init
					{
						// nop
					}else{
						sprintf(path_cmdline,"/proc/%d/cmdline",iPID);
						if ((fd = open(path_cmdline, O_RDONLY)) != -1) {
							int iReadLen;
							memset(szReadLine,0x00,sizeof(szReadLine));
							iReadLen=read(fd,szReadLine,sizeof(szReadLine));
							if (iReadLen>0){
								if (plist->iCnt +1 >= PDATA_MAX) {
									perror("Err:exceed max process");
									exit(EXIT_FAILURE);
								}else{
									size_t nmatch = 0;
									bool   fStore=false;
									if( ppreg != NULL ){
										if ( regexec(ppreg, szReadLine, nmatch, pmatch, 0) == 0 ){
											fStore=true;
//											fprintf(stderr,"Dbg:store!:%s\n",szReadLine);
										}else{
											// nop
										}
									}else{
										fStore=true;
									}
									if (fStore == true){
										plist->pdata[plist->iCnt].PID=iPID;
										plist->pdata[plist->iCnt].iLenCmdLine=iReadLen;
										if((plist->pdata[plist->iCnt].CmdLine=MyAssignMem(plist->pdata[plist->iCnt].iLenCmdLine*sizeof(char)))==NULL){
											perror("Err:alloc memory cmdine[2]");
											exit(EXIT_FAILURE);
										}else{
											int i;
											// if line contain 0x00 ,then replace blank .
											memcpy(plist->pdata[plist->iCnt].CmdLine,szReadLine,plist->pdata[plist->iCnt].iLenCmdLine);
											for(i=0;i<iReadLen-1;i++) if(plist->pdata[plist->iCnt].CmdLine[i]==0x00) plist->pdata[plist->iCnt].CmdLine[i]=' ';
										}
										if ( plist->iMaxPIDDigit < iLenPID) plist->iMaxPIDDigit=iLenPID;
										plist->iCnt++;
									}
								}
							}else{
								// nop!
							}
							close(fd);
						}else{
							// continue ...
						}
					}
				}else{
					// nop
				}
				break;
			default:
				break;
		}
	}
	closedir(dir);

	return 0;
}

int main(int argc,char **argv)
{
	Display *disp=NULL;
	MyWinList WinList;
	int iRegField=REG_FIELD_NONE;
	int iRc;
//	static size_t line_length = 80;
//	struct winsize ws;

	setlocale(LC_ALL, "");

	if (! (disp = XOpenDisplay(NULL))) {
		fprintf(stderr,"Err:cannot open X display.\n");
		return EXIT_FAILURE;
	}

//	// get terminal size
//	if( ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws ) != -1 ) {
//		fprintf(stderr,"Dbg:terminal_width  =%d\n", ws.ws_col);
//		if( 0 < ws.ws_col )	line_length = ws.ws_col;
//	}

//	fprintf(stderr,"argc:%d\n",argc);

	if ( argc > 1 ){
		char szBuf[BUF_LEN];
		char szExec[BUF_LEN]="";
		int i;
		regex_t preg;
		regmatch_t* pmatch = NULL;
		size_t nmatch = 0;
		int option;
		int  action=ACTION_LIST;
		char szSearchString[BUF_LEN]="";
		bool fDisplaySearch=false;
		int  action_activate= ACTION_ACTIVATE_NONE;
		int  option_group= OPTION_GROUP_NO;

		if ( ( argc == 2 ) && argv[1] ) {
			if (strcmp(argv[1], "--help") == 0) {
				fputs(HELP, stdout);
				XCloseDisplay(disp);
				return EXIT_SUCCESS;
			}else if ( strcmp(argv[1], "--version") == 0 ) {
				fputs(VERSION, stdout);
				XCloseDisplay(disp);
				return EXIT_SUCCESS;
			}
		}


		for (i=0;i<SORT_MAX;i++){
			SortConfig[i].iSortField	= SORT_NONE;
			SortConfig[i].iOrder		= SORT_ORDER_ASCENDING;
		}

		// parse arguments
		while((option = getopt(argc,argv,"lL:aAc:swo:pP:mg")) != -1){
			switch(option){
				case 'l':
					action = ACTION_LIST;
					break;
				case 'L':
					action = ACTION_LIST_SPECIFIED;
					if( strlen(optarg) < sizeof(szSearchString) ){
						strcpy(szSearchString,optarg);
					}else{
						fprintf(stderr,"Err:too long argument (-L)\n");
						XCloseDisplay(disp);
						return EXIT_FAILURE;
					}
					if (iRegField==REG_FIELD_NONE)	iRegField=REG_FIELD_TITLE;
					break;
				case 'a':
					action_activate = ACTION_ACTIVATE_SWITCH_DESKTOP;
					break;
				case 'A':
					action_activate = ACTION_ACTIVATE_MOVE_DESKTOP;
					break;
				case 'g':
					option_group = OPTION_GROUP_YES;
					break;
				case 'c':
					if( strlen(optarg) > sizeof(szExec)-1 ){
						fprintf(stderr,"Err:too long argument (-c)\n");
						XCloseDisplay(disp);
						return EXIT_FAILURE;
					}
					sprintf(szExec,"%s&",optarg);
					break;
				case 's':
					fDisplaySearch=true;
					break;
				case 'w':
					iRegField=REG_FIELD_CLASS;
					fprintf(stderr,"Inf:Check filed is class instead of title\n");
					break;
				case 'm':
					iRegField=REG_FIELD_MACHINE;
					fprintf(stderr,"Inf:Check filed is machine instead of title\n");
					break;
				case 'o':  // order by
					{
						int i,j;
						i=0;j=0;
						for ( i = 0 ; i < SORT_MAX ; i++ ){
							SortConfig[i].iSortField=optarg[j]-'0';
							if(( SortConfig[i].iSortField > SORT_MAX ) || ( SortConfig[i].iSortField < 1) )
							{
								fprintf(stderr, "Err:invalid sort filed (-o) %c\n",optarg[j] );
								XCloseDisplay(disp);
								return EXIT_FAILURE;
							}
							if ( optarg[j+1] == ',' ){
								if (optarg[j+2] == 'a'){
									SortConfig[i].iOrder=SORT_ORDER_ASCENDING;
								}else if ( optarg[j+2] == 'd' ){
									SortConfig[i].iOrder=SORT_ORDER_DESCENDING;
								}else{
									fprintf(stderr, "Err:invalid sort order (-o) %c.\n",optarg[j+2] );
									XCloseDisplay(disp);
									return EXIT_FAILURE;
								}
								if ( optarg[j+3] == ',' ){
									j=j+4;
								}else{
									break; // exit for
								}
							}else if (optarg[j+1] != '\0' ){
								fprintf(stderr, "Err:invalid format sort (-o) \n" );
								XCloseDisplay(disp);
								return EXIT_FAILURE;
							}else{
								break; // exit for
							}
						} // end for
					}
					break;
				case 'p':
					action = ACTION_LIST_PID;
					break;
				case 'P':
					action = ACTION_LIST_PID_SPECIFIED;
					if( strlen(optarg) < sizeof(szSearchString) ){
						strcpy(szSearchString,optarg);
					}else{
						fprintf(stderr,"Err:too long argument (-P)\n");
						XCloseDisplay(disp);
						return EXIT_FAILURE;
					}
					break;
				default:
					fprintf(stderr,"Err:invalid option\n");
					XCloseDisplay(disp);
					return EXIT_FAILURE;
					break;
			}	/* end switch */
		}		/* end while */

		if( szSearchString[0] != 0x00 ){
			int iRc;
			if (fDisplaySearch==true)
			{
				fprintf(stderr,"Inf:search-string is [%s]\n",szSearchString);
			}
			// compile regexp
			iRc=regcomp(&preg,szSearchString,REG_EXTENDED|REG_NEWLINE);
			if ( iRc != 0 ) {
				// Error!, print error reason
				regerror(iRc, &preg, szBuf, sizeof(szBuf));
				fprintf(stderr,"Err:regcomp() failed with '%s'\n", szBuf);
				XCloseDisplay(disp);
				return EXIT_FAILURE;
			}else{
				// compile complete! allocate pmatch
				nmatch = preg.re_nsub + 1;
				if(( pmatch = calloc(nmatch,sizeof(regmatch_t))) == NULL ){
					fprintf(stderr, "Err:cannot allocate memory for pmatch\n" );
					XCloseDisplay(disp);
					return EXIT_FAILURE;
				}
			}
		}else{
			if (( action == ACTION_LIST ) ||  (action == ACTION_LIST_PID)){
				// nop
			}else{
				// unexpected case
				fprintf(stderr, "Err:<SEARCH> not specifed!\n");
				XCloseDisplay(disp);
				return EXIT_FAILURE;
			}
		}

		if (( action == ACTION_LIST_PID ) ||
			( action == ACTION_LIST_PID_SPECIFIED )){
			MyPList plist;
			int 	i;

			plist.iCnt=0;

			if (action == ACTION_LIST_PID){
				GetProcessList(&plist,NULL,NULL);
			}else{
				GetProcessList(&plist,&preg,pmatch);
			}

			if ( plist.iCnt == 0 ) {	// no match!
				fprintf(stderr, "Inf:count of process is 0\n");
				if ( szExec[0] != 0x00 ){
					fprintf(stderr, "Inf:try to executecount:%s\n",szExec);
					system(szExec);
				}else{
				}
				XCloseDisplay(disp);
				return EXIT_SUCCESS;
			}else{
				if (SortConfig[0].iSortField!=SORT_NONE) {
					qsort(plist.pdata,plist.iCnt,sizeof(MyPData),MyCompPList);
				}

				if ((action == ACTION_LIST_PID) ||
					(action == ACTION_LIST_PID_SPECIFIED)){
					// list pid
					int iDigitCnt=digit(plist.iCnt);
					fprintf(stdout,"[%*s] %*s cmdline\n",iDigitCnt,"#",plist.iMaxPIDDigit,"PID");
					for(i=0;i<plist.iMaxPIDDigit+iDigitCnt+13;i++){
						fprintf(stdout,"-");
					}
					fprintf(stdout,"\n");
					for(i=0;i<plist.iCnt;i++){
						fprintf(stdout,"[%*d] %*d %s\n",iDigitCnt,i,plist.iMaxPIDDigit,plist.pdata[i].PID,plist.pdata[i].CmdLine);
					}
				}
					//
				if ( action_activate != ACTION_ACTIVATE_NONE ){
					action = ACTION_LIST_SPECIFIED;
					regfree(&preg);
					iRegField=REG_FIELD_PID;
					if ( option_group == OPTION_GROUP_NO) {
						fprintf(stderr, "Inf:list window of 1st PID and activate\n");
						sprintf(szSearchString,"^%d$",plist.pdata[0].PID);
					}else{
						int j=0,k;
						fprintf(stderr, "Inf:list window and activate all\n");
						for (k=0;k<plist.iCnt;k++){
							sprintf(&szSearchString[j],"^%d$|",plist.pdata[k].PID);
							j=strlen(szSearchString);
							if ( j >= sizeof(szSearchString)) break;
						}
						if ( j >= sizeof(szSearchString)) {
							fprintf(stderr,"Err:exceed limit of buffer:szSearchString.abort!\n");
							XCloseDisplay(disp);
							return EXIT_FAILURE;
						}
						szSearchString[strlen(szSearchString)-1]=0x00; // erase last '|'
					}
					iRc=regcomp(&preg,szSearchString,REG_EXTENDED|REG_NEWLINE);
					if ( iRc != 0 ) {
						// Error!, print error reason
						regerror(iRc, &preg, szBuf, sizeof(szBuf));
						fprintf(stderr,"Err:regcomp() failed with '%s'\n", szBuf);
						XCloseDisplay(disp);
						return EXIT_FAILURE;
					}else{
						// compile complete! allocate pmatch
						nmatch = preg.re_nsub + 1;
						if(( pmatch = calloc(nmatch,sizeof(regmatch_t))) == NULL )
						{
							fprintf(stderr, "Err:cannot allocate memory for pmatch\n" );
							XCloseDisplay(disp);
							return EXIT_FAILURE;
						}
					}
					// continue ....
				}else{
						XCloseDisplay(disp);
						return EXIT_SUCCESS;
				}
			}
		}

		// get Window List
		if ( iRegField != REG_FIELD_NONE ){
			iRc=getWinList(disp,&WinList,iRegField,&preg,pmatch);
			regfree(&preg);
		}else{
			iRc=getWinList(disp,&WinList,REG_FIELD_NONE,NULL,NULL);
		}
		if (iRc==EXIT_SUCCESS){
			if (WinList.iCnt>0){
				if (SortConfig[0].iSortField!=SORT_NONE) {
					qsort(WinList.data,WinList.iCnt,sizeof(MyWinData),MyCompWin);
				}else{
//					fprintf(stderr,"Dbg:SORT_NONE\n");
				}
				if((action==ACTION_LIST)||(action==ACTION_LIST_SPECIFIED)){
						DisplayWinList(&WinList);
				}
				if ( action_activate == ACTION_ACTIVATE_NONE ){
					// nop
				}else{
					if(action_activate == ACTION_ACTIVATE_SWITCH_DESKTOP ){
						if(option_group == OPTION_GROUP_NO){
							activate_window(disp, WinList.win[WinList.data[0].iOriginalIndex],true);
						}else{
							int k;
							for(k=0;k<WinList.iCnt;k++)
								activate_window(disp, WinList.win[WinList.data[k].iOriginalIndex],true);
						}
					}else if (action_activate == ACTION_ACTIVATE_MOVE_DESKTOP ){
						if(option_group == OPTION_GROUP_NO){
							move_and_activate_window(disp, WinList.win[WinList.data[0].iOriginalIndex],WinList.data[0].wm_class);
						}else{
							int k;
							for(k=0;k<WinList.iCnt;k++){
								move_and_activate_window(disp, WinList.win[WinList.data[k].iOriginalIndex],WinList.data[k].wm_class);
								if(WAIT_MICROSECOND_AWTIVATE_AFTER_MOVE>0)  usleep(WAIT_MICROSECOND_AWTIVATE_AFTER_MOVE);
							}
						}
					}else{
						// nop
					}
				}
			}else{
				if ( szExec[0] != 0x00 ){
					system(szExec);
				}else{
					fprintf(stderr, "Inf:count of window is 0\n" );
				}
			}
		}else{
			fprintf(stderr,"Err:cannot getWinList .\n");
			XCloseDisplay(disp);
			return EXIT_FAILURE;
		}

	}else{
		// simply list
		iRc=getWinList(disp,&WinList,REG_FIELD_NONE,NULL,NULL);
		DisplayWinList(&WinList);
	}

	XCloseDisplay(disp);

	return EXIT_SUCCESS;
}