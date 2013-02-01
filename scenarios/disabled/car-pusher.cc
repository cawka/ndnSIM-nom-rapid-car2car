/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/ndnSIM-module.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

using namespace ns3;
using namespace boost;

NS_LOG_COMPONENT_DEFINE ("CarPusher");

class Logger2
{
public:
  Logger2 (bool events, const string &file)
  : run (0)
  {
    std::cout << "The following files will be created/rewritten:\n";

    if (events)
      {
        os1 = make_shared<ofstream> (("result/events/"+file).c_str (), ios::trunc);
        *os1 << "Run\tDensity\tTime\tNodeId\tEvent\tSeqId\n";
        std::cout << "    result/events/" << file << "\n";
      }
  }

  void
  SetRun (uint32_t _run)
  {
    run = _run;
  }

  void
  SetDistance (double _distance)
  {
    distance = _distance;
  }


  void InInterest (std::string context, Ptr<const CcnxInterestHeader> header, Ptr<const CcnxFace> face)
  {
    char_separator<char> sep("/");
    tokenizer< char_separator<char> > tokens(context, sep);

    tokenizer< char_separator<char> >::iterator i = tokens.begin ();
    if (*i == "NodeList")
      {
	i++;

	*os1 << run << "\t" << distance << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t"
	     << *i << "\t" << "Incoming interest" << "\t" << header->GetName ()->GetLastComponent () << "\n";
	// NS_LOG_INFO( Simulator::Now ().ToDouble (Time::S) << " sec \tNode #" << " received interest seq#" << header->GetName ()->GetLastComponent () );
      }
  }

  void PhyOutData (std::string context,  Ptr<const Packet> packet)
  {
    char_separator<char> sep("/");
    tokenizer< char_separator<char> > tokens(context, sep);

    tokenizer< char_separator<char> >::iterator i = tokens.begin ();
    if (*i == "NodeList")
      {
	i++;
	*os1 << run << "\t" << distance << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t"
	     << *i << "\t" << "Broadcasting" << "\t" << packet->PeekPacketTag<CcnxNameComponentsTag> ()->GetName ()->GetLastComponent () << "\n";
      }

    // // NS_LOG_INFO(face->GetNode()->GetId() << " sends out data seq#" << header->GetName()->GetLastComponent ());
    // NS_LOG_INFO( "physical layer sending out packet");
  }

  void InCache (std::string context, Ptr<Ccnx> ccnx, Ptr<const CcnxContentObjectHeader> header, Ptr<const Packet> packet)
  {
    char_separator<char> sep("/");
    tokenizer< char_separator<char> > tokens(context, sep);

    tokenizer< char_separator<char> >::iterator i = tokens.begin ();
    if (*i == "NodeList")
      {
	i++;
	// NS_LOG_INFO( Simulator::Now ().ToDouble (Time::S) << " sec \tNode #" << ccnx->GetObject<Node> ()->GetId () << " cached seq#" << header->GetName ()->GetLastComponent () );
	*os1 << run << "\t" << distance << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t"
	     << *i << "\t" << "Data cached" << "\t" << header->GetName ()->GetLastComponent () << "\n";
      }
  }

  void MacOutData (std::string context,  Ptr<const Packet> packet)
  {
    char_separator<char> sep("/");
    tokenizer< char_separator<char> > tokens(context, sep);

    tokenizer< char_separator<char> >::iterator i = tokens.begin ();
    if (*i == "NodeList")
      {
	i++;
	*os1 << run << "\t" << distance << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t"
	     << *i << "\t" << "MAC-layer scheduling" << "\t" << packet->PeekPacketTag<CcnxNameComponentsTag> ()->GetName ()->GetLastComponent () << "\n";
      }
    // NS_LOG_INFO(face->GetNode()->GetId() << " sends out data seq#" << header->GetName()->GetLastComponent ());
    // NS_LOG_INFO( "mac layer enqueue");
  }

  void InterferenceDrop(string context, Ptr<const Packet> packet)
  {
    char_separator<char> sep("/");
    tokenizer< char_separator<char> > tokens(context, sep);

    tokenizer< char_separator<char> >::iterator i = tokens.begin ();
    if (*i == "NodeList")
      {
	i++;
	// NS_LOG_INFO( "Dropping packet due to noise or error model calculation." );
	*os1 << run << "\t" << distance << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t"
	     << *i << "\t" << "Interference drop" << "\t" << packet->PeekPacketTag<CcnxNameComponentsTag> ()->GetName ()->GetLastComponent () << "\n";
      }
  }

  void Canceling (string context, Ptr<Node> node, Ptr<const Packet> packet)
  {
    char_separator<char> sep("/");
    tokenizer< char_separator<char> > tokens(context, sep);

    tokenizer< char_separator<char> >::iterator i = tokens.begin ();
    if (*i == "NodeList")
      {
	i++;
	// NS_LOG_INFO( "Dropping packet due to noise or error model calculation." );
	*os1 << run << "\t" << distance << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t"
	     << *i << "\t" << "Canceling transmission" << "\t" << packet->PeekPacketTag<CcnxNameComponentsTag> ()->GetName ()->GetLastComponent () << "\n";
      }
  }

  // void OtherDrop(string context, Ptr<const Packet> packet)
  // {
  //   // NS_LOG_INFO( "Other types of dropping packet." );
  //   *os1 << run << "\t" << distance << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t"
  // 	 << context << "Drop" << "\t" << packet->PeekPacketTag<CcnxNameComponentsTag> ()->GetName ()->GetLastComponent () << "\n";
  // }

private:
  uint32_t run;
  double distance;

  shared_ptr<ofstream> os1;
};

static Logger2 *distanceLogger = 0;


int
main (int argc, char *argv[])
{
  // disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("OfdmRate24Mbps"));


  // vanet hacks to CcnxL3Protocol
  Config::SetDefault ("ns3::CcnxL3Protocol::CacheUnsolicitedData", StringValue ("true"));
  Config::SetDefault ("ns3::CcnxBroadcastNetDeviceFace::MaxDelay", StringValue ("2ms"));
  Config::SetDefault ("ns3::CcnxBroadcastNetDeviceFace::MaxDelayLowPriority", StringValue ("5ms"));
  Config::SetDefault ("ns3::CcnxBroadcastNetDeviceFace::MaxDistance", StringValue ("150"));
  Config::SetDefault ("ns3::CcnxPit::PitEntryPruningTimout", StringValue ("1s"));

  CommandLine cmd;
  bool events = false;
  uint32_t numberOfRuns = 1;
  double fixedDistance = -1;
  double initialDensity = 10;
  double maxDensity = 160;
  double stepDensity = 20;
  string batches = "2s 1";
  double interestRate = 10; // 10 packets a second
  string ofile = "push-events.txt";
  bool cacheOnly = false;

  cmd.AddValue ("runs", "Number of runs", numberOfRuns);
  cmd.AddValue ("e", "Enable events trace", events);
  cmd.AddValue ("c", "Enable only cache events", cacheOnly);
  cmd.AddValue ("batches", "Consumer interest batches", batches);
  cmd.AddValue ("rate", "Interest rate", interestRate);

  cmd.AddValue ("dist", "Fixed distance (number of cars will be adjusted to this distance if present)", fixedDistance);

  cmd.AddValue ("iDens", "Initial density", initialDensity);
  cmd.AddValue ("mDens", "Max density", maxDensity);
  cmd.AddValue ("sDens", "Density step", stepDensity);

  cmd.AddValue ("output", "Output file name", ofile);

  cmd.Parse (argc,argv);

  distanceLogger = new Logger2 (events, ofile);
  // several runs
  for (double distance = initialDensity; distance < maxDensity; distance += stepDensity)
    {
      distanceLogger->SetDistance (distance);
      std::cout << "Car density: " << distance << ": ";

      double maxCarDistance = distance;
      double minCarDistance = distance;
      uint32_t numberOfCars = 1000;
      if (fixedDistance > 0)
        {
          numberOfCars = fixedDistance / distance + 1;
        }

      for (uint32_t run = 0; run < numberOfRuns; run ++)
        {
          distanceLogger->SetRun (run);

          std::cout << " " << run << std::flush;
          if (numberOfRuns>1)
            Config::SetGlobal ("RngRun", IntegerValue (run));

          //////////////////////
          //////////////////////
          //////////////////////
          WifiHelper wifi = WifiHelper::Default ();
          // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
          wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
          wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode", StringValue ("OfdmRate24Mbps"));

          YansWifiChannelHelper wifiChannel;// = YansWifiChannelHelper::Default ();
          wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
          wifiChannel.AddPropagationLoss ("ns3::ThreeLogDistancePropagationLossModel");
          wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");

          //YansWifiPhy wifiPhy = YansWifiPhy::Default();
          YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default ();
          wifiPhyHelper.SetChannel (wifiChannel.Create ());
          wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
          wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));


          NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
          wifiMac.SetType("ns3::AdhocWifiMac");

          MobilityHelper mobility;
          mobility.SetPositionAllocator ("ns3::HighwayPositionAllocator",
                                         "Start", VectorValue(Vector(0.0, 0.0, 0.0)),
                                         "Direction", DoubleValue(0.0),
                                         "Length", DoubleValue(1000.0),
                                         "MinGap", DoubleValue(minCarDistance),
                                         "MaxGap", DoubleValue(maxCarDistance));

          mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel",
                                    "ConstantVelocity", VectorValue(Vector(26.8224, 0, 0)));

	  // Create nodes
          NodeContainer nodes;
          nodes.Create (numberOfCars);

	  // 1. Install Wifi
	  NetDeviceContainer wifiNetDevices = wifi.Install (wifiPhyHelper, wifiMac, nodes);

	  // 2. Install Mobility model
	  mobility.Install (nodes);

	  // 3. Install CCNx stack
	  NS_LOG_INFO ("Installing CCNx stack");
	  CcnxStackHelper ccnxHelper;
	  ccnxHelper.SetDefaultRoutes(true);
	  ccnxHelper.Install(nodes);

	  Config::Connect ("/NodeList/*/$ns3::CcnxL3Protocol/ContentStore/InCache", MakeCallback (&Logger2::InCache, distanceLogger));
          if (!cacheOnly)
            {
              Config::Connect ("/NodeList/*/$ns3::CcnxL3Protocol/InInterests", MakeCallback (&Logger2::InInterest, distanceLogger));
              Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin", MakeCallback (&Logger2::PhyOutData, distanceLogger));

              Config::Connect ("/NodeList/*/$ns3::CcnxL3Protocol/FaceList/*/$ns3::CcnxBroadcastNetDeviceFace/Canceling",
                               MakeCallback (&Logger2::Canceling, distanceLogger ));

              // Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&Logger2::MacOutData, distanceLogger));
              Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxInteferenceDrop", MakeCallback( &Logger2::InterferenceDrop, distanceLogger ));
              // Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback( &Logger2::OtherDrop, distanceLogger ));
            }

	  // 4. Set up applications
	  NS_LOG_INFO ("Installing Applications");

          CcnxAppHelper consumerHelper ("ns3::CcnxConsumerBatches");
	  consumerHelper.SetPrefix ("/very-long-prefix-requested-by-client/this-interest-hundred-bytes-long-");
	  consumerHelper.SetAttribute ("Batches", StringValue (batches));
	  consumerHelper.SetAttribute ("InterestRate", UintegerValue (interestRate));

          CcnxAppHelper producerHelper ("ns3::CcnxProducer");
	  producerHelper.SetPrefix ("/");
	  producerHelper.SetAttribute ("PayloadSize", StringValue("300"));

          // install producer at one end
          producerHelper.Install (nodes.Get (0));
          // install consumer at the other end
	  consumerHelper.Install (nodes.Get (0));

          Simulator::Stop (Seconds (300.0));

          Simulator::Run ();
          Simulator::Destroy ();
	}
      cout << "\n";
    }

  delete distanceLogger;

  return 0;
}
