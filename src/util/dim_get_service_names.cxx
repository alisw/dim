#include <stdio.h>
#include "dic.hxx"
#include <string>
#include "dim.h"

int main(int argc, char **argv)
{
  std::string Agg="";
  if (argc < 2)
  {
    printf("Missing Service name pattern\n");
  }
  else
  {
      DimBrowser dbr;
      dbr.getServices(argv[1]);
      char *svc,*fmt;
      while (dbr.getNextService(svc,fmt)>0)
      {
        Agg.append(svc);
        Agg+=" ";
      }
  }
  printf ("%s\n",Agg.c_str());
  return (1);
}

