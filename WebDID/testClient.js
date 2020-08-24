 
Ext.require(['*']);

Ext.onReady(function(){


    var randomNumber = Math.floor(Math.random()*10000001);

    var HTTPPacket = new XMLHttpRequest();
    HTTPPacket.onreadystatechange = process;
    var requestNumber = 0;
    var forceUpdate = 1;


    function getService(name)
    {
	var name1 = name.replace(/\?/g,"%3F");
	name1 = name1.replace(/\&/g,"%26");
	requestNumber = requestNumber + 1;
//	HTTPPacket.open( "GET", "/didServiceData.json/src?dimservice="+name1+"&id=src&reqNr="+requestNumber+"&reqId="+randomNumber+"&force="+forceUpdate, true ); 
	HTTPPacket.open( "GET", "http://pclhcb161.cern.ch:2500/didServiceData.json/src?dimservice="+name1+"&id=src&reqNr="+requestNumber+"&reqId="+randomNumber+"&force="+forceUpdate, true ); 
	HTTPPacket.send( null );
    }
    function process() 
    {
console.log("received answer", HTTPPacket.readyState, HTTPPacket.responseText);
	if ( HTTPPacket.readyState != 4 )
		return; 
console.log("received answer", HTTPPacket.responseText);
    } 

    getService("SMI/LHCB/LHCB");
    setTimeout(function(){
        console.log("timeout");
    }, 100000);
});
