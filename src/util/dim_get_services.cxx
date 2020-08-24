#include <ctype.h>
#include <stdio.h>
#include <dic.hxx>
#include <string>
#include <vector>

int no_link = 0x0afefead;
int version;

char str[132];
int type, mode;
int received = 0;
enum
{
  OT_Standard=1,
  OT_ConcatOnly,
  OT_Full
};
int outtype=OT_Standard;
int output_type;
static std::string Agg("");
void print_service(void *buff, int *size, char * format);
void rout(void *vtag, void *vbuffer, int *size)
{
  char *format;
  int *buffer = (int*) vbuffer;
  int *tag = (int*) vtag;
  format = dic_get_format(0);

  if (tag)
  {
  }
  if (outtype != OT_ConcatOnly)
  {
    if ((*size == 4) && (*buffer == no_link))
    {
      printf("Service %s Not Available\n", str);
    }
    else
    {
      printf("Service %s Contents (Format '%s'):\n", str, format);
      if (*size != 0)
      {
        int siz = ((*size - 1) / 4) + 1;
        print_service(vbuffer, &siz, format);
      }
    }
  }
  if (strcmp(format,"C")== 0)
  {
    Agg=Agg+std::string((char*)vbuffer);
  }
  received = 1;
#ifdef WIN32
  wake_up();
#endif
}

int main(int argc, char **argv)
{
  Agg="";
  std::string opt;
  std::vector<std::string> services;
  bool optswitch=false;
//  Parse the command line...
  for (int i=1;i<argc;i++)
  {
    if (optswitch)
    {
      if (opt == "o")
      {
        if (strcmp(argv[i],"Standard")==0)
        {
          outtype = OT_Standard;
        }
        else if (strcmp(argv[i],"ConcatOnly")==0)
        {
          outtype = OT_ConcatOnly;
        }
        else if (strcmp(argv[i],"Full")==0)
        {
          outtype = OT_Full;
        }
      }
      else
      {
        printf("Illegal option -%s %s. Ignoring\n",opt.c_str(),argv[i]);
      }
      optswitch = false;
      continue;
    }
    else if (argv[i][0] == '-')
    {
      optswitch = true;
      opt = std::string(argv[i]+1);
    }
    else
    {
      services.push_back(std::string(argv[i]));
    }
  }
  if (argc < 2)
  {
    printf("Missing arguments. Usage:\n   dim_get_services [-o <Standard|ConcatOnly|Full>] service1 service2 ...\n");
    fflush(stdout);
    return 0;
  }
  else
  {
    for (size_t i = 0; i < services.size(); i++)
    {
      sprintf(str, "%s", services[i].c_str());
      received = 0;
      dic_info_service(str, ONCE_ONLY, 60, 0, 0, rout, 0, &no_link, 4);
      while (!received)
        dim_wait();
      usleep(1000);
    }
  }
  if ((outtype == OT_ConcatOnly) || (outtype == OT_Full))
  {
    printf ("%s\n",Agg.c_str());
  }
  fflush(stdout);
  return (1);
}

void print_service(void *b, int *size, char * format)
{
  int i, j;
  char *asc;
  int last[4];
  int *ibuff = (int*) b;
  if (format)
  {
  }

  asc = (char *) b;
  for (i = 0; i < *size; i++)
  {
    if (!(i % 4))
      printf("H");
    printf("   %08X ", ibuff[i]);
    last[i % 4] = ibuff[i];
    if (i % 4 == 3)
    {
      printf("    '");
      for (j = 0; j < 16; j++)
      {
        if (isprint(asc[j]))
          printf("%c", asc[j]);
        else
          printf(".");
      }
      printf("'\n");
      for (j = 0; j < 4; j++)
      {
        if (j == 0)
          printf("D");
        printf("%11d ", last[j]);
      }
      printf("\n");
      asc = (char *) &ibuff[i + 1];
    }
  }
  if (i % 4)
  {

    for (j = 0; j < 4 - (i % 4); j++)
      printf("            ");
    printf("    '");
    for (j = 0; j < (i % 4) * 4; j++)
    {
      if (isprint(asc[j]))
        printf("%c", asc[j]);
      else
        printf(".");
    }
    printf("'\n");
    for (j = 0; j < (i % 4); j++)
    {
      if (j == 0)
        printf("D");
      printf("%11d ", last[j]);
    }
    printf("\n");
  }
}
