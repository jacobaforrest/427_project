#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/trace-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

/* Default Network Topology

//             WiFi                                                 Point-to-Point             
//                                            AP                                  n0
//  *     *                      *                                                 | (0-0)
//  |     |                      |                                                 | ↓
//  |     |                      |                                                 | 
//  |     |                      |                                                 | 10.1.1.0
//  |     |                      |                                                 | 
//  |     |                      |                                                 | ↑
//  |     |                      |           10.1.3.0                              | (1-0)         10.1.2.0
// b0_2,   b0_1,                    b0_0 ---------------------------------------- n1 --------------------------------------- n2
//       10.2.1.0                    ← (3-1)    (3-0) →           ← (1-2)       /  |     (1-1) →                     ← (2-0)
//                                                                             /   | (1-n)
//                                                                            /    | ↓
//                                                                           /     |
//                                                                    .     /      | 10.1.4.0
//                                                                        .        |
//                                                                            .    |
//                                                                                 | ↑
//                                                 10.3.1.0               ← (n-1)  | (n-0)
//                                            bn_2,      bn_1,                   bn_0
//                                              |          |                       |
//                                              |          |                       |
//                                              |          |                       |
//                                              |          |                       |
//                                              *          *                       *
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

#define SINK // UDP Sink server

//#define ECHO // UDP Echo server
#define ERROR_MODEL


#define NUM_BOT_NETWORKS 5
#define SIM_TIME 20
#define NUM_PACKETS 5000000
#define DATA_RATE "100Mbps"
#define TCP_RATE "100Mbps"
#define DDOS_RATE "10Mbps"
#define CHANNEL_DELAY "2ms"
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
  Nodes.Create (3);


  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (DATA_RATE));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (CHANNEL_DELAY));
  pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("100p"));

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



  NodeContainer Bot_AP_Nodes;
  Bot_AP_Nodes.Create (NUM_BOT_NETWORKS);
  NetDeviceContainer devices_Bot_AP[NUM_BOT_NETWORKS];
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    devices_Bot_AP[i] = pointToPoint.Install (Nodes.Get(1), Bot_AP_Nodes.Get(i));
  }

//                         n0
//                          |
//                          |
//   b0_0 ---------------- n1 ---------------- n2
//                       /  |
//                      /   |
//                     /    |
//                    /     |
//                 ...    bn_0
//
//                 point-to-point




  NodeContainer wifi_AP_Nodes[NUM_BOT_NETWORKS];
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    wifi_AP_Nodes[i] = Bot_AP_Nodes.Get(i);
  }


  NodeContainer bot_wifiStaNodes[NUM_BOT_NETWORKS];
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    bot_wifiStaNodes[i].Create (2);
  }

//      wifi                           point-to-point
//   *       *           *                   n0   
//   |       |           |                    |                   
//   |       |           |                    |
// b0_2,   b0_1,       b0_0 ---------------- n1 ---------------- n2
//       wifi                              /  |
//                                        /   |
//                                       /    |
//                                     ...    |
//           bn_2,  bn_1,                   bn_0
//             |      |                       |
//             |      |                       |
//             *      *                       *
//               WiFi     


  YansWifiChannelHelper channels[NUM_BOT_NETWORKS];
  YansWifiPhyHelper phy[NUM_BOT_NETWORKS];
  WifiHelper wifi[NUM_BOT_NETWORKS];
  WifiMacHelper mac[NUM_BOT_NETWORKS];
  Ssid ssid[NUM_BOT_NETWORKS];
  NetDeviceContainer bot_staDevices[NUM_BOT_NETWORKS];
  NetDeviceContainer bot_apDevices[NUM_BOT_NETWORKS];

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    channels[i] = YansWifiChannelHelper::Default ();
    phy[i].SetChannel (channels[i].Create ());
    wifi[i].SetRemoteStationManager ("ns3::AarfWifiManager");
    ssid[i] = Ssid ("ns-3-ssid");
    mac[i].SetType ("ns3::StaWifiMac",
              "Ssid", SsidValue (ssid[i]),
              "ActiveProbing", BooleanValue (false));
    bot_staDevices[i] = wifi[i].Install (phy[i], mac[i], bot_wifiStaNodes[i]);
    mac[i].SetType("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid[i]));
    bot_apDevices[i] = wifi[i].Install (phy[i], mac[i], wifi_AP_Nodes[i]);
    mobility.Install (bot_wifiStaNodes[i]);
    mobility.Install (wifi_AP_Nodes[i]);

  }


  InternetStackHelper stack;
  stack.Install (Nodes);
  stack.Install (Bot_AP_Nodes);

  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    stack.Install (bot_wifiStaNodes[i]);
  }

  Ipv4AddressHelper address;

  // Assign IP to nodes
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_n1_n0;
  interfaces_n1_n0 = address.Assign (devices_n1_n0);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_n1_n2;
  interfaces_n1_n2 = address.Assign (devices_n1_n2);

  // Assign IP to AP nodes
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_n1_b[NUM_BOT_NETWORKS];
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    interfaces_n1_b[i] = address.Assign (devices_Bot_AP[i]);
  }

  // Assign IP to bots
  address.SetBase ("10.2.1.0", "255.255.255.0");
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    address.Assign(bot_apDevices[i]);
    address.Assign(bot_staDevices[i]);
  }


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
  onoff.SetConstantRate(DataRate(DDOS_RATE));
  onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=30]"));
  onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute("PacketSize", UintegerValue(1024));

    // Install application in all bots
  ApplicationContainer onOffApp[NUM_BOT_NETWORKS * 2];
  int j = 0;
  for (int i = 0; i < NUM_BOT_NETWORKS * 2; i = i + 2)
  {
    
      onOffApp[i] = onoff.Install(bot_wifiStaNodes[j].Get(0));
      onOffApp[i+1] = onoff.Install(bot_wifiStaNodes[j].Get(1));


      onOffApp[i].Start(Seconds(0.0));   
      onOffApp[i].Stop(Seconds(SIM_TIME));

      onOffApp[i+1].Start(Seconds(0.0));   
      onOffApp[i+1].Stop(Seconds(SIM_TIME));

      j++;

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
  app->Setup (ns3TcpSocket, sinkAddress, 1040, NUM_PACKETS, DataRate (TCP_RATE));
  Nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (0.0));
  app->SetStopTime (Seconds (SIM_TIME));

  //devices_n1_n2.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("BotNet.tr"));
  pointToPoint.EnablePcapAll("BotNet");
  
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    phy[i].EnablePcap ("BotNet", bot_apDevices[i].Get (0));
  }


  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();


  Simulator::Stop (Seconds (SIM_TIME));
  Simulator::Run ();
  Simulator::Destroy ();

  flowMonitor->SerializeToXmlFile("flowBotNet.xml", true, true);

  return 0;
}