/* account.c  -- count for no. of logins */
/* $Id: account.c,v 1.1 2000/01/15 01:45:39 edwardc Exp $ */

#include <time.h>
#include <stdio.h>
#include "bbs.h"
#define MAX_LINE        (15)

struct {
	int     no[24];		/* ���� */
	int     sum[24];	/* �`�X */
}       st;


char   *
Ctime(date)
time_t *date;
{
	static char buf[80];
	strcpy(buf, (char *) ctime(date));
	buf[strlen(buf) - 1] = '\0';
	return buf;
}

int
main(argc, argv)
char   *argv[];
{
	FILE   *fp;
	char    buf[256], *p;
	char    date[80];
	int     now;
	int     hour, max = 0, item, total = 0;
	int     totaltime = 0;
	int     i, j;
	char   *blk[10] =
	{
		"��", "��", "�b", "�c", "�d",
		"�e", "�f", "�g", "�h", "�i",
	};
	
	sprintf(buf,"%s/usies", BBSHOME);
	if ((fp = fopen(buf, "r")) == NULL) {
		printf("can't open usies\n");
		return 1;
	}
	now = time(0);
	sprintf(date, "%6.6s", Ctime(&now) + 4);
	while (fgets(buf, 256, fp)) {
		hour = atoi(buf + 7);
		if (hour < 0 || hour > 23) {
			printf("%s", buf);
			continue;
		}
		if (strncmp(buf, date, 6))
			continue;
		if (!strncmp(buf + 21, "ENTER", 5)) {
			st.no[hour]++;
			continue;
		}
		if (p = (char *) strstr(buf + 40, "Stay:")) {
			st.sum[hour] += atoi(p + 6);
			continue;
		}
	}
	fclose(fp);

	for (i = 0; i < 24; i++) {
		total += st.no[i];
		totaltime += st.sum[i];
		if (st.no[i] > max)
			max = st.no[i];
	}

	if ( max != 0 )
		item = max / MAX_LINE + 1;
	else {
		printf("max is 0.\n");
		exit(0);
	}

	sprintf(buf,"%s/0Announce/bbslist/countusr", BBSHOME);
	if ((fp = fopen(buf, "w")) == NULL) {
		printf("Cann't open countusr\n");
		return 1;
	}
	fprintf(fp, "\n[1;36m    �z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{\n");
	for (i = MAX_LINE; i >= 0; i--) {
		fprintf(fp, "[1;37m%4d[36m�x[33m", (i + 1) * item);
		for (j = 0; j < 24; j++) {
			if ((item * (i) > st.no[j]) && (item * (i - 1) <= st.no[j]) && st.no[j]) {
				fprintf(fp, "[35m%-3d[33m", (st.no[j]));
				continue;
			}
			if (st.no[j] - item * i < item && item * i < st.no[j])
				fprintf(fp, "%s ", blk[((st.no[j] - item * i) * 10) / item]);
			else if (st.no[j] - item * i >= item)
				fprintf(fp, "%s ", blk[9]);
			else
				fprintf(fp, "   ");
		}
		fprintf(fp, "[1;36m�x\n");
	}
	
	fprintf(fp, "   [37m0[36m�|�w�w�w[37m%s�C�p�ɨ�X�H���έp[36m�w�w�w[37m%s[36m�w�w�w�}\n"
		"    [;36m  00 01 02 03 04 05 06 07 08 09 10 11 [1;31m12 13 14 15 16 17 18 19 20 21 22 23\n\n"
		"                 [32m1 [33m�h [32m= [37m%-5d [32m�`�@�W���H���G[37m%-9d[32m�����ϥήɶ��G[37m%d[m\n"
		,BBSNAME, Ctime(&now), item, total, ( totaltime == 0 ) ? 0 : totaltime / total + 1);
	fclose(fp);
}
