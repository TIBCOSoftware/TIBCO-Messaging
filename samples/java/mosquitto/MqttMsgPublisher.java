/* 
 * Copyright (c) 2018 TIBCO Software Inc. 
 * All Rights Reserved. Confidential & Proprietary.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: MqttMsgPublisher.java $
 * 
 */

/*
 * This is a sample MQTT publisher that uses the Eclipse Paho Java Client.
 *
 * Usage:  java MqttMsgPublisher  [options] <message-text>
 *
 *  where options are:
 *
 *   -broker    <broker-url>  Broker URL.
 *                            If not specified this sample assumes a
 *                            brokerUrl of "tcp://localhost:1883"
 *   -user      <user-name>   User name. Default is null.
 *   -password  <password>    User password. Default is null.
 *   -topic     <topic-name>  Topic name. Default value is "sample"
 *   -cid       <client-id>   Use persistent connection with client identifier
 *   -qos       <QoS>         Quality of service 0,1,2 default 1
 *   -retained                The broker should retain the message
 *   -async                   Send asynchronously, default is false
 *   -count     <n>           Number of messages to send, default 1
 *   -interval  <millis>      Interval between each send in millis, default 1000
 *
 */


import org.eclipse.paho.client.mqttv3.*;

public class MqttMsgPublisher implements MqttCallbackExtended
{
   /*-----------------------------------------------------------------------
    * Parameters
    *----------------------------------------------------------------------*/

    String          brokerUrl    = "tcp://localhost:1883";
    String          userName     = null;
    String          password     = null;
    String          clientId     = null;
    String          topicName    = "sample";
    String          message      = "Test Message";
    int             count        = 1;
    int             interval     = 1000;
    int		    qos		 = 1;
    boolean         retained     = false;
    boolean cleanSession 	 = true;	// Do not maintain state across restarts unless clientId set
    boolean         useAsync     = false;

   /*-----------------------------------------------------------------------
    * Variables
    *----------------------------------------------------------------------*/
    MqttClient  client = null;
        
   /*-----------------------------------------------------------------------
    * Publisher
    *----------------------------------------------------------------------*/
    public MqttMsgPublisher(String[] args)
    {
        parseArgs(args);

        // print parameters
        System.out.println("\n------------------------------------------------------------------------");
        System.out.println("MqttMsgPublisher Sample");
        System.out.println("------------------------------------------------------------------------");
        System.out.println("Broker....................... "+((brokerUrl != null)?brokerUrl:"localhost"));
        System.out.println("User......................... "+((userName != null)?userName:"(null)"));
        System.out.println("Topic........................ "+topicName);
        System.out.println("QoS.......................... "+qos);
        System.out.println("Retained..................... "+retained);
        System.out.println("Count........................ "+count);
	if (clientId != null)
	    System.out.println("Client Id.................... "+clientId);
        System.out.println("------------------------------------------------------------------------\n");

        try 
        {
            MqttMessage msg;

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
             
	    // create topic for async publish
	    MqttTopic 	topic = null;
	    if (useAsync)
	    {
		topic = client.getTopic(topicName);
	    }

            // publish messages
	    MqttDeliveryToken token = null;
            for (int i = 0; i<count; i++)
            {
                // create message
                msg = new MqttMessage(message.getBytes());
		msg.setQos(qos);
		msg.setRetained(retained);

                // publish message
		if (topic != null)
		{
		    // async (non-blocking) publish
		    token = topic.publish(msg);
		}
		else
		{
		    // sync (blocking) publish
		    client.publish(topicName, msg);
		}
		synchronized (this)
		{
		    if (token != null)
			System.out.println("Published message: id: " + token.getMessageId()+" payload: "+ msg.toString());
		    else
			System.out.println("Published message: "+ msg.toString());
		}

                try { Thread.sleep(interval); }
                catch (InterruptedException e) { }
            }

	    // for async wait for last message to be delivered 
	    if (token != null)
		token.waitForCompletion();

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
	synchronized (this)
	{
	    System.out.println("Delivery complete for message id: "+token.getMessageId());
	}
    }

    public void messageArrived(String topic, MqttMessage message) throws MqttException 
    {
	// Called when a message arrives from the broker that matches any subscription
	// An acknowledgment is not sent back to the broker until this method returns

	// Not used in this sample
    }

   /*-----------------------------------------------------------------------
    * usage
    *----------------------------------------------------------------------*/
    private void usage()
    {
        System.out.println("\nUsage: java MqttMsgPublisher [options] <message-text>");
        System.out.println("\n");
        System.out.println("   where options are:");
        System.out.println("");
        System.out.println("   -broker   <broker URL>  - MQTT broker URL, default is local broker");
        System.out.println("   -user     <user name>   - user name, default is null");
        System.out.println("   -password <password>    - password, default is null");
        System.out.println("   -topic    <topic-name>  - topic name, default is \"sample\"");
        System.out.println("   -cid      <client-id>   - Use persistent connection with client identifier");
        System.out.println("   -qos      <QoS>         - quality of service 0,1,2 default 1");
        System.out.println("   -retained               - the broker should retain the message");
        System.out.println("   -async                  - send asynchronously, default is false");
        System.out.println("   -count     <n>          - Number of messages to send, default 1");
        System.out.println("   -interval  <millis>     - Interval between each send in millis, default 1000");
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
                    System.out.println("Error: Unrecognized -qos: "+args[i+1]);
                    usage();
                }
                i += 2;
            }
            else
            if (args[i].compareTo("-async")==0)
            {
                i += 1;
                useAsync = true;
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
	    else
            if (args[i].compareTo("-retained")==0)
            {
                retained = true;
                i += 1;
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
            else if (args[i].compareTo("-interval")==0)
            {
                if ((i+1) >= args.length) usage();
                try 
                {
                    interval = Integer.parseInt(args[i+1]);
                }
                catch (NumberFormatException e)
                {
                    System.out.println("Error: invalid value of -interval parameter");
                    usage();
                }
                i += 2;
            }
            else
            if (args[i].compareTo("-help")==0)
            {
                usage();
            }
            else if ((i+1) == args.length)
            {
                message = args[i];
		i++;
	    }
        }
    }

   /*-----------------------------------------------------------------------
    * main
    *----------------------------------------------------------------------*/
    public static void main(String[] args)
    {
        MqttMsgPublisher t = new MqttMsgPublisher(args);
    }
}

