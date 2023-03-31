#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/trace-helper.h"

/* Default Network Topology

//             WiFi                                                 Point-to-Point             
//                                            AP                                  n0
//  *        *         *                      *                                    | (0-0)
//  |        |         |                      |                                    | ↓
//  |        |         |                      |                                    | 
//  |        |         |                      |                                    | 10.1.1.0
//  |        |         |                      |                                    | 
//  |        |         |                      |                                    | ↑
//  |        |         |                      |           10.1.3.0                 | (1-0)         10.1.2.0
// bn,     ....,      b0,                    n3 --------------------------------- n1 --------------------------------------- n2
//       10.2.1.0                    ← (3-1)    (3-0) →                   ← (1-2)  |     (1-1) →                     ← (2-0)
//                                                                                 | (1-3)
//                                                                                 | ↓
//                                                                                 |
//                                                                                 | 10.1.4.0
//                                                                                 |
//                                                                                 | ↑
//                                          10.3.1.0                      ← (4-1)  | (4-0)
//                                  bn+m,      ...,      bn+1,                    n4
//                                    |         |          |                       |
//                                    |         |          |                       |
//                                    |         |          |                       |
//                                    |         |          |                       |
//                                    *         *          *                       *
//                                                                                 AP
//                                             WiFi     
//

*/

// Resources:
// third.cc
// fifth.cc
// https://infosecwriteups.com/ddos-simulation-in-ns-3-c-12f031a7b38c

/* 
  ./waf --run BotNet.cc
  gnuplot plotcongestion.p
*/

//#define SINK // UDP Sink server
#define ECHO // UDP Echo server
#define ERROR_MODEL


#define NUM_BOTS_n 0
#define NUM_BOTS_m 0
#define SIM_TIME 120
#define NUM_PACKETS 25000
#define PORT 25565
#define PORT2 25566

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BotNet");



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
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
}

static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << rtt);
  *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newRtt << std::endl;
}

/*
static void
RxDrop (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}
*/


int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("tcp_cwnd.dat");
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("tcp_rtt.dat");

  NodeContainer Nodes;
  Nodes.Create (5);


  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  //pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("100p"));

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
  #ifdef ERROR_MODEL
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
    devices_n1_n2.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  #endif

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

  NetDeviceContainer devices_n1_n4;
  devices_n1_n4 = pointToPoint.Install (Nodes.Get(1), Nodes.Get(4));

//                       n0
//                        |
//                        |
//   n3 ---------------- n1 ---------------- n2
//                        |
//                        |
//                       n4
//
//                 point-to-point

  NodeContainer botn_wifiStaNodes;
  botn_wifiStaNodes.Create (NUM_BOTS_n);
  NodeContainer wifiApNode_n3 = Nodes.Get (3);


//                      AP                 n0
//  *    *    *         *                   |
//  |    |    |         |                   |
//  |    |    |         |                   |
// bn, ...., b0,       n3 ---------------- n1 ---------------- n2
//                                          |
//                                          |
//                                         n4
//
//      Wifi                      point-to-point

  NodeContainer botm_wifiStaNodes;
  botm_wifiStaNodes.Create (NUM_BOTS_m);
  NodeContainer wifiApNode_n4 = Nodes.Get (4);


//     WiFi                          Point-to-Point
//                      AP                 n0
//  *    *    *         *                   |
//  |    |    |         |                   |
//  |    |    |         |                   |
// bn, ...., b0,       n3 ---------------- n1 ---------------- n2
//                                          |
//                                          |
//                bn+m, ..., bn+1,         n4
//                 |     |    |             |
//                 |     |    |             |
//                 *     *    *             *
//                                          AP
//                     Wifi


  YansWifiChannelHelper channel_n = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy_n;
  phy_n.SetChannel (channel_n.Create ());
  YansWifiChannelHelper channel_m = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy_m;
  phy_m.SetChannel (channel_m.Create ());

  WifiHelper wifi_n;
  wifi_n.SetRemoteStationManager ("ns3::AarfWifiManager");
  WifiHelper wifi_m;
  wifi_m.SetRemoteStationManager ("ns3::AarfWifiManager");


  WifiMacHelper mac_n;
  Ssid ssid_n = Ssid ("ns-3-ssid");
  mac_n.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid_n),
               "ActiveProbing", BooleanValue (false));
  WifiMacHelper mac_m;
  Ssid ssid_m = Ssid ("ns-3-ssid");
  mac_m.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid_m),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer botn_staDevices;
  botn_staDevices = wifi_n.Install (phy_n, mac_n, botn_wifiStaNodes);

  NetDeviceContainer botm_staDevices;
  botm_staDevices = wifi_m.Install (phy_m, mac_m, botm_wifiStaNodes);

  mac_n.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid_n));
  mac_m.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid_m));

  NetDeviceContainer apDevices_n3;
  apDevices_n3 = wifi_n.Install (phy_n, mac_n, wifiApNode_n3);

  NetDeviceContainer apDevices_n4;
  apDevices_n4 = wifi_n.Install (phy_m, mac_m, wifiApNode_n4);


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
  mobility.Install (botn_wifiStaNodes);
  mobility.Install (wifiApNode_n3);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (botm_wifiStaNodes);
  mobility.Install (wifiApNode_n4);


  InternetStackHelper stack;
  stack.Install (Nodes);
  stack.Install (botn_wifiStaNodes);
  stack.Install (botm_wifiStaNodes);

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

  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_n1_n4;
  interfaces_n1_n4 = address.Assign (devices_n1_n4);

  // Assign IP to bots
  address.SetBase ("10.2.1.0", "255.255.255.0");
  address.Assign (apDevices_n3);
  address.Assign (botn_staDevices);

  address.SetBase ("10.3.1.0", "255.255.255.0");
  address.Assign (apDevices_n4);
  address.Assign (botm_staDevices);

  #ifdef SINK
    // UDPSink
    PacketSinkHelper UDPsink("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), PORT)));
    ApplicationContainer UDPSinkApp = UDPsink.Install(Nodes.Get(2));
    UDPSinkApp.Start(Seconds(0.0));
    UDPSinkApp.Stop(Seconds(SIM_TIME));
  #endif

  #ifdef ECHO
   	// UDPecho
    UdpEchoServerHelper echoServer (PORT);
    ApplicationContainer serverApp = echoServer.Install(Nodes.Get(2));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(SIM_TIME));
  #endif

  // Bot onoff Application Behaviour
  OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(interfaces_n1_n2.GetAddress(1), PORT)));
  onoff.SetConstantRate(DataRate("5Mbps"));
  onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=30]"));
  onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute("PacketSize", UintegerValue(1024));

    // Install application in all bots
  ApplicationContainer onOffApp_n[NUM_BOTS_n];
  for (int i = 0; i < NUM_BOTS_n; i++)
  {
      onOffApp_n[i] = onoff.Install(botn_wifiStaNodes.Get(i));
      onOffApp_n[i].Start(Seconds(1.0));
      onOffApp_n[i].Stop(Seconds(SIM_TIME));
  }

  ApplicationContainer onOffApp_m[NUM_BOTS_m];
  for (int i = 0; i < NUM_BOTS_m; i++)
  {
      onOffApp_m[i] = onoff.Install(botm_wifiStaNodes.Get(i));
      onOffApp_m[i].Start(Seconds(0.0));
      onOffApp_m[i].Stop(Seconds(SIM_TIME));
  }

  // TCP connection between node n0 and node n2
  Address sinkAddress (InetSocketAddress(interfaces_n1_n2.GetAddress (1), PORT2));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",
    InetSocketAddress (Ipv4Address::GetAny (), PORT2));
  ApplicationContainer sinkApps = packetSinkHelper.Install (Nodes.Get (2));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (SIM_TIME));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (Nodes.Get (0), TcpSocketFactory::GetTypeId ());
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));
  ns3TcpSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream2));

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1040, NUM_PACKETS, DataRate ("5Mbps"));
  Nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.0));
  app->SetStopTime (Seconds (SIM_TIME));

  //devices_n1_n2.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));



  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("BotNet.tr"));
  pointToPoint.EnablePcapAll("BotNet");
  phy_n.EnablePcap ("BotNetn", apDevices_n3.Get (0));
  phy_m.EnablePcap ("BotNetm", apDevices_n4.Get (0));

  Simulator::Stop (Seconds (SIM_TIME));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}