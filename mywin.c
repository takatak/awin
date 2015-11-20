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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <regex.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>  /* for XTextProperty */
#include <math.h>
//#include <errno.h>
#include <sys/resource.h>
//#include <glib.h>

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#include "mymem.h"
#include "awin.h"

#include "mywin.h"


#ifdef BSD
 typedef unsigned long ulong;
#endif
/* global variable */
extern MySortConfig SortConfig[SORT_MAX];


#define MAX_PROPERTY_VALUE_LEN 4096

typedef struct xMyNotMoveList{
    int iCnt;
    char *wmclass[10];
}MyNotMoveList;

/* ------------------------------- */
/* prototype for internal func(s)  */
/* ------------------------------- */

int iReadNotMove(char *filename);
unsigned char *get_prop (Display *disp, Window win,
						Atom xa_prop_type, const char *prop_name, int *pret_format,ulong *pret_ntimes);
Window *get_client_list(Display *disp, ulong *size);
unsigned long get_pid(Display *disp,Window win);
char *get_property (Display *disp, Window win,
					Atom xa_prop_type, const char *prop_name, ulong *size);
int client_msg(Display *disp, Window win, char *msg,
		unsigned long data0, unsigned long data1,
		unsigned long data2, unsigned long data3,
		unsigned long data4);
int window_to_desktop (Display *disp, Window win, int desktop);
char *get_machine(Display *disp,Window win);
char *get_wmclass(Display *disp,Window win);
char *get_title(Display *disp,Window win);
int MyCompWinA(const void *a,const void *b,const int iCompTimes);


MyNotMoveList ListNotMove;

/* ------------------------------- */
/* public func(s)                  */
/* ------------------------------- */
int digit(long num){
	return ((int)log10(num) + 1);
}

void activate_window (Display *disp, Window win,
						bool switch_desktop) {
	unsigned long *desktop;

	/* desktop ID */
	if ((desktop = (unsigned long *)get_property(disp, win,
													XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
		if ((desktop = (unsigned long *)get_property(disp, win,
													XA_CARDINAL, "_WIN_WORKSPACE", NULL)) == NULL) {
			fprintf(stderr,"Err:cannot find desktop ID of the window.\n");
		}
	}

	if (switch_desktop && desktop) {
		if (client_msg(disp, DefaultRootWindow(disp),
							"_NET_CURRENT_DESKTOP",*desktop, 0, 0, 0, 0) != EXIT_SUCCESS) {
			fprintf(stderr,"Err:cannot switch desktop.\n");
		}
	}

	client_msg(disp, win, "_NET_ACTIVE_WINDOW",0, 0, 0, 0, 0);
	XMapRaised(disp, win);
}

void move_and_activate_window (Display *disp, Window win,char *wm_class){
	bool fActivate=true;
	if (ListNotMove.iCnt==0 ){
		iReadNotMove("notmove.list");
	}
	if (ListNotMove.iCnt>0){
		int i;
		for (i=0;i<ListNotMove.iCnt;i++){
			if (strcmp(wm_class,ListNotMove.wmclass[i])==0){
//				fprintf(stderr,"Dbg:[%s] will not be moved\n",wm_class);
				fActivate=false;
				break;
			}
		}
	}else{
	}
	if (fActivate==true){
		if (window_to_desktop(disp, win, -1) == EXIT_SUCCESS) {
								setpriority(PRIO_PROCESS, 0, 19); // set this program's priority to min.
			if(WAIT_MICROSECOND_AWTIVATE_AFTER_MOVE>0)  usleep(WAIT_MICROSECOND_AWTIVATE_AFTER_MOVE);
		}
		activate_window (disp, win, false);
	}
}

int getWinList(Display *disp,MyWinList *pWinList,int iRegField,regex_t *ppreg, regmatch_t* pmatch){
	int i,j;
	Window *client_list;
	unsigned long client_list_size;

	if (pWinList == NULL) return EXIT_FAILURE;

	pWinList->max_client_machine_len =0;
	pWinList->max_wmclass_len =0;
	pWinList->max_title_len =0;
	pWinList->max_pid_digit =0;
	pWinList->iCnt =0;

	if ((client_list = (Window *)get_client_list(disp, &client_list_size)) == NULL) {
		return EXIT_FAILURE;
	}

	pWinList->win=client_list;
	pWinList->data=MyAssignMem(sizeof(MyWinData)*(client_list_size / sizeof(Window)));

	/* find the longest client_machine name */
	for (i = 0,j=0; i < client_list_size / sizeof(Window); i++) {
		char *client_machine;
		char *wm_name;
		char *wm_class;
		unsigned long wm_pid;
		bool fStore;

		client_machine=NULL;
		wm_name=NULL;
		wm_class=NULL;
		wm_pid=0;

		fStore=true;


		if ((client_machine=get_machine(disp,client_list[i]))==NULL){
			fprintf(stderr,"Err:cannot get client_machine[%d]\n",i);
//			return EXIT_FAILURE;
		}else{
			if (iRegField==REG_FIELD_MACHINE){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					if ( regexec(ppreg, client_machine, nmatch, pmatch, 0) == 0 ){
					}else{
						fStore=false;
					}
				}
			}
		}

		if ((wm_name=get_title(disp,client_list[i]))==NULL){
			fprintf(stderr,"Err:cannot get title [%d].\n",i);
//			exit(EXIT_FAILURE);
		}else{
			if (iRegField==REG_FIELD_TITLE){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					if ( regexec(ppreg, wm_name, nmatch, pmatch, 0) == 0 ){
					}else{
						fStore=false;
					}
				}
			}
		}
		pWinList->data[j].wm_class=wm_class;
//        fprintf(stderr,"Dbg:get name of the window manager:[%s].\n",wm_name);

		if((wm_class=get_wmclass(disp,client_list[i]))==NULL){
			fprintf(stderr,"Err:cannot get class of the window manager (WM_CLASS)[%d].\n",i);
//            exit(EXIT_FAILURE);
		}else{
			if (iRegField==REG_FIELD_CLASS){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					if ( regexec(ppreg, wm_class, nmatch, pmatch, 0) == 0 ){
					}else{
						fStore=false;
					}
				}
			}
		}
//		fprintf(stderr,"Dbg:get class of the window manager:[%s].\n",wm_class);

		if ((wm_pid=get_pid(disp,client_list[i]))==0){
			fprintf(stderr,"Err:cannot get pid of the window manager [%d].",i);
			if ( wm_class!= NULL) fprintf(stderr,"it's class is [%s].",wm_class);
			if ( wm_name != NULL) fprintf(stderr,"it's title is [%s].",wm_name );
			fprintf(stderr,"\n");
//			exit(EXIT_FAILURE);
		}else{
			if (iRegField==REG_FIELD_PID){
				if( ppreg != NULL ){
					size_t nmatch = 0;
					char szPID[100];
					sprintf(szPID,"%lu",wm_pid);
					if ( regexec(ppreg, szPID, nmatch, pmatch, 0) == 0 ){
					}else{
						fStore=false;
					}
				}
			}
		}
//		fprintf(stderr,"Dbg:get PID of the window manager:[%lu].\n",wm_pid);
		if(fStore==true){
			int n;

			pWinList->data[j].iOriginalIndex=i;

			if (client_machine!= NULL){
				n=strlen(client_machine);
				if (pWinList->max_client_machine_len < n) pWinList->max_client_machine_len=n;
				pWinList->data[j].client_machine=client_machine;
			}else{
				pWinList->data[j].client_machine=NULL;
			}
			if (wm_name!=NULL){
				n=strlen(wm_name);
				if (pWinList->max_title_len<n) pWinList->max_title_len=n;
				pWinList->data[j].title=wm_name;
			}else{
				pWinList->data[j].title=NULL;
			}

			if (wm_class!=NULL){
				n=strlen(wm_class);
				if (pWinList->max_wmclass_len<n) pWinList->max_wmclass_len=n;
				pWinList->data[j].wm_class=wm_class;
			}else{
				pWinList->data[j].wm_class=NULL;
			}

			n=digit(wm_pid);
			if (n>pWinList->max_pid_digit) pWinList->max_pid_digit=n;
			pWinList->data[j].pid=wm_pid;

			j++;
		}
	} // end for
	pWinList->iCnt =j;

	return EXIT_SUCCESS;
}

int MyCompWin( const void * a , const void * b){
	return MyCompWinA(a,b,0);
}

void DisplayWinList(MyWinList *pWinList){
	int iD=digit(pWinList->iCnt);
	int i;

	fprintf(stdout,"[%*s] %-*s %-*s %-*s title\n",iD,"#",
					pWinList->max_pid_digit,			"pid",
					pWinList->max_client_machine_len,	"machine",
					pWinList->max_wmclass_len,			"class");

	for(i=0;i<iD+pWinList->max_pid_digit+pWinList->max_client_machine_len+pWinList->max_wmclass_len+12;i++){
		fprintf(stdout,"-");
	}
	fprintf(stdout,"\n");

	for (i=0;i<pWinList->iCnt;i++){
		fprintf(stdout,"[%*d] %*lu %-*s %-*s %s\n",iD,i,
						pWinList->max_pid_digit,			pWinList->data[i].pid,
						pWinList->max_client_machine_len,	pWinList->data[i].client_machine,
						pWinList->max_wmclass_len,			pWinList->data[i].wm_class,
															pWinList->data[i].title);
	}
}

/* ------------------------------- */
/* internal func(s)...             */
/* ------------------------------- */
int iReadNotMove(char *filename){
    FILE    *fp;
    int     i;
    char    szBuf[1024];
    char    *home;
    char    szFileName[1024];


    if (filename==NULL) return -1;

    if ((home=getenv("HOME"))==NULL){
        sprintf(szFileName,"/etc/awin/%s",filename);
    }else{
        sprintf(szFileName,"%s/.config/awin/%s",home,filename);
    }


    fp=fopen(szFileName,"r");
    if ( fp == NULL) {
        sprintf(szFileName,"/etc/awin/%s",filename);
        fp=fopen(szFileName,"r");
    }
    if ( fp == NULL) {
        perror(szFileName);
        return -1;
    }else{
        while( fgets( szBuf , sizeof( szBuf ) , fp ) != NULL ) {
            i=strlen(szBuf);
            if ( szBuf[i-1]==0x0a ) szBuf[--i]=0x00;
            if (szBuf[0]=='#') {
                // comment line
            }else{
                ListNotMove.wmclass[ListNotMove.iCnt]=MyAssignMem((i+1)*sizeof(char));
                memcpy(ListNotMove.wmclass[ListNotMove.iCnt],szBuf,i);
                ListNotMove.iCnt++;
            }
        }
        fclose(fp);
    }
    return 0;
}

unsigned char *get_prop (Display *disp, Window win,
						Atom xa_prop_type, const char *prop_name, int *pret_format,ulong *pret_ntimes) {

	Atom xa_prop_name;
	Atom xa_ret_type;
	int ret_format;
	ulong ret_nitems;
	ulong ret_bytes_after;
	unsigned char *ret_prop;

	xa_prop_name = XInternAtom(disp, prop_name, False);


	if (XGetWindowProperty(disp, win, xa_prop_name, 0,
							MAX_PROPERTY_VALUE_LEN / 4, False,
							xa_prop_type, &xa_ret_type, &ret_format,
							&ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
		fprintf(stderr,"Err:cannot get %s property.\n", prop_name);
		exit(EXIT_FAILURE);
	}

	if (xa_ret_type != xa_prop_type) {
//		fprintf(stderr,"Invalid type of %s property.\n", prop_name);
		XFree(ret_prop);
		return NULL;
	}

	*pret_format = ret_format;
	*pret_ntimes = ret_nitems;

	return ret_prop;
}


Window *get_client_list(Display *disp, ulong *size) {
	int ret_format;
	ulong ret_nitems;
	ulong tmp_size;
	unsigned char *ret_prop;
	Window *ret=NULL;

	ret_prop = get_prop(disp, DefaultRootWindow(disp),XA_WINDOW, "_NET_CLIENT_LIST", &ret_format,&ret_nitems);

	if (ret_prop == NULL){
//		fprintf(stderr, "Dbg:in get_client_list,ret_prop is null!\n" );
	}else{
			/* null terminate the result to make string handling easier */
		tmp_size = (ret_format / 8) * ret_nitems;
			/* Correct 64 Architecture implementation of 32 bit data */
		if(ret_format==32) tmp_size *= sizeof(long)/4;
		ret = (Window *)MyAssignMem(tmp_size+1);
		memcpy(ret, ret_prop, tmp_size);

		if (size) *size = tmp_size;

		XFree(ret_prop);
	}
	return ret;
}




char *get_property (Display *disp, Window win,
					Atom xa_prop_type, const char *prop_name, ulong *size) {
	int ret_format;
	ulong ret_nitems;
	ulong tmp_size;
	unsigned char *ret_prop;
	char *ret=NULL;

	ret_prop = get_prop(disp, win,xa_prop_type, prop_name, &ret_format,&ret_nitems);

	if (ret_prop == NULL){
//		fprintf(stderr, "Dbg:in get_priority,ret_prop is null!\n" );
	}else{
		/* null terminate the result to make string handling easier */
	tmp_size = (ret_format / 8) * ret_nitems;
		/* Correct 64 Architecture implementation of 32 bit data */
	if(ret_format==32) tmp_size *= sizeof(long)/4;
		ret = (char *)MyAssignMem(tmp_size+1);
		memcpy(ret, ret_prop, tmp_size);

		if (size) *size = tmp_size;

		XFree(ret_prop);
	}
	return ret;
}

int client_msg(Display *disp, Window win, char *msg,
		unsigned long data0, unsigned long data1,
		unsigned long data2, unsigned long data3,
		unsigned long data4) {
	XEvent event;
	long mask = SubstructureRedirectMask | SubstructureNotifyMask;

	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.message_type = XInternAtom(disp, msg, False);
	event.xclient.window = win;
	event.xclient.format = 32;
	event.xclient.data.l[0] = data0;
	event.xclient.data.l[1] = data1;
	event.xclient.data.l[2] = data2;
	event.xclient.data.l[3] = data3;
	event.xclient.data.l[4] = data4;

	if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
		return EXIT_SUCCESS;
	}else {
		fprintf(stderr, "Err:cannot send %s event.\n", msg);
		exit( EXIT_FAILURE);
	}
}



int window_to_desktop (Display *disp, Window win, int desktop) {
	unsigned long *cur_desktop = NULL;
	Window root = DefaultRootWindow(disp);

	if (desktop == -1) {
		if (! (cur_desktop = (unsigned long *)get_property(disp, root,
								XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
			if (! (cur_desktop = (unsigned long *)get_property(disp, root,
								XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
				fprintf(stderr,"Err:cannot get current desktop properties. "
						"(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE property)"
						"\n");
				exit ( EXIT_FAILURE );
			}
		}
		desktop = *cur_desktop;
	}
//	if(cur_desktop!=NULL) free(cur_desktop);

	return client_msg(disp, win, "_NET_WM_DESKTOP", (unsigned long)desktop,
						0, 0, 0, 0);
}



unsigned long get_pid(Display *disp,Window win) {
	int ret_format;
	ulong ret_nitems;
	unsigned char *ret_prop=0;
	unsigned long ret;

	ret_prop = get_prop(disp, win,XA_CARDINAL, "_NET_WM_PID", &ret_format,&ret_nitems);
	if (ret_prop==NULL){
		ret=(unsigned long)0;
//		fprintf(stderr,"Err:cannot get pid\n");
	}else{
		ret = ret_prop[1] * 256 + ret_prop[0];
		XFree(ret_prop);
	}
	return ret;
}

char *get_machine(Display *disp,Window win){
	char *client_machine=NULL;
	XTextProperty text_prop_return;

	if(XGetWMClientMachine(disp, win, &text_prop_return)){
		client_machine=MyAssignMem((text_prop_return.nitems+1)*text_prop_return.format/8);
		memcpy(client_machine,text_prop_return.value,text_prop_return.nitems*text_prop_return.format/8);
	}else{
		fprintf(stderr,"Err:cannot get machine.\n");
	}
	return client_machine;
}

char *get_wmclass(Display *disp,Window win){
	char *wm_class=NULL;
	unsigned long size;

	XClassHint class_hints_return;

	if( XGetClassHint(disp, win, &class_hints_return)){
		size=strlen(class_hints_return.res_name)+strlen(class_hints_return.res_class)+1;
		wm_class=MyAssignMem((size+1));
		sprintf(wm_class,"%s.%s",class_hints_return.res_name,class_hints_return.res_class);
		if (class_hints_return.res_name!=NULL) XFree(class_hints_return.res_name);
		if (class_hints_return.res_name!=NULL) XFree(class_hints_return.res_class);
	}else{
		fprintf(stderr,"Err:cannot get class.\n");
	}
	return wm_class;
}

char *get_title(Display *disp,Window win){
	char *wm_name=NULL;

	if ((wm_name = get_property(disp, win,
								XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL))==NULL) {
            // name is not utf8 !
		if ((wm_name = get_property(disp, win,
									XA_STRING, "_NET_WM_NAME", NULL))==NULL) {
			XTextProperty text_prop_return;

			if (XGetWMName( disp, win, &text_prop_return )){
				if (text_prop_return.nitems==0){
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
	int 	iRc;

	switch(SortConfig[iCompTimes].iSortField){
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
//			fprintf(stderr,"Err:invalid sort filed:%d\n",SortConfig[iCompTimes].iSortField);
			return 0;
	}
	if( iRc == 0 ){
		if ( iCompTimes < SORT_MAX -1){
			iRc = MyCompWinA( a,b,iCompTimes+1);
		}
	}else{
			iRc=iRc*(SortConfig[iCompTimes].iOrder);
	}

	return iRc;
}

