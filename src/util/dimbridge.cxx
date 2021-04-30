#include <dic.hxx>
#include <dis.hxx>
#include <dim.h>
#include <iostream>
#if __cplusplus <= 201103L
#define DIMXX_OVERRIDE
#else
#define DIMXX_OVERRIDE override
#endif

using namespace std;

static int no_link = 0xdeaddead;
static char from_node[128], to_node[128], bridge_name[128];
static int from_port, to_port;
static char *services = 0, *eservices = 0;
static char *sstrings = 0, *rstrings = 0, *myown = 0;
int n_replaces = 0;
int max_replaces = 10;
int poll_interval = 30;

void get_srv_name_pub(char *srv_name, char *srv_name_pub)
{
	char *p1, *p2, *ptrs, *ptrr;
	int i;

	strcpy(srv_name_pub, srv_name);
	if (n_replaces)
	{
		ptrs = sstrings;
		ptrr = rstrings;
		for (i = 0; i < n_replaces; i++)
		{
			p1 = strstr(srv_name_pub, ptrs);
			if (p1)
			{
				p2 = srv_name + (p1 - srv_name_pub);
				p2 += strlen(ptrs);
				*p1 = '\0';
				strcat(p1, ptrr);
				strcat(p1, p2);
			}
			ptrs += strlen(ptrs) + 1;
			ptrr += strlen(ptrr) + 1;
		}
	}
}

class BridgeService: public DimStampedInfo 
{
  char srv_name[256];
  char srv_name_pub[256];
  char srv_format[256];
  int declared;
  DimService *srv;
  void *srv_data, *data;
  int srv_size;
  int cmnd;
  int found;
  int copyFlag;
  time_t timestamp;
  int milliseconds;
  int quality;

  void infoHandler() DIMXX_OVERRIDE {
	char *server;
    data = DimStampedInfo::getData();
    srv_size = DimStampedInfo::getSize();
	timestamp = getTimestamp();
	milliseconds = getTimestampMillisecs();
	quality = getQuality();
// Sometimes gets a packet from DIS_DNS not understood why!!!"
	server = DimClient::getServerName();
	if(strstr(server,"DIS_DNS") != 0)
	{
//dim_print_date_time();
//cout << "received from " << server << " size = " << srv_size << endl;
		if(strstr(srv_name,"DIS_DNS") == 0)
			return;
	}
	srv_data = data;
    if(*(int *)srv_data == no_link)
	{
		if(srv)
		{
//			dim_print_date_time();
//			cout << "Deleting Service (no link)" << srv_name << endl;
			delete(srv);
			srv = 0;
		}
		declared = 0;
//  		cout << "Disconnecting bridge for: " << srv_name << endl;
		poll_interval = 5;
	}
    else if(!declared)
    {
//		DimServer::setDnsNode(to_node);
		srv = new DimService(srv_name_pub, srv_format, srv_data, srv_size);
		if(copyFlag)
			srv->setData(srv_data, srv_size);
		srv->setQuality(quality);
		srv->setTimestamp(timestamp, milliseconds);
		DimServer::start(bridge_name);
		declared = 1;
//		DimClient::setDnsNode(from_node);
	}
    else
	{
		if(srv)
		{
			srv->setQuality(quality);
			srv->setTimestamp(timestamp, milliseconds);
			if (copyFlag)
			{
				srv->setData(srv_data, srv_size);
				srv->updateService();
			}
			else
			{
				srv->updateService(srv_data, srv_size);
			}
		}
	}
  }

  public:
    BridgeService(char *name, char *format, int copy):
		DimStampedInfo(name, &no_link, sizeof(no_link)), declared(0), srv(0), copyFlag(copy)
		{ 
			strcpy(srv_format, format);
			strcpy(srv_name, name);
			get_srv_name_pub(srv_name, srv_name_pub);
			cmnd = 0;
			found = 1;
//			cout << "Bridging Service: " << name << endl;
		}
	BridgeService(char *name, char *format, int rate, int copy):
		DimStampedInfo(name, rate, &no_link, sizeof(no_link)), declared(0), srv(0), copyFlag(copy)
		{ 
			strcpy(srv_format, format);
			strcpy(srv_name, name);
			get_srv_name_pub(srv_name, srv_name_pub);
			cmnd = 0;
			found = 1;
//			cout << "Bridging Service: " << name << ", rate = " << rate << " seconds" << endl;
		}
  	~BridgeService()
  	{
  		if(declared)
		{
			if(srv)
			{
				delete(srv);
				srv = 0;
			}
		}
  		declared = 0;
//  		cout << "Stopped bridge for: " << srv_name << endl;
  	}
    char *getName() { return srv_name; };
	char *getNamePub() { return srv_name_pub; };
	void clear() { found = 0; };
    void set() {found = 1;};
    int find() {return found;};
    int isCmnd() { return cmnd;};
};

class BridgeCommand: public DimCommandHandler
{
  char srv_name[256];
  char srv_name_pub[256];
  char srv_format[256];
  int declared;
//  DimService *srv;
  DimCommand *srv;
  void *srv_data;
  int srv_size;
  int cmnd;
  int found;

  void commandHandler()   DIMXX_OVERRIDE  {
//    srv_data = DimCommand::getData();
//    srv_size = DimCommand::getSize();
	srv_data = srv->getData();
	srv_size = srv->getSize();
	DimClient::sendCommandNB(srv_name, srv_data, srv_size);
  }

  public:
    BridgeCommand(char *name, char *format)/*:*/
//		DimCommand(name, format) 
		{ 
			cmnd = 1;
			found = 1;
			strcpy(srv_name, name);
			get_srv_name_pub(srv_name, srv_name_pub);
			srv = new DimCommand(srv_name_pub, format, this);
			DimServer::start(bridge_name);
			//			cout << "Bridging Command: " << name << endl;
		}
	~BridgeCommand()
	{
//		if (declared)
//		{
			if (srv)
			{
//				dim_print_date_time();
//				cout << "Deleting Command " << srv_name << endl;
				delete(srv);
				srv = 0;
			}
//		}
//		declared = 0;
		//  		cout << "Stopped bridge for: " << srv_name << endl;
	}
	char *getName() { return srv_name; };
	char *getNamePub() { return srv_name_pub; };
	void clear() { found = 0; };
    void set() {found = 1;};
    int find() {return found;};
    int isCmnd() { return cmnd;};
};

void print_usage_old()
{
	cout << "Usage: DimBridge [from_node] to_node services [time_interval] [-copy]" << endl;
	cout << "    from_node - by default DIM_DNS_NODE" << endl;
	cout << "    to_node - the complete node name of the new DNS" << endl;
	cout << "    services - the list of service names (wildcards allowed)" << endl;
	cout << "    time_interval - the interval in seconds to be used for updating the services" << endl;
	cout << "    -copy - copy internally the service data" << endl;
}

void print_usage()
{
	cout << "Usage: DimBridge [-f(rom) <from_node>[:<port_number>]] -t(o) <to_node>[:<port_number>] -s <services> [-e <services>] [-i(nterval) <time_interval>] [-c(opy)] ([-r(replace) <search_string> <replace_string>])* " << endl;
	cout << "    -f, -from\t\tfrom_node - the DNS node name (optionally a port number), by default DIM_DNS_NODE (DIM_DNS_PORT)" << endl;
	cout << "    -t, -to\t\tto_node - the complete node name of the new DNS (optionally a port number)" << endl;
	cout << "    -s, -services\tservices - the list of service names to bridge as a comma separated list of patterns" << endl;
	cout << "    -e, -exclude\tservices - a list of service names to be excluded as a comma separated list of patterns" << endl;
	cout << "    -i, -interval\ttime_interval - the interval in seconds to be used for updating the services" << endl;
	cout << "    -c, -copy\t\tcopy internally the service data" << endl;
	cout << "    -r, -replace\tsearch_string, replace_string - replace a part of the service name when bridging, can be repeated" << endl;
}

int parse_cmd_line(int argc, char **argv, int *rate, int *copyFlag)
{
	int index = 1;
	char *ptrs = 0, *ptrr = 0;
	int size;
	char *ptr, *ptr1;
	int i;

	strcpy(from_node, "");
	ptr = DimClient::getDnsNode();
	if(ptr) 
		strcpy(from_node, ptr);
	strcpy(to_node, "");
	n_replaces = 0;
	*rate = 0;
	*copyFlag = 0;
	if (argc < 2)
		return 0;
	if (argv[1][0] != '-')	// old style command line
		return -1;
	while (index < argc)
	{
//cout << "arg" << index << " " << argv[index] << endl;
		if (argv[index][0] != '-')	// bad command line
			return 0;
		if (argv[index][1] == 'f')
		{
			index++;
			if (index >= argc)
				return 0;
			strcpy(from_node, argv[index]);
		}
		else if (argv[index][1] == 't')
		{
			index++;
			if (index >= argc)
				return 0;
			strcpy(to_node, argv[index]);
		}
		else if (argv[index][1] == 's')
		{
			index++;
			if (index >= argc)
				return 0;
			services = (char *)malloc(strlen(argv[index])+1);
			strcpy(services, argv[index]);
		}
		else if (argv[index][1] == 'e')
		{
			index++;
			if (index >= argc)
				return 0;
			eservices = (char *)malloc(strlen(argv[index]) + 1);
			strcpy(eservices, argv[index]);
		}
		else if (argv[index][1] == 'i')
		{
			index++;
			if (index >= argc)
				return 0;
			sscanf(argv[index], "%d", rate);
		}
		else if (argv[index][1] == 'c')
		{
			*copyFlag = 1;
		}
		else if (argv[index][1] == 'r')
		{
			if (n_replaces == 0)
			{
				sstrings = (char *)malloc(max_replaces * MAX_NAME);
				rstrings = (char *)malloc(max_replaces * MAX_NAME);
				strcpy(sstrings, "");
				strcpy(rstrings, "");
				ptrs = sstrings;
				ptrr = rstrings;
			}
			index++;
			if (index >= argc)
				return 0;
			strcpy(ptrs, argv[index]);
			index++;
			if (index >= argc)
				return 0;
			strcpy(ptrr, argv[index]);
			ptrs += strlen(ptrs) + 1;
			ptrr += strlen(ptrr) + 1;
			n_replaces++;
			if (n_replaces == max_replaces)
			{
				max_replaces += 10;
				sstrings = (char *)realloc(sstrings, max_replaces * MAX_NAME);
				rstrings = (char *)realloc(rstrings, max_replaces * MAX_NAME);
			}
		}
		index++;
	}
	if (n_replaces)
	{
		size = n_replaces * MAX_NAME;
		myown = (char *)malloc(size);
		ptr = myown;
		ptr1 = rstrings;
		for (i = 0; i < (n_replaces - 1); i++)
		{
			strcpy(ptr, "*");
			strcat(ptr, ptr1);
			strcat(ptr, "*,");
			ptr += strlen(ptr);
			ptr1 += strlen(ptr1) + 1;
		}
		strcpy(ptr, "*");
		strcat(ptr, ptr1);
		strcat(ptr, "*");
	}
	if (!services)
		return 0;
	if ((to_node[0] == '\0') || (services[0] == '\0'))
		return 0;
	return 1;
}

int parse_cmd_line_old(int argc, char **argv, int *rate, int *copyFlag)
{
	if (argc < 3)
	{
		return 0;
	}
	else if (argc == 3)
	{
		strcpy(from_node, DimClient::getDnsNode());
		strcpy(to_node, argv[1]);
		services = (char *)malloc(strlen(argv[2]) + 1);
		strcpy(services, argv[2]);
	}
	else if (argc == 4)
	{
		//		if(sscanf(argv[3],"%d", &rate))
		if (isdigit(argv[3][0]))
		{
			sscanf(argv[3], "%d", rate);
			strcpy(from_node, DimClient::getDnsNode());
			strcpy(to_node, argv[1]);
			services = (char *)malloc(strlen(argv[2]) + 1);
			strcpy(services, argv[2]);
		}
		else if (argv[3][0] == '-')
		{
			*rate = 0;
			strcpy(from_node, DimClient::getDnsNode());
			strcpy(to_node, argv[1]);
			services = (char *)malloc(strlen(argv[2]) + 1);
			strcpy(services, argv[2]);
			*copyFlag = 1;
		}
		else
		{
			*rate = 0;
			strcpy(from_node, argv[1]);
			strcpy(to_node, argv[2]);
			services = (char *)malloc(strlen(argv[3]) + 1);
			strcpy(services, argv[3]);
		}
	}
	else if (argc == 5)
	{
		//		if(sscanf(argv[4],"%d", &rate))
		if (isdigit(argv[4][0]))
		{
			sscanf(argv[4], "%d", rate);
			strcpy(from_node, argv[1]);
			strcpy(to_node, argv[2]);
			services = (char *)malloc(strlen(argv[3]) + 1);
			strcpy(services, argv[3]);
		}
		else if (argv[4][0] == '-')
		{
			*copyFlag = 1;
			//			if(sscanf(argv[3],"%d", &rate))
			if (isdigit(argv[3][0]))
			{
				sscanf(argv[3], "%d", rate);
				strcpy(from_node, DimClient::getDnsNode());
				strcpy(to_node, argv[1]);
				services = (char *)malloc(strlen(argv[2]) + 1);
				strcpy(services, argv[2]);
			}
			else
			{
				*rate = 0;
				strcpy(from_node, argv[1]);
				strcpy(to_node, argv[2]);
				services = (char *)malloc(strlen(argv[3]) + 1);
				strcpy(services, argv[3]);
			}
		}
	}
	else if (argc == 6)
	{
		strcpy(from_node, argv[1]);
		strcpy(to_node, argv[2]);
		services = (char *)malloc(strlen(argv[3]) + 1);
		strcpy(services, argv[3]);
		sscanf(argv[4], "%d", rate);
		*copyFlag = 1;
	}
	else
	{
		return 0;
	}
	return 1;
}

char ServerList_no_link[10] = "";
char ServiceList_no_link[10] = "";

typedef struct serv {
	struct serv *next;
	struct serv *prev;
	char name[MAX_NAME];
	int type;
	char format[MAX_NAME];
	int registered;
	void *serverPtr;
	void *browserServicePtr;
} SERVICE;

int multiple_match(const char*, const char*, int case_sensitive);
int bridge_hash_service_init();
int bridge_hash_service_insert(SERVICE *);
int bridge_hash_service_registered(int, SERVICE *);
int bridge_hash_service_remove(SERVICE *);
SERVICE *bridge_hash_service_exists(char *);
SERVICE *bridge_hash_service_get_next(int *, SERVICE *, int);
int bridge_hash_service_remove_many(int);
/*
class ServerInfo : public DimInfo, public SLLItem
{
	char name[256];

	void infoHandler() {
		char *data, *list, *format;
		char *ptr, *ptr1, *ptr2, *ptr3;
		int mode = 0, type;
		SERVICE *servp;

		data = DimInfo::getString();
//		cout << "***** Received from Server : " << data << endl;
		list = new char[(int)strlen(data) + 1];
		strcpy(list, data);
		ptr = list;
		if (*ptr == '\0')
		{
		}
		else if (*ptr == '+')
		{
			mode = 1;
			ptr++;
		}
		else if (*ptr == '-')
		{
			mode = -1;
			ptr++;
		}
		else
		{
			mode = 1;
		}
		while (*ptr)
		{
			if ((ptr1 = strchr(ptr, '\n')))
			{
				*ptr1 = '\0';
				ptr1++;
			}
			else
			{
				ptr1 = ptr;
				ptr1 += strlen(ptr);
			}
			if ((ptr2 = strchr(ptr, '|')))
			{
				*ptr2 = '\0';
				ptr2++;
				if ((ptr3 = strchr(ptr2, '|')))
				{
					*ptr3 = '\0';
					format = ptr2;
					ptr3++;
					if (*ptr3 == '\0')
						type = DimSERVICE;
					else if (*ptr3 == 'C')
						type = DimCOMMAND;
					else if (*ptr3 == 'R')
						type = DimRPC;
				}
//				cout << "***** Checking Service " << ptr << endl;
				if (!strcmp(ptr, bridge_name))
				{
					ptr = ptr1;
					continue;
				}
				if (!strcmp(ptr, "DIS_DNS"))
				{
					ptr = ptr1;
					continue;
				}
				if (mode == 1)
				{
					if (type != DimRPC)
					{
						if (multiple_match(ptr, services, 1))
						{
							poll_interval = 3;
							servp = (SERVICE *)malloc(sizeof(SERVICE));
							strncpy(servp->name, ptr, (size_t)MAX_NAME);
							servp->type = type;
							strcpy(servp->format, format);
							servp->serverPtr = (void *)this;
							servp->registered = 0;
							bridge_hash_service_insert(servp);
						}
					}
				}
				else if (mode == -1)
				{
					poll_interval = 3;
					servp = bridge_hash_service_exists(ptr);
					if (servp)
					{
//						bridge_hash_service_remove(servp);
//						free(servp);
						servp->registered = -1;
					}
				}
			}
			ptr = ptr1;
		}
	}
public:
	ServerInfo(char *server_name, char *service_name) :
		DimInfo(service_name, ServiceList_no_link)
	{
		strcpy(name, server_name);
	}
	~ServerInfo()
	{
		SERVICE *servp;
		int hash_index;

		hash_index = -1;
		servp = 0;
		while ((servp = bridge_hash_service_get_next(&hash_index, servp, 0)))
		{
			if (servp->serverPtr == (void *)this)
			{
				servp->registered = -1;
			}
		}
	}
	char *getName(){ return name; }
};
*/
//int GotInitialList = 0;


class ServerList : public DimInfo
{
//	ServerInfo *srv, *aux_srv;
	char name[256], srv_name[256];
	SLList servers;
//	char *serverList;
//	int serverListSize;

	void infoHandler()   DIMXX_OVERRIDE {
		char *data;
//		int size;
//		char *ptr;

		data = DimInfo::getString();
//		dim_print_date_time();
//		cout << "***** Received from DNS " << data << endl;
/*
		size = (int)strlen(data) + 1;
		if (!serverListSize)
		{
			serverList = new char[size];
		}
		else if (size > serverListSize)
		{
			delete serverList;
			serverList = new char[size];
			serverListSize = size;
		}
		strcpy(serverList, data);
		if ((serverList[0] == '+') || (serverList[0] == '-'))
*/
		if ((data[0] == '+') || (data[0] == '-'))
		{
			poll_interval = 3;
//			parseServerList();
		}
		else if (data[0] == '\0')
		{
			poll_interval = 0;
			dim_print_date_time();
			cout << "Disconnected from " << DimClient::getServerName() << endl;
		}
		else
		{
			poll_interval = 30;
			dim_print_date_time();
			cout << "Connected to " << DimClient::getServerName() << endl;
		}
//		GotInitialList = 1;
	}

public:
	ServerList(char *name) :
		DimInfo(name, ServerList_no_link)
	{
	}
	~ServerList()
	{
	}
//	int parseServerList();
};
/*
int ServerList::parseServerList()
{
	char *ptr, *ptr1, *ptr2;
	int mode = 0;

	ptr = serverList;
	if (*ptr == '\0')
	{
	}
	else if (*ptr == '+')
	{
		mode = 1;
		ptr++;
	}
	else if (*ptr == '-')
	{
		mode = -1;
		ptr++;
	}
	else
	{
		mode = 1;
	}
	while (*ptr)
	{
		if ((ptr1 = strchr(ptr, '|')))
		{
			*ptr1 = '\0';
			ptr1++;
		}
		else
		{
			ptr1 = ptr;
			ptr1 += strlen(ptr);
		}
		if ((ptr2 = strchr(ptr, '@')))
		{
			*ptr2 = '\0';
//			dim_print_date_time();
//			cout << "***** Checking Server " << ptr << endl;
			if (!strcmp(ptr, bridge_name))
			{
				ptr = ptr1;
				continue;
			}
			if (!strcmp(ptr, "DIS_DNS"))
			{
				ptr = ptr1;
				continue;
			}
			if (mode == 1)
			{
				strcpy(srv_name, ptr);
				strcat(srv_name, "/SERVICE_LIST");
				strcpy(name, ptr);
				dim_print_date_time();
				cout << "***** Subscribing Server " << ptr << " " << srv_name << endl;
				srv = new ServerInfo(name, srv_name);
				servers.add(srv);
			}
			else if (mode == -1)
			{
				srv = (ServerInfo *)servers.getHead();
				while (srv)
				{
					aux_srv = 0;
					//						cout << "Checking remove " << ptr << " " << srv->getName() << endl;
					if (!(strcmp(srv->getName(), ptr)))
					{
						dim_print_date_time();
						cout << "***** Removing Server " << srv->getName() << endl;
						servers.remove(srv);
						aux_srv = srv;
						break;
					}
					srv = (ServerInfo *)servers.getNext();
				}
				if (aux_srv)
				{
					delete aux_srv;
				}
			}
		}
		ptr = ptr1;
	}
	return 1;
}
*/
int checkBridgeServices()
{
	SERVICE *servp;
	int hash_index;

	servp = 0;
	hash_index = -1;
	//		dbr.getServices(services);
	//		while ((type = dbr.getNextService(service, format)))
	while ((servp = bridge_hash_service_get_next(&hash_index, servp, 0)))
	{
//		if (servp->registered == 2)
			servp->registered = 1;
	}
	return 1;
}

int doBridgeService(char *service, char *format, int type)
{
	SERVICE *servp;
	int ret = 0;

	if (!(servp = bridge_hash_service_exists(service)))
	{
		if (multiple_match(service, services, 1))
		{
			if ((multiple_match(service, myown, 1)) ||
				(multiple_match(service, eservices, 1)))
			{
//				poll_interval = 3;
				servp = (SERVICE *)malloc(sizeof(SERVICE));
				strncpy(servp->name, service, (size_t)MAX_NAME);
				servp->type = type;
				strcpy(servp->format, format);
//				servp->serverPtr = (void *)this;
				servp->registered = 2;
				servp->browserServicePtr = (void *)0;
				bridge_hash_service_insert(servp);
//				cout << "Scanning " << service << "registered " << servp->registered << endl;
				ret = 1;
			}
			else
			{
				poll_interval = 3;
				servp = (SERVICE *)malloc(sizeof(SERVICE));
				strncpy(servp->name, service, (size_t)MAX_NAME);
				servp->type = type;
				strcpy(servp->format, format);
				servp->browserServicePtr = (void *)0;
				servp->registered = 0;
				bridge_hash_service_insert(servp);
				//				cout << "Scanning " << service << " registered " << servp->registered << endl;
				ret = 1;
			}
		}
	}
	else
	{
		servp->registered = 2;
//		cout << "Scanning " << service << "registered " << servp->registered << endl;
		ret = 2;
	}
	return ret;
}

int dontBridgeServices()
{
	SERVICE *servp;
	int hash_index;

	servp = 0;
	hash_index = -1;
	//		dbr.getServices(services);
	//		while ((type = dbr.getNextService(service, format)))
	while ((servp = bridge_hash_service_get_next(&hash_index, servp, 0)))
	{
		if (servp->registered == 1)
			servp->registered = -1;
	}
	return 1;
}

int main(int argc, char **argv)
{
DimBrowser dbr;
char *service, *format, *p;
int type, known;
BridgeService *ptrs;
BridgeCommand *ptrc;
int rate = 0;
int copyFlag = 0;
int i, ret, done;
ServerList *serverList;
SERVICE *servp;
int hash_index;
char *sptr, *rptr;

//dic_set_debug_on();
//	cout << "n pars " << argc << endl;
//	for (i = 0; i < argc; i++)
//		cout << "arg " << i << " " << argv[i] << endl;
	bridge_hash_service_init();
	ret = parse_cmd_line(argc, argv, &rate, &copyFlag);
	if (ret == -1)
	{
		ret = parse_cmd_line_old(argc, argv, &rate, &copyFlag);
		if (!ret)
		{
			print_usage_old();
			return 0;
		}
	}
	else if (!ret)
	{
		print_usage();
		return 0;
	}
	cout << "Starting DimBridge from " << from_node << " to " << to_node << endl;
	cout << "for " << services << endl;
	if (eservices)
	{
		cout << "excluding " << eservices << endl;
	}
	if (n_replaces)
	{
		sptr = sstrings;
		rptr = rstrings;
		for (i = 0; i < n_replaces; i++)
		{
			cout << "replacing " << sptr << " by " << rptr << endl;
			sptr += strlen(sptr) + 1;
			rptr += strlen(rptr) + 1;
		}
	}
	if ((rate) || (copyFlag))
		cout << "option(s):";
	if (rate)
		cout << " update interval=" << rate;
	if ((rate) && (copyFlag))
		cout << ",";
	if (copyFlag)
		cout << " internal data copy";
	if ((rate) || (copyFlag))
		cout << endl;

	strcpy(bridge_name,"Bridge_");
	strcat(bridge_name, from_node);
	if ((p = strchr(bridge_name, ':')))
		*p = '\0';
	if ((p = strchr(bridge_name, ',')))
		*p = '\0';
	if ((p = strchr(bridge_name, '.')))
		*p = '\0';
/*
	strcat(bridge_name, "_");
	strcat(bridge_name, to_node);
	if ((p = strchr(bridge_name, ':')))
		*p = '\0';
	if ((p = strchr(bridge_name, ',')))
		*p = '\0';
	if ((p = strchr(bridge_name, '.')))
		*p = '\0';
*/
	p = bridge_name + strlen(bridge_name);
#ifndef WIN32
	sprintf(p,"_%d",getpid());
#else
	sprintf(p,"_%d",_getpid());
#endif
	from_port = 0;
	to_port = 0;
	if ((p = strchr(from_node, ':')))
	{
		*p = '\0';
		p++;
		sscanf(p, "%d", &from_port);
	}
	if ((p = strchr(to_node, ':')))
	{
		*p = '\0';
		p++;
		sscanf(p, "%d", &to_port);
	}
//	cout << "Starting1 DimBridge from " << from_node << ":" << from_port <<" to " << to_node << ":" << to_port <<" for " << services << endl;
	if (!from_port)
		DimClient::setDnsNode(from_node);
	else
		DimClient::setDnsNode(from_node, from_port);
	if(!to_port)
		DimServer::setDnsNode(to_node);
	else
		DimServer::setDnsNode(to_node, to_port);
	serverList = new ServerList((char *)"DIS_DNS/SERVER_LIST");
	if (serverList){} // to avoid a warning
	//	while (!GotInitialList)
//		sleep(1);
//	serverList->parseServerList();

	while (1)
	{
		done = 0;
		checkBridgeServices();
		dbr.getServices("*");
		while ((type = dbr.getNextService(service, format)))
		{
			doBridgeService(service, format, type);
		}
		dontBridgeServices();
		servp = 0;
		hash_index = -1;
		while ((servp = bridge_hash_service_get_next(&hash_index, servp, 0)))
		{
			service = servp->name;
			type = servp->type;
			format = servp->format;
//			dim_print_date_time();
//			cout << "Checking " << service << " reg "<< servp->registered << " type " << type << endl;
			known = servp->registered;
			if (known == 0)
			{
				done = 1;
				if(type == DimSERVICE)
				{
					dim_print_date_time();
					cout << "Start Bridging Service: " << service << endl;
					if (!rate)
						ptrs = new BridgeService(service, format, copyFlag);
					else
						ptrs = new BridgeService(service, format, rate, copyFlag);
					servp->browserServicePtr = (void *)ptrs;
				}
				else if (type == DimCOMMAND)
				{
					dim_print_date_time();
					cout << "Start Bridging Command: " << service << endl;
					//					DimClient::setDnsNode(to_node);
					ptrc = new BridgeCommand(service, format);
					servp->browserServicePtr = (void *)ptrc;
//					DimClient::setDnsNode(from_node);
				}
				servp->registered = 1;
			}
			else if (known == -1)
			{
				done = -1;
				if (type == DimSERVICE)
				{
					ptrs = (BridgeService *)servp->browserServicePtr;
					if (ptrs)
					{
						dim_print_date_time();
						cout << "Stop Bridging Service: " << ptrs->getName() << endl;
						delete ptrs;
					}
				}
				else if (type == DimCOMMAND)
				{
					ptrc = (BridgeCommand *)servp->browserServicePtr;
					if (ptrc)
					{
						dim_print_date_time();
						cout << "Stop Bridging Command: " << ptrc->getName() << endl;
						delete ptrc;
					}
				}
				servp->registered = -2;
			}
		}
		if (poll_interval == 2)
		{
//			sleep(2);
//			dbr.getServices("*");
			poll_interval = 5;
//			continue;
		}
		if (!done)
		{
			poll_interval = 30;
		}
		else if (done < 0)
		{
			bridge_hash_service_remove_many(-2);
			poll_interval = 2;
		}
		else
		{
			poll_interval = 5;
		}
//		cout << "done " << done << ", poll_interval " << poll_interval << endl;
		for (i = 0; i < poll_interval; i++)
		{
			sleep(1);
			if (poll_interval == 0)
				i = -2;
		}
//		cout << "out of sleep, poll_interval " << poll_interval << endl;
	}
	return 1;
}






/////////////////////////////
//////// Function for glob pattern matching. See http://www.cse.yorku.ca/~oz/
/*
* robust glob pattern matcher
* ozan s. yigit/dec 1994
* public domain
*
* glob patterns:
*	*	matches zero or more characters
*	?	matches any single character
*	[set]	matches any character in the set
*	[^set]	matches any character NOT in the set
*		where a set is a group of characters or ranges. a range
*		is written as two characters seperated with a hyphen: a-z denotes
*		all characters between a to z inclusive.
*	[-set]	set matches a literal hypen and any character in the set
*	[]set]	matches a literal close bracket and any character in the set
*
*	char	matches itself except where char is '*' or '?' or '['
*	\char	matches char, including any pattern character
*
* examples:
*	a*c		ac abc abbc ...
*	a?c		acc abc aXc ...
*	a[a-z]c		aac abc acc ...
*	a[-a-z]c	a-c aac abc ...
*
* $Log: glob.c,v $
* Revision 1.3  1995/09/14  23:24:23  oz
* removed boring test/main code.
*
* Revision 1.2  94/12/11  10:38:15  oz
* cset code fixed. it is now robust and interprets all
* variations of cset [i think] correctly, including [z-a] etc.
*
* Revision 1.1  94/12/08  12:45:23  oz
* Initial revision
*/

#ifndef NEGATE
#define NEGATE	'^'			/* std cset negation char */
#endif

#define TRUE    1
#define FALSE   0

int
amatch(char *str, char *p)
{
	int negate;
	int match;
	int c;

	while (*p) {
		if (!*str && *p != '*')
			return FALSE;

		switch (c = *p++) {

		case '*':
			while (*p == '*')
				p++;

			if (!*p)
				return TRUE;

			if (*p != '?' && *p != '[' && *p != '\\')
				while (*str && *p != *str)
					str++;

			while (*str) {
				if (amatch(str, p))
					return TRUE;
				str++;
			}
			return FALSE;

		case '?':
			if (*str)
				break;
			return FALSE;
			/*
			* set specification is inclusive, that is [a-z] is a, z and
			* everything in between. this means [z-a] may be interpreted
			* as a set that contains z, a and nothing in between.
			*/
		case '[':
			if (*p != NEGATE)
				negate = FALSE;
			else {
				negate = TRUE;
				p++;
			}

			match = FALSE;

			while (!match && (c = *p++)) {
				if (!*p)
					return FALSE;
				if (*p == '-') {	/* c-c */
					if (!*++p)
						return FALSE;
					if (*p != ']') {
						if (*str == c || *str == *p ||
							(*str > c && *str < *p))
							match = TRUE;
					}
					else {		/* c-] */
						if (*str >= c)
							match = TRUE;
						break;
					}
				}
				else {			/* cc or c] */
					if (c == *str)
						match = TRUE;
					if (*p != ']') {
						if (*p == *str)
							match = TRUE;
					}
					else
						break;
				}
			}

			if (negate == match)
				return FALSE;
			/*
			* if there is a match, skip past the cset and continue on
			*/
			while (*p && *p != ']')
				p++;
			if (!*p++)	/* oops! */
				return FALSE;
			break;

		case '\\':
			if (*p)
				c = *p++;
		default:
			if (c != *str)
				return FALSE;
			break;

		}
		str++;
	}

	return !*str;
}



/////////////////////////////// END OF CODE FROM http://www.cse.yorku.ca/~oz/

char *strlwr(char *str)
{
	unsigned char *p = (unsigned char *)str;

	while (*p) {
		*p = tolower((unsigned char)*p);
		p++;
	}

	return str;
}

// ps is a comma separated list of patterns. The function returns 1 as soon as one pattern matches
int multiple_match(const char* str, const char* ps, int case_sensitive) {
	if (ps == 0) return 0;
	if (strlen(ps) == 0) return 0;
	char* str2 = new char[strlen(str) + 1];
	strcpy(str2, str);

	char* p;
	// the pattern must be copied in new memory because strtok modifies the string
	char* p2 = new char[strlen(ps) + 1];
	strcpy(p2, ps);

	if (case_sensitive == 0) {
		strlwr(str2);
		strlwr(p2);
	}

	p = strtok(p2, ",");
	int result = 0;
	while (p != NULL)
	{
		if (amatch(str2, p) == 1) {
			result = 1;
			break;
		}
		p = strtok(NULL, ",");
	}
	delete [] p2;
	p2 = NULL;
	delete [] str2;
	str2 = NULL;
	return result;
}

#define MAX_HASH_ENTRIES 5000

static SERVICE *BridgeService_hash_table[MAX_HASH_ENTRIES];
static int Service_new_entries[MAX_HASH_ENTRIES];

int bridge_hash_service_init()
{

	int i;
	static int done = 0;

	dim_lock();
	if (!done)
	{
		for (i = 0; i < MAX_HASH_ENTRIES; i++)
		{
			/*
			BridgeService_hash_table[i] = (SERVICE *) malloc(sizeof(SERVICE));
			dll_init((DLL *) BridgeService_hash_table[i]);
			*/
			BridgeService_hash_table[i] = 0;
			Service_new_entries[i] = 0;
		}
		done = 1;
	}
	dim_unlock();
	return(1);
}

int bridge_hash_service_insert(SERVICE *servp)
{
	int index;

	dim_lock();
	index = HashFunction(servp->name, MAX_HASH_ENTRIES);
	if (!BridgeService_hash_table[index])
	{
		BridgeService_hash_table[index] = (SERVICE *)malloc(sizeof(SERVICE));
		dll_init((DLL *)BridgeService_hash_table[index]);
	}
	Service_new_entries[index]++;
	dll_insert_queue((DLL *)BridgeService_hash_table[index],
		(DLL *)servp);
	dim_unlock();
	return(1);
}

int bridge_hash_service_registered(int index, SERVICE *servp)
{
	dim_lock();
	servp->registered = 1;
	Service_new_entries[index]--;
	if (Service_new_entries[index] < 0)
		Service_new_entries[index] = 0;
	dim_unlock();
	return 1;
}

int bridge_hash_service_remove(SERVICE *servp)
{
	int index;

	dim_lock();
	index = HashFunction(servp->name, MAX_HASH_ENTRIES);
	if (!BridgeService_hash_table[index])
	{
		dim_unlock();
		return(0);
	}
	dll_remove((DLL *)servp);
	dim_unlock();
	return(1);
}


SERVICE *bridge_hash_service_exists(char *name)
{
	int index;
	SERVICE *servp;

	dim_lock();
	index = HashFunction(name, MAX_HASH_ENTRIES);
	if (!BridgeService_hash_table[index])
	{
		dim_unlock();
		return((SERVICE *)0);
	}
	if ((servp = (SERVICE *)dll_search(
		(DLL *)BridgeService_hash_table[index],
		name, (int)strlen(name) + 1)))
	{
		dim_unlock();
		return(servp);
	}
	dim_unlock();
	return((SERVICE *)0);
}

SERVICE *bridge_hash_service_get_next(int *curr_index, SERVICE *prevp, int new_entries)
{
	int index;
	SERVICE *servp = 0;
	/*
	if(!prevp)
	{
	index = -1;
	}
	*/
	dim_lock();
	index = *curr_index;
	if (index == -1)
	{
		index++;
		prevp = BridgeService_hash_table[index];
	}
	if (!prevp)
	{
		prevp = BridgeService_hash_table[index];
	}
	do
	{
		if (prevp)
		{
			if ((!new_entries) || (Service_new_entries[index] > 0))
			{
				servp = (SERVICE *)dll_get_next(
					(DLL *)BridgeService_hash_table[index],
					(DLL *)prevp);
				if (servp)
					break;
			}
		}
		index++;
		if (index == MAX_HASH_ENTRIES)
		{
			*curr_index = -1;
			dim_unlock();
			return((SERVICE *)0);
		}
		prevp = BridgeService_hash_table[index];
	} while (!servp);
	*curr_index = index;
	dim_unlock();
	return(servp);
}

int bridge_hash_service_remove_many(int registered)
{
	SERVICE *servp, *prevp;
	int hash_index, old_index;
	int n = 0;

	dim_lock();
	prevp = 0;
	hash_index = -1;
	old_index = -1;
	while ((servp = bridge_hash_service_get_next(&hash_index, prevp, 0)))
	{
		if (servp->registered == registered)
		{
			servp->registered = 0;
			bridge_hash_service_remove(servp);
			free(servp);
			n++;
			if (old_index != hash_index)
				prevp = 0;
		}
		else
		{
			prevp = servp;
			old_index = hash_index;
		}
	}
	dim_unlock();
	return(n);
}
