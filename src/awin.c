// awin.c
//
// This is a tool to raise the window(s) top or to lounch application.
//
// Copyright 2015-2017 Takataka
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
#include <sys/wait.h>

#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <libgen.h>

#include "mymem.h"
#include "mywin.h"
#include "awin.h"

#define VERSION "2.3"
#define HELP "\nawin " VERSION "\n\n" \
" Usage: awin [OPTION]...\n" \
"\n" \
" OPTION: \n" \
"   -l             list all window managed by the window manager.\n" \
"   -L <SEARCH>    list window only having window title specified <SEARCH>. \n" \
"   -w             check WM_CLASS instead of window title if specified <SEARCH>.\n" \
"   -m             check machine (client) instead of window title if specified <SEARCH>.\n" \
"   -p             list all processes. PID(Process ID) and cmmand line \n" \
"   -P <SEARCH>    list processes having command line specified <SEARCH>. \n" \
"   -s             display <SEARCH> if specified\n" \
"   -a             activate window ,1st listed (switch desktop if defferent). \n"   \
"   -A             activate window ,1st listed (move to current desktop if defferent).\n"   \
"   -t <microsec>  wait time(microsecond) befor activate ( when -A option specified ).\n" \
"   -f             full of listed  is tartget to activate.\n" \
"   -c <COMMAND>   execute command if not listed.\n" \
"   -C <COMMAND>   execute command if not listed.No check whether <COMMAND> is exist as executable or not. \n" \
"   -g x,y         grid ( x , y ) if activated [ if -f option specified then ignore this ].\n" \
"   -G x,y         grid ( x , y ) if executed .\n" \
"   -T <microsec>  wait time(microsecond) after execute  ( when -G , -c option specified and not executing ).\n" \
"   -o <sort condition> \n" \
"                  specify sort condition \n" \
"                     filed,order[,filed,order[,filed,order[,filed,order]]]\n" \
"                       filed : \n" \
"                          1 : PID (Process ID)      3 : WM_CLASS\n" \
"                          2 : machine (client)      4 : Title of Window\n" \
"                       order : \n" \
"                          a : ascending             d : descending \n" \
"                  in case of set -p or -P , then you can set only field:1 (PID) or 2 (cmdline)\n" \
"   -S <state>     change the window's state ( with -a / -A option )\n" \
"                    format: action,state\n" \
"                        action : 'a' or 'r' or 't' (a:add,r:remove,t:toggle)\n" \
"                        state  : one of followings\n" \
"                                 MODAL,STICKY,MAXIMIZED_VERT,MAXIMIZED_HORZ,SHADED,SKIP_TASKBAR,\n" \
"                                 SKIP_PAGER,HIDDEN,FULLSCREEN,ABOVE,BELOW,DEMANDS_ATTENTION\n"  \
"   -D             set the window should appear on all desktops.\n" \
"\n" \
"Notion  : <SEARCH>  you can use regular expression.\n" \
"          if no option passed then same  'awin -l'.\n\n" \
" example:  awin -w -L lxterminal.Lxterminal -o 1,d,2,a -a -c /usr/bin/lxterminal -g 0,0 -G 100,100 -b a,ABOVE\n" \
"\n" \
"Copyright 2015-2017 Takataka.\n" \
"Released under the GNU General Public License. \n"

#define BUF_LEN		1024

//#define DEBUG

//  ref  http://tricky-code.net/nicecode/code10.php
#ifdef DEBUG
#define debug_printf fprintf
#else
#define debug_printf 1 ? (void) 0 : fprintf
#endif

/* global variable */
MySortConfig SortConfig[SORT_MAX];
extern bool 	 fActionAllDesktop;

/* ----------------------------------
 *  function : check string contain only num or not , and calc number of digits 
 *  parms    : pText - string  
 *  return   : number of digits  ( 0 : means contain not numeric )
 */
int IsNumber( const char *pText ){
	int	i;

	if( pText == NULL ) return -1;
	for( i = 0 ; pText[i] ; i++ ){
		if( ( pText[i] < '0' )||( '9' < pText[i] ) )
			return 0;		// containing not only 0-9 ,then return 0
	}

	return i;				// only 0-9 return length (digit)
}

// compare function for qsort ( wrapper )
int MyCompPList( const void * a , const void * b){
	return MyCompPListA(a,b,0);
}

// compare func for PID LIST
// N.B. refering global variable[SortConfig] and recursive call
int MyCompPListA(const void *a,const void *b,const int iCompTimes){
	int	iRc;

	if( iCompTimes > 1 ) return 0;

	switch( SortConfig[iCompTimes].iSortField ){
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
			;;;;;;;;;;debug_printf(stderr,"Err:invalid sort filed:%d\n",SortConfig[iCompTimes].iSortField);
			return 0;
	}
	if( iRc == 0 ){
		if( iCompTimes < 1 ){
			iRc = MyCompPListA( a , b , iCompTimes+1 );
		}
	}else{
		iRc = iRc*(SortConfig[iCompTimes].iOrder);
	}
	return iRc;
}

/* -------------------------------------------------------------------------
 *  get process list
 *  parm   : 1st  - address of plist
 *           2nd  - ppreg    - for regular expression ( compiled regular expression )
 *           3rd  - pmatch   - for regular expression ( result )
 *  return :   0  - ok
 *             1  - error
 */
int GetProcessList(MyPList *plist,regex_t *ppreg, regmatch_t* pmatch){
	DIR   *dir;
	struct dirent  *dp;
	char   path_cmdline[100];
	int    iLenPID;
	pid_t  myPID;
	int    iPID;
	char   szReadLine[1024];
	int    fd;

	if( plist == NULL ) return -1;

	errno = 0;

	if( (dir=opendir("/proc")) == NULL ){
		perror("Err:opendir /proc ");
		exit(EXIT_FAILURE);
	}

	if( plist->iCnt == 0 ) plist->iMaxPIDDigit = 0;

	myPID=getpid();

	for( dp=readdir(dir) ; dp != NULL ; dp=readdir(dir) ){
		switch( dp->d_type ){
			case DT_DIR:      // This is a directory.
				iLenPID=IsNumber(dp->d_name);
				if( iLenPID > 0 ){ // all is number -> pid !
					iPID=atoi(dp->d_name);
					if( ( iPID == (int)myPID ) ||		//  me!
						 ( iPID == 1 )){				// init
						// nop
					}else{
						sprintf(path_cmdline,"/proc/%d/cmdline",iPID);
						if( (fd = open(path_cmdline, O_RDONLY)) != -1 ){
							int iReadLen;
							memset(szReadLine,0x00,sizeof(szReadLine));
							iReadLen=read(fd,szReadLine,sizeof(szReadLine));
							if( iReadLen > 0 ){
								if( plist->iCnt + 1 >= PDATA_MAX ){
									perror("Err:exceed max process");
									exit(EXIT_FAILURE);
								}else{
									size_t nmatch = 0;
									bool   fStore=false;
									if( ppreg != NULL ){
										if( regexec(ppreg, szReadLine, nmatch, pmatch, 0) == 0 ){
											fStore=true;
										}else{
											// nop
										}
									}else{
										fStore=true;
									}
									if( fStore == true ){
										plist->pdata[plist->iCnt].PID=iPID;
										plist->pdata[plist->iCnt].iLenCmdLine=iReadLen;
										if(( plist->pdata[plist->iCnt].CmdLine=MyAssignMem(plist->pdata[plist->iCnt].iLenCmdLine*sizeof(char))) == NULL ){
											perror("Err:alloc memory cmdine[2]");
											exit(EXIT_FAILURE);
										}else{
											int i;
											// if line contain 0x00 ,then replace blank .
											memcpy(plist->pdata[plist->iCnt].CmdLine,szReadLine,plist->pdata[plist->iCnt].iLenCmdLine);
											for( i=0 ; i < iReadLen-1 ; i++ ) if( plist->pdata[plist->iCnt].CmdLine[i] == 0x00 ) plist->pdata[plist->iCnt].CmdLine[i]=' ';
										}
										if( plist->iMaxPIDDigit < iLenPID ) plist->iMaxPIDDigit=iLenPID;
										plist->iCnt++;
									}
								}
							}else{
								// nop!
							}
							close(fd);
						}else{
							// continue ...  !?!
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

/* -------------------------------------------------------------------------
 *  print list of PID
 *  parm   : 1st  - number of digits
 *           2nd  - address of pList
 *  return : none
 */
void print_list_pid(int iDigitCnt,MyPList *plist){
	int i;
	// print header
	fprintf(stdout,"[%*c] %*s cmdline\n",iDigitCnt,'#',plist->iMaxPIDDigit,"PID");
	// print separater
	for( i=0 ; i < plist->iMaxPIDDigit+iDigitCnt+13 ; i++ ){
		fprintf(stdout,"-");
	}
	fprintf(stdout,"\n");
	// print data line(s)
	for( i=0 ; i < plist->iCnt ; i++ ){
		fprintf(stdout,"[%*d] %*d %s\n",iDigitCnt,i+1,plist->iMaxPIDDigit,plist->pdata[i].PID,plist->pdata[i].CmdLine);
	}
}

/* -------------------------------------------------------------------------
 *  exit this program
 *  parm   : 1st  - display
 *           2nd  - status of exit
 *  return : none
 */
void MyExit(Display *disp,int status){
	if( disp != NULL) 	XCloseDisplay(disp);
	exit(status);
}

#define ERR_MSG_MEM_SHORT_STRDUP_DIR   "Err:in ParseDir(),memory shortage [strdup(dir)]\n"
#define ERR_MSG_MEM_SHORT_STRDUP_BASEC "Err:in ParseDir(),memory shortage [strdup(basec)]\n"
#define ERR_MSG_MEM_SHORT_STRDUP_REST  "Err:in ParseDir(),memory shortage [strdup(rest)]\n"
#define ERR_MSG_NOT_EXIST_DIR          "Err:in ParseDir(),not exist the directory.\n"
#define ERR_MSG_NOT_EXIST_OR_NOT_EXEC  "Err:in ParseDir(),not exist as executable file or not exist.\n"
#define ERR_MSG_PARSEDIR               "Err:ParseDir()=%d\n"
#define INF_MSG_EXECCMD                "Inf:execute cmd is [%s]\n"

#define ERR_RC_MEM_SHORT_STRDUP_DIR   -10
#define ERR_RC_MEM_SHORT_STRDUP_BASEC -11
#define ERR_RC_MEM_SHORT_STRDUP_REST  -12
#define ERR_RC_NOT_EXIST_DIR            1
#define ERR_RC_NOT_EXIST_OR_NOT_EXEC    2

/* -------------------------------------------------------------------------
 *  check whether file is executable or not
 *  parm   : 1st  - dirname ( fullpath )
 *           2nd  - basename
 *  return :   0  - executable
 *             1  - not executable
 *            99  - not fullpath
 *           998  - too long parm(s)
 *          -999  - Null parm(s) passed
 */
int Is_Executable(char *dname,char *bname){
	char 	szTmp[1024]={0x00};
	int 	len_dname,len_bname;
	int 	rc=-999;
	
	;;;;;;;;;;debug_printf(stderr,"Dbg:[Is_Executable()] begin.\n");
	if( ( dname == NULL )||( bname == NULL )){
		goto end_func;
	}

	len_dname = strlen(dname);
	len_bname = strlen(bname);
	
	;;;;;;;;;;debug_printf(stderr,"Dbg:in [Is_Executable]--------\n");
	;;;;;;;;;;debug_printf(stderr,"Dbg:dname(%s),len=%d\n",dname,len_dname);
	;;;;;;;;;;debug_printf(stderr,"Dbg:bname(%s),len=%d\n",bname,len_bname);
	if( len_dname + len_bname + 1 < sizeof(szTmp) -1 ){
		if( len_dname == 1 ){
			if( dname[0] == '/' ){
				sprintf(szTmp,"/%s",bname);
			}else{
				rc = 99;
				goto end_func;
			}
		}else{
			sprintf(szTmp,"%s/%s",dname,bname);
		}
		rc = access(szTmp, X_OK);
		if( rc != 0 ){
			#ifdef DEBUG
				int saved_errno = errno;
				;;;;;;;;;;debug_printf(stderr,"Dbg:access(%s),rc=%d,errno=%d\n",szTmp,rc,saved_errno);
				;;;;;;;;;;debug_printf(stderr,"Dbg:access error reason:%s\n",strerror( saved_errno )); 
			#endif
			rc = 1;
			goto end_func;
		}
	}else{
		;;;;;;;;;;debug_printf(stderr,"Dbg:access, too long parm(s)\n");
		rc = 998;
		goto end_func;
	}
end_func:
	;;;;;;;;;;debug_printf(stderr,"Dbg:[Is_Executable()] begin.\n");
	return rc;
}

/* -------------------------------------------------------------------------
 *  check whether parm is existing directory or not
 *  parm   : 1st  - dirname ( fullpath )
 *  return :   0  - existing directory
 *             1  - existing but not directory
 *            -1  - error from stat()
 */
int Is_Directory(char *dname){
	int 		rc;
	struct stat buf ;

	;;;;;;;;;;debug_printf(stderr,"Dbg:[Is_Directory()] begin.\n");
	;;;;;;;;;;debug_printf(stderr,"Dbg:dname[%s]\n",dname);

	rc = stat(dname,&buf);
	if( rc == 0 ){
		if( S_ISDIR(buf.st_mode) ){
		}else{
			#ifdef DEBUG
				int saved_errno = errno;
				;;;;;;;;;;debug_printf(stderr,"Dbg:stat(),rc=%d,errno=%d,not directory?\n",rc,saved_errno);
				;;;;;;;;;;debug_printf(stderr,"Dbg:stat error reason:%s\n",strerror( saved_errno )); 
			#endif
			rc = 1;
		}
	}
	;;;;;;;;;;debug_printf(stderr,"Dbg:[Is_Directory()] end.\n");
	return rc;
}

/* -------------------------------------------------------------------------
 *  separate parm to dir / base / parm . dir is checked existing or not and base is executable or not.
 *  parm   : string ( input  )  - fullpath  and parm  
 *           string ( output )  - path ( if error , then null )
 *           string ( output )  - base ( if error , then null )
 *           string ( output )  - parm ( if not set or error , then null )
 *          NB) output should be free by caller.
 *  return :   0                            - check ok and separated
 *            ERR_RC_MEM_SHORT_STRDUP_DIR   - memory shortage
 *            ERR_RC_MEM_SHORT_STRDUP_BASEC - memory shortage
 *            ERR_RC_MEM_SHORT_STRDUP_REST  - memory shortage
 *            ERR_RC_NOT_EXIST_DIR          - dir  is not exist
 *            ERR_RC_NOT_EXIST_OR_NOT_EXEC  - base is not executable
 */
int ParseDir(const char *source,char **dir,char **base,char **rest,int *length_dir,int *length_base,int *length_rest){
	int		 rc=-999;
	char	*dirc=NULL, *basec=NULL, *dname, *bname;
	int		 i,len,len_dir=-1,len_name=-1,headblank;

	;;;;;;;;;;debug_printf(stderr,"Dbg:[ParseDir()] begin.\n");
	// 1st parse directory 
	for( headblank=0 ; source[headblank] == ' ' ; headblank++ );
	dirc = strdup(&source[headblank]);
	if( dirc == NULL ){
		fprintf(stderr,ERR_MSG_MEM_SHORT_STRDUP_DIR);
		rc= ERR_RC_MEM_SHORT_STRDUP_DIR;
		goto end_func;
	}
	;;;;;;;;;;debug_printf(stderr,"Dbg:dirc[%s]\n",dirc);
	i     = strlen(dirc);
	dname = dirname(dirc);
	rc    = Is_Directory(dname);
	;;;;;;;;;;debug_printf(stderr,"Dbg:1st,dname[%s]\n",dname);

	while(( rc != 0 )&&( i > 0 )){
		;;;;;;;;;;debug_printf(stderr,"Dbg:[@][%s]\n",dirc);
		for( ; (dirc[i] != ' ' )&&( i > 0 ) ; i-- );
		if( dirc[i] == ' ' ) dirc[i]=0x00;
		dname = dirname(dirc);
		rc    = Is_Directory(dname);
		;;;;;;;;;;debug_printf(stderr,"Dbg:(-),dname[%s]\n",dname);
	}
	if( rc == 0 ){
		// directory is checked!

		len_dir = strlen(dname);
		;;;;;;;;;;debug_printf(stderr,"Dbg:stat(),rc=%d,directory!\n",rc);
		;;;;;;;;;;debug_printf(stderr,"Dbg:len_dir=%d\n",len_dir);
		;;;;;;;;;;debug_printf(stderr,"Dbg:dname[%s]\n",dname);
		;;;;;;;;;;debug_printf(stderr,"Dbg:      0----+----1----+----2----+----3----+----4----+----5\n");

		// parse basename
		//rc = -2;
		if( len_dir == 1 ){
			basec = strdup(&source[headblank+len_dir]);
		}else{
			basec = strdup(&source[headblank+len_dir+1]);
		}
		if( basec == NULL ){
			fprintf(stderr,ERR_MSG_MEM_SHORT_STRDUP_BASEC);
			rc= ERR_RC_MEM_SHORT_STRDUP_BASEC;
			goto end_func;
		}
		;;;;;;;;;;debug_printf(stderr,"Dbg:---------------------------------------------------------------\n");
		;;;;;;;;;;debug_printf(stderr,"Dbg:headblank=%d,len_dir=%d\n",headblank,len_dir);
		;;;;;;;;;;debug_printf(stderr,"Dbg:basec[%s]\n",basec);
		;;;;;;;;;;debug_printf(stderr,"Dbg:      0----+----1----+----2----+----3----+----4----+----5\n");
		
		// try to check entire 
		rc = Is_Executable(dname,basec);
		if( rc != 0 ){
			;;;;;;;;;;debug_printf(stderr,"Dbg:    try to check entire -> error!!!!!!!!!!\n");
			len = strlen(basec);
			// Shortest match principle!
			i = 0;
			while( ( rc != 0 )&&(i < len ) ){
				for( ; ( basec[i] != ' ' )&&( i < len ) ; i++ ){
					if( basec[i] == '/' ){ // '/' : this char could not be used as filename ( UTF-8 only ? )
						fprintf(stderr,ERR_MSG_NOT_EXIST_DIR);
						rc = ERR_RC_NOT_EXIST_DIR;
						goto end_func;
					}
				}
				if( basec[i] == ' ' ){
					basec[i] = 0x00;
					rc = Is_Executable(dname,basec);
					if( rc != 0 ){
						basec[i++] = ' '; // restore blank
					}else{
						int j;
						// remove blank of tail
						for( j=strlen(&basec[i+1]) ; ( j > 0 )&&( basec[i+j] == ' ' ) ; j-- ){
							basec[i+j] = 0x00;
						}
					    *rest=strdup(&basec[i+1]);	
						if( *rest == NULL ){
							rc = ERR_RC_MEM_SHORT_STRDUP_REST;
							fprintf(stderr,ERR_MSG_MEM_SHORT_STRDUP_REST);
							goto end_func;
						}
					}
				}
			}
		}else{
			*rest = NULL;
			;;;;;;;;;;debug_printf(stderr,"Dbg:    try to entire -> OK!\n");
		}
		if( rc == 0 ){
			bname    = basec;
			len_name = strlen(bname);
		}else{
			// len_name = -1;
			rc       = ERR_RC_NOT_EXIST_OR_NOT_EXEC;
			fprintf(stderr,ERR_MSG_NOT_EXIST_OR_NOT_EXEC);
			goto end_func;
		}
	}else{
		fprintf(stderr,ERR_MSG_NOT_EXIST_DIR);
		rc = ERR_RC_NOT_EXIST_DIR;
	}
	if( rc == 0 ){
		*dir         = strdup(dname);
		*base        = strdup(bname);
		*length_dir  = len_dir;
		*length_base = len_name;
		if( *rest != NULL ){
			*length_rest = strlen(*rest);
		}else{
			*length_rest = 0;
		}
	}
end_func:
	free(dirc);
	free(basec);
	;;;;;;;;;;debug_printf(stderr,"Dbg:[ParseDir()] end.\n");
	return rc;
}

/* -------------------------------------------------------------------------
 *  store command / check command / execute command
 *  parm   : 
 *           iProcType           text            uiNum
 *           ------------------- -------------  ---------------
 *           MYEXEC_PROC_STORE   command line    'c' or 'C'
 *           MYEXEC_PROC_CHECK    (not use)      (not use)
 *           MYEXEC_PROC_EXECUTE  (not use)      sleep microsec
 *  return :  0       - ok
 *            others  - error
 *  N.B. not thread safe
 */
int MyExec(const int iProcType,const char *text ,const unsigned int  uiNum){
	static char szExec[BUF_LEN]={0};
	static bool fCheckFile = true;
	switch( iProcType ){
		case MYEXEC_PROC_STORE:
			if( text != NULL ){ // this is command
				int i,k;
				for( i=0 ; text[i] == ' ' ; i++ ); // skip blank of head
				if( text[i] != '/' ){
					fprintf(stderr,"Err:command is not set as fullpath (-c/-C)\n");
					return -1;
				}
				k=strlen(&text[i]);
				if( k < sizeof(szExec) ){
					if( szExec[0] != 0x00 ) memset(szExec,0x00,sizeof(szExec));
					memcpy(szExec,&text[i],k);
					;;;;;;;;;;debug_printf(stderr,"Dbg:stored exec=[%s]\n",szExec);
				}else{
					fprintf(stderr,"Err:command is too long set as fullpath (-c/-C)\n");
					return -1;
				}
				if( uiNum == (unsigned int)'C' ){
					fCheckFile = false;
				}else{ // 'c'
					fCheckFile = true;
				}
				return 0;
			}else{
				fprintf(stderr,"Err:command is NULL in MyExec()\n");
				return -1;
			}
			break;
		case MYEXEC_PROC_CHECK:
			if( szExec[0] != 0x00 ){
				if( fCheckFile == true ){
					char *dirname  = NULL;
					char *basename = NULL;
					char *rest     = NULL;
					int   len_dir  = 0;
					int   len_name = 0;
					int   len_rest = 0;
					int   rc       = ParseDir(szExec,&dirname,&basename,&rest,&len_dir,&len_name,&len_rest);
					if( rc == 0  ){
						if( rest != NULL ){
							if( len_dir + len_name + len_rest + 8 < sizeof(szExec) ){
								sprintf(szExec,"\"%s\"/\"%s\" %s&",dirname,basename,rest);
							}else{
								fprintf(stderr,"Err: too long command line\n");
								return -70;
							}
						}else{
							if( len_dir + len_name + 7 < sizeof(szExec) ){
								sprintf(szExec,"\"%s\"/\"%s\"&",dirname,basename);
							}else{
								fprintf(stderr,"Err: too long command line\n");
								return -70;
							}
						}
					}
				}else{
					char szTmp[BUF_LEN] = {0x00};
					int  len;
					len=strlen(szExec);
					if( len < (sizeof(szExec)-4) ){
						szTmp[0] = '"';
						memcpy(&szTmp[1],szExec,len);
						szTmp[len+1] = '"';
						szTmp[len+2] = '&';
						memcpy(szExec,szTmp,len+3);
					}
				}
				fprintf(stderr,"Inf:command is [%s]\n",szExec);
				return 0;
			}else{
				return -1;
			}
			break;
		case MYEXEC_PROC_EXECUTE:
			if( szExec[0] != 0x00 ){
				int  rc;
				;;;;;;;;;;debug_printf(stderr,"Dbg:in MyExec(%s,%u)\n",szExec,uiNum);
				rc=system(szExec);
				if( WIFEXITED(rc) ){
					if( WEXITSTATUS(rc) == 0 ){
						if( uiNum > 0 ){
							MySetPriority(MY_PRIORITY_SET,19 );// set this program's priority to min.
							usleep(uiNum);
							MySetPriority(MY_PRIORITY_RESTORE, 0 );// restore  this program's priority.
						}
						return 0;
					}
				}
				return -5;
			}else{
				return -1;
			}
			break;
		default:
			return -99;
	}
	return -999;
}

/* ---------------------------------------------
 * main proc
 */
int main(int argc,char **argv){
	Display 	*disp = NULL;
	MyWinList 	 WinList;
	int 		 iRegField=REG_FIELD_NONE;
	int 		 iRc;

	setlocale(LC_ALL, "");

	if( !(disp = XOpenDisplay(NULL)) ){
		fprintf(stderr,"Err:cannot open X display.\n");
		MyExit(NULL,EXIT_FAILURE);
	}

	if( argc > 1 ){
		char 			 szBuf[BUF_LEN];
		int 			 i;
		regex_t 		 preg;
		regmatch_t		*pmatch = NULL;
		size_t 			 nmatch = 0;
		int  			 option;
		int  			 action          = ACTION_LIST;
		char 			 szSearchString[BUF_LEN]={0x00};
		bool 			 fDisplaySearch  = false;
		int  			 action_activate = ACTION_ACTIVATE_NONE;
		int  			 action_target   = TARGET_1ST;
		int  			 action_move     = ACTION_MOVE_NONE;
		long 			 move_x       = -1, move_y       = -1;
		long 			 move_x_Exec  = -1, move_y_Exec  = -1;
		long 			 move_x_Exist = -1, move_y_Exist = -1;
		unsigned int	 ui_usleep_wait_exec = WAIT_AFTER_EXEC_MICROSECOND;
		unsigned int	 ui_usleep_wait=WAIT_MICROSECOND_ACTIVATE_AFTER_MOVE;
		int 			 i_retry_cnt_winget=MAX_RETRY_CNT_WINGET;
		bool    		 fExec = false;

		if( ( argc == 2 ) && argv[1] ){
			if( strcmp(argv[1], "--help" ) == 0 ){
				fputs(HELP, stdout);
				MyExit(disp,EXIT_SUCCESS);
			}else if( strcmp(argv[1], "--version" ) == 0 ){
				fputs(VERSION, stdout);
				MyExit(disp,EXIT_SUCCESS);
			}
		}

		for( i=0 ; i < SORT_MAX ; i++ ){
			SortConfig[i].iSortField	= SORT_NONE;
			SortConfig[i].iOrder		= SORT_ORDER_ASCENDING;
		}

		// parse arguments
		while( (option = getopt(argc,argv,"lL:aAc:C:swo:pP:mft:g:G:T:r:S:D")) != -1 ){
			switch( option ){
				case 'D':
					fActionAllDesktop  = true;
					break;
				case 'l':
					action = ACTION_LIST;
					break;
				case 'L':
					action = ACTION_LIST_SPECIFIED;
					if( strlen(optarg) > sizeof(szSearchString) ){
						fprintf(stderr,"Err:too long argument (-L)\n");
						MyExit(disp,EXIT_FAILURE);
					}
					strcpy(szSearchString,optarg);
					if( iRegField == REG_FIELD_NONE )	iRegField=REG_FIELD_TITLE;
					break;
				case 'a':
					action_activate = ACTION_ACTIVATE_SWITCH_DESKTOP;
					break;
				case 'A':
					action_activate = ACTION_ACTIVATE_MOVE_DESKTOP;
					break;
				case 'f':
					action_target  = TARGET_FULL;
					break;
				case 'c':
				case 'C':
					if( MyExec(MYEXEC_PROC_STORE,optarg ,(unsigned int) option) != 0 ){
						MyExit(disp,EXIT_FAILURE);
					}
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
				case 'g':	// grid (x,y) -- existing
				case 'G':	// grid (x,y) -- at exec
					if( sscanf(optarg, "%ld,%ld", &move_x, &move_y) != 2 ){
						fprintf(stderr, "Err:invalid format grid ( -g / -G ) \n" );
						MyExit(disp,EXIT_FAILURE);
					}else{
						int screen = DefaultScreen(disp);
						if( move_x > DisplayWidth(disp, screen) ){
							fprintf(stderr, "Err:invalid value x in -g / -G ( too large )\n" );
							MyExit(disp,EXIT_FAILURE);
						}
						if( move_y > DisplayHeight(disp, screen) ){
							fprintf(stderr, "Err:invalid value y in -g / -G ( too large )\n" );
							MyExit(disp,EXIT_FAILURE);
						}
						if( option == 'g' ){
							move_x_Exist = move_x;
							move_y_Exist = move_y;
							if( action_move == ACTION_MOVE_NONE ){
								action_move   = ACTION_MOVE_MOVE_ONLY_EXIST;
							}else{
								action_move   = ACTION_MOVE_MOVE_BOTH;
							}
						}else{ // 'G'
							move_x_Exec = move_x;
							move_y_Exec = move_y;
							if( action_move == ACTION_MOVE_NONE ){
								action_move   = ACTION_MOVE_MOVE_ONLY_EXECUTE;
							}else{
								action_move   = ACTION_MOVE_MOVE_BOTH;
							}
						}
					}
					break;
				case 'o':  // order by
					{
						int i,j=0;
						for( i = 0 ; i < SORT_MAX ; i++ ){
							SortConfig[i].iSortField=optarg[j]-'0';
							if(( SortConfig[i].iSortField > SORT_MAX ) || ( SortConfig[i].iSortField < 1) ){
								fprintf(stderr, "Err:invalid sort filed (-o) %c\n",optarg[j] );
								MyExit(disp,EXIT_FAILURE);
							}
							if( optarg[j+1] == ',' ){
								if( optarg[j+2] == 'a' ){
									SortConfig[i].iOrder=SORT_ORDER_ASCENDING;
								}else if( optarg[j+2] == 'd' ){
									SortConfig[i].iOrder=SORT_ORDER_DESCENDING;
								}else{
									fprintf(stderr, "Err:invalid sort order (-o) %c.\n",optarg[j+2] );
									MyExit(disp,EXIT_FAILURE);
								}
								if( optarg[j+3] == ',' ){
									j=j+4;
								}else{
									break; // exit for
								}
							}else if( optarg[j+1] != '\0' ){
								fprintf(stderr, "Err:invalid format sort (-o) \n" );
								MyExit(disp,EXIT_FAILURE);
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
					if( strlen(optarg) > sizeof(szSearchString) ){
						fprintf(stderr,"Err:too long argument (-P)\n");
						MyExit(disp,EXIT_FAILURE);
					}
					strcpy(szSearchString,optarg);
					break;
				case 't':
					if( atoi(optarg) < 0 ){
						fprintf(stderr,"Err:too small wait time (-t)\n");
						MyExit(disp,EXIT_FAILURE);
					}
					ui_usleep_wait=(unsigned int)atoi(optarg);
//					ui_usleep_wait=(unsigned int)strtoul(optarg, 0, 10);
					break;
				case 'T':
					if( atoi(optarg) < 0 ){
						fprintf(stderr,"Err:too small wait time (-T)\n");
						MyExit(disp,EXIT_FAILURE);
					}
					ui_usleep_wait_exec=(unsigned int)atoi(optarg);
//					ui_usleep_wait_exec=(unsigned int)strtoul(optarg, 0, 10);
					break;
				case 'r':
					if( atoi(optarg) < 0 ){
						fprintf(stderr,"Err:too small retry count (-r)\n");
						MyExit(disp,EXIT_FAILURE);
					}
					i_retry_cnt_winget=atoi(optarg);
					break;
				case 'S':
					{
						bool  			 fInvalidParm = false;
						if(strlen(optarg) > 3 ){ 
							if(			optarg[0] == 'a' ){
							}else if(	optarg[0] == 'r' ){
							}else if(	optarg[0] == 't' ){
							}else{
								fInvalidParm = true;
							}
							if( fInvalidParm == false ){
								if( optarg[1] == ',' ){
									if( MyChengeState(MYSTATE_PROC_STORE,optarg[0],&optarg[2],disp,0) != EXIT_SUCCESS ){
										fInvalidParm = true;
									}
								}else{
									fInvalidParm = true;
								}
							}
						}
						if( fInvalidParm == true ){
							fprintf(stderr,"Err:invalid format (-S)\n");
							MyExit(disp,EXIT_FAILURE);
						}
					}
					break;
				default:
					fprintf(stderr,"Err:invalid option\n");
					fputs(HELP, stdout);
					MyExit(disp,EXIT_FAILURE);
					break;
			}	/* end switch */
		}		/* end while getopt()*/

		if( action == ACTION_LIST ){
			if( iRegField != REG_FIELD_NONE ){
				if( iRegField == REG_FIELD_CLASS ){
					fprintf(stderr,"Inf:ignore -w option becaseof not -L option specified.\n");
				}else if( iRegField == REG_FIELD_MACHINE ){
					fprintf(stderr,"Inf:ignore -m option becaseof not -L option specified.\n");
				}
				iRegField = REG_FIELD_NONE;
			}
		}

		if( action_move != ACTION_MOVE_NONE ){
			if( action_activate == ACTION_ACTIVATE_NONE ){
				fprintf(stderr,"Inf:ignore -g / -G option becaseof -a or -A option not specified.\n");
				action_move = ACTION_MOVE_NONE ;
			}
			if( action_target  == TARGET_FULL ){
				if( ( action_move == ACTION_MOVE_MOVE_ONLY_EXIST )||
					 ( action_move == ACTION_MOVE_MOVE_BOTH )){
					fprintf(stderr,"Inf:ignore -g option becaseof -f option specified.\n");
					if( action_move == ACTION_MOVE_MOVE_ONLY_EXIST ){
						action_move = ACTION_MOVE_NONE ;
					}else{
						action_move = ACTION_MOVE_MOVE_ONLY_EXECUTE;
					}
				}
			}
		}

		// prepare regex
		if( szSearchString[0] != 0x00 ){
			int iRc;
			if( fDisplaySearch == true ){
				fprintf(stderr,"Inf:search-string is [%s]\n",szSearchString);
			}
			// compile regexp
			iRc = regcomp(&preg,szSearchString,REG_EXTENDED|REG_NEWLINE);
			if( iRc != 0 ){
				// Error!, print error reason
				regerror(iRc, &preg, szBuf, sizeof(szBuf));
				fprintf(stderr,"Err:regcomp() failed with '%s'\n", szBuf);
				MyExit(disp,EXIT_FAILURE);
			}else{
				// compile complete! allocate pmatch
				nmatch = preg.re_nsub + 1;
				if( ( pmatch = calloc(nmatch,sizeof(regmatch_t))) == NULL ){
					fprintf(stderr, "Err:cannot allocate memory for pmatch\n" );
					MyExit(disp,EXIT_FAILURE);
				}
			}
		}else{
			if(( action == ACTION_LIST ) ||  ( action == ACTION_LIST_PID )){
				// nop
			}else{
				// unexpected case
				fprintf(stderr, "Err:<SEARCH> not specified!\n");
				MyExit(disp,EXIT_FAILURE);
			}
		}

		if(( action == ACTION_LIST_PID ) ||
			( action == ACTION_LIST_PID_SPECIFIED )){
			// specified -p / -P 
			// get Process List
			MyPList plist;

			plist.iCnt=0;

			if( action == ACTION_LIST_PID ){
				GetProcessList(&plist,NULL,NULL);
			}else{
				GetProcessList(&plist,&preg,pmatch);
			}

			if( plist.iCnt == 0 ){
				if( MyExec(MYEXEC_PROC_CHECK,NULL ,0) == 0 ){
					if(( action_move == ACTION_MOVE_MOVE_ONLY_EXECUTE )||
						( action_move == ACTION_MOVE_MOVE_BOTH)){
						if( MyExec(MYEXEC_PROC_EXECUTE,NULL ,ui_usleep_wait_exec) == 0 ){
							if( action == ACTION_LIST_PID ){
								GetProcessList(&plist,NULL,NULL);
							}else{
								GetProcessList(&plist,&preg,pmatch);
								regfree(&preg);
							}
						}else{
							MyExit(disp,EXIT_FAILURE);
						}
					}else{
						if( MyExec(MYEXEC_PROC_EXECUTE,NULL ,0) == 0 ){
							if( action != ACTION_LIST_PID ){
								regfree(&preg);
							}
							MyExit(disp,EXIT_SUCCESS);
						}else{
							MyExit(disp,EXIT_FAILURE);
						}
					}
					fExec = true;
				}else{
					fprintf(stderr, "Inf:count of process is 0\n");
					if( action != ACTION_LIST_PID ){
						regfree(&preg);
					}
					MyExit(disp,EXIT_FAILURE);
				}
			}else{
				ui_usleep_wait_exec = 0;
			}
			if( plist.iCnt > 0 ){
				int iDigitCnt = digit(plist.iCnt+1);
				if( SortConfig[0].iSortField != SORT_NONE ){
					qsort(plist.pdata,plist.iCnt,sizeof(MyPData),MyCompPList);
				}
				print_list_pid(iDigitCnt,&plist);
				if( action_activate != ACTION_ACTIVATE_NONE ){
					iRc=MyGetWinList(disp,&WinList,REG_FIELD_NONE,NULL,NULL,ui_usleep_wait_exec,i_retry_cnt_winget);
					if( iRc == EXIT_SUCCESS ){
						if( WinList.iCnt > 0 ){
							int k,m,p=1;
							SortConfig[0].iSortField = SORT_BY_PID;
							SortConfig[0].iOrder=SORT_ORDER_ASCENDING;
							qsort(WinList.data,WinList.iCnt,sizeof(MyWinData),MyCompWin);
							if( action_target  == TARGET_FULL ){
								p=plist.iCnt;
							}
							for( k=0 ; k < p ; k++ ){
								for( m=0 ; m < WinList.iCnt ; m++ ){
									if( WinList.data[m].pid == (unsigned long)plist.pdata[k].PID ){
										if( action_activate == ACTION_ACTIVATE_SWITCH_DESKTOP ){
											activate_window(disp, WinList.win[WinList.data[m].iOriginalIndex],true);
										}else{ // move desktop
											move_and_activate_window(disp, WinList.win[WinList.data[m].iOriginalIndex],ui_usleep_wait);
										}
										if( action_move != ACTION_MOVE_NONE ){
											if( fExec == true ){
												if( action_move != ACTION_MOVE_MOVE_ONLY_EXIST ){
													window_move(disp, WinList.win[WinList.data[m].iOriginalIndex], move_x_Exec,move_y_Exec,fExec);
												}
											}else{
												if( action_move != ACTION_MOVE_MOVE_ONLY_EXECUTE ){
													window_move(disp, WinList.win[WinList.data[m].iOriginalIndex], move_x_Exist,move_y_Exist,fExec);
												}
											}
										}
									}else if( WinList.data[m].pid > (unsigned long)plist.pdata[k].PID ){
										break;	// for performance
									}
								}
							}
							MyExit(disp,EXIT_SUCCESS);
						}
					}else{
						fprintf(stderr,"Err:cannot getWinList .\n");
						MyExit(disp,EXIT_FAILURE);
					}	
				}
			}
		}else{
			// specified -l / -L 
			// get Window List
			;;;;;;;;;;debug_printf(stderr,"Dbg:call MyGetWinList\n");
			if( iRegField != REG_FIELD_NONE ){
				iRc=MyGetWinList(disp,&WinList,iRegField,&preg,pmatch,0,i_retry_cnt_winget);
			}else{
				iRc=MyGetWinList(disp,&WinList,REG_FIELD_NONE,NULL,NULL,0,i_retry_cnt_winget);
			}
			;;;;;;;;;;debug_printf(stderr,"Dbg:MyGetWinList,rc=%d.\n",iRc);
			if( iRc == EXIT_SUCCESS ){
				if( WinList.iCnt == 0 ){
					;;;;;;;;;;debug_printf(stderr,"Dbg:WinList Cnt == 0.\n");
					if( MyExec(MYEXEC_PROC_CHECK,NULL ,0) == 0 ){
						unsigned int wait_microsec = 0;
						if(	( action_move 		== ACTION_MOVE_MOVE_ONLY_EXECUTE 	)	||
								( action_move 		== ACTION_MOVE_MOVE_BOTH 			)	||
								( action_activate 	== ACTION_ACTIVATE_SWITCH_DESKTOP 	)	||
								( action_activate 	== ACTION_ACTIVATE_MOVE_DESKTOP   	) ){
							wait_microsec = ui_usleep_wait_exec;
							if( wait_microsec == 0 ) wait_microsec = 1;
						}
						;;;;;;;;;;debug_printf(stderr,"Dbg:command will be execute.wait_sec=%u\n",wait_microsec);
						if( MyExec(MYEXEC_PROC_EXECUTE,NULL ,wait_microsec) == 0 ){
							fExec = true ;
							if( wait_microsec > 0 ){
								if( iRegField != REG_FIELD_NONE ){
									iRc=MyGetWinList(disp,&WinList,iRegField,&preg,pmatch,ui_usleep_wait_exec,i_retry_cnt_winget);
									regfree(&preg);
								}else{
									iRc=MyGetWinList(disp,&WinList,REG_FIELD_NONE,NULL,NULL,ui_usleep_wait_exec,i_retry_cnt_winget);
								}
								;;;;;;;;;;debug_printf(stderr,"Dbg:MyGetWinList(2),rc=%d.\n",iRc);
								if( iRc != EXIT_SUCCESS ){
									fprintf(stderr,"Err:cannot getWinList(2) .\n");
									MyExit(disp,EXIT_FAILURE);
								}else{
									if( WinList.iCnt > 0 ){
										if(	( action_move == ACTION_MOVE_MOVE_ONLY_EXECUTE )||
											( action_move == ACTION_MOVE_MOVE_BOTH )){
											window_move(disp, WinList.win[WinList.data[0].iOriginalIndex], move_x_Exec,move_y_Exec,fExec);
											;;;;;;;;;;debug_printf(stderr,"Dbg:Window Moved after exec.\n");
										}
										if(	( action_activate == ACTION_ACTIVATE_SWITCH_DESKTOP )	||
											( action_activate == ACTION_ACTIVATE_MOVE_DESKTOP   ) ){
											if( action_activate == ACTION_ACTIVATE_SWITCH_DESKTOP ){
												activate_window(disp, WinList.win[WinList.data[0].iOriginalIndex],true);
											}else{
												move_and_activate_window(disp, WinList.win[WinList.data[0].iOriginalIndex],ui_usleep_wait);
											}
										}
									}else{
										;;;;;;;;;;debug_printf(stderr,"Dbg:WinList.iCnt==0.\n");
									}
								}
							}
							MyExit(disp,EXIT_SUCCESS);
						}else{
							MyExit(disp,EXIT_FAILURE);
						}
					}else{
						// check error.
						;;;;;;;;;;debug_printf(stderr,"Dbg:MyExec(MYEXEC_PROC_CHECK,NULL ,0)!=0.\n");
						MyExit(disp,EXIT_FAILURE);
					}
				}else{
					if( iRegField != REG_FIELD_NONE ){
						regfree(&preg);
					}
				}
				if( WinList.iCnt > 0 ){
					if( SortConfig[0].iSortField != SORT_NONE ){
						qsort(WinList.data,WinList.iCnt,sizeof(MyWinData),MyCompWin);
					}else{
						;;;;;;;;;;debug_printf(stderr,"Dbg:SORT_NONE\n");
					}
					switch( action ){
						case ACTION_LIST:
						case ACTION_LIST_SPECIFIED:
							DisplayWinList(&WinList);
							break;
						default:
							// nop!
							break;
					}
					if( (action_activate == ACTION_ACTIVATE_SWITCH_DESKTOP )||( action_activate == ACTION_ACTIVATE_MOVE_DESKTOP )){
						int k,m=1;
						if( action_target  == TARGET_FULL ){
							m=WinList.iCnt;
						}
						for( k=0 ; k < m ; k++ ){
							if( action_activate == ACTION_ACTIVATE_SWITCH_DESKTOP ){
								activate_window(disp, WinList.win[WinList.data[k].iOriginalIndex],true);
							}else{
								move_and_activate_window(disp, WinList.win[WinList.data[k].iOriginalIndex],ui_usleep_wait);
							}
							if(( action_move == ACTION_MOVE_MOVE_ONLY_EXIST )||
								( action_move == ACTION_MOVE_MOVE_BOTH )){
								window_move(disp, WinList.win[WinList.data[k].iOriginalIndex], move_x_Exist,move_y_Exist,fExec);
							}
						}
					}else{ // nop
					}
				}
			}else{
				fprintf(stderr,"Err:cannot getWinList .\n");
				MyExit(disp,EXIT_FAILURE);
			}
		}
	}else{
		// simply list
		iRc=MyGetWinList(disp,&WinList,REG_FIELD_NONE,NULL,NULL,0,0);
		if( iRc == EXIT_SUCCESS ){
			DisplayWinList(&WinList);
		}else{
			fprintf(stderr,"Err:cannot get Window List.\n");
			MyExit(disp,EXIT_FAILURE);
		}
	}
	MyExit(disp,EXIT_SUCCESS);
	return 0;
}