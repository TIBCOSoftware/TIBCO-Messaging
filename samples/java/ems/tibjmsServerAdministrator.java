/* 
 * Copyright (c) 2005-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsServerAdministrator.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

import java.util.*;
import javax.jms.Topic;
import javax.jms.Queue;
import javax.jms.Destination;
import com.tibco.tibjms.admin.*;

/*
 * The tibjmsServerAdministrator class is used by the tibjmsPerfMaster class
 * to change various settings in the EMS server.
 */
public class tibjmsServerAdministrator
{
    TibjmsAdmin[] _admin = null;
    String queue = null;
    String topic = null;
    String[] topics = null;
    String[] queues = null;
    String topicPattern = null;
    String queuePattern = null;

    /**
     * Creates an admin client on the specified server
     * and then walks any routes to other servers creating
     * admin clients on the discovered servers as well.
     * 
     * @param serverUrl server URL on which to connect
     * @param userName the administrator name
     * @param password the administrator password
     */
    public tibjmsServerAdministrator(
        String  serverUrl,
        String  userName,
        String  password)
    {
        Map<String,TibjmsAdmin> map = new HashMap<String,TibjmsAdmin>();
        addAdmin(serverUrl, userName, password, map);
        _admin = new TibjmsAdmin[map.size()];
        map.values().toArray(_admin);
    }

    private void addAdmin(String serverUrl, String userName, String password, Map<String,TibjmsAdmin> map)
    {
        try
        {
            System.err.println("connecting as "+userName+" to "+serverUrl);
            TibjmsAdmin admin = new TibjmsAdmin(serverUrl,userName,password);
            ServerInfo si = admin.getInfo();
            
            /* enable statistics */
            si.setStatisticsEnabled(true);
            admin.updateServer(si);
            
            if (!map.containsKey(si.getServerName()))
            {
                map.put(si.getServerName(), admin);
                RouteInfo[] ri = admin.getRoutes();
                for (int i = 0; i < ri.length; i++)
                {
                    if (!map.containsKey(ri[i].getName()) && ri[i].isConnected())
                        addAdmin(ri[i].getURL(), userName, password, map);
                }
            }
            else
            {
                admin.close();
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }        
    }
    
    /**
     * Create a topic on all servers.
     * 
     * @param name topic name
     * @param failsafe failsafe setting
     * @param flowControl flow control setting
     * @param channel multicast channel name
     */
    public void createTopic(String name, boolean failsafe, long flowControl,
                            String channel, String storeName)
    {
        try
        {
            TopicInfo ti = new TopicInfo(name);
            ti.setGlobal(_admin.length > 1);

            if (failsafe)
                ti.setFailsafe(failsafe);

            if (storeName != null)
                ti.setStore(storeName);

            ti.setFlowControlMaxBytes(flowControl);
            ti.setChannel(channel);

            System.err.println("creating topic "+name);
            for (int i = 0; i < _admin.length; i++)
            {
                _admin[i].destroyTopic(name);
                _admin[i].createTopic(ti);
            }
            topic = name;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    public void createTopics(String name, long flowControl, String storeName, 
                             boolean uniqueStores, int numTopics)
    {
        try
        {
            int j=0;
            topics = new String[numTopics];
            String topicName = null;
            topicPattern = name + ".>";

            for (int i=0; i<numTopics; i++)
            {
                int suffix = ++j;
                topicName = name + "." + suffix;
                topics[i] = name;
                TopicInfo ti = new TopicInfo(topicName);

                if (uniqueStores)
                    ti.setStore(storeName + suffix);

                ti.setFlowControlMaxBytes(flowControl);
            
                System.err.println("creating topic "+ topicName);
                for (int l = 0; l < _admin.length; l++)
                {
                    _admin[l].destroyTopic(topicName);
                    _admin[l].createTopic(ti);
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }

    }

    public void createQueues(String name, long flowControl, int prefetch, String storeName,
                             boolean uniqueStores, int numQueues)
    {
        try
        {
            int j=0;
            queues = new String[numQueues];
            String queueName = null;
            queuePattern = name + ".>";

            for (int i=0; i<numQueues; i++)
            {
                int suffix = ++j;
                queueName = name + "." + suffix;
                queues[i] = name;
                QueueInfo qi = new QueueInfo(queueName);

                if (uniqueStores)
                    qi.setStore(storeName + suffix);

                qi.setFlowControlMaxBytes(flowControl);
                qi.setPrefetch(prefetch);
            
                System.err.println("creating queue "+ queueName);
                for (int l = 0; l < _admin.length; l++)
                {
                    _admin[l].destroyQueue(queueName);
                    _admin[l].createQueue(qi);
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    /**
     * Create a queue on all servers.
     * 
     * @param name queue name
     * @param server name of queue's home server
     * @param failsafe failsafe setting
     * @param flowControl flow control setting
     * @param prefetch prefetch setting
     */
    public void createQueue(String name, String server, boolean failsafe,
        long flowControl, int prefetch, String storeName)
    {
        String fullName = name;

        if (server != null)
            fullName = name + '@' + server;
        
        try
        {
            QueueInfo qi = new QueueInfo(fullName);
            qi.setGlobal(_admin.length > 1);

            if (failsafe)
                qi.setFailsafe(failsafe);

            if (storeName != null)
               qi.setStore(storeName);

            qi.setFlowControlMaxBytes(flowControl);
            qi.setPrefetch(prefetch);

            System.err.println("creating queue "+fullName);
            for (int i = 0; i < _admin.length; i++)
            {
                _admin[i].destroyQueue(name);
                _admin[i].createQueue(qi);
            }
            queue = name;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    /**
     * Return the number of receivers from all servers
     * for a particular destination.
     * 
     * @param dest the destination
     * @return the number of receivers for the destination
     */
    public int getNumberOfReceivers(Destination dest)
    {
        int numReceivers = 0;

        try
        {
            for (int i = 0; i < _admin.length; i++)
            {
                DestinationInfo info;

                if (dest instanceof Topic)
                    info = _admin[i].getTopic(((Topic)dest).getTopicName());
                else 
                    info = _admin[i].getQueue(((Queue)dest).getQueueName());
                
                ConsumerInfo[] ci = _admin[i].getConsumersStatistics(null, null, info);
                if (ci != null)
                    numReceivers += ci.length;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }

        return numReceivers;
    }

    /**
     * Check if flow control is enabled for all servers.
     * 
     * @return true iff flow control is enabled on all servers
     */
    public boolean isFlowControlEnabled()
    {
        boolean flowControlEnabled = true;
        
        try
        {
            for (int i = 0; i < _admin.length; i++)
            {
                ServerInfo si = _admin[i].getInfo();
                if (!si.isFlowControlEnabled())
                    flowControlEnabled = false;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }

        return flowControlEnabled;
    }
    
    /**
     * Remove the destination from all servers.
    */
    public void cleanup(String controlTopic)
    {
        try
        {
            for (int i = 0; i < _admin.length; i++)
            {
                if (topic != null)
                {
                    TopicInfo[] info = _admin[i].getTopics(topic);
                    for (int j = 0; j < info.length; j++)
                    {
                        if (!controlTopic.equals(info[j].getName()))
                            _admin[i].destroyTopic(info[j].getName());
                    }
                }
                if (queue != null)
                {
                    QueueInfo[] info = _admin[i].getQueues(queue);
                    for (int j = 0; j < info.length; j++)
                    {
                        _admin[i].destroyQueue(info[j].getName());
                    }
                }
                if (topics != null)
                {
                    TopicInfo[] info = _admin[i].getTopics(topicPattern);
                    for (int j = 0; j < info.length; j++)
                    {
                        if (!controlTopic.equals(info[j].getName()))
                            _admin[i].destroyTopic(info[j].getName());
                    }
                }
                if (queues != null)
                {
                    QueueInfo[] info = _admin[i].getQueues(queuePattern);
                    for (int j = 0; j < info.length; j++)
                    {
                        _admin[i].destroyQueue(info[j].getName());
                    }
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }
}
