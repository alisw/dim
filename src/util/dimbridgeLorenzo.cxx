/* #define USE_INI  */
#include <dic.hxx>
#include <dis.hxx>
#include <dim.h>
#ifdef USE_INI
#include "IniReader.h"
#endif
#include <iostream>
#include <cstdlib.>
#include <time.h>
#include <string>

#ifdef USE_INI
#define GETINTPARAMETER(a,b)   getIntParameter(a,b,reader);
#define GETSTRINGPARAMETER(a)   getStringParameter(a,reader);
#endif
#ifndef USE_INI
#define GETINTPARAMETER(a,b)   getIntParameter(a,b);
#define GETSTRINGPARAMETER(a)   getStringParameter(a);
#endif

#define DIM_BRIDGE_SERVICES_LENGTH 2048
#define NO_LINK_PEER -99
using namespace std;


static int no_link = 0xdeaddead;
static char from_node[64], to_node[64], bridge_name[64], service_name[128];

static char* nullStr = NULL;
/*
DimBridge - modified by Lorenzo Masetti to include timestamp and quality information,
to provide an option to set the refresh time from the DNS and increase the default refresh to 
10 minutes (instead of 5 seconds).
Several configurable parameters.
Configuration can be done:
	- through environment variables (if USE_INI is not defined)
	- through a simple INI file: this requires inih_r27 library (download it from http://code.google.com/p/inih/)

 - Can connect to a dim item to connect to PVSS redundancy status and run in parallel in two hosts to be ready to switch over
 - dimBridge should be run in a batch or shell script that restarts it automatically if it exits (because the peer is not active anymore) or if it crashes
*/

void dimbridge_print_date_time()
{
	time_t t;
	char str[128];

	t = time((time_t *)0);

	my_ctime(&t, str, 128);
	str[strlen(str)-1] = '\0';	
	printf("%s  : ",str );
}


class HostnameInfo : public DimInfo 
{
	int peer ; /* 1 or 2 */
	int status ; /* -1= initializing 0 = not active 1 = active */
	int must_rescan;
	int last_peer;


	void infoHandler() {
		int prev_status = status;
		if (prev_status != -1) {
			dimbridge_print_date_time();
			cout << "Active peer changed to " << getPeer() << endl;
		}

		if (peerMatches()) {
			status =1;				
		} else {
			status = 0;
		}

		if ((prev_status == 0) && (status ==1)) {
			must_rescan = 1;
		} else if ((prev_status == 1) && (status == 0)) {
			// Restart the bridge
			dimbridge_print_date_time();
			cout << " I am not active anymore. I need to restart..." << endl;
			exit(0);
		}

	}
	
public:
	HostnameInfo(const char *name, int myPeer):	  DimInfo(name,NO_LINK_PEER ) {
		  peer = myPeer;		  
		  status = -1;
		  must_rescan = 0;
		  last_peer = -1;
	}
	
	bool peerMatches() {
		
		
		return (peer== getPeer());
	}

	bool getBool() {
		//cout << "getInt " << getInt() << endl;
		return ((getInt() &1) != 0); 
	}

	int getPeer() {
		//cout <<"getInt() " <<  getInt() << endl;
		//cout << "getBool " << getBool() << endl;
		if (getInt() == NO_LINK_PEER) return last_peer;

		last_peer= (getBool())?1:2;
		return last_peer;
	
	}

	int isInitialized() {
		return (status > -1);
	}

	int mustRescan() {
		if (must_rescan == 1) {
			must_rescan = 0;
			return 1;
		}
		return 0;
	}

	bool isActive() {
		return (status ==1);
	}
};


/*
Changes to include timestamp and quality: inherit from DimStampedInfo instead of DimInfo
*/
class BridgeService: public DimStampedInfo, public SLLItem 
{
	char srv_name[256];
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

	int debug;




	void infoHandler() {
		char *server;
		data = DimStampedInfo::getData();
		srv_size = DimStampedInfo::getSize();
		/*
		New fields to get time and quality
		*/
		timestamp = getTimestamp();
		milliseconds = getTimestampMillisecs();
		quality = getQuality();

		//	cout << "Got time "<< timestamp << " Quality " << quality << endl;

		// Sometimes gets a packet from DIS_DNS not understood why!!!"
		server = DimClient::getServerName();
		if(strstr(server,"DIS_DNS") != 0)
		{
			dim_print_date_time();
			//cout << "received from " << server << " size = " << srv_size << endl;
			if(strstr(srv_name,"DIS_DNS") == 0)
				return;
		}
		srv_data = data;
		if(*(int *)srv_data == no_link)
		{
			if(srv)
			{
				delete(srv);
				srv = 0;
			}
			declared = 0;
			if (debug >= 1) {
				dimbridge_print_date_time();
				cout << " Disconnecting bridge for " << srv_name << endl;
			}
		}
		else if(!declared)
		{
			//		DimServer::setDnsNode(to_node);
			srv = new DimService(srv_name, srv_format, srv_data, srv_size);
			if(copyFlag)
				srv->setData(srv_data, srv_size);
			srv->setQuality(quality);
			srv->setTimestamp(timestamp,milliseconds);

			DimServer::start(bridge_name);
			declared = 1;
			//		DimClient::setDnsNode(from_node);
		}
		else
		{
			if(srv)
			{
				srv->setQuality(quality);
				srv->setTimestamp(timestamp,milliseconds);
				if(copyFlag)
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
		  cmnd = 0;
		  found = 1;
		  debug = 0;
		  //cout << "Bridging Service: " << name << endl;
	  }
	  BridgeService(char *name, char *format, int rate, int copy):
	  DimStampedInfo(name, rate, &no_link, sizeof(no_link)), declared(0), srv(0), copyFlag(copy)
	  { 
		  strcpy(srv_format, format);
		  strcpy(srv_name, name);
		  cmnd = 0;
		  found = 1;
		  debug = 0;
		  //cout << "Bridging Service: " << name << ", rate = " << rate << " seconds" << endl;
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
		  if (debug >= 1) {
				dimbridge_print_date_time();
				cout << " Deleted bridge for " << srv_name << endl;
		   }
		  
	  }
	  char *getName() { return srv_name; };
	  void clear() { found = 0;};
	  void set() {found = 1;};
	  int find() {return found;};
	  int isCmnd() { return cmnd;};
	  void setDebug(int d) {
		  debug = d;
	  }
};

class BridgeCommand: public DimCommand, public SLLItem 
{
	char srv_name[256];
	char srv_format[256];
	int declared;
	DimService *srv;
	void *srv_data;
	int srv_size;
	int cmnd;
	int found;

	void commandHandler() {
		srv_data = DimCommand::getData();
		srv_size = DimCommand::getSize();
		DimClient::sendCommandNB(srv_name, srv_data, srv_size);
	}

public:
	BridgeCommand(char *name, char *format):
	  DimCommand(name, format) 
	  { 
		  DimServer::start(bridge_name);
		  cmnd = 1;
		  found = 1;
		  strcpy(srv_name, name);
		  //			cout << "Bridging Command: " << name << endl;
	  }
	  char *getName() { return srv_name; };
	  void clear() { found = 0;};
	  void set() {found = 1;};
	  int find() {return found;};
	  int isCmnd() { return cmnd;};
};

void print_usage()
{
#ifdef USE_INI
	cout << "Usage: dimBridge config_file" << endl;
	cout << "The config file is a INI file with a [dimBridge] section";
	cout << "Elements to set in the config file: "<< endl;
#endif
#ifndef USE_INI
	cout << "Usage: dimBridge" << endl;
	cout << "The configuration is done through environment variables";
	cout << "Variables to set: "<< endl;
#endif
	cout << "   FROM_NODE - source DNS" << endl;
	cout << "   TO_NODE - the complete node name of the target DNS" << endl;
	cout << "   SERVICES - the list of service names to bridge (wildcards allowed)" << endl;
	cout << "   RATE - the interval in seconds to be used for updating the services (if 0 on change)" << endl;
	cout << "   REFRESH - the interval in seconds to be used for refreshing the list of services from DNS (default 10 minutes)" << endl;
	cout << "   COPY - copy internally the service data (default 0)" << endl;
	cout << "	ALLOWED_NODES  -  comma separated list of nodes in order to restrict the bridge to a list of hosts in the from_node DNS (whitelist)" << endl;
	cout << "	NOT_ALLOWED_NODES - comma separated list of nodes in order to avoid bridging from one of these hosts (blacklist)" << endl;
	cout << "	BRIDGE_COMMANDS - set it to 0 in order not to bridge commands. Commands are bridged by default" << endl;
	cout << "	VERBOSE - debug level (from 0 to 3)" << endl;
	cout << "	TESTING - set it to 1 in order to start in testing mode. No bridge is performed but the DNS is queried. If set to 2, will start in testing mode and will exit after the first scan" << endl;	
	cout << "	BRIDGE_IF_ALREADY_BRIDGED - set it to 1 in order to bridge also publications coming from another bridge (default: 0). The default behaviour avoids the circular bridges." << endl;	
	cout << "	DISCONNECTION_DEBUG - 0 no debug 1 debug for disconnection and stopping 2 debug for each scan, 3 debug for disconnection, stopping and scan" << endl;
	cout << "	EXCLUDE_PATTERN - Comma separated list of services to be EXCLUDED (wildcards allowed)" << endl;
	cout << "   NOT_ALLOWED_PUBLISHERS - Comma separated blacklist of publisher names to be excluded (wildcards allowed)" << endl;
	cout << "	TIMEOUT_SCAN - Timeout (in seconds) for the requests to services (default = 10 seconds)" << endl;
	cout << "	DIM_ACTIVE_HOST - Name of the dim publication that determines the active host " << endl;
	cout << "   INACTIVE_REFRESH - Refresh time when inactive (default 20 seconds)  " << endl;
	cout << "	PVSS_REDU - host1,host2 for PVSS redundancy " << endl;

#ifndef USE_INI
	cout << endl;
	cout << "	Alternatively you can use the old usage that only supports few parameters:  " << endl;
	cout << "   Usage: DimBridge [from_node] to_node services [time_interval] [-copy]" << endl;
#endif
}

void replace_char (char *s, char find, char replace) {
	while (*s != 0) {
	if (*s == find)
		*s = replace;
	s++;
	}
}

// defined later

int
multiple_match(const char *str, const char *ps, int case_sensitive = 1); 
int
amatch(char *str, char *p);
int checkChars();

#ifdef USE_INI
void getIntParameter(int& parameter, const char* name, INIReader reader) {
	parameter = reader.GetInteger("dimBridge",name, parameter);

	//cout << "Reading " << name <<  " " << parameter << endl;

}

const char* getStringParameter(const char* name,  INIReader reader) {
	
	string res = reader.Get("dimBridge", name, "");
	if (res == "") return NULL;

	//cout << "Reading " << name << " " <<  res << endl;
	char* result = new char[res.length()+1];
    strcpy(result,res.c_str());
    return result;
	
}
#endif 

#ifndef USE_INI
void getIntParameter(int& parameter, const char* name) {
	const char* val = getenv(name);
	if (val) {
			parameter = atoi(val);
	}	

}

char* getStringParameter(const char* name) {
	return getenv(name);
	
}
#endif
int main(int argc, char **argv)
{	DimBrowser dbr;
	char *service, *format, *p;
	int type, known;
	BridgeService *ptrs, *aux_ptrs;
	BridgeCommand *ptrc, *aux_ptrc;
	SLList lists, listc;
	int rate = 0;
	int disconnection_debug = 0;
	int scan_debug = 0;
	int timeout_scan = 10; // default timeout for getServerServices

	int refreshDelay = 600; // 10 minutes
	int bridgeCommands = 1;
	int copyFlag = 0;
	int verbose = 0;
	int testing = 0;
	int exit_after_first_scan = 0;
	// if 0 automatically skips the publisher that contains Bridge_ in the name (to avoid infinite loops in bridges)
	int bridge_already_bridged = 0;
	int createAdditionalPublication = 0;
	int port = 2506;
	int check_chars = 1;
	int inactive_refresh = 20;





#ifdef USE_INI
   if (argc == 1) {
		cerr << "Please specify the config file " << endl << endl;
		print_usage();
		return 0;
	}
   INIReader reader(argv[1]);

   if (reader.ParseError() < 0) {
    cerr << " ERROR : Unable to open  file or invalid syntax for " << argv[1] << endl;
	exit(-1);
  }

#endif


  char* from_node = GETSTRINGPARAMETER("FROM_NODE");
  char* to_node = GETSTRINGPARAMETER("TO_NODE");
  GETINTPARAMETER(port, "DIM_PORT");

  GETINTPARAMETER(rate,"RATE");
  GETINTPARAMETER(refreshDelay, "REFRESH");
  GETINTPARAMETER(copyFlag, "COPY");

  char* services = GETSTRINGPARAMETER("SERVICES");

  GETINTPARAMETER(bridgeCommands, "BRIDGE_COMMANDS");
  GETINTPARAMETER(disconnection_debug, "DISCONNECTION_DEBUG");
  
  scan_debug = disconnection_debug >> 1;
  disconnection_debug = disconnection_debug & 1;

  GETINTPARAMETER(verbose, "VERBOSE");
  GETINTPARAMETER(testing, "TESTING");
  GETINTPARAMETER(check_chars, "CHECK_CHARS");
  if (testing == 2) {
	testing = 1;
	exit_after_first_scan = 1;
  }

  GETINTPARAMETER(createAdditionalPublication,"CREATE_ADDITIONAL_PUBLICATION");
  const char* allowed_nodes = GETSTRINGPARAMETER("ALLOWED_NODES");
  const char* not_allowed_nodes = GETSTRINGPARAMETER("NOT_ALLOWED_NODES");
  const char* not_allowed_services = GETSTRINGPARAMETER("EXCLUDE_PATTERN");

  GETINTPARAMETER(bridge_already_bridged, "BRIDGE_IF_ALREADY_BRIDGED");

  const char* not_allowed_publishers = GETSTRINGPARAMETER("NOT_ALLOWED_PUBLISHERS");

  GETINTPARAMETER(timeout_scan, "TIMEOUT_SCAN");

  GETINTPARAMETER(inactive_refresh, "INACTIVE_REFRESH");

  const char* active_host_publication = GETSTRINGPARAMETER("DIM_ACTIVE_HOST");
  const char* pvss_hosts = GETSTRINGPARAMETER("PVSS_REDU");
  
  
  // backward compatibility of arguments
  if (argc >= 3) {
		from_node = new char[64];
		to_node = new char[64];
  }
  if( argc == 3)
    {
		strcpy(from_node, DimClient::getDnsNode());
		strcpy(to_node, argv[1]);
		strcpy(services, argv[2]);
    }
	else if (argc == 4)
	{
		if(sscanf(argv[3],"%d", &rate))
		{
			strcpy(from_node, DimClient::getDnsNode());
			strcpy(to_node, argv[1]);
			strcpy(services, argv[2]);
		}
		else if(argv[3][0] == '-')
		{
			rate = 0;
			strcpy(from_node, DimClient::getDnsNode());
			strcpy(to_node, argv[1]);
			strcpy(services, argv[2]);
			copyFlag = 1;
		}
		else
		{
			rate = 0;
			strcpy(from_node, argv[1]);
			strcpy(to_node, argv[2]);
			strcpy(services, argv[3]);
		}
    }
	else if(argc == 5)
	{
		if(sscanf(argv[4],"%d", &rate))
		{
			strcpy(from_node, argv[1]);
			strcpy(to_node, argv[2]);
			strcpy(services, argv[3]);
		}
		else if(argv[4][0] == '-')
		{
			copyFlag = 1;
			if(sscanf(argv[3],"%d", &rate))
			{
				strcpy(from_node, DimClient::getDnsNode());
				strcpy(to_node, argv[1]);
				strcpy(services, argv[2]);
			}
			else
			{
				rate = 0;
				strcpy(from_node, argv[1]);
				strcpy(to_node, argv[2]);
				strcpy(services, argv[3]);
			}
		}
	}
	else if(argc == 6)
	{
		strcpy(from_node, argv[1]);
		strcpy(to_node, argv[2]);
		strcpy(services, argv[3]);
		sscanf(argv[4],"%d", &rate);
		copyFlag = 1;
    }

	if (! from_node)  {
		strcpy(from_node, DimClient::getDnsNode());
	}

  if ((! from_node ) || (! to_node)) {
		cerr << "Please specify FROM_NODE and TO_NODE " << endl;
		print_usage();
		exit(-1);
  }
  if (! services) {
	cerr << "Please specify SERVICES " << endl;
	print_usage();
	exit(-1);
  }

  	// increase buffer size
	dim_set_write_buffer_size(10000000); 

	
	

	dimbridge_print_date_time();
	cout << " Starting DimBridge " << endl;
	cout << " FROM "<<from_node<<endl <<" TO "<<to_node<< endl << " FOR "<< services << endl;
	if(rate)
		cout << " interval=" << rate; 
	if(copyFlag)
		cout << " (internal data copy)"; 
	cout << " refresh services every " << refreshDelay << " seconds";
	if (bridgeCommands == 1) {
		cout << " commands are also bridged. ";
	} else {
		cout << " commands are not bridged ";
	}
	cout << " disconnection debug = " << disconnection_debug << ", scan debug = " << scan_debug << endl;
	cout << " timeout during scan " << timeout_scan;
	cout << endl;
	
	if (allowed_nodes) cout << "List of allowed nodes specified " << endl;
	if (not_allowed_nodes) cout << "Blacklist of nodes specified " << not_allowed_nodes << endl;
	if (bridge_already_bridged == 1) cout << "Warning: also services coming from another bridge are forwarded" << endl;
	if (testing) cout << "TEST MODE : NO REAL BRIDGING IS DONE !!" << endl;
	
	cout << endl;


	

#ifndef WIN32
	sprintf(bridge_name,"_%d",getpid());
#else
	sprintf(bridge_name,"_%d",_getpid());
#endif

	string my_name = string("Bridge_") + string(from_node)   + string(bridge_name);
	if (my_name.length() > 64) my_name = my_name.substr(0,64);
	
	strcpy(bridge_name, my_name.c_str());
	replace_char(bridge_name,',','_');


	//cout << "Bridge name " << bridge_name << endl;





	char* server, *node;


	/*

	// Impossible to do that: it seems that the port is a global variable in dim

	int from_port,to_port ;
	from_port = to_port = 0;

	if ((p = strchr(to_node, ':'))) {		
	sscanf(&(p[1]),"%d", &to_port);
	*p = '\0';
	cout << "Setting dns node " << to_node << " port " << to_port << endl;
	DimClient::setDnsNode(to_node, to_port);	
	} else {
	DimClient::setDnsNode(to_node);	
	}


	if ((p = strchr(from_node, ':'))) {

	sscanf(&(p[1]),"%d", &from_port);
	*p = '\0';
	cout << "Setting dns node " << from_node << " port " << from_port << endl;
	DimClient::setDnsNode(from_node, from_port);	
	} else {
	DimClient::setDnsNode(from_node);	
	}
	*/

	DimClient::setDnsNode(from_node, port);
	DimServer::setDnsNode(to_node, port);

	int countNodes, countServices, countServers;

	int servedIntVal = -1;
	


	strcpy(service_name,bridge_name);
	strcat(service_name, "/DIM_BRIDGE_N_FOUND_PUBLISHERS");

	DimService* servint = NULL;
		
	if (createAdditionalPublication) {
		servint = new DimService(service_name,servedIntVal);
    
		servint->updateService();
	}

	// check characters
	if (check_chars == 1) {
		int n_strange_chars = checkChars();
		if (verbose>=1) {
			cout << n_strange_chars << " strange char found in the server list " << endl;
		}
	}

	HostnameInfo* hostInfo = NULL;

	if (active_host_publication != NULL) {
			const char* my_hostname = getenv("COMPUTERNAME");
			cout << "My Hostname is " << my_hostname << endl;
			if (pvss_hosts == NULL) {
				cerr << "Configuration missing. Please enter PVSS_REDU configuration in the config file " << endl;
				exit(-1);
			}

			int peer = 0;
			const char* pos = strstr(pvss_hosts, my_hostname);
			if (pos == NULL) {
				cerr << "Error. dimBridge is not running on neither of the redundant hosts " << pvss_hosts << endl;
				exit(-1);
			}
			if (pos == pvss_hosts) { // matches at the start
				peer = 1;
			} else {
				peer = 2;
			}

			cout << " I am peer " << peer << " in PVSS redundancy " << endl;
			if (verbose >= 2) {
				cout << "Connecting publication " << active_host_publication << endl;
			}
			hostInfo = new HostnameInfo(active_host_publication,  peer);
			
			int countWait = 0;
			while ((countWait < 120) && (! hostInfo->isInitialized())) {
				sleep(1);
				countWait++;		
			}
		
	}

	while(1)
	{
		if (hostInfo) {
			if (! hostInfo->isActive()) {
				if (verbose >=2) {
					cout << "I am not active... Skipping cycle" << endl;
				}
				sleep(inactive_refresh);
				continue;
				
			}

			if (hostInfo->mustRescan()) {
				if (verbose >=1) {			
					dimbridge_print_date_time();
					cout << "Must rescan services.... Wait until the other bridge is properly closed" << endl;
				}
				sleep(3);
				
			}
		}
		if (scan_debug > 0) {
			dimbridge_print_date_time();
			cout <<  " Scanning..." << endl;
		}
		countNodes = countServices = countServers = 0;

		ptrs = (BridgeService *)lists.getHead();
		while(ptrs)
		{
			ptrs->clear();
			ptrs = (BridgeService *)lists.getNext();
		}
		ptrc = (BridgeCommand *)listc.getHead();
		while(ptrc)
		{
			ptrc->clear();
			ptrc = (BridgeCommand *)listc.getNext();
		}

	

		// get the servers from the dim browser
		int n_servers = dbr.getServers();
		if (verbose >= 2) cout << " " << n_servers << " publishers found " << endl;

		while (dbr.getNextServer(server, node)) {
			countServers++;
			if (verbose >=2) {
				cout << "Publisher "  << countServers << ": " << server << "@" << node << endl;
			}
			if (strlen(server) == 0) {
				dimbridge_print_date_time();
				cout << " Warning: Server with empty name detected: " << server << "@" << node << endl;
				continue;
			}
			if ((strchr(server, '@') -server) >=0) {
					cout << "Info: server with @ inside : " << server << "@" << node << endl;
			}
			// if server does not match continue		
			if ((allowed_nodes) // if it is specified
				&& ( ! multiple_match(node, allowed_nodes,0) )) {// and does not match
					if (verbose >= 2) cout << "Skipping not allowed node " << node << " for server " << server <<  endl;		
					continue; 
			}
			if ((not_allowed_nodes) // if it is specified
				&& (  multiple_match(node, not_allowed_nodes,0) )) {// and does match
					if (verbose >= 2) cout << "Skipping node in the blacklist " << node << " for server " << server <<  endl;		
					continue; 
			}
			if ((bridge_already_bridged == 0) && (strstr(server, "Bridge_") == server)) { // if the position of the string "Bridge_" in server is equal to server then the server name starts with Bridge_
				if (verbose >= 2) cout << "Skipping server coming from dimBridge " << server << "@" << node <<  endl;		
				continue; 
			}
		
			if ((not_allowed_publishers) && (multiple_match(server, not_allowed_publishers))) {
				if (verbose >= 2) cout << "Skipping server from not allowed publisher " << server << "@" << node <<  endl;		
				continue; 
			}

			if (verbose) cout << "Processing server " << server << "@" << node << endl;
			countNodes++;
			// get the services published by the server
			if (dbr.getServerServices(server, timeout_scan) == 0) {
			  if (verbose>=2) {
					dimbridge_print_date_time();
					cout << "WARNING NO SERVICES FOUND FOR " << server << endl;		
			   }
			}			
			while( (type = dbr.getNextServerService(service, format)) )
			{
				if (! multiple_match(service, services) ) {
					if (verbose >= 3) cout << "		Skipping not matching service " << service << endl;
					continue;
				}
				
				if (not_allowed_services != NULL) { // if it is specified
					if (multiple_match(service, not_allowed_services)) { // and it matches
						if (verbose>= 3) cout << "Skipping excluded service " << service << endl;
						continue;
					}
				}

				known = 0;
				ptrs = (BridgeService *)lists.getHead();
				while(ptrs)
				{
					if(!strcmp(ptrs->getName(), service))
					{
						known = 1;
						ptrs->set();
						break;
					}
					ptrs = (BridgeService *)lists.getNext();
				}
				ptrc = (BridgeCommand *)listc.getHead();
				while(ptrc)
				{
					if(!strcmp(ptrc->getName(), service))
					{
						known = 1;
						ptrc->set();
						break;
					}
					ptrc = (BridgeCommand *)listc.getNext();
				}
				if(strstr(service,"DIS_DNS"))
					known = 1;
				if(!known)
				{					
					if(type == DimSERVICE)
					{
						if (verbose) cout << "		-> Processing service " << service << endl;
						countServices++;
						if (! testing) {     
							if(!rate)
								ptrs = new BridgeService(service, format, copyFlag);
							else
								ptrs = new BridgeService(service, format, rate, copyFlag);
							ptrs->setDebug(disconnection_debug);
							lists.add(ptrs);
						}
					}
					else if (type == DimCOMMAND)
					{						
						if (bridgeCommands == 1) { // Only bridge the commands if the environment variable DIM_BRIDGE_COMMANDS is set to 1
							if (verbose) cout << "		-> Processing service  (command) " << service << endl;
							countServices++;
							if (! testing) {
								ptrc = new BridgeCommand(service, format);
								listc.add(ptrc);
							}
						}
					}
				}

			}
		}
		if (countServers<n_servers) {
			cout << "WARNING: Scanning of the servers ended prematurely at position " << countServers << "/"  << n_servers << endl;
		}


		servedIntVal = countServers;
		if (createAdditionalPublication) {
			servint->updateService();
		}
		
		ptrs = (BridgeService *)lists.getHead();
		while(ptrs)
		{
			aux_ptrs = 0;
			if(!ptrs->find())
			{
				lists.remove(ptrs);
				aux_ptrs = ptrs;
			}
			ptrs = (BridgeService *)lists.getNext();
			if(aux_ptrs)
			{
				delete aux_ptrs;
			}
		}
		ptrc = (BridgeCommand *)listc.getHead();
		while(ptrc)
		{
			aux_ptrc = 0;
			if(!ptrc->find())
			{
				listc.remove(ptrc);
				aux_ptrc = ptrc;
			}
			ptrc = (BridgeCommand *)listc.getNext();
			if(aux_ptrc)
			{
				delete aux_ptrc;
			}
		}
		
		if (scan_debug > 0) {
			dimbridge_print_date_time();
			cout  << "  End Of Scanning." << endl;
		}
		if (countServices > 0) {
			dimbridge_print_date_time();
			cout <<   "  Bridged " << countServices <<  " new services (searching in " << countNodes << " publishers)" << endl;
		} 
		if (exit_after_first_scan == 1) {

			return 0;
		}
		sleep(refreshDelay);
	}
	return 1;
}



int checkChars() 
{
	int timeout = 0;
	char *str;
	
	DimCurrentInfo srv((char *)"DIS_DNS/SERVER_LIST", timeout, (char *)"\0");
	str = srv.getString();
	
	unsigned c;
	int result = 0;
	int start, stop = 0;
	for (int i=0; i< strlen(str); i++) {
		c= (unsigned) str[i];
		if ((c<32) || (c>127)) {
			cout << "Strange char found /" <<  c << " : " << ((char) c) << " at position " << i << endl;
			start = i-30; stop = i+30;
			if (start < 0) start = 0; 
			if (stop > strlen(str)) stop = strlen(str);
			cout << "Around " << endl;
			for (int j= start; j< stop; j++) {
				cout << str[j] ;
			}
			cout << endl;
			result++;
		}
	}


	return result;
}


// ps is a comma separated list of patterns. The function returns 1 as soon as one pattern matches
int multiple_match(const char* str, const char* ps, int case_sensitive) {
	if (strlen(ps) == 0) return 1;
	char* str2 = new char[strlen(str)+1];
	strcpy(str2, str);
	
	char* p; 
	// the pattern must be copied in new memory because strtok modifies the string
	char* p2 = new char[strlen(ps)+1];
	strcpy(p2,ps);

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
	free(p2);
	p2 = NULL;
	free(str2);
	str2=NULL;
	return result;
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