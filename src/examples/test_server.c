#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dis.h>

char str[10][80];

typedef struct {
	int i;
	int j;
	int k;
	double d;
	short s;
	char c;
	short t;
	float f;
	char str[20];
}TT;

TT t;


int big_buff[1024];

int testDimInt = 0;
int testDimLong = 0;
longlong testDimXLong = 0;


void cmnd_rout(int *tag, TT *buf, int *size)
{

	if(tag){}
	dim_print_date_time();
	printf("Command received, size = %d, TT size = %d:\n", *size,
	       (int)sizeof(TT));
	printf("buf->i = %d, buf->d = %2.2f, buf->s = %d, buf->c = %c, buf->f = %2.2f, buf->str = %s\n",
			buf->i,buf->d,buf->s,buf->c,buf->f,buf->str);
}

void cmnd_rout_long(int *tag, longlong *buf, int *size)
{

	if (tag) {}
	dim_print_date_time();
	printf("Command received, size = %d\n", *size);
	printf("contents = %lld, %llx\n", *buf, *buf);
}

void client_exited(int *tag)
{
	char name[84];

	if(dis_get_client(name))
		printf("Client %s (%d) exited\n", name, *tag);
	else
		printf("Client %d exited\n", *tag);
}

void exit_cmnd(int *code)
{
	printf("Exit_cmnd %d\n", *code);
	exit(*code);
}

int NewData;
int NewIds[11];

int more_ids[1024];
int curr_more_index = 0;
char more_str[1024][80];

/*
int atlas_ids[210];
float atlas_arr[10];
*/
int main(int argc, char **argv)
{
	int i, id/*, big_ids[20]*/, xlong_id, int_id;
	char aux[80];
	char name[84]/*, name1[132]*/;
	char srvname[64];
/*
	int on = 0;
*/
	dim_long dnsid = 0;
	char extra_dns[128];
	int new_dns = 0;
	int index = 0;
/*
	dim_set_write_timeout(1);
*/
/*
	int buf_sz, buf_sz1;
*/
/*
dis_set_debug_on();
*/
/*
	int status;
	regex_t re;

	if(regcomp(&re, "abc*",REG_EXTENDED|REG_NOSUB) != 0)
		printf("regcomp error\n");
	status = regexec(&re,"abcdef", (size_t)0, NULL, 0);
	regfree(&re);
	printf("result = %d\n", status); 
*/
	dim_print_date_time();
	printf("Dim Server Starting up...\n");
	fflush(stdout);
	if(argc){}
	new_dns = dim_get_env_var("EXTRA_DNS_NODE", extra_dns, sizeof(extra_dns));
	if(new_dns)
		dnsid = dis_add_dns(extra_dns,0);
	if(dnsid){}
/*
	buf_sz = dim_get_write_buffer_size();
	dim_set_write_buffer_size(10000000);
	buf_sz1 = dim_get_write_buffer_size();
printf("socket buffer size = %d, after = %d\n",buf_sz, buf_sz1);
*/
	dis_add_exit_handler(exit_cmnd);
	dis_add_client_exit_handler(client_exited);

	strcpy(srvname, argv[1]);
//	if(!strcmp(srvname,"xx1"))
//		strcpy(srvname,"xx");
	for(i = 0; i< 10; i++)
	{
 	        snprintf(str[i],sizeof(str[i]),"%s/Service_%03d",srvname,i);
		dis_add_service( str[i], "C", str[i], (int)strlen(str[i])+1, 
			(void *)0, 0 );
	}
	t.i = 123;
	t.j = 456;
	t.k = 789;
	t.d = 56.78;
	t.s = 12;
	t.t = 12;
	t.c = 'a';
	t.f = (float)4.56;
	strcpy(t.str,"hello world");

	snprintf(aux, sizeof(aux),"%s/TEST_SWAP",srvname);
	id = dis_add_service( aux, "l:3;d:1;s:1;c:1;s:1;f:1;c:20", &t, sizeof(t), 
		(void *)0, 0 );
//	if(!strcmp(argv[1],"xx1"))
//	{
		snprintf(aux, sizeof(aux),"%s/TEST_SWAP1",srvname);
		id = dis_add_service( aux, "l:3;d:1;s:1;c:1;s:1;f:1;c:20", &t, sizeof(t), 
			(void *)0, 0 );
//	}
	if(id){}
	snprintf(aux, sizeof(aux),"%s/TEST_CMD",srvname);
	dis_add_cmnd(aux,"l:3;d:1;s:1;c:1;s:1;f:1;c:20",cmnd_rout, 0);

	snprintf(aux, sizeof(aux), "%s/Service_DimInt", srvname);
	int_id = dis_add_service(aux, "I", &testDimInt, sizeof(testDimInt),
		(void *)0, 0);
	snprintf(aux, sizeof(aux), "%s/Service_DimLong", srvname);
	dis_add_service(aux, "L", &testDimLong, sizeof(testDimLong),
		(void *)0, 0);
	snprintf(aux, sizeof(aux), "%s/Service_DimXLong", srvname);
	xlong_id = dis_add_service(aux, "X", &testDimXLong, sizeof(testDimXLong),
		(void *)0, 0);
/*
	printf("Dim X size long %zd\n", sizeof(long));
	printf("Dim X size pointer %zd\n", sizeof(void *));
	printf("Dim X size dim_long %zd\n", sizeof(dim_long));
	printf("Dim X size longlong %zd\n", sizeof(longlong));
	printf("Dim X size long long %zd\n", sizeof(long long));
*/
	snprintf(aux, sizeof(aux), "%s/TEST_XCMD", srvname);
	dis_add_cmnd(aux, "X", cmnd_rout_long, 0);
	/*
	big_buff[0] = 1;
	for(i = 0; i < 20; i++)
	{
		snprintf(aux, sizeof(aux),"%s/TestMem_%d",argv[1], i);
		big_ids[i] = dis_add_service( aux, "I", big_buff, 1024*sizeof(int), 
			(void *)0, 0 );
	}
*/

/*
	for(i = 1; i <= 200; i++)
	{
		snprintf(aux, sizeof(aux),"%s/ATLAS_Service%d",argv[1],i);
		atlas_ids[i] = dis_add_service( aux, "F", atlas_arr, 10*sizeof(float), 
			(void *)0, 0 );
	}
*/
	dis_start_serving( argv[1] );

	if(dis_get_client(name))
	{
		printf("client %s\n",name);
	}
/*
	for(i = 0; i < 5; i++)
	{
		sleep(10);

	}
	dis_stop_serving();
	sleep(59);
*/
	while(1)
	{
		index++;
		testDimXLong++;
		dis_update_service(xlong_id);
		testDimInt = -index;
		dis_update_service(int_id);
/*
		for(i = 0; i < 20; i++)
		{
			index++;
			big_buff[0] = index;
			dis_update_service(big_ids[i]);
		}
		sleep(1);
*/
/*
		pause();
		*/
		sleep(1);
/*
		dis_update_service(id);
*/
/*		
		for(i = 1; i <= 200; i++)
		{
			dis_update_service(atlas_ids[i]);
		}
*/
/*
		if(curr_more_index < 1000)
		{
			for(i = 1; i <= 10; i++)
			{
				sprintf(more_str[curr_more_index],"%s/More_Service_%03d",argv[1],curr_more_index);
				more_ids[curr_more_index] = dis_add_service( more_str[curr_more_index], "C", 
					more_str[curr_more_index], (int)strlen(more_str[curr_more_index])+1, 
					(void *)0, 0 );
printf("Adding service %s\n",more_str[curr_more_index]);
				curr_more_index++;
				dis_start_serving(argv[1]);
				dis_start_serving(argv[1]);
			}
		}
*/
		/*
		if(new_dns)
		{
			if(!on)
			{
printf("Connecting New DNS \n");
				for(i = 0; i < 10; i++)
				{
					sprintf(name1,"NewService%d",i);
					NewIds[i] = dis_add_service_dns(dnsid, name1, "i", &NewData, sizeof(NewData), 
						(void *)0, 0 );
				}
				NewIds[10] = 0;
				dis_start_serving_dns(dnsid, "xx_new");
				on = 1;
			}
			else
			{
printf("DisConnecting New DNS \n");
				for(i = 0; i < 10; i++)
				{
					dis_remove_service(NewIds[i]);
				}
				on = 0;
			}
		}
		*/
	}
	return 1;
}

