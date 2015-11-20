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
#ifdef INCLED_MYWIN_H
// nop
#else

#define  INCLED_MYWIN_H

/* make sure the WM has enough time to move the window, before we activate it */
//#define WAIT_MICROSECOND_AWTIVATE_AFTER_MOVE    200000     /* unit : microsecond  200000 is 0.2 sec. */
#define WAIT_MICROSECOND_AWTIVATE_AFTER_MOVE    1     /* unit : microsecond  200000 is 0.2 sec. */

#define REG_FIELD_NONE     0
#define REG_FIELD_PID      1
#define REG_FIELD_MACHINE  2
#define REG_FIELD_CLASS    3
#define REG_FIELD_TITLE    4
#define REG_FIELD_MAX      4

typedef struct xMyWinData
{
	char	*client_machine;
	char	*wm_class;
	char	*title;
	unsigned long pid;
	int		iOriginalIndex;
} MyWinData;

typedef struct xMyWinList
{
	int	iCnt;
	Window		*win;
	int 		max_client_machine_len ;
	int 		max_wmclass_len ;
	int 		max_title_len ;
	int 		max_pid_digit ;
	MyWinData	*data;
} MyWinList;

/* prototype */
int 	digit(long );
void 	activate_window (Display *, Window ,bool ) ;
void 	move_and_activate_window (Display *, Window ,char *);
int 	getWinList(Display *,MyWinList *,int ,regex_t *, regmatch_t * );
int 	MyCompWin( const void *  , const void * );
void 	DisplayWinList(MyWinList *);

#endif
