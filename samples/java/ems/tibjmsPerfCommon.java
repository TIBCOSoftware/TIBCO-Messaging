/* 
 * Copyright (c) 2009-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsPerfCommon.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

import java.util.*;
import javax.jms.*;
import javax.naming.*;
import javax.transaction.xa.*;

public class tibjmsPerfCommon
{
    protected int    connections = 1;
    protected Vector<Connection> connsVector;
    
    protected String serverUrl = "tcp://localhost:7222";
    protected String username = null;
    protected String password = null;
    protected String durableName = null;
    protected String destName = "topic.sample";
    protected String factoryName = null;

    protected boolean useTopic = true;
    protected boolean uniqueDests = false;

    protected int connIter = 0;
    protected int destIter = 0;
    protected int nameIter = 0;
    protected boolean xa = false;

    public tibjmsPerfCommon() {}

    public void createConnectionFactoryAndConnections() throws NamingException, JMSException
    {
        // lookup the connection factory
        ConnectionFactory factory = null;
        if (factoryName != null)
        {
            tibjmsUtilities.initJNDI(serverUrl, username, password);
            factory = (ConnectionFactory) tibjmsUtilities.lookup(factoryName);
        }
        else 
        {
            factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);
        }
        
        // create the connections
        connsVector = new Vector<Connection>(connections);
        for (int i=0;i<connections;i++)
        {
            Connection conn = factory.createConnection(username, password);
            conn.start();
            connsVector.add(conn);
        }
    }
        
    public void createXAConnectionFactoryAndXAConnections() throws NamingException, JMSException
    {
        // lookup the connection factory
        XAConnectionFactory factory = null;
        if (factoryName != null)
        {
            tibjmsUtilities.initJNDI(serverUrl, username, password);
            factory = (XAConnectionFactory) tibjmsUtilities.lookup(factoryName);
        }
        else 
        {
            factory = new com.tibco.tibjms.TibjmsXAConnectionFactory(serverUrl);
        }
        
        // create the connections
        connsVector = new Vector<Connection>(connections);
        for (int i=0;i<connections;i++)
        {
            XAConnection conn = factory.createXAConnection(username,password);
            conn.start();
            connsVector.add(conn);
        }
    }

    public void cleanup() throws JMSException
    {
        // close the connections
        for (int i=0;i<this.connections;i++) 
        {
            if (!xa)
            {
                Connection conn = connsVector.elementAt(i);
                conn.close();
            }
            else
            {
                XAConnection conn = (XAConnection)connsVector.elementAt(i);
                conn.close();
            }
        }
    }

    /**
     * Returns a connection, synchronized because of multiple prod/cons threads
     */
    public synchronized Connection getConnection()
    {
        Connection connection = connsVector.elementAt(connIter++);
        if (connIter == connections)
            connIter = 0;

        return connection;
    }

    /**
     * Returns a connection, synchronized because of multiple prod/cons threads
     */
    public synchronized XAConnection getXAConnection()
    {
        XAConnection connection = (XAConnection)connsVector.elementAt(connIter++);
        if (connIter == connections)
            connIter = 0;

        return connection;
    }

    /**
     * Returns a destination, synchronized because of multiple prod/cons threads
     */
    public synchronized Destination getDestination(Session s) throws JMSException
    {
        if (useTopic)
        {
            if (!uniqueDests)
                return s.createTopic(destName);
            else
                return s.createTopic(destName + "." + ++destIter);
        }
        else
        {
            if (!uniqueDests)
                return s.createQueue(destName);
            else
                return s.createQueue(destName + "." + ++destIter);
        }
    }

    /**
     * Returns a unique subscription name if durable
     * subscriptions are specified, synchronized because of multiple prod/cons threads
     */
    public synchronized String getSubscriptionName()
    {
        if (durableName != null)
            return durableName + ++nameIter;
        else
            return null;
    }

    /**
     * Returns a txn helper object for beginning/commiting transaction
     * synchronized because of multiple prod/cons threads
     */
    public synchronized tibjmsPerfTxnHelper getPerfTxnHelper(boolean xa)
    {
        return new tibjmsPerfTxnHelper(xa);
    }
    
    /**
     * Helper class for beginning/commiting transactions, maintains
     * any requried state. Each prod/cons thread needs to get an instance of 
     * this by calling getPerfTxnHelper().
     */
    public class tibjmsPerfTxnHelper
    {
        public boolean startNewXATxn = true;
        public Xid     xid = null;
        public boolean xa = false;

        public tibjmsPerfTxnHelper(boolean xa)
        { 
            this.xa = xa;
        }

        public void beginTx(XAResource xaResource) throws JMSException
        {
            if (xa && startNewXATxn)
            {
                /* create a transaction id */
                java.rmi.server.UID uid = new java.rmi.server.UID();
                this.xid = new com.tibco.tibjms.TibjmsXid(0, uid.toString(), "branch");
                
                /* start a transaction */
                try
                {
                    xaResource.start(xid, XAResource.TMNOFLAGS);
                }
                catch (XAException e)
                {
                    System.err.println("XAException: " + " errorCode=" + e.errorCode);
                    e.printStackTrace();
                    System.exit(0);
                }
                startNewXATxn = false;
            }
        }
        
        public void commitTx(XAResource xaResource, Session session) throws JMSException
        {
            if (xa)
            {
                if (xaResource != null && xid != null)
                {
                    /* end and prepare the transaction */
                    try
                    {
                        xaResource.end(xid, XAResource.TMSUCCESS);
                        xaResource.prepare(xid);
                    }
                    catch (XAException e) 
                    {
                        System.err.println("XAException: " + " errorCode=" + e.errorCode);
                        e.printStackTrace();
                        
                        Throwable cause = e.getCause();
                        if (cause != null)
                        {
                            System.err.println("cause: ");
                            cause.printStackTrace();
                        }
                        
                        try
                        { 
                            xaResource.rollback(xid); 
                        }
                        catch (XAException re) {}
                        
                        System.exit(0);
                    }
                    
                    /* commit the transaction */
                    try
                    {
                        xaResource.commit(xid, false);
                    } 
                    catch (XAException e) 
                    {
                        System.err.println("XAException: " + " errorCode=" + e.errorCode);
                        e.printStackTrace();
                        
                        Throwable cause = e.getCause();
                        if (cause != null)
                        {
                            System.err.println("cause: ");
                            cause.printStackTrace();
                        }
                        
                        System.exit(0);
                    }
                    startNewXATxn = true;
                    xid = null;
                }
            }
            else
            {
                session.commit();
            }
        }
    }
}

