/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "custom-constant-velocity-mobility-model.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CustomConstantVelocityMobilityModel);

TypeId CustomConstantVelocityMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CustomConstantVelocityMobilityModel")
    .SetParent<MobilityModel> ()
    .AddConstructor<CustomConstantVelocityMobilityModel> ()

    .AddAttribute ("ConstantVelocity", "The constant velocity for the mobility model.",
                   VectorValue (Vector (0.0, 0.0, 0.0)),
                   MakeVectorAccessor (&CustomConstantVelocityMobilityModel::SetConstantVelocity,
                                       &CustomConstantVelocityMobilityModel::GetConstantVelocity),
                   MakeVectorChecker ())
    ;

  return tid;
}

CustomConstantVelocityMobilityModel::CustomConstantVelocityMobilityModel ()
{
}

CustomConstantVelocityMobilityModel::~CustomConstantVelocityMobilityModel ()
{
}

void
CustomConstantVelocityMobilityModel::SetConstantVelocity (const Vector &speed)
{
  SetVelocity(speed);
}

Vector
CustomConstantVelocityMobilityModel::GetConstantVelocity (void) const
{
  return DoGetVelocity();
}

void
CustomConstantVelocityMobilityModel::SetVelocity (const Vector &speed)
{
  m_helper.Update ();
  m_helper.SetVelocity (speed);
  m_helper.Unpause ();
  NotifyCourseChange ();
}


Vector
CustomConstantVelocityMobilityModel::DoGetPosition (void) const
{
  m_helper.Update ();
  return m_helper.GetCurrentPosition ();
}
void
CustomConstantVelocityMobilityModel::DoSetPosition (const Vector &position)
{
  m_helper.SetPosition (position);
  NotifyCourseChange ();
}
Vector
CustomConstantVelocityMobilityModel::DoGetVelocity (void) const
{
  return m_helper.GetVelocity ();
}

} // namespace ns3
