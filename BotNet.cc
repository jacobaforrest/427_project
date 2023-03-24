#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"

/* Default Network Topology

//                          
//                                            AP                                  n0
//  *        *         *                      *                                    | (0-0)
//  |        |         |                      |                                    |
//  |        |         |                      |                                    | 
//  |        |         |                      |                                    | 10.1.1.0
//  |        |         |                      |                                    | 
//  |        |         |                      |                                    | 
//  |        |         |                      |           10.1.3.0                 | (1-0)         10.1.2.0
// bn,     ....,      b0,                    n3 --------------------------------- n1 --------------------------------------- n2
//       10.1.4.0                      (3-1)    (3-0)                       (1-2)    (1-1)                             (2-0)
//                                  
//      Wifi                                                        point-to-point
//                                   
// 
               
*/

#define SINK // UDP Sink server


//#define ECHO // UDP Echo server


#define FIFTH // TCP w Tracing (adapted from fifth.cc tutorial)
/* 
  ./waf --run BotNet.cc > cwnd.dat 2>&1
  $ gnuplot
  gnuplot> set terminal png size 640,480
  gnuplot> set output "cwnd.png"
  gnuplot> plot "cwnd.dat" using 1:2 title 'Congestion Window' with linespoints
  gnuplot> exit

*/
//#define REAL_USER // UDP source from n0


#define NUM_BOTS 15
#define SIM_TIME 20
#define NUM_PACKETS 100000
#define PORT 25565
#define PORT2 25566

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BotNet");


#ifdef FIFTH
  class MyApp : public Application 
  {
  public:

    MyApp ();
    virtual ~MyApp();

    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

  private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleTx (void);
    void SendPacket (void);

    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_nPackets;
    DataRate        m_dataRate;
    EventId         m_sendEvent;
    bool            m_running;
    uint32_t        m_packetsSent;
  };

  MyApp::MyApp ()
    : m_socket (0), 
      m_peer (), 
      m_packetSize (0), 
      m_nPackets (0), 
      m_dataRate (0), 
      m_sendEvent (), 
      m_running (false), 
      m_packetsSent (0)
  {
  }

  MyApp::~MyApp()
  {
    m_socket = 0;
  }

  void
  MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
  {
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
  }

  void
  MyApp::StartApplication (void)
  {
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind ();
    m_socket->Connect (m_peer);
    SendPacket ();
  }

  void 
  MyApp::StopApplication (void)
  {
    m_running = false;

    if (m_sendEvent.IsRunning ())
      {
        Simulator::Cancel (m_sendEvent);
      }

    if (m_socket)
      {
        m_socket->Close ();
      }
  }

  void 
  MyApp::SendPacket (void)
  {
    Ptr<Packet> packet = Create<Packet> (m_packetSize);
    m_socket->Send (packet);

    if (++m_packetsSent < m_nPackets)
      {
        ScheduleTx ();
      }
  }

  void 
  MyApp::ScheduleTx (void)
  {
    if (m_running)
      {
        Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
        m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
      }
  }

  static void
  CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
  {
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  }

  /*static void
  RxDrop (Ptr<const Packet> p)
  {
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  }*/
#endif

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);

  NodeContainer Nodes;
  Nodes.Create (4);


  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices_n1_n0;
  devices_n1_n0 = pointToPoint.Install (Nodes.Get(1), Nodes.Get(0));
      
//   n0
//    |
//    |
//   n1
//
//   point-to-point

              
  NetDeviceContainer devices_n1_n2;
  devices_n1_n2 = pointToPoint.Install (Nodes.Get(1), Nodes.Get(2));

//   n0
//    |
//    |
//   n1 ---------------- n2
//
//   point-to-point


  NetDeviceContainer devices_n1_n3;
  devices_n1_n3 = pointToPoint.Install (Nodes.Get(1), Nodes.Get(3));

//                       n0
//                        |
//                        |
//   n3 ---------------- n1 ---------------- n2
//
//                 point-to-point

  NodeContainer bot_wifiStaNodes;
  bot_wifiStaNodes.Create (NUM_BOTS);
  NodeContainer wifiApNode = Nodes.Get (3);


//                      AP                 n0
//  *    *    *         *                   |
//  |    |    |         |                   |
//  |    |    |         |                   |
// bn, ...., b0,       n3 ---------------- n1 ---------------- n2
//
//      Wifi                      point-to-point


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer bot_staDevices;
  bot_staDevices = wifi.Install (phy, mac, bot_wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  /*
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (bot_wifiStaNodes);
  */

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (bot_wifiStaNodes);
  mobility.Install (wifiApNode);


  InternetStackHelper stack;
  stack.Install (Nodes);
  stack.Install (bot_wifiStaNodes);


  Ipv4AddressHelper address;

  // Assign IP to nodes
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_n1_n0;
  interfaces_n1_n0 = address.Assign (devices_n1_n0);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_n1_n2;
  interfaces_n1_n2 = address.Assign (devices_n1_n2);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_n1_n3;
  interfaces_n1_n3 = address.Assign (devices_n1_n3);

  // Assign IP to bots
  address.SetBase ("10.1.4.0", "255.255.255.0");
  address.Assign (apDevices);
  address.Assign (bot_staDevices);

  #ifdef SINK
      // UDPSink
      PacketSinkHelper UDPsink("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), PORT)));
      ApplicationContainer UDPSinkApp = UDPsink.Install(Nodes.Get(2));
      UDPSinkApp.Start(Seconds(1.0));
      UDPSinkApp.Stop(Seconds(SIM_TIME));

      // Bot onoff Application Behaviour
      OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(interfaces_n1_n2.GetAddress(1), PORT)));
      onoff.SetConstantRate(DataRate("10Gbps"));
      onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=30]"));
      onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
      onoff.SetAttribute("PacketSize", UintegerValue(1024));

      // Install application in all bots
      ApplicationContainer onOffApp[NUM_BOTS];
      for (int i = 0; i < NUM_BOTS; i++)
      {
          onOffApp[i] = onoff.Install(bot_wifiStaNodes.Get(i));
          onOffApp[i].Start(Seconds(2.0));
          onOffApp[i].Stop(Seconds(SIM_TIME));
      }
  #endif

  #ifdef ECHO
   	// UDPecho
    UdpEchoServerHelper echoServer (PORT);
    ApplicationContainer serverApp = echoServer.Install(Nodes.Get(2));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(SIM_TIME));
    
    // Bot echoClient Application Behaviour
    UdpEchoClientHelper echoClient(interfaces_n1_n2.GetAddress(1), PORT);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (NUM_PACKETS));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.0001)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    //Install application in all bots
    ApplicationContainer clientApp[NUM_BOTS];
    for (int i = 0; i < NUM_BOTS; i++)
    {	
        clientApp[i] = echoClient.Install(bot_wifiStaNodes.Get(i));	
        clientApp[i].Start(Seconds(2.0));
        clientApp[i].Stop(Seconds(SIM_TIME));	
    }
  #endif

  #ifdef FIFTH
    // TCP connection between node n0 and node n2
    Address sinkAddress (InetSocketAddress(interfaces_n1_n2.GetAddress (1), PORT2));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",
      InetSocketAddress (Ipv4Address::GetAny (), PORT2));
    ApplicationContainer sinkApps = packetSinkHelper.Install (Nodes.Get (2));
    sinkApps.Start (Seconds (0));
    sinkApps.Stop (Seconds (SIM_TIME));

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (Nodes.Get (0), TcpSocketFactory::GetTypeId ());
    ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3TcpSocket, sinkAddress, 1040, 100000, DataRate ("5Mbps"));
    Nodes.Get (0)->AddApplication (app);
    app->SetStartTime (Seconds (1));
    app->SetStopTime (Seconds (SIM_TIME));

    //devices_n1_n2.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
  #endif

  #ifdef REAL_USER
  		
    // Read user echoClient Application Behaviour	
    UdpEchoClientHelper echoClient_real(interfaces_n1_n2.GetAddress(1), PORT);	
    echoClient_real.SetAttribute ("MaxPackets", UintegerValue (NUM_PACKETS));	
    echoClient_real.SetAttribute ("Interval", TimeValue (Seconds (2.0)));	
    echoClient_real.SetAttribute ("PacketSize", UintegerValue (1024));	

    //Install application in node n0	
    ApplicationContainer clientApp_real = echoClient_real.Install(Nodes.Get(0));	
    clientApp_real.Start(Seconds(2.0));	
    clientApp_real.Stop(Seconds(SIM_TIME));	

  #endif
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("BotNet.tr"));
  pointToPoint.EnablePcapAll("BotNet");
  phy.EnablePcap ("BotNet", apDevices.Get (0));

  Simulator::Stop (Seconds (SIM_TIME));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}