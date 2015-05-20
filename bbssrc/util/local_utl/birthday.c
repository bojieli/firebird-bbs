/* $Id: birthday.c,v 1.1 2000/01/15 01:45:40 edwardc Exp $ */

#include <time.h>
#include <stdio.h>
#include "bbs.h"

main(argc, argv)
  char *argv[];
{
  FILE *fp, *fout;
  time_t now;
  int i, j=0;
  struct userec aman;
  struct userec allman[ MAXUSERS ];
  struct tm *tmnow;
  char buf[256];

  sprintf(buf,"%s/.PASSWDS", BBSHOME);
  if ((fp = fopen(buf, "rb")) == NULL)
  {
    printf("Can't open record data file.\n");
    return 1;
  }
  sprintf(buf,"%s/0Announce/bbslist/birthday", BBSHOME);
  if ((fout = fopen(buf, "w")) == NULL)
  {
    printf("Can't write to birthday file.\n");
    return 1;
  }

  now=time(0);
  tmnow = localtime(&now);
  fprintf(fout,"\n%s����جP�W��\n\n", BBSNAME);
  fprintf(fout,"�H�U�O %d �� %d �骺�جP:\n\n",tmnow->tm_mon+1,tmnow->tm_mday+1);

  for (i=0; ; i++) {
    if (fread(&allman[ i ], sizeof( aman ), 1, fp ) <= 0) break;
    if (allman[i].birthmonth == 0) continue;
    if (allman[i].birthmonth == tmnow->tm_mon + 1 &&
        allman[i].birthday == tmnow->tm_mday + 1) {
        fprintf(fout," ** %-15.15s (%s)\n",allman[i].userid,allman[i].username);
        j++;
    }
  }
  fprintf(fout,"\n\n�`�@�� %d ��جP�C\n",j);
  fclose(fout);
  fclose(fp);
}
