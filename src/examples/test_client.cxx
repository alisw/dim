#include <iostream>
#include <dic.hxx>
using namespace std;

class ErrorHandler : public DimErrorHandler
{
	void errorHandler(int severity, int code, char *msg)
	{
		int index = 0;
		char **services;
		if(code){}
		cout << severity << " " << msg << endl;
		services = DimClient::getServerServices();
		cout<< "from "<< DimClient::getServerName() << " services:" << endl;
		while(services[index])
		{
			cout << services[index] << endl;
			index++;
		}
	}
public:
	ErrorHandler() {DimClient::addErrorHandler(this);}
};

class StrService : public DimInfo
{

	void infoHandler()
	{
		int index = 0;
		char **services;
		char *format;
//		cout << "Dns Node = " << DimClient::getDnsNode() << endl;
		format = getFormat();
		cout << "Received STRVAL : " << getString() << " format = " << format << endl;
		services = DimClient::getServerServices();
		cout<< "from "<< DimClient::getServerName() << " services:" << endl;
		while(services[index])
		{
			cout << services[index] << endl;
			index++;
		}
		int inCallback = DimClient::inCallback();
		cout << "infoHandler: In callback "<< inCallback << endl; 
	}
public :
	StrService() : DimInfo("TEST/STRVAL",(char *)"not available") {};
	StrService(dim_long dnsid) : DimInfo(dnsid, "TEST/STRVAL", (char *)"not available") {};
};

int main()
{
		
	ErrorHandler errHandler;
	StrService servstr, *servstr1;
	char *server, *ptr, *ptr1;
	DimBrowser br, *br1;
	int type, n, pid;

//	DimClient::addErrorHandler(errHandler);
	
	n = br.getServices("*");
	cout << "found " << n << " services" << endl; 
	
	while((type = br.getNextService(ptr, ptr1))!= 0)
	{
		cout << "type = " << type << " - " << ptr << " " << ptr1 << endl;
	}
	
	br.getServers();
	while(br.getNextServer(server, ptr1, pid))
	{
		cout << server << " @ " << ptr1 << ", pid = " << pid << endl;
	}

	br.getServerClients("DIS_DNS");
	while(br.getNextServerClient(ptr, ptr1))
	{
		cout << ptr << " @ " << ptr1 << endl;
	}

	DimInfo servint("TEST/INTVAL",-1); 

	dim_long dnsid;
	dnsid = DimClient::addDns("lxplus613.cern.ch", 0);
	
	br1 = new DimBrowser(dnsid);
	n = br1->getServices("*");
	cout << "found " << n << " services" << endl;

	while ((type = br1->getNextService(ptr, ptr1)) != 0)
	{
		cout << "type = " << type << " - " << ptr << " " << ptr1 << endl;
	}

	br1->getServers();
	while (br1->getNextServer(server, ptr1, pid))
	{
		cout << server << " @ " << ptr1 << ", pid = " << pid << endl;
	}

	br1->getServerClients("DIS_DNS");
	while (br1->getNextServerClient(ptr, ptr1))
	{
		cout << ptr << " @ " << ptr1 << endl;
	}

	DimInfo servint1(dnsid, "TEST/INTVAL", -1);
	servstr1 = new StrService(dnsid);
	if (servstr1) {}
	while(1)
	{
		sleep(10);
		
		cout << "Current INTVAL : " << servint.getInt() << endl;
		cout << "Current INTVAL(1) : " << servint1.getInt() << endl;
		DimClient::sendCommand("TEST/CMND","UPDATE_STRVAL");
		DimClient::sendCommand(dnsid, "TEST/CMND", "UPDATE_STRVAL");
		//		int inCallback = DimClient::inCallback();
//		cout << "main: In callback "<< inCallback << endl;

		DimClient::addErrorHandler(0);
	}
	return 0;
}
