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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <regex.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

// #include <math.h>
#include <sys/resource.h>

#include "mymem.h"
#include "awin.h"

#include "mywin.h"

//#define DEBUG

//  ref  http://tricky-code.net/nicecode/code10.php
#ifdef DEBUG
#define debug_printf fprintf
#else
#define debug_printf 1 ? (void) 0 : fprintf
#endif


/* global variable */
extern MySortConfig SortConfig[SORT_MAX];
bool 	fActionAllDesktop  = false;


#define MAX_PROPERTY_VALUE_LEN 4096


/* ------------------------------- */
/* prototype for internal func(s)  */
/* ------------------------------- */

bool fGetWinFramePos(Display *disp, const Window win,int *x,int *y);
unsigned char	*get_prop(Display *disp, Window win,
							Atom xa_prop_type, const char *prop_name, int *pret_format,unsigned long *pret_ntimes);
Window 			*get_client_list(Display *disp, unsigned long *size);
unsigned long 	 get_pid(Display *disp,Window win);
char 			*get_property(Display *disp, Window win,
								Atom xa_prop_type, const char *prop_name, unsigned long *size);
int 			 client_msg(Display *disp, Window win, char *msg,unsigned long data0, unsigned long data1,
											unsigned long data2, unsigned long data3,unsigned long data4);
int 			 window_to_desktop(Display *disp, Window win, int desktop);
char 			*get_machine(Display *disp,Window win);
char 			*get_wmclass(Display *disp,Window win);
char 			*get_title(Display *disp,Window win);
int 			 MyCompWinA(const void *a,const void *b,const int iCompTimes);
int 			 MyCheckDuplicate(const MyWinData *a,const MyWinData *b);
int 			 window_to_all_desktop(Display *disp, Window win);


/* ------------------------------- */
/* public func(s)                  */
/* ------------------------------- */

/* ----------------------------------
 *  function : Calc number of digits 
 *  parms    : num - number
 *  return   : number of digits
 */
int digit(const unsigned long num){
	int iRc=1;
	if( num > 9 ){
		if( num > 99 ){
			if( num > 999 ){
				if( num > 9999 ){
					if( num > 99999 ){
						if( num > 999999 ){
							if( num > 9999999 ){
								if( num > 99999999 ){
									if( num > 999999999 ){
										if( num > 9999999999 ){
											iRc = 11;
										}else{
											iRc = 10;
										}
									}else{
										iRc = 9;
									}
								}else{
									iRc = 8;
								}
							}else{
								iRc = 7;
							}
						}else{
							iRc = 6;
						}
					}else{
						iRc = 5;
					}
				}else{
					iRc = 4;
				}
			}else{
				iRc = 3;
			}
		}else{
			iRc = 2;
		}
	}
	return iRc;
//	return ((int)log10(num) + 1);
}

/* ----------------------------------
 *  function : Set / Restore Priority of this process 
 *  parms    :  iProcType - MY_PRIORITY_SET / MY_PRIORITY_RESTORE
 *              iPrio     - priority to set
 */
void MySetPriority(int iProcType,int iPrio ){
	static int saved_prio = 9999;
	switch(iProcType){
		case MY_PRIORITY_SET:
			errno = 0;
			saved_prio = getpriority(PRIO_PROCESS,0);
			if( errno == 0 ){
				if( iPrio >  19 ) iPrio =  19; // lowest nice value
				if( iPrio < -20 ) iPrio = -20; // highest nice value
				setpriority(PRIO_PROCESS, 0, iPrio);
			}else{
				;;;;;;;;;;debug_printf(stderr,"Dbg:MySetPriority(MY_PRIORITY_SET),error(%d)\n",errno);
				saved_prio = 9999;
			}
			break;
		case MY_PRIORITY_RESTORE: // not use iPrio 
			if( saved_prio != 9999 ){
				errno = 0;
				setpriority(PRIO_PROCESS, 0, saved_prio);
				if( errno != 0 ){
					;;;;;;;;;;debug_printf(stderr,"Dbg:MySetPriority(MY_PRIORITY_RESTORE),error(%d)\n",errno);
				}
				saved_prio = 9999;
			}else{
				;;;;;;;;;;debug_printf(stderr,"Dbg:MySetPriority(MY_PRIORITY_RESTORE),saved_prio == 9999\n");
			}
	}
}

#define _NET_WM_STATE_UNSET        -1    /* unset */
#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

/* ----------------------------------
 *  function : change state of window 
 *  parms    :  iProcType - MYSTATE_PROC_STORE / MYSTATE_PROC_DO
 *              action    - 'a' / 'r' / 't'
 *              option    - state
 *              disp      - display
 *              win       - Window
 *  return   : EXIT_SUCCESS - no error
 *             EXIT_FAILURE - error
 */
int	MyChengeState(int iProcType,char action, char *option,Display *disp, Window win){
	static unsigned long	 saved_action=_NET_WM_STATE_UNSET;
	static char 			 saved_option[2][35]={{0x00},{0x00}};
	int 					 i;
	char					*ListValid[]={
								"ABOVE",
								"BELOW",
								"STICKY",
								"FULLSCREEN",
								"MAXIMIZED_VERT",
								"MAXIMIZED_HORZ",
								"SHADED",
								"SKIP_TASKBAR",
								"SKIP_PAGER",
								"HIDDEN",
								"MODAL",
								"DEMANDS_ATTENTION"
							}; // https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html
	switch(iProcType){
		case MYSTATE_PROC_STORE: // not use disp,win
			;;;;;;;;;;debug_printf(stderr,"Dbg:action:[%c],option[%s]\n",action,option);
			saved_action = _NET_WM_STATE_UNSET;
			switch( action ){
				case 'a':
				case 'A':
					saved_action = _NET_WM_STATE_ADD;
					break;
				case 'r':
				case 'R':
					saved_action = _NET_WM_STATE_REMOVE;
					break;
				case 't':
				case 'T':
					saved_action = _NET_WM_STATE_TOGGLE;
					break;
			}
			if( saved_action != _NET_WM_STATE_UNSET ){
				char *pComma= strchr(option, ',');
				bool  fOption1=false,fOption2=false;
				if( pComma != NULL ){
					;;;;;;;;;;debug_printf(stderr,"Dbg:pComma-option=%d\n",(int)(pComma-option));
					sprintf(saved_option[0],"_NET_WM_STATE_%.*s",(int)(pComma-option),option);
					sprintf(saved_option[1],"_NET_WM_STATE_%s",++pComma);
				}else{
					sprintf(saved_option[0],"_NET_WM_STATE_%s",option);
				}
				;;;;;;;;;;debug_printf(stderr,"Dbg:saved_option[0][14][%s]\n",&(saved_option[0][14]));
				;;;;;;;;;;debug_printf(stderr,"Dbg:saved_option[1][14][%s]\n",&(saved_option[1][14]));
				for( i=0 ; i < ( sizeof(ListValid)/sizeof(char *)) ; i++ ){
					;;;;;;;;;;debug_printf(stderr,"Dbg:[%d][%s]\n",i,ListValid[i]);
					if( strcmp(&(saved_option[0][14]),ListValid[i]) == 0 ){
						fOption1=true;
						if(( saved_option[1][0] == 0x00 ) ||
						   ( fOption2 == true )) break;
					}
					if( saved_option[1][0] != 0x00 ){
						if( strcmp(&(saved_option[1][14]),ListValid[i]) == 0 ){
							fOption2=true;
							if( fOption1 == true ) break;
						}
					}
				}
				;;;;;;;;;;debug_printf(stderr,"Dbg:saved_option[0][%s]\n",saved_option[0]);
				;;;;;;;;;;debug_printf(stderr,"Dbg:saved_option[1][%s]\n",saved_option[1]);
				if( fOption1 == true ){
					if(( saved_option[1][0] == 0x00 ) || ( fOption2 == true )){
						return EXIT_SUCCESS;
					}
				}
				// error! -> re-initialize static variable
				memset(saved_option,0x00,sizeof(saved_option));
				saved_action = _NET_WM_STATE_UNSET;
			}
			break;
		case MYSTATE_PROC_DO: // not use action, option
			if( ( saved_action != _NET_WM_STATE_UNSET )&&( saved_option[0] != 0x00 )){
				Atom 			 prop[2]={0,0};
				prop[0] = XInternAtom(disp, saved_option[0], False);
				if( saved_option[1] != 0x00 ){
					prop[1] = XInternAtom(disp, saved_option[1], False);
				}
				return client_msg(disp, win, "_NET_WM_STATE", saved_action,
								  (unsigned long)prop[0], (unsigned long)prop[1], 0, 0);
			}else{
				if( saved_action == _NET_WM_STATE_UNSET ){
					// notiong to do 
					return EXIT_SUCCESS;
				}
			}
			break;
		default:
			;;;;;;;;;;debug_printf(stderr,"Dbg:invalid iProcType in MyChengeState().\n");
	}
	return EXIT_FAILURE;
}


/* ----------------------------------
 *  function : Move window 
 *  parms    :  disp  - display
 *              win   - Window
 *              x     - geometry ( x )
 *              y     - geometry ( y )
 *  return   : EXIT_SUCCESS - no error
 *             EXIT_FAILURE - error
 *  N.B.     : If x , y is under 0 , then use the value of current window position.
 *             Sometimes, XMoveWindow() is done but the window was not moved, so retrying
 */

int window_move(Display *disp, const Window win, long x,long y,const bool fExec){
	Status 				win_state;
	XWindowAttributes 	win_attributes;
	int  				rc     = EXIT_FAILURE;
	int 				org_x, org_y;

	if(( x < 0 )&&( y < 0 )){
		// no process
		return EXIT_SUCCESS;
	}

	MySetPriority(MY_PRIORITY_SET,19 );// set this program's priority to min.

	// wait window become Drawable ( if not mapped(minimized),then map(restore) )
	for(win_state = BadDrawable;win_state == BadDrawable;){
		win_state=XGetWindowAttributes(disp,win,&win_attributes);
		if( win_state == BadDrawable ){
			;;;;;;;;;;debug_printf(stderr,"Dbg:BadDrawable\n");
		}else if( win_state == BadWindow ){ // window is closed?
			;;;;;;;;;;debug_printf(stderr,"Dbg:BadWindow\n");
			break;
		}else{
			switch( win_attributes.map_state ){
				case IsViewable:
				case IsUnviewable:
					;;;;;;;;;;debug_printf(stderr,"Dbg:IsViewable / IsUnviewable\n");
					break;
				case IsUnmapped:
					;;;;;;;;;;debug_printf(stderr,"Dbg:IsUnmapped\n");
					XMapWindow(disp,win);
					XFlush(disp);
					usleep(WAIT_MICROSECOND_AFTER_MAP);
					break;
				default:
					win_state = BadDrawable;   // to continue for loop
					usleep(WAIT_MICROSECOND_AFTER_MAP);
			} /* end switch */
		}
	} /* end for */
	
	if( win_state == BadWindow ){
	}else{
		if(( x < 0 )||( y < 0 )){
			if( fGetWinFramePos(disp, win,&org_x,&org_y) ){
				if( x < 0 ) x = org_x;
				if( y < 0 ) y = org_y;
			}
		}
		// any error ?
		if( x < 0 ) x = 0;
		if( y < 0 ) y = 0;
		if( fExec == false ){
			;;;;;;;;;;debug_printf(stderr,"Dbg:fExec == false \n");
			if( XMoveWindow(disp, win, x, y) != BadWindow ){
				;;;;;;;;;;debug_printf(stderr,"Dbg:window_move(%ld,%ld) end SUCCESS\n",x,y);
				rc = EXIT_SUCCESS;
			}
		}else{
			int  moved_x,moved_y;
			bool fSaveOrgPos = false;
			int  cntRetry    = 0;
			;;;;;;;;;;debug_printf(stderr,"Dbg:fExec == true \n");

			// wait to be manipulatable (really?)
//			XMapWindow(disp,win);						// if the window is already mapped, then no effect. so safety.
//			XSync(disp, False);						// flush buffer and wait X-server complete process ( need to handle error? )
//			XFlush(disp);
//			usleep(WAIT_MICROSECOND_AFTER_MAP);
			// wait end

			// store current position
			if( fGetWinFramePos(disp, win,&org_x,&org_y) ){
				;;;;;;;;;;debug_printf(stderr,"Dbg:x,y=(%d,%d))\n",org_x,org_y);
				fSaveOrgPos = true;
			}else{
				;;;;;;;;;;debug_printf(stderr,"Dbg:couldnot save org_x,org_y\n");
			}
			// move window and check	
			XSynchronize(disp,true);
			win_state = XMoveWindow(disp, win, x, y);
			XSynchronize(disp,false);
			for( ;( win_state != BadWindow )&&( cntRetry < MAX_RETRY_CNT_WIN_MOVE );cntRetry++ ){
				;;;;;;;;;;debug_printf(stderr,"Dbg:window_move(%ld,%ld) end SUCCESS\n",x,y);
				usleep(WAIT_MICROSECOND_AFTER_MOVE); // need to wait a little.
				if( fGetWinFramePos(disp, win,&moved_x,&moved_y) ){
					if( ( moved_x == x )&&(moved_y == y ) ){
						// move complete
						;;;;;;;;;;debug_printf(stderr,"Dbg:window moved correctly.x,y=(%ld,%ld)\n",x,y);
						rc = EXIT_SUCCESS;
						break; // exit for loop
					}else{
						if( fSaveOrgPos == true ){
							if( (moved_x != org_x )||(moved_y != org_y ) ){
								// perhaps move complete
								;;;;;;;;;;debug_printf(stderr,"Dbg:window moved perhaps complete.x,y=(%ld,%ld)\n",x,y);
								rc = EXIT_SUCCESS;
								break; // exit for loop
							}else{
								// move is not complete
								;;;;;;;;;;debug_printf(stderr,"Dbg:window moved not complete.x,y=(%ld,%ld)\n",x,y);
								XSynchronize(disp,true);
								win_state = XMoveWindow(disp, win, x, y);
								XSynchronize(disp,false);
							}
						}else{
							;;;;;;;;;;debug_printf(stderr,"Dbg:retry to move and sleep instead of check whether moved or not\n");
							win_state = XMoveWindow(disp, win, x, y);
							XFlush(disp);
							usleep(WAIT_MICROSECOND_AFTER_MOVE);
							break; // exit for loop
						}
					}
				}else{
					;;;;;;;;;;debug_printf(stderr,"Dbg:could not get moved_x,moved_y\n");
					rc = EXIT_FAILURE;
					break; // exit for loop
				}
			} // end for
			if( win_state == BadWindow ){
				rc = EXIT_FAILURE;
			}
		}
	}
	;;;;;;;;;;debug_printf(stderr,"Dbg:window_move end rc=%d\n",rc);
	MySetPriority(MY_PRIORITY_RESTORE, 0 );// restore  this program's priority.
	return rc;
}

/* ----------------------------------
 *  function : Activate window 
 *  parms    :  disp           - display
 *              win            - Window
 *              switch_desktop - switch to the desktop containing the window or not
 *  return   : none
 */
void activate_window(Display *disp, Window win,bool switch_desktop){
	unsigned long *desktop;

	/* desktop ID */
	if(( desktop = (unsigned long *)get_property(disp, win,
													XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL ){
		if(( desktop = (unsigned long *)get_property(disp, win,
													XA_CARDINAL, "_WIN_WORKSPACE", NULL)) == NULL ){
			fprintf(stderr,"Err:cannot find desktop ID of the window.\n");
		}
	}

	if( switch_desktop && desktop ){
		if( client_msg(disp, DefaultRootWindow(disp),
						"_NET_CURRENT_DESKTOP",
						*desktop, 0, 0, 0, 0) != EXIT_SUCCESS ){
			fprintf(stderr,"Err:cannot switch desktop.\n");
		}
	}

	client_msg(disp, win, "_NET_ACTIVE_WINDOW",0, 0, 0, 0, 0);
	XMapRaised(disp, win);
	MyChengeState(MYSTATE_PROC_DO,0x00, NULL,disp,win);
	if( fActionAllDesktop  == true ){
		window_to_all_desktop(disp,win);
	}
}

/* ----------------------------------
 *  function : Activate window ( if window is on different desktop, then move to current desktop )
 *  parms    :  disp           - display
 *              win            - Window
 *              ui_usleep_wait - wait before activate 
 *  return   : none
 */
void move_and_activate_window(Display *disp, Window win,unsigned int ui_usleep_wait){
	if( window_to_desktop(disp, win, -1) == EXIT_SUCCESS ){
		setpriority(PRIO_PROCESS, 0, 19); // set this program's priority to min.
		if( ui_usleep_wait > 0 ){
			usleep(ui_usleep_wait);
		}
	}
	activate_window(disp, win, false);
}
/* ----------------------------------
 *  function : wrapper for getWinList ( retry )
 *  parms    :  disp           - display
 *              pWinList       - address of MyWindowList
 *              iRegField      - the field to be checked by regular expression 
 *              ppreg          - for regular expression ( compiled regular expression )
 *              pmatch         - for regular expression ( result )
 *              uiNum          - microseconds for usleep
 *  return   : EXIT_SUCCESS - no error ( caller should check pWinList->iCnt is 0 or not )
 *             EXIT_FAILURE - error
 */
int MyGetWinList(Display *disp,MyWinList *pWinList,const int iRegField,regex_t *ppreg, regmatch_t* pmatch,unsigned int uiSleepMS,int iRetryCnt){
	int rc;
	;;;;;;;;;;debug_printf(stderr,"Dbg:in MyGetWinList---[%u]\n",uiSleepMS);
	rc = getWinList(disp,pWinList,iRegField,ppreg,pmatch);
	;;;;;;;;;;debug_printf(stderr,"Dbg:getWinList(),rc=%d\n",rc);
	;;;;;;;;;;if( rc == EXIT_SUCCESS ) debug_printf(stderr,"Dbg:pWinList->iCnt=%d\n",pWinList->iCnt);
	if((( rc == EXIT_SUCCESS )&&( pWinList->iCnt == 0 ))&&( uiSleepMS > 0 )&&( iRetryCnt > 0 )){
		int 	i;

		MySetPriority(MY_PRIORITY_SET,19 );// set this program's priority to min.
		for( i = 0 ; i < iRetryCnt ; i++ ){
			;;;;;;;;;;debug_printf(stderr,"Dbg:sleep in MyGetWinList[%d]\n",i);
			usleep(uiSleepMS);
			rc = getWinList(disp,pWinList,iRegField,ppreg,pmatch);
			if(( rc == EXIT_SUCCESS )&&( pWinList->iCnt > 0 )){
				break;
			}
		}
		MySetPriority(MY_PRIORITY_RESTORE, 0 );// restore  this program's priority.
	}
	;;;;;;;;;;debug_printf(stderr,"Dbg:out MyGetWinList---[%d]\n",rc);
	return rc;
}

/* ----------------------------------
 *  function : Get window list
 *  parms    :  disp           - display
 *              pWinList       - address of MyWindowList
 *              iRegField      - the field to be checked by regular expression 
 *              ppreg          - for regular expression ( compiled regular expression )
 *              pmatch         - for regular expression ( result )
 *  return   : EXIT_SUCCESS - no error ( caller should check pWinList->iCnt is 0 or not )
 *             EXIT_FAILURE - error
 */
int getWinList(Display *disp,MyWinList *pWinList,const int iRegField,regex_t *ppreg, regmatch_t* pmatch){
	int 			 i,j;
	Window 			*client_list;
	unsigned long 	 client_list_size;

	if( pWinList == NULL ) return EXIT_FAILURE;

	pWinList->max_client_machine_len 	= 0;
	pWinList->max_wmclass_len 			= 0;
	pWinList->max_title_len 			= 0;
	pWinList->max_pid_digit 			= 0;
	pWinList->iCnt 						= 0;

	if(( client_list = (Window *)get_client_list(disp, &client_list_size) ) == NULL ){
		return EXIT_FAILURE;
	}

	pWinList->win=client_list;
	pWinList->data=MyAssignMem(sizeof(MyWinData)*(client_list_size / sizeof(Window)));

	/* find the longest client_machine name */
	for( i=0, j=0 ; i < client_list_size / sizeof(Window); i++ ){
		char 			*client_machine;
		char 			*wm_name;
		char 			*wm_class;
		unsigned long 	 wm_pid;
		bool 			 fStore;

		client_machine	=NULL;
		wm_name			=NULL;
		wm_class		=NULL;
		// wm_pid			=0;

		if( iRegField == REG_FIELD_NONE ){
			fStore=true;
		}else{
			fStore=false;
		}

		if(( client_machine=get_machine(disp,client_list[i])) == NULL ){
			fprintf(stderr,"Err:cannot get client_machine[%d]\n",i);
		}else{
			if( iRegField == REG_FIELD_MACHINE ){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					if( regexec(ppreg, client_machine, nmatch, pmatch, 0) == 0 ){
						fStore=true;
					}
				}
			}
		}

		if((wm_name=get_title(disp,client_list[i])) == NULL ){
			fprintf(stderr,"Err:cannot get title [%d].\n",i);
		}else{
			if( iRegField == REG_FIELD_TITLE ){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					if( regexec(ppreg, wm_name, nmatch, pmatch, 0) == 0 ){
						fStore=true;
					}
				}
			}
		}
		pWinList->data[j].wm_class=wm_class;
		;;;;;;;;;;debug_printf(stderr,"Dbg:get name of the window manager:[%s].\n",wm_name);

		if( (wm_class = get_wmclass(disp,client_list[i])) == NULL ){
			fprintf(stderr,"Err:cannot get class of the window manager (WM_CLASS)[%d].\n",i);
		}else{
			if( iRegField == REG_FIELD_CLASS ){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					if( regexec(ppreg, wm_class, nmatch, pmatch, 0) == 0 ){
						fStore=true;
					}
				}
			}
		}
		;;;;;;;;;;debug_printf(stderr,"Dbg:get class of the window manager:[%s].\n",wm_class);

		if(( wm_pid = get_pid(disp,client_list[i]) ) == 0 ){
			fprintf(stderr,"Err:cannot get pid of the window manager [%d].",i);
			if( wm_class != NULL ) fprintf(stderr,"it's class is [%s].",wm_class);
			if( wm_name  != NULL ) fprintf(stderr,"it's title is [%s].",wm_name );
			fprintf(stderr,"\n");
		}else{
			if( iRegField == REG_FIELD_PID ){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					char szPID[100];
					sprintf(szPID,"%lu",wm_pid);
					if( regexec(ppreg, szPID, nmatch, pmatch, 0) == 0 ){
						fStore=true;
					}
				}
			}
		}

		;;;;;;;;;;debug_printf(stderr,"Dbg:get PID of the window manager:[%lu].\n",wm_pid);
		if( fStore == true ){
			int n;

			pWinList->data[j].iOriginalIndex=i;

			if( client_machine != NULL ){
				n=strlen(client_machine);
				if( pWinList->max_client_machine_len < n ) pWinList->max_client_machine_len=n;
				pWinList->data[j].client_machine=client_machine;
			}else{
				pWinList->data[j].client_machine=NULL;
			}
			if( wm_name != NULL ){
				n=strlen(wm_name);
				if( pWinList->max_title_len < n ) pWinList->max_title_len=n;
				pWinList->data[j].title=wm_name;
			}else{
				pWinList->data[j].title=NULL;
			}

			if( wm_class != NULL ){
				n=strlen(wm_class);
				if( pWinList->max_wmclass_len < n ) pWinList->max_wmclass_len=n;
				pWinList->data[j].wm_class=wm_class;
			}else{
				pWinList->data[j].wm_class=NULL;
			}

			n=digit(wm_pid);
			if( n > pWinList->max_pid_digit ) pWinList->max_pid_digit=n;
			pWinList->data[j].pid=wm_pid;

			j++;
		}
	} // end for
	pWinList->iCnt =j;

	return EXIT_SUCCESS;
}

/* ----------------------------------
 *  function : compare for qsort 
 *             wrapper function for MyCompWinA
 */
int MyCompWin( const void * a , const void * b){
	return MyCompWinA(a,b,0);
}

/* ----------------------------------
 *  function : Display all of window list
 *  parms    :  pWinList       - address of MyWindowList
 *  return   : none
 */
void DisplayWinList(MyWinList *pWinList){
	int iD=digit(pWinList->iCnt+1);
	int i,k=1;
	// print header
	fprintf(stdout,"[%*c] %-*s %-*s %-*s title\n",iD,'#',
							pWinList->max_pid_digit,            "pid",
							pWinList->max_client_machine_len,   "machine",
							pWinList->max_wmclass_len,          "class");

	// print separater
	for(i=0;i<iD+pWinList->max_pid_digit+pWinList->max_client_machine_len+pWinList->max_wmclass_len+12;i++){
		fprintf(stdout,"-");
	}
	fprintf(stdout,"\n");

	// print data(s)
	for(i=0;i<pWinList->iCnt;i++){
		if(( i == 0 )||( MyCheckDuplicate(&(pWinList->data[i-1]),&(pWinList->data[i])) != 0 )){
						fprintf(stdout,"[%*d] %*lu %-*s %-*s %s\n",iD,k++,
						pWinList->max_pid_digit,            pWinList->data[i].pid,
						pWinList->max_client_machine_len,   pWinList->data[i].client_machine,
						pWinList->max_wmclass_len,          pWinList->data[i].wm_class,
						pWinList->data[i].title);
		}
	}
}

/* ------------------------------- */
/* internal func(s)...             */
/* ------------------------------- */

/* ----------------------------------
 *  function : Get position of Frame window 
 *  parms    :  disp  - display
 *              win   - Window
 *              x     - address of geometry ( x )
 *              y     - address of geometry ( y )
 *  return   : true   - no error
 *             false  - error
 */
bool fGetWinFramePos(Display *disp, const Window win,int *x,int *y){
	XWindowAttributes 	win_attributes;
	Status 				status;
	Window 				winframe;
	
	if(!XGetWindowAttributes(disp, win, &win_attributes)){
		return false;
	}
	
	// get window manager frame
	winframe = win;
	for(;;){
		Window 		 root, parent;
		Window 		*childlist;
		unsigned int ujunk;

		status = XQueryTree(disp, winframe, &root, &parent, &childlist, &ujunk);
		if( ( parent == root ) || !parent || !status ) break;
		winframe = parent;
		if( status && childlist )	XFree((char *)childlist);
	}
	if( winframe != win ){
		XWindowAttributes frame_attr;

		if( !XGetWindowAttributes(disp, winframe, &frame_attr) ){
			fprintf(stderr,"Err:Can't get frame attributes in fGetWinFramePos().\n");
			return false;
		}
		*x = frame_attr.x;
		*y = frame_attr.y;
	}else{
		// another way ?
		unsigned int 	wwidth, wheight, bw, depth;
		int 			src_x, src_y;
		Window 			dest_win;
		status = XGetGeometry(disp,win,&dest_win,&src_x, &src_y,&wwidth, &wheight, &bw, &depth);
		if( status ){
			Window junkwin;
			int rx,ry;
			XTranslateCoordinates(disp, win, win_attributes.root, 
									-win_attributes.border_width,-win_attributes.border_width,
									&rx, &ry, &junkwin);

			*x = rx;
			*y = ry;
		}else{
			fprintf(stderr,"Err:Can't get size of window in fGetWinFramePos().\n");
			return false;
		}
	}
	return true;
}

/* ----------------------------------
 *  function : get property of window
 *  parms    :  disp           - display
 *              win            - window
 *              xa_prop_type   - property type 
 *              prop_name      - property name
 *              pret_format    - address of ret_format
 *              pret_ntimes    - address of ret_ntimes
 *  return   :  property
 */
unsigned char *get_prop (Display *disp, Window win,
							Atom xa_prop_type, const char *prop_name, int *pret_format,unsigned long *pret_ntimes){

	Atom 			 xa_prop_name;
	Atom 			 xa_ret_type;
	int 			 ret_format;
	unsigned long 	 ret_nitems;
	unsigned long 	 ret_bytes_after;
	unsigned char 	*ret_prop;

	xa_prop_name = XInternAtom(disp, prop_name, False);

	if( XGetWindowProperty(disp, win, xa_prop_name, 0,
							MAX_PROPERTY_VALUE_LEN / 4, False,
							xa_prop_type, &xa_ret_type, &ret_format,
							&ret_nitems, &ret_bytes_after, &ret_prop) != Success ){
		fprintf(stderr,"Err:cannot get %s property.\n", prop_name);
		MyExit(disp,EXIT_FAILURE);
	}

	if( xa_ret_type != xa_prop_type ){
		;;;;;;;;;;debug_printf(stderr,"Invalid type of %s property.\n", prop_name);
		XFree(ret_prop);
		return NULL;
	}

	*pret_format = ret_format;
	*pret_ntimes = ret_nitems;

	return ret_prop;
}

/* ----------------------------------
 *  function : get all Window managed by Window manager
 *  parms    :  disp           - display
 *              size           - address of size
 *  return   :  ret            - address of list
 */
Window *get_client_list(Display *disp, unsigned long *size){
	int 			 ret_format;
	unsigned long 	 ret_nitems;
	unsigned long 	 tmp_size;
	unsigned char 	*ret_prop;
	Window 			*ret=NULL;

	ret_prop = get_prop(disp, DefaultRootWindow(disp),XA_WINDOW, "_NET_CLIENT_LIST", &ret_format,&ret_nitems);

	if( ret_prop == NULL ){
		;;;;;;;;;;debug_printf(stderr, "Dbg:in get_client_list,ret_prop is null!\n" );
	}else{
		/* null terminate the result to make string handling easier */
		tmp_size = (ret_format / 8) * ret_nitems;
		/* Correct 64 Architecture implementation of 32 bit data */
		if( ret_format == 32 ) tmp_size *= sizeof(long)/4;
		ret = (Window *)MyAssignMem(tmp_size+1);
		memcpy(ret, ret_prop, tmp_size);

		if( size ){
			*size = tmp_size;
		}

		XFree(ret_prop);
	}
	return ret;
}

/* ----------------------------------
 *  function : get property
 *  parms    :  disp           - display
 *              win            - window
 *              xa_prop_type   - property type 
 *              prop_name      - property name
 *              size           - address of size
 *  return   :  ret            - address of property
 */
char *get_property (Display *disp, Window win,
					Atom xa_prop_type, const char *prop_name, unsigned long *size){
	int 			ret_format;
	unsigned long 	ret_nitems;
	unsigned long 	tmp_size;
	unsigned char 	*ret_prop;
	char 			*ret=NULL;

	ret_prop = get_prop(disp, win,xa_prop_type, prop_name, &ret_format,&ret_nitems);

	if( ret_prop == NULL ){
		;;;;;;;;;;debug_printf(stderr, "Dbg:in get_property,ret_prop is null!\n" );
	}else{
		/* null terminate the result to make string handling easier */
		tmp_size = (ret_format / 8) * ret_nitems;
		/* Correct 64 Architecture implementation of 32 bit data */
		if( ret_format == 32 ) tmp_size *= sizeof(long)/4;
		ret = (char *)MyAssignMem(tmp_size+1);
		memcpy(ret, ret_prop, tmp_size);

		if( size ){
			*size = tmp_size;
		}

		XFree(ret_prop);
	}
	return ret;
}

/* ----------------------------------
 *  function : wrapper for ClientMessage ( XSendEvent )
 *  parms    :  disp           - display
 *              win            - window
 *              msg            - address of message
 *              data0          - 1st data
 *              data1          - 2nd data
 *              data2          - 3rd data
 *              data3          - 4th data
 *              data4          - 5th data
 *  return   :  EXIT_SUCCESS   - no error
 *              EXIT_FAILURE   - error
 */
int client_msg(Display *disp, Window win, char *msg,
				unsigned long data0, unsigned long data1,
				unsigned long data2, unsigned long data3,
				unsigned long data4){
	XEvent 	event;
	long 	mask = SubstructureRedirectMask | SubstructureNotifyMask;
	Status 	status;
	
	event.xclient.type 			= ClientMessage;
	event.xclient.serial 		= 0;
	event.xclient.send_event 	= True;
	event.xclient.message_type 	= XInternAtom(disp, msg, False);
	event.xclient.window 		= win;
	event.xclient.format 		= 32;
	event.xclient.data.l[0] 	= data0;
	event.xclient.data.l[1] 	= data1;
	event.xclient.data.l[2] 	= data2;
	event.xclient.data.l[3] 	= data3;
	event.xclient.data.l[4] 	= data4;
	status=XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event);
	if( status == BadValue ){
		fprintf(stderr, "Err:cannot send %s event[BadValue].\n", msg);
		MyExit(disp,EXIT_FAILURE);
		return EXIT_FAILURE;
	}else if( status == BadWindow ){
		fprintf(stderr, "Err:cannot send %s event[BadWindow].\n", msg);
		MyExit(disp,EXIT_FAILURE);
		return EXIT_FAILURE;
	}else{
		return EXIT_SUCCESS;
	}
}

/* ----------------------------------
 *  function : make window to all desktop
 *  parms    :  disp           - display
 *              win            - window
 *  return   :  EXIT_SUCCESS   - no error
 *              EXIT_FAILURE   - error
 */
int window_to_all_desktop(Display *disp, Window win){
	return client_msg(disp, win, "_NET_WM_DESKTOP", (unsigned long)0xFFFFFFFF,
						0, 0, 0, 0);
}
/* ----------------------------------
 *  function : move window to desktop
 *  parms    :  disp           - display
 *              win            - window
 *              desktop        - desktop num ( -1 : current ) 
 *  return   :  EXIT_SUCCESS   - no error
 *              EXIT_FAILURE   - error
 */
int window_to_desktop(Display *disp, Window win, int desktop){
	unsigned long	 *cur_desktop 	= NULL;
	Window 			 root 			= DefaultRootWindow(disp);

	if( desktop == -1 ){
		if(! (cur_desktop = (unsigned long *)get_property(disp, root,
															XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))){
			if(! (cur_desktop = (unsigned long *)get_property(disp, root,
																XA_CARDINAL, "_WIN_WORKSPACE", NULL))){
				fprintf(stderr,"Err:cannot get current desktop properties. "
								"(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE property)"
								"\n");
				MyExit(disp,EXIT_FAILURE);
			}
		}
		desktop = *cur_desktop;
	}
//    if( cur_desktop != NULL ) free(cur_desktop);

	return client_msg(disp, win, "_NET_WM_DESKTOP", (unsigned long)desktop,
						0, 0, 0, 0);
}

/* ----------------------------------
 *  function : get process id of the window
 *  parms    :  disp           - display
 *              win            - window
 *  return   :  0              - error
 *              otherwise      - process id
 */
unsigned long get_pid(Display *disp,Window win){
	int 			ret_format;
	unsigned long	ret_nitems;
	unsigned char  *ret_prop=0;
	unsigned long   ret;

	ret_prop = get_prop(disp, win,XA_CARDINAL, "_NET_WM_PID", &ret_format,&ret_nitems);
	if( ret_prop == NULL ){
		ret=(unsigned long)0;
		;;;;;;;;;;debug_printf(stderr,"Err:cannot get pid\n");
	}else{
		ret = ret_prop[1] * 256 + ret_prop[0];
		XFree(ret_prop);
	}
	return ret;
}

/* ----------------------------------
 *  function : get client machine of the window
 *  parms    :  disp           - display
 *              win            - window
 *  return   :  0              - error
 *              otherwise      - client machine
 */
char *get_machine(Display *disp,Window win){
	char 			*client_machine=NULL;
	XTextProperty	 text_prop_return;

	if( XGetWMClientMachine(disp, win, &text_prop_return) != 0 ){
		client_machine=MyAssignMem((text_prop_return.nitems+1)*text_prop_return.format/8);
		memcpy(client_machine,text_prop_return.value,text_prop_return.nitems*text_prop_return.format/8);
	}else{
		fprintf(stderr,"Err:cannot get machine.\n");
	}
	return client_machine;
}

/* ----------------------------------
 *  function : get client machine of the window
 *  parms    :  disp           - display
 *              win            - window
 *  return   :  0              - error
 *              otherwise      - wm_class ( res_name.res_class )
 */
char *get_wmclass(Display *disp,Window win){
	char 			*wm_class=NULL;
	unsigned long	 size;
	XClassHint 		 class_hints_return;
	Status     		 rc;

	if(( (rc=XGetClassHint(disp, win, &class_hints_return)) != BadWindow )&&( rc != 0 )){
		int len_name=-1,len_class=-1;
		if( class_hints_return.res_name != NULL ){
			len_name = strlen(class_hints_return.res_name);
		}
		if( class_hints_return.res_class != NULL ){
			len_class= strlen(class_hints_return.res_class);
		}
		if(( len_name == -1 )||( len_class == -1 )){
		}else{
			size     = len_name+len_class + 2;
			wm_class = MyAssignMem((size+1));
			sprintf(wm_class,"%s.%s",class_hints_return.res_name,class_hints_return.res_class);
		}
		if( class_hints_return.res_name != NULL ) XFree(class_hints_return.res_name);
		if( class_hints_return.res_name != NULL ) XFree(class_hints_return.res_class);
	}else{
		;;;;;;;;;;debug_printf(stderr,"Dbg:cannot get class.\n");
	}
	return wm_class;
}

/* ----------------------------------
 *  function : get title of the window
 *  parms    :  disp           - display
 *              win            - window
 *  return   :  0              - error
 *              otherwise      - title
 */
char *get_title(Display *disp,Window win){
	char *wm_name=NULL;

	if(( wm_name = get_property(disp, win,
									XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL)) == NULL ){
            // name is not utf8 !
		if( ( wm_name = get_property(disp, win,
										XA_STRING, "_NET_WM_NAME", NULL)) == NULL ){
			XTextProperty text_prop_return;

			if( XGetWMName( disp, win, &text_prop_return ) ){
				if( text_prop_return.nitems == 0 ){
					fprintf(stderr,"Err:get window title's length is 0\n");
				}else{
					wm_name=MyAssignMem((text_prop_return.nitems+1)*text_prop_return.format/8);
					memcpy(wm_name,text_prop_return.value,text_prop_return.nitems*text_prop_return.format/8);
				}
			}else{
				fprintf(stderr,"Err:cannot get window title\n");
			}
		}
	}
	return wm_name;
}

// compare func for Window LIST
int MyCompWinA(const void *a,const void *b,const int iCompTimes){
	int         iRc;

	switch( SortConfig[iCompTimes].iSortField ){
		case SORT_NONE:
			return 0;
			break;
		case SORT_BY_PID:
			iRc=((MyWinData *)a)->pid - ((MyWinData *)b)->pid;
			break;
		case SORT_BY_MACHINE:
			iRc=strcmp(((MyWinData *)a)->client_machine,((MyWinData *)b)->client_machine);
			break;
		case SORT_BY_WM_CLASS:
			iRc=strcmp(((MyWinData *)a)->wm_class,((MyWinData *)b)->wm_class);
			break;
		case SORT_BY_WINDOWTITLE:
			iRc=strcmp(((MyWinData *)a)->title,    ((MyWinData *)b)->title);
			break;
		default:
			;;;;;;;;;;debug_printf(stderr,"Dbg:invalid sort filed:%d\n",SortConfig[iCompTimes].iSortField);
			return 0;
	}
	if( iRc == 0 ){
		if( iCompTimes < SORT_MAX -1 ){
			iRc = MyCompWinA( a,b,iCompTimes+1);
		}
	}else{
		iRc=iRc*(SortConfig[iCompTimes].iOrder);
	}

	return iRc;
}

/* ----------------------------------
 *  function : check duplicate
 *  parms    :  a              - address of window list
 *              b              - address of window list
 *  return   :  0              - same
 *              otherwise      - different
 */
int MyCheckDuplicate(const MyWinData *a,const MyWinData *b){
	int iRc = iRc=a->pid - b->pid;
	if( iRc == 0 ){
		iRc=strcmp(a->client_machine,b->client_machine);
		if( iRc == 0 ){
			iRc=strcmp(a->wm_class,b->wm_class);
			if( iRc == 0 ){
				iRc=strcmp(a->title,b->title);
			}
		}
	}
	return iRc;
}