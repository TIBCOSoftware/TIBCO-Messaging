/* 
 * Copyright (c) 2018 TIBCO Software Inc. 
 * All Rights Reserved. Confidential & Proprietary.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: MqttMsgSubscriber.java $
 * 
 */

/*
 * This is a sample MQTT subscriber that uses the Eclipse Paho Java Client.
 *
 * Usage:  java MqttMsgSubscriber [options]
 *
 *    where options are:
 *
 *   -broker    <broker-url>  Broker URL.
 *                            If not specified this sample assumes a
 *                            brokerUrl of "tcp://localhost:1883"
 *   -user      <user-name>   User name. Default is null.
 *   -password  <password>    User password. Default is null.
 *   -topic     <topic-name>  Topic name. Default value is "sample"
 *   -cid       <client-id>   Use persistent connection with client identifier
 *   -qos       <QoS>         Quality of service 0,1,2 default 1
 *   -count     <n>           Number of messages to receive, default 0 - endless
 *
 *
 */

import org.eclipse.paho.client.mqttv3.*;

public class MqttMsgSubscriber implements MqttCallbackExtended
{
    /*-----------------------------------------------------------------------
     * Parameters
     *----------------------------------------------------------------------*/

    String          brokerUrl   = "tcp://localhost:1883";
    String          userName    = null;
    String          password    = null;
    String          topicName   = "sample";
    int		    qos	 	= 1;
    String          clientId    = null;
    int             count       = 0;
    boolean cleanSession 	= true;	// Only use durable subscriptions if clientId set

   /*-----------------------------------------------------------------------
    * Variables
    *----------------------------------------------------------------------*/
    MqttClient      client = null;
    int             received    = 0;

   /*-----------------------------------------------------------------------
    * Subscriber
    *----------------------------------------------------------------------*/
    public MqttMsgSubscriber(String[] args)
    {
        parseArgs(args);

        // print parameters
        System.out.println("\n------------------------------------------------------------------------");
        System.out.println("MqttMsgSubscriber Sample (press Enter to exit)");
        System.out.println("------------------------------------------------------------------------");
        System.out.println("Broker....................... "+((brokerUrl != null)?brokerUrl:"localhost"));
        System.out.println("User......................... "+((userName != null)?userName:"(null)"));
        System.out.println("Topic........................ "+topicName);
        System.out.println("QoS.......................... "+qos);
        System.out.println("Count........................ "+count);
	if (clientId != null)
	    System.out.println("Durable Client Id............ "+clientId);
        System.out.println("------------------------------------------------------------------------\n");

        try 
        {
            // set the connection options
	    MqttConnectOptions 	conOpt = new MqttConnectOptions();

	    conOpt.setAutomaticReconnect(true);
	    conOpt.setCleanSession(cleanSession);

	    if (userName != null)
	    {
		conOpt.setUserName(userName);
	    }

	    if (password != null )
	    {
		conOpt.setPassword(password.toCharArray());
	    }

	    if (clientId == null)
	    {
		clientId = MqttClient.generateClientId();
	    }

            // create the client
            client = new MqttClient(brokerUrl, clientId);

	    client.setCallback(this);

            // open the connection
	    client.connect(conOpt);

	    // create subscriber
	    client.subscribe(topicName, qos);

	    // Continue waiting for messages until count is reached
	    synchronized (this) {
		try { this.wait(); }
		catch (InterruptedException e) { }
	    }

            // close the connection
            client.disconnect();
	    System.exit(0);
        } 
        catch (MqttException e) 
        {
            e.printStackTrace();
            System.exit(1);
        }
    }


   /*-----------------------------------------------------------------------
    * Callback Methods
    *----------------------------------------------------------------------*/

    public void connectComplete(boolean reconnect, java.lang.String serverURI)
    {
	if (reconnect)
	    System.out.println("Reconnected successfully to " + serverURI);
	else
	    System.out.println("Connected successfully to " + serverURI);
    }

    public void connectionLost(Throwable cause)
    {
	// Called when the connection to the broker has been lost.
	System.out.println("Connection to " + brokerUrl + " lost! " + cause);
    }

    public void deliveryComplete(IMqttDeliveryToken token)
    {
	// Called when delivery for a message has been completed, and all acknowledgments have been received.
	// For QoS 0 messages it is called once the message has been handed to the network for delivery.
	// For QoS 1 it is called when PUBACK is received 
	// For QoS 2 when PUBCOMP is received. 
        
	// Not used in this sample
    }

    public void messageArrived(String topic, MqttMessage message) throws MqttException 
    {
	// Called when a message arrives from the broker that matches any subscription
	// An acknowledgment is not sent back to the broker until this method returns
	System.out.println("Received "+(message.isDuplicate()?"duplicate ":"")+(message.isRetained()?"retained ":"")+"message: id: " + message.getId()+" message: "+ message.toString());
	//System.out.println("Received message: id: " + message.getId()+" message: "+ message.toString());
	received++;
	if (count > 0 && received >= count)
	{
	    synchronized (this) {
		this.notify();
	    }
	}
    }

   /*-----------------------------------------------------------------------
    * usage
    *----------------------------------------------------------------------*/
    void usage()
    {
        System.out.println("\nUsage: MqttMsgSubscriber [options] ");
        System.out.println("");
        System.out.println("   where options are:");
        System.out.println("");
        System.out.println("   -broker   <broker URL>  - MQTT broker URL, default is local broker");
        System.out.println("   -user     <user name>   - user name, default is null");
        System.out.println("   -password <password>    - password, default is null");
        System.out.println("   -topic    <topic-name>  - topic name, default is \"sample\"");
        System.out.println("   -cid      <client-id>   - Use persistent connection with client identifier");
        System.out.println("   -qos      <QoS>         - quality of service 0,1,2 default 1");
        System.out.println("   -count    <n>           - Number of messages to receive, default 0 - endless");
        System.exit(0);
    }

   /*-----------------------------------------------------------------------
    * parseArgs
    *----------------------------------------------------------------------*/
    void parseArgs(String[] args)
    {
        int i=0;

        while (i < args.length)
        {
            if (args[i].compareTo("-broker")==0)
            {
                if ((i+1) >= args.length) usage();
                brokerUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-topic")==0)
            {
                if ((i+1) >= args.length) usage();
                topicName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-qos")==0)
            {
                if ((i+1) >= args.length) usage();
		if (args[i+1].compareTo("0")==0)
                    qos = 0;
                else if (args[i+1].compareTo("1")==0)
                    qos = 1;
                else if (args[i+1].compareTo("2")==0)
                    qos = 2;
                else
                {
                    System.out.println("Unrecognized -qos: "+args[i+1]);
                    usage();
                }
                i += 2;
            }
	    else
            if (args[i].compareTo("-user")==0)
            {
                if ((i+1) >= args.length) usage();
                userName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-password")==0)
            {
                if ((i+1) >= args.length) usage();
                password = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-cid")==0)
            {
                if ((i+1) >= args.length) usage();
                clientId = args[i+1];
		cleanSession = false;
                i += 2;
            }
            else if (args[i].compareTo("-count")==0)
            {
                if ((i+1) >= args.length) usage();
                try 
                {
                    count = Integer.parseInt(args[i+1]);
                }
                catch (NumberFormatException e)
                {
                    System.out.println("Error: invalid value of -count parameter");
                    usage();
                }
                i += 2;
            }
            else
            if (args[i].compareTo("-help")==0)
            {
                usage();
            }
            else
            {
                System.out.println("Error: Unrecognized parameter: "+args[i]);
                usage();
            }
        }
    }

   /*-----------------------------------------------------------------------
    * main
    *----------------------------------------------------------------------*/
    public static void main(String[] args)
    {
        new MqttMsgSubscriber(args);
    }
}
