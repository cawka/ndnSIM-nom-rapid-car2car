/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2013 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"

#include "ns3/ndnSIM-module.h"

#include "ndn-v2v-net-device-face.h"
#include "car-relay-tracer.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace ns3;
using namespace boost;

NS_LOG_COMPONENT_DEFINE ("Experiment");

Ptr<ndn::NetDeviceFace>
V2vNetDeviceFaceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn, Ptr<NetDevice> device)
{
  // NS_LOG_DEBUG ("Creating ndn::V2vNetDeviceFace on node " << node->GetId ());

  Ptr<ndn::NetDeviceFace> face = CreateObject<ndn::V2vNetDeviceFace> (node, device);
  ndn->AddFace (face);
  // NS_LOG_LOGIC ("Node " << node->GetId () << ": added NetDeviceFace as face #" << *face);

  return face;
}

int
main (int argc, char *argv[])
{
  // disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("OfdmRate24Mbps"));

  // vanet hacks to CcnxL3Protocol
  Config::SetDefault ("ns3::ndn::ForwardingStrategy::CacheUnsolicitedData", StringValue ("true")); // not necessary, but for clarity

  // v2v device
  Config::SetDefault ("ns3::ndn::V2vNetDeviceFace::MaxDelay", StringValue ("2ms"));
  Config::SetDefault ("ns3::ndn::V2vNetDeviceFace::MaxDelayLowPriority", StringValue ("5ms"));
  Config::SetDefault ("ns3::ndn::V2vNetDeviceFace::MaxDistance", StringValue ("250"));

  // Config::SetDefault ("ns3::CcnxBroadcastNetDeviceFace::MaxRetransmissionAttempts", StringValue ("2"));

  // !!! very important parameter !!!
  // Should keep PIT entry to prevent duplicate interests from re-propagating
  Config::SetDefault ("ns3::ndn::Pit::PitEntryPruningTimout", StringValue ("10s"));

  CommandLine cmd;

  bool distanceDelay = false;
  bool jumpDistance = false;
  bool retx = false;
  bool cachedTime = false;

  uint32_t run = 1;
  cmd.AddValue ("run", "Run", run);

  double distance = 10;
  cmd.AddValue ("distance", "Distance between cars (default 10 meters)", distance);

  double fixedDistance = -1;
  cmd.AddValue ("fixedDistance", "Length of the highway. Number of cars will be set as (fixedDistance / distance + 1). If not set, there are 1000 cars", fixedDistance);

  cmd.Parse (argc,argv);

  uint32_t numberOfCars = 1000;
  if (fixedDistance > 0)
    {
      numberOfCars = fixedDistance / distance + 1;
    }

  Config::SetGlobal ("RngRun", IntegerValue (run));

  // distanceLogger = new Logger (distanceDelay, jumpDistance, retx, cachedTime);

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


  NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default ();
  wifiMacHelper.SetType("ns3::AdhocWifiMac");

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::HighwayPositionAllocator",
                                 "Start", VectorValue(Vector(0.0, 0.0, 0.0)),
                                 "Direction", DoubleValue(0.0),
                                 "Length", DoubleValue(1000.0),
                                 "MinGap", DoubleValue(distance),
                                 "MaxGap", DoubleValue(distance));

  mobility.SetMobilityModel("ns3::CustomConstantVelocityMobilityModel",
                            "ConstantVelocity", VectorValue(Vector(26.8224, 0, 0)));

  NodeContainer nodes;
  nodes.Create (numberOfCars);

  ////////////////
  // 1. Install Wifi
  NetDeviceContainer wifiNetDevices = wifi.Install (wifiPhyHelper, wifiMacHelper, nodes);

  // 2. Install Mobility model
  mobility.Install (nodes);

  // 3. Install CCNx stack
  NS_LOG_INFO ("Installing NDN stack");
  ndn::StackHelper ndnHelper;
  ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback (V2vNetDeviceFaceCallback));
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::V2v");
  ndnHelper.SetContentStore ("ns3::ndn::cs::Lru",
                             "MaxSize", "10000");
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.Install (nodes);

  // 4. Set up applications
  NS_LOG_INFO ("Installing Applications");

  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerBatches");
  consumerHelper.SetPrefix ("/very-long-prefix-requested-by-client/this-interest-hundred-bytes-long-interest");
  consumerHelper.SetAttribute ("Batches", StringValue ("2s 1"));

  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetPrefix ("/");
  producerHelper.SetAttribute ("PayloadSize", StringValue("300"));

  // install producer and consumer on the same node
  NS_LOG_INFO("Making node("<< nodes.Get(0)->GetId() <<") both a consumer and producer. "<<
	      "It will pull data from it self and then data will be propagated out.");
  consumerHelper.Install (nodes.Get (0));
  producerHelper.Install (nodes.Get (0));

  ////////////////

  string prefix = "results/car-relay-" + lexical_cast<string> (run) + "-" + lexical_cast<string> (distance) + "-";

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<boost::shared_ptr<ndn::CarRelayTracer> > >
    tracing1 = ndn::CarRelayTracer::InstallAll (prefix+"distance.txt", ndn::CarRelayTracer::DISTANCE_WAITING);

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<boost::shared_ptr<ndn::CarRelayTracer> > >
    tracing2 = ndn::CarRelayTracer::InstallAll (prefix+"jump-distance.txt", ndn::CarRelayTracer::JUMP_DISTANCE);

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<boost::shared_ptr<ndn::CarRelayTracer> > >
    tracing3 = ndn::CarRelayTracer::InstallAll (prefix+"tx.txt", ndn::CarRelayTracer::TX);

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<boost::shared_ptr<ndn::CarRelayTracer> > >
    tracing4 = ndn::CarRelayTracer::InstallAll (prefix+"in-cache.txt", ndn::CarRelayTracer::IN_CACHE);

  Simulator::Stop (Seconds (30.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
