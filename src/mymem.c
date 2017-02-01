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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mymem.h"

        // please adjust to your envirenment or using style .
#define MYALLOC_MAX_POOL            10
#define MYALLOC_DEFAULT_ALLOC_UNIT  65536

// private function
int MyManageMem(int iProcType,int iSize,void **pp);

#define SET_UNIT    1
#define ASSIGN_MEM  2
#define FREE_MEM    3
#define FREE_ALLMEM 4


void *MyAssignMem(int size){
	void *p=NULL;

	MyManageMem(ASSIGN_MEM,size,&p);
	if( p == NULL ){
		fprintf(stderr,"Err:alloc mem for size:%d\n",size);
		exit(EXIT_FAILURE);
	}
	return  p;
}

void MyChangeUnitMem(int iSize){
	MyManageMem(SET_UNIT,iSize,NULL);
}

void MyFreeMem(void *p){
	MyManageMem(FREE_MEM,0,&p);
}

int MyManageMem(int iProcType,int iSize,void **pp){
	static int 		 iAllocedPoolCnt=0;
	static void		*AllocedPool[MYALLOC_MAX_POOL]={NULL};
	static long		 amount[MYALLOC_MAX_POOL]={0};
	static long		 freepos[MYALLOC_MAX_POOL]={0};
	static long		 alloc_unit = MYALLOC_DEFAULT_ALLOC_UNIT;
	int				 iRet=EXIT_SUCCESS;

	switch( iProcType ){
		case SET_UNIT:
			alloc_unit=abs(iSize);                   // change allocation unit
			break;
		case ASSIGN_MEM:
			{
				size_t  al;

				if( AllocedPool[iAllocedPoolCnt] == NULL){         // 1st call
					for( al=alloc_unit ; al < iSize+1 ; al+=alloc_unit );
					AllocedPool[iAllocedPoolCnt] = malloc(al);
					if( AllocedPool[iAllocedPoolCnt] == NULL ){
						perror("Err:alloc mem");
						exit(EXIT_FAILURE);
					}
					memset(AllocedPool[iAllocedPoolCnt],0x00,al);
					amount[iAllocedPoolCnt]=al;
					freepos[iAllocedPoolCnt]=0;
				}else{
					if( freepos[iAllocedPoolCnt]+iSize+1 >= amount[iAllocedPoolCnt] ){    // need new pool
						if( ++iAllocedPoolCnt < MYALLOC_MAX_POOL ){
							for( al=alloc_unit ; al < iSize+1 ; al+=alloc_unit );
							AllocedPool[iAllocedPoolCnt] = malloc(al);
							if( AllocedPool[iAllocedPoolCnt] == NULL ){
								perror("Err:alloc mem");
								exit(EXIT_FAILURE);
							}
							memset(AllocedPool[iAllocedPoolCnt],0x00,al);
							amount[iAllocedPoolCnt]=al;
							freepos[iAllocedPoolCnt]=0;
						}else{
							perror("Err:Exceed allocation pool");
							exit(EXIT_FAILURE);
						}
					}
				}
				*pp = AllocedPool[iAllocedPoolCnt]+freepos[iAllocedPoolCnt];
				freepos[iAllocedPoolCnt] += iSize+1;
			}
			break;
		case FREE_MEM:
			// now no operation
			break;
		case FREE_ALLMEM:
//            { // now not used/tested 、ソcomment outed
//                int i;
//                for( i=0 ; i < MYALLOC_MAX_POOL ; i++ ){
//                    if( AllocedPool[i] != NULL ) free(AllocedPool[i]);
//                    AllocedPool[i]=NULL;
//                    amount[i]=0;
//                    freepos[i]=0;
//                }
//            }
			break;
		default:
			iRet=EXIT_FAILURE;
	}

	return iRet;
}