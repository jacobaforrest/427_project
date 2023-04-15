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

//                     WiFi                                                 Point-to-Point             
//      real users                  bots                       AP                                          n0
//     *          *              *       *                     *                                            | (0-0)
//     |          |              |       |                     |                                            | ↓
//     |          |              |       |                     |                                            | 
//     |          |              |       |                     |                                            | 10.1.1.0
//     |          |              |       |                     |                                            | 
//     |          |              |       |                     |                                            | ↑
//     |          |              |       |                     |           10.1.3.x                         | (1-0)         10.1.2.0
//   r0_0, ..., r0_3,   ...,   b0_2,   b0_1,                 b0_0 ---------------------------------------- n1 --------------------------------------- n2
//                    10.2.x.x                        ← (3-2)  | (3-0) →                   ← (1-2)       /  |     (1-1) →                     ← (2-0)
//                                                             | (3-1)                                  /   | (1-2+n)
//                                                             | ↓                                     /    | ↓
//                                           10.1.4.x          |                                      /     |
//                                                             | (2+n+1-0)                           /      | 10.1.3.x
//                                                            j0                                  ...       |
//                                                                                                          |
//                                                                                                          | ↑
//                    10.2.x.x                                                                   ← (2+n-2)  | (2+n-0)     10.1.4.x
//   rn_0, ..., rn_3,   ...,   bn_2,   bn_1,                                                              bn_0 -------------------- jn
//     |          |              |       |                                                                  |  (2+n-1) →   ← (2+n+n-0)
//     |          |              |       |                                                                  |
//     |          |              |       |                                                                  |
//     |          |              |       |                                                                  |
//     *          *              *       *                                                                  *
//      real users                  bots                                                                   AP
//                      WiFi     
//

*/


#define BACKGROUND_TRAFFIC

#define NUM_BOT_NETWORKS 120
#define SIM_TIME 60
#define NUM_PACKETS 5000000
#define DATA_RATE "100Mbps"
#define TCP_RATE "100Mbps"
#define DDOS_RATE "10Mbps"
#define BACKGROUND_RATE "10Mbps"
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

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  devices_n1_n2.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));


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
  //                  ...     |
  //                        bn_0
  //
  //                 point-to-point

  NodeContainer J_Nodes;
  J_Nodes.Create(NUM_BOT_NETWORKS);
  NetDeviceContainer devices_J_Nodes[NUM_BOT_NETWORKS];
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    devices_J_Nodes[i] = pointToPoint.Install (Bot_AP_Nodes.Get(i), J_Nodes.Get(i));
  }

  //                         n0
  //                          |
  //                          |
  //   b0_0 ---------------- n1 ---------------- n2
  //     |                 /  |
  //     |                /   |
  //     |               /    |
  //    j0            ...     |
  //                        bn_0 --------------- jn
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

  //     WiFi                           point-to-point
  //   *      *           *                   n0   
  //   |      |           |                    |                   
  //   |      |           |                    |
  // b0_2,  b0_1,       b0_0 ---------------- n1 ---------------- n2
  //                      |                 /  |
  //                      |                /   |
  //                      |               /    |
  //                     j0              /     |
  //                                   ...     |
  // bn_2,  bn_1,                            bn_0 --------------- jn
  //   |      |                                |
  //   |      |                                |
  //   *      *                                *
  //     WiFi     


  NodeContainer real_wifiStaNodes[NUM_BOT_NETWORKS];
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    real_wifiStaNodes[i].Create (4);
  }

  //                     WiFi                           Point-To-Point
  //     real users                     bots             AP              
  //    *          *                  *      *           *                   n0   
  //    |          |                  |      |           |                    |                   
  //    |          |                  |      |           |                    |
  //  r0_0, ..., r0_7,    ...,      b0_2,  b0_1,       b0_0 ---------------- n1 ---------------- n2
  //                                                     |                 /  |
  //                                                     |                /   |
  //                                                     |               /    |
  //                                                    j0              /     |
  //                                                                  ...     |
  //  rn_0, ..., rn_7,    ...,      bn_2,  bn_1,                            bn_0 --------------- jn
  //    |          |                  |      |                                |
  //    |          |                  |      |                                |
  //    *          *                  *      *                                *
  //     real users                     bots                                  AP
  //                     WiFi




  YansWifiChannelHelper channels[NUM_BOT_NETWORKS];
  YansWifiPhyHelper phy[NUM_BOT_NETWORKS];
  WifiHelper wifi[NUM_BOT_NETWORKS];
  WifiMacHelper mac[NUM_BOT_NETWORKS];
  Ssid ssid[NUM_BOT_NETWORKS];
  NetDeviceContainer bot_staDevices[NUM_BOT_NETWORKS];
  NetDeviceContainer real_staDevices[NUM_BOT_NETWORKS];
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
    real_staDevices[i] = wifi[i].Install (phy[i], mac[i], real_wifiStaNodes[i]);
    mac[i].SetType("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid[i]));
    bot_apDevices[i] = wifi[i].Install (phy[i], mac[i], wifi_AP_Nodes[i]);
    mobility.Install (bot_wifiStaNodes[i]);
    mobility.Install (real_wifiStaNodes[i]);
    mobility.Install (wifi_AP_Nodes[i]);

  }


  InternetStackHelper stack;
  stack.Install (Nodes);
  stack.Install (Bot_AP_Nodes);
  stack.Install (J_Nodes);

  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    stack.Install (bot_wifiStaNodes[i]);
    stack.Install (real_wifiStaNodes[i]);
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

  // Assign IP to J nodes
  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces_b_j[NUM_BOT_NETWORKS];
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    interfaces_b_j[i] = address.Assign (devices_J_Nodes[i]);
  }

  // Assign IP to WiFi nodes
  address.SetBase ("10.2.0.0", "255.255.0.0");
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    address.Assign(bot_apDevices[i]);
    address.Assign(bot_staDevices[i]);
    address.Assign(real_staDevices[i]);
  }



  // UDPSink
  PacketSinkHelper UDPsink("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), PORT)));
  ApplicationContainer UDPSinkApp = UDPsink.Install(Nodes.Get(2));
  UDPSinkApp.Start(Seconds(0.0));
  UDPSinkApp.Stop(Seconds(SIM_TIME));

  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    ApplicationContainer UDPSinkApp = UDPsink.Install(J_Nodes.Get(i));
    UDPSinkApp.Start(Seconds(0.0));
    UDPSinkApp.Stop(Seconds(SIM_TIME));
  }

  #ifdef BACKGROUND_TRAFFIC
    // Real user onoff Application Behaviour
    OnOffHelper background_onoff("ns3::UdpSocketFactory", Address());
    background_onoff.SetConstantRate(DataRate(BACKGROUND_RATE));
    background_onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=5]"));
    background_onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=5]"));
    background_onoff.SetAttribute("PacketSize", UintegerValue(1024));

    // Install application in real wifi users
    ApplicationContainer background_onOffApp[NUM_BOT_NETWORKS * 4];
    int t = 0;
    for (int i = 0; i < NUM_BOT_NETWORKS * 4; i = i + 4)
    {
      background_onoff.SetAttribute("Remote", AddressValue(InetSocketAddress(interfaces_b_j[t].GetAddress(1), PORT)));
    
      background_onOffApp[i] = background_onoff.Install(real_wifiStaNodes[t].Get(0));
      background_onOffApp[i+1] = background_onoff.Install(real_wifiStaNodes[t].Get(1));
      background_onOffApp[i+2] = background_onoff.Install(real_wifiStaNodes[t].Get(2));
      background_onOffApp[i+3] = background_onoff.Install(real_wifiStaNodes[t].Get(3));      


      background_onOffApp[i].Start(Seconds(0.0));   
      background_onOffApp[i].Stop(Seconds(SIM_TIME));

      background_onOffApp[i+1].Start(Seconds(2.5));   
      background_onOffApp[i+1].Stop(Seconds(SIM_TIME));

      background_onOffApp[i+2].Start(Seconds(5.0));   
      background_onOffApp[i+2].Stop(Seconds(SIM_TIME));

      background_onOffApp[i+3].Start(Seconds(7.5));   
      background_onOffApp[i+3].Stop(Seconds(SIM_TIME));


      t++;


    }
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
  
  /*
  pointToPoint.EnablePcapAll("BotNet");
  for (int i = 0; i < NUM_BOT_NETWORKS; i++)
  {
    phy[i].EnablePcap ("BotNet", bot_apDevices[i].Get (0));
  }
  */


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