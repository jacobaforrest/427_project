# Project: Investigation into Wireless Device Botnets

### ENSC 427: Communication Networks - Spring 2023

---

[View Final Project Report (PDF)](https://github.com/jacobaforrest/427_project/blob/main/FinalProjectReport_Group1_ENSC427.pdf)

## Overview

Understanding the impact that IoT botnets can have on network performance is fundamental for recognizing the consequences of their malicious activities and the need for preventative and mitigative measures. To gain insight on the effectiveness of these botnets, we utilized ns-3 to simulate and measure the impact of a DDoS attack against a single target node. With regards to the scope of the simulation, we limited the ns-3 experimentations to simulations of a UDP Flood DDoS attack executed by botnets of wirelessly connected devices. Specifically, we modelled our simulation around a botnet of IoT devices connected to various home Wi-Fi networks. Through these simulations, we were able to measure the network performance degradation caused by the botnet traffic with metrics such as TCP Packet Mean Delay, TCP Packet Loss Ratio, TCP RX Bitrate, TCP TX Bitrate, and TCP Congestion Window Size (CWND).

---

## Topology

![image](https://github.com/jacobaforrest/427_project/assets/91097464/1b8af5ab-9a56-4280-a465-c778af38a8b7)


## Future Work

- Implementing a botnet traffic detection algorithm.
- Exploring variable numbers of bot and legitimate traffic devices.
- Expanding simulations to include other wireless technologies.

---

## Files

- `BotNet.cc`: [ns-3 simulation script]
- `Results`: [Raw Simulation Results & Plots]
- `Scripts`: [Matlab Plotting Scripts & Python script to parse the results from the FlowMonitor ns-3 module]
- `BotnetNetworks_final.pptx`: [Project Powerpoint Presentation]
- `FinalProjectReport_Group1_ENSC427.pdf`: [Final Project Report]
