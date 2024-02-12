# Project: Investigation into Wireless Device Botnets

### ENSC 427: Communication Networks - Spring 2023

## Summary

Understanding the impact that IoT botnets can have on network performance is fundamental for recognizing the consequences of their malicious activities and the need for preventative and mitigative measures. To gain insight on the effectiveness of these botnets, ns-3 was utilized to simulate and measure the impact of a DDoS attack against a single target node. With regards to the scope of the simulation, the ns-3 experimentations were limited to simulations of a UDP Flood DDoS attack executed by botnets of wirelessly connected devices. Specifically, the simulation was modeled around a botnet of IoT devices connected to various home Wi-Fi networks. Through these simulations, the network performance degradation caused by the botnet traffic was measured with metrics such as TCP Packet Mean Delay, TCP Packet Loss Ratio, TCP RX Bitrate, TCP TX Bitrate, and TCP Congestion Window Size (CWND).

---

## Scenario A: IoT Botnet DDoS Simulation

## Overview

The primary goal of the simulation was to emulate the infamous Mirai IoT botnet and investigate the impact of a DDoS attack launched from a similar network of IoT-based smart home consumer devices.

The ns-3 network simulator was used to simulate the DDoS attack. In the simulations, the botnet nodes were connected to Wi-Fi networks and generated UDP traffic at a constant bitrate to flood and overwhelm the victim node. A single legitimate user competed with the botnet for network bandwidth for a TCP connection with the victim node. The simulation was run with a varying number of infected Wi-Fi networks and botnet nodes, specifically with 0, 4, 8, 10, 12, and 16 bots.

The experiment aimed to assess the impact on the performance of the legitimate user’s TCP connection with the victim node during the DDoS attack. Specifically, metrics such as congestion window size over time, mean packet delay, packet loss ratio, and transmitting & receiving bitrates were measured.

Ultimately, the simulations showed that the botnet was very effective at denying the legitimate user from service with the targeted victim server.

## Topology

A diagram of the simulated network topology is showcased in Figure 1. The red boxes signify
infected IoT devices. The green box indicates the single legitimate user. Each Wi-Fi network
consisted of two bot nodes. In the simulation, the server node is the recipient of both the botnet’s
generated traffic as well as the legitimate user’s TCP connection traffic. All point-to-point links in
the network were configured for a bandwidth of 100 Mbps, a delay of 2ms, and implemented a
Drop Tail Queue of 100 packets. Additionally, a channel error rate model was implemented with
a probability of 0.0001.

![image](https://github.com/jacobaforrest/427_project/assets/91097464/f1463c63-4601-4c2f-89ef-d9f93c52d723)  
*Figure 1: Simulation Topology*

## Applications

For the legitimate user, a TCP connection with the victim server was simulated. The TCP connection transmitted 1024-byte packets with a data rate of 100 Mbps. For the bot nodes, a UDP flood application was simulated, transmitting 1024-byte packets at a constant bit rate of 10 Mbps.

## Results

The resulting measurements from the simulations are plotted in Figures 2 through 11. The results illustrate that as the volume of botnet traffic increases, the network's performance degrades rapidly. These findings underscore the importance of implementing effective DDoS mitigation strategies, such as traffic filtering, firewalls, and intrusion detection systems, to safeguard against such attacks. Failure to do so can lead to serious outages for legitimate customers.

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/mean_delay.png)  
*Figure 2: Scenario A – TCP Mean Delay Results*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/packet_loss_ratio.png)  
*Figure 3: Scenario A – TCP Packet Loss Ratio Results*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/RX_bitrate.png)  
*Figure 4: Scenario A – TCP RX Bitrate Results*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/TX_bitrate.png)  
*Figure 5: Scenario A – TCP TX Bitrate Results*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/tcp_cwnd_0.png)  
*Figure 6: Scenario A – TCP Congestion Window Results – 0 Bots*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/tcp_cwnd_4.png)  
*Figure 7: Scenario A – TCP Congestion Window Results – 4 Bots*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/tcp_cwnd_8.png)  
*Figure 8: Scenario A – TCP Congestion Window Results – 8 Bots*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/tcp_cwnd_10.png)  
*Figure 9: Scenario A – TCP Congestion Window Results – 10 Bots*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/tcp_cwnd_12.png)  
*Figure 10: Scenario A – TCP Congestion Window Results – 12 Bots*

![image](https://github.com/jacobaforrest/427_project/blob/main/Results/NoBackgroundTraffic/Final%20Plots/tcp_cwnd_16.png)  
*Figure 11: Scenario A – TCP Congestion Window Results – 16 Bots*

---

## Scenario B: IoT Botnet DDoS Simulation with Background Wi-Fi Traffic

## Overview
In scenario A, all devices in the Wi-Fi networks were active bots. However, in a real home Wi-Fi network, there are typically many connected devices, and it is unlikely that all of them are infected with botnet malware. Therefore, the goal of scenario B was to add background traffic to each Wi-Fi network to create a more realistic network simulation.

Scenario B was also implemented with the ns-3 network simulator. The traffic generating applications installed on the original botnet nodes and the original legitimate user node were unchanged. In addition to the two bot nodes in each Wi-Fi network, four legitimate Wi-Fi nodes were added to generate background UDP traffic. In this simulation, the bot nodes must compete for bandwidth in the Wi-Fi network with the legitimate Wi-Fi nodes.

For scenario B, the simulation was conducted with varying numbers of infected Wi-Fi networks and botnet nodes. Specifically, the simulation was run with 0, 20, 40, 80, 160, and 240 bots. The same performance indicators of the legitimate user’s TCP connection with the victim node as in scenario A were measured.

The results of scenario B will demonstrate that the performance of the botnet’s DDoS attack is significantly poorer with the addition of background traffic in each Wi-Fi network.

## Topology

A diagram illustrating the simulated network topology for this experiment is showcased in Figure 12. In the diagram, red boxes indicate the network’s bot nodes, while green boxes represent nodes generating legitimate user traffic. The purple box denotes the DDoS attack victim, targeted with UDP packets by the bot nodes. The TCP connection being observed and measured in the simulation is between the DDoS victim and the point-to-point connected legitimate user node. Blue boxes indicate servers that will receive only traffic generated by legitimate nodes in each Wi-Fi network.

Each Wi-Fi network consists of two bot nodes and four legitimate user nodes. All point-to-point links in the network were configured with a bandwidth of 100 Mbps, a delay of 2 ms, and implemented a Drop Tail Queue of 100 packets. Additionally, a channel error rate model was implemented with a probability of 0.0001.

![output-onlinepngtools](https://github.com/jacobaforrest/427_project/assets/91097464/2861808b-64e1-49f7-bc73-904b46b9974f)  
Figure 12: Simulation Topology (Background Wi-Fi Traffic)

## Applications

For the point-to-point connected legitimate user node, a TCP connection with the victim server was simulated. The TCP connection transmitted 1024-byte packets with a data rate of 100 Mbps. For the bot Wi-Fi nodes, a UDP flood application was simulated, transmitting 1024-byte packets at a constant bit rate of 10 Mbps.

For the real Wi-Fi user nodes, background traffic was simulated using an ns-3 on/off UDP application with a duty cycle of 50% (5 seconds on, 5 seconds off). The application was configured to send 1024-byte packets at a rate of 10 Mbps to a specific server associated with each Wi-Fi network.

## Results

The resulting measurements from the simulations are plotted in Figures 13 through 22. The results demonstrate that compared to scenario A, the number of bots required to effectively deny bandwidth to the legitimate TCP connection was much higher. This is likely because the bots must compete for bandwidth with the legitimate users within their Wi-Fi networks.

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/mean_delay.png)  
*Figure 13: Scenario B – TCP Mean Delay Results*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/packet_loss_ratio.png)  
*Figure 14: Scenario B – TCP Packet Loss Ratio Results*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/RX_bitrate.png)  
*Figure 15: Scenario B – TCP RX Bitrate Results*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/TX_bitrate.png)  
*Figure 16: Scenario B – TCP TX Bitrate Results*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/tcp_cwnd_0.png)  
*Figure 17: Scenario B – TCP Congestion Window Results – 0 Bots*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/tcp_cwnd_20.png)  
*Figure 18: Scenario B – TCP Congestion Window Results – 20 Bots*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/tcp_cwnd_40.png)  
*Figure 19: Scenario B – TCP Congestion Window Results – 40 Bots*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/tcp_cwnd_80.png)  
*Figure 20: Scenario B – TCP Congestion Window Results – 80 Bots*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/tcp_cwnd_160.png)  
*Figure 21: Scenario B – TCP Congestion Window Results – 160 Bots*

![image](https://github.com/jacobaforrest/427_project/tree/main/Results/BackgroundTraffic/Final%20Plots/tcp_cwnd_240.png)  
*Figure 22: Scenario B – TCP Congestion Window Results – 240 Bot*

---

## Tracing

### PCAP Tracing

PCAP tracing is a feature in ns-3 that captures and saves network packets generated during simulations. It was used to trace packets transmitted over the point-to-point net devices and AP Wi-Fi net devices in the project.

### Flow Monitor

The ns-3 FlowMonitor is a tool used to monitor traffic flows in a simulated network. In the simulations, the FlowMonitor ns3 module was utilized to record statistics of the TCP connection’s traffic flow, such as the TX bitrate, the RX bitrate, the Mean Delay, and the Packet Loss Ratio.

### Application - Trace Callbacks

The ns-3 simulator provides the ‘trace callback’ mechanism which enables applications to call specific callback functions whenever a specified event occurs or a variable changes value.

In the project, the trace callback mechanism was used to record updates to the congestion window size and the round-trip time of the TCP traffic flow throughout the duration of the simulations. A custom application was implemented based on the code provided in the fifth.cc script of the ns-3 tutorial. Trace sink functions were created to output changes to the ‘cwnd’ and ‘rtt’ variables of the TCP flow to separate output files and configured to be called whenever the corresponding variables are updated during the simulation. By using these trace callbacks, the behavior of the TCP flow during the simulation was recorded, and this data was then plotted using a Matlab script.

## Future Work

- Implementing a botnet traffic detection algorithm.
- Exploring variable numbers of bot and legitimate traffic devices.
- Expanding simulations to include other wireless technologies.

---

## Files

- `BotNet.cc`: [ns-3 simulation script]
- `Results`: [Raw Simulation Results & Plots]
- `Scripts`: [Matlab Plotting Scripts & Python script to parse the results from the FlowMonitor ns-3 module]
