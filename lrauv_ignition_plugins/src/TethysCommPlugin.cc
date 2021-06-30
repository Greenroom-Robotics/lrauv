/*
 * Copyright (C) 2021 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <chrono>

#include <ignition/gazebo/Util.hh>
#include <ignition/gazebo/components/AngularVelocity.hh>
#include <ignition/gazebo/components/JointPosition.hh>
#include <ignition/gazebo/components/LinearVelocity.hh>
#include <ignition/gazebo/components/Pose.hh>
#include <ignition/msgs/double.pb.h>
#include <ignition/msgs/empty.pb.h>
#include <ignition/msgs/header.pb.h>
#include <ignition/msgs/time.pb.h>
#include <ignition/msgs/vector3d.pb.h>
#include <ignition/plugin/Register.hh>
#include <ignition/transport/TopicUtils.hh>

#include "lrauv_command.pb.h"
#include "lrauv_state.pb.h"

#include "TethysCommPlugin.hh"

using namespace tethys_comm_plugin;

void AddAngularVelocityComponent(
  const ignition::gazebo::Entity &_entity,
  ignition::gazebo::EntityComponentManager &_ecm)
{
  if (!_ecm.Component<ignition::gazebo::components::AngularVelocity>(
      _entity))
  {
    _ecm.CreateComponent(_entity,
      ignition::gazebo::components::AngularVelocity());
  }
  // Create an angular velocity component if one is not present.
  if (!_ecm.Component<ignition::gazebo::components::WorldAngularVelocity>(
      _entity))
  {
    _ecm.CreateComponent(_entity,
      ignition::gazebo::components::WorldAngularVelocity());
  }
}

void AddWorldPose (
  const ignition::gazebo::Entity &_entity,
  ignition::gazebo::EntityComponentManager &_ecm)
{
  if (!_ecm.Component<ignition::gazebo::components::WorldPose>(
      _entity))
  {
    _ecm.CreateComponent(_entity,
      ignition::gazebo::components::WorldPose());
  }
  // Create an angular velocity component if one is not present.
  if (!_ecm.Component<ignition::gazebo::components::WorldPose>(
      _entity))
  {
    _ecm.CreateComponent(_entity,
      ignition::gazebo::components::WorldPose());
  }
}

void AddJointPosition(
  const ignition::gazebo::Entity &_entity,
  ignition::gazebo::EntityComponentManager &_ecm)
{
  auto jointPosComp =
      _ecm.Component<ignition::gazebo::components::JointPosition>(_entity);
  if (jointPosComp == nullptr)
  {
    _ecm.CreateComponent(
      _entity, ignition::gazebo::components::JointPosition());
  }
}

void AddWorldLinearVelocity(
  const ignition::gazebo::Entity &_entity,
  ignition::gazebo::EntityComponentManager &_ecm)
{
  if (!_ecm.Component<ignition::gazebo::components::WorldLinearVelocity>(
      _entity))
  {
    _ecm.CreateComponent(_entity,
      ignition::gazebo::components::WorldLinearVelocity());
  }
}

void TethysCommPlugin::Configure(
  const ignition::gazebo::Entity &_entity,
  const std::shared_ptr<const sdf::Element> &_sdf,
  ignition::gazebo::EntityComponentManager &_ecm,
  ignition::gazebo::EventManager &_eventMgr)
{
  // Get namespace
  std::string ns {""};
  if (_sdf->HasElement("namespace"))
  {
    ns = _sdf->Get<std::string>("namespace");
  }
  else
  {
    ns = "tethys";
  }

  // Parse SDF parameters
  if (_sdf->HasElement("command_topic"))
  {
    this->commandTopic = _sdf->Get<std::string>("command_topic");
  }
  if (_sdf->HasElement("state_topic"))
  {
    this->stateTopic = _sdf->Get<std::string>("state_topic");
  }

  // Initialize transport
  if (!this->node.Subscribe(this->commandTopic,
      &TethysCommPlugin::CommandCallback, this))
  {
    ignerr << "Error subscribing to topic " << "[" << commandTopic << "]. "
      << std::endl;
    return;
  }

  this->statePub =
    this->node.Advertise<lrauv_ignition_plugins::msgs::LRAUVState>(
    this->stateTopic);
  if (!this->statePub)
  {
    ignerr << "Error advertising topic [" << stateTopic << "]"
      << std::endl;
  }

  SetupControlTopics(ns);
  SetupEntities(_entity, _sdf, _ecm, _eventMgr);
}

void TethysCommPlugin::SetupControlTopics(const std::string &_ns)
{
  this->thrusterTopic = ignition::transport::TopicUtils::AsValidTopic(
    "/model/" + _ns + "/joint/" + this->thrusterTopic);
  this->thrusterPub =
    this->node.Advertise<ignition::msgs::Double>(this->thrusterTopic);
  if (!this->thrusterPub)
  {
    ignerr << "Error advertising topic [" << this->thrusterTopic << "]"
      << std::endl;
  }

  this->rudderTopic = ignition::transport::TopicUtils::AsValidTopic(
    "/model/" + _ns + "/joint/" + this->rudderTopic);
  this->rudderPub =
    this->node.Advertise<ignition::msgs::Double>(this->rudderTopic);
  if (!this->rudderPub)
  {
    ignerr << "Error advertising topic [" << this->rudderTopic << "]"
      << std::endl;
  }

  this->elevatorTopic = ignition::transport::TopicUtils::AsValidTopic(
    "/model/" + _ns + "/joint/" + this->elevatorTopic);
  this->elevatorPub =
    this->node.Advertise<ignition::msgs::Double>(this->elevatorTopic);
  if (!this->elevatorPub)
  {
    ignerr << "Error advertising topic [" << this->elevatorTopic << "]"
      << std::endl;
  }

  this->massShifterTopic = ignition::transport::TopicUtils::AsValidTopic(
    "/model/" + _ns + "/joint/" + this->massShifterTopic);
  this->massShifterPub =
    this->node.Advertise<ignition::msgs::Double>(this->massShifterTopic);
  if (!this->massShifterPub)
  {
    ignerr << "Error advertising topic [" << this->massShifterTopic << "]"
      << std::endl;
  }

  this->buoyancyEngineCmdTopic = ignition::transport::TopicUtils::AsValidTopic(
    "/model/" + _ns + "/" + this->buoyancyEngineCmdTopic);
  this->buoyancyEnginePub =
    this->node.Advertise<ignition::msgs::Double>(this->buoyancyEngineCmdTopic);
  if (!this->buoyancyEnginePub)
  {
    ignerr << "Error advertising topic [" << this->buoyancyEngineCmdTopic << "]"
      << std::endl;
  }

  this->buoyancyEngineStateTopic = ignition::transport::TopicUtils::AsValidTopic(
    "/model/" + _ns + "/" + this->buoyancyEngineStateTopic);
  if (!this->node.Subscribe(this->buoyancyEngineStateTopic,
      &TethysCommPlugin::BuoyancyStateCallback, this))
  {
    ignerr << "Error subscribing to topic " << "["
      << this->buoyancyEngineStateTopic << "]. " << std::endl;
    return;
  }

  this->dropWeightTopic = ignition::transport::TopicUtils::AsValidTopic("/model/" +
    _ns + "/" + this->dropWeightTopic);
  this->dropWeightPub =
    this->node.Advertise<ignition::msgs::Empty>(this->dropWeightTopic);
  if(!this->dropWeightPub)
  {
    ignerr << "Error advertising topic [" << this->dropWeightTopic << "]"
      << std::endl;
  }

  this->dropWeightTopic = ignition::transport::TopicUtils::AsValidTopic("/model/" +
    _ns + "/" + this->dropWeightTopic);
  this->dropWeightPub =
    this->node.Advertise<ignition::msgs::Double>(this->dropWeightTopic);
  if(!this->dropWeightPub)
  {
    ignerr << "Error advertising topic [" << this->dropWeightTopic << "]"
      << std::endl;
  }
}

void TethysCommPlugin::SetupEntities(
  const ignition::gazebo::Entity &_entity,
  const std::shared_ptr<const sdf::Element> &_sdf,
  ignition::gazebo::EntityComponentManager &_ecm,
  ignition::gazebo::EventManager &_eventMgr)
{
  if (_sdf->HasElement("model_link"))
  {
    this->baseLinkName = _sdf->Get<std::string>("model_link");
  }

  if (_sdf->HasElement("propeller_link"))
  {
    this->thrusterLinkName = _sdf->Get<std::string>("propeller_link");
  }

  if (_sdf->HasElement("rudder_joint"))
  {
    this->rudderJointName = _sdf->Get<std::string>("rudder_joint");
  }

  if (_sdf->HasElement("elavator_joint"))
  {
    this->elevatorJointName = _sdf->Get<std::string>("elavator_joint");
  }

  if (_sdf->HasElement("mass_shifter_joint"))
  {
    this->massShifterJointName = _sdf->Get<std::string>("mass_shifter_joint");
  }

  auto model = ignition::gazebo::Model(_entity);

  this->modelLink = model.LinkByName(_ecm, this->baseLinkName);
  this->thrusterLink = model.LinkByName(_ecm, this->thrusterLinkName);
  this->rudderJoint = model.JointByName(_ecm, this->rudderJointName);
  this->elevatorJoint = model.JointByName(_ecm, this->elevatorJointName);
  this->massShifterJoint = model.JointByName(_ecm, this->massShifterJointName);

  AddAngularVelocityComponent(this->thrusterLink, _ecm);
  AddWorldPose(this->modelLink, _ecm);
  AddJointPosition(this->rudderJoint, _ecm);
  AddJointPosition(this->elevatorJoint, _ecm);
  AddJointPosition(this->massShifterJoint, _ecm);
  AddWorldLinearVelocity(this->modelLink, _ecm);
}

void TethysCommPlugin::CommandCallback(
  const lrauv_ignition_plugins::msgs::LRAUVCommand &_msg)
{
  // Lazy timestamp conversion just for printing
  //if (std::chrono::seconds(int(floor(_msg.time_()))) - this->prevSubPrintTime
  //    > std::chrono::milliseconds(1000))
  {
    igndbg << "Received command: " << std::endl
      << "  propOmegaAction_: " << _msg.propomegaaction_() << std::endl
      << "  rudderAngleAction_: " << _msg.rudderangleaction_() << std::endl
      << "  elevatorAngleAction_: " << _msg.elevatorangleaction_() << std::endl
      << "  massPositionAction_: " << _msg.masspositionaction_() << std::endl
      << "  buoyancyAction_: " << _msg.buoyancyaction_() << std::endl
      << "  density_: " << _msg.density_() << std::endl
      << "  dt_: " << _msg.dt_() << std::endl
      << "  time_: " << _msg.time_() << std::endl;

    this->prevSubPrintTime = std::chrono::seconds(int(floor(_msg.time_())));
  }

  // Rudder
  ignition::msgs::Double rudderAngMsg;
  rudderAngMsg.set_data(_msg.rudderangleaction_());
  this->rudderPub.Publish(rudderAngMsg);

  // Elevator
  ignition::msgs::Double elevatorAngMsg;
  elevatorAngMsg.set_data(_msg.elevatorangleaction_());
  this->elevatorPub.Publish(elevatorAngMsg);

  // Thruster
  ignition::msgs::Double thrusterMsg;
  // TODO(arjo):
  // Conversion from rpm-> force b/c thruster plugin takes force
  // Maybe we should change that?
  auto angVel = _msg.propomegaaction_();
  auto force = -0.004422 * 1000 * 0.0016 * angVel * angVel;
  if (angVel < 0)
  {
    force *=-1;
  }
  thrusterMsg.set_data(force);
  this->thrusterPub.Publish(thrusterMsg);

  // Mass shifter
  ignition::msgs::Double massShifterMsg;
  massShifterMsg.set_data(_msg.masspositionaction_());
  this->massShifterPub.Publish(massShifterMsg);

  // Buoyancy Engine
  ignition::msgs::Double buoyancyEngineMsg;
  buoyancyEngineMsg.set_data(_msg.buoyancyaction_());
  this->buoyancyEnginePub.Publish(buoyancyEngineMsg);

  // Drop weight
  auto dropweight = _msg.dropweightstate_();
  if(dropweight != 0)
  {
    ignition::msgs::Empty dropWeightCmd;
    this->dropWeightPub.Publish(dropWeightCmd);
  }
}

void TethysCommPlugin::BuoyancyStateCallback(
  const ignition::msgs::Double &_msg)
{
  this->buoyancyBladderVolume = _msg.data();
}

void TethysCommPlugin::PostUpdate(
  const ignition::gazebo::UpdateInfo &_info,
  const ignition::gazebo::EntityComponentManager &_ecm)
{
  ignition::gazebo::Link baseLink(modelLink);
  auto modelPose = ignition::gazebo::worldPose(modelLink, _ecm);

  // Publish state
  lrauv_ignition_plugins::msgs::LRAUVState stateMsg;

  stateMsg.mutable_header()->mutable_stamp()->set_sec(
    std::chrono::duration_cast<std::chrono::seconds>(_info.simTime).count());
  stateMsg.mutable_header()->mutable_stamp()->set_nsec(
    int(std::chrono::duration_cast<std::chrono::nanoseconds>(
    _info.simTime).count()) - stateMsg.header().stamp().sec() * 1000000000);

  auto rph = modelPose.Rot().Euler();
  ignition::msgs::Set(stateMsg.mutable_rph_(), rph);
  stateMsg.set_depth_(-modelPose.Pos().Z());

  // Linear velocity
  auto linearVelocity =
    _ecm.Component<ignition::gazebo::components::WorldLinearVelocity>(
    modelLink);
  stateMsg.set_speed_(linearVelocity->Data().Length());

  // Rudder position
  auto rudderPosComp =
    _ecm.Component<ignition::gazebo::components::JointPosition>(rudderJoint);
  if (rudderPosComp->Data().size() != 1)
  {
    ignerr << "Rudder joint has wrong size\n";
    return;
  }
  stateMsg.set_rudderangle_(rudderPosComp->Data()[0]);

  // Elevator position
  auto elevatorPosComp =
    _ecm.Component<ignition::gazebo::components::JointPosition>(elevatorJoint);
  if (elevatorPosComp->Data().size() != 1)
  {
    ignerr << "Elavator joint has wrong size\n";
    return;
  }
  stateMsg.set_elevatorangle_(elevatorPosComp->Data()[0]);

  // Mass shifter position
  auto massShifterPosComp =
    _ecm.Component<ignition::gazebo::components::JointPosition>(
    massShifterJoint);
  if (massShifterPosComp->Data().size() != 1)
  {
    ignerr << "Mass shifter joint component has the wrong size ("
      << massShifterPosComp->Data().size() << "), expected 1\n";
    return;
  }
  stateMsg.set_massposition_(massShifterPosComp->Data()[0]);

  // TODO(anyone)
  // Follow up https://github.com/ignitionrobotics/ign-gazebo/pull/519
  auto latlon = sphericalCoords.SphericalFromLocalPosition(modelPose.Pos());
  stateMsg.set_latitudedeg_(latlon.X());
  stateMsg.set_longitudedeg_(latlon.Y());

  ignition::gazebo::Link propLink(thrusterLink);
  auto propOmega = propLink.WorldAngularVelocity(_ecm)->Length();
  stateMsg.set_propomega_(propOmega);
  stateMsg.set_buoyancyposition_(buoyancyBladderVolume);
  this->statePub.Publish(stateMsg);

  if (_info.simTime - this->prevPubPrintTime > std::chrono::milliseconds(1000))
  {
    igndbg << "Published state to " << this->stateTopic
      << " at time: " << stateMsg.header().stamp().sec()
      << "." << stateMsg.header().stamp().nsec() << std::endl
      << "\tpropOmega: " << stateMsg.propomega_() << std::endl
      << "\tSpeed: " << stateMsg.speed_() << std::endl
      << "\tElevator angle: " << stateMsg.elevatorangle_() << std::endl
      << "\tRudder angle: " << stateMsg.rudderangle_() << std::endl
      << "\tMass shifter (m): " << stateMsg.massposition_() << std::endl;
    this->prevPubPrintTime = _info.simTime;
  }
}

IGNITION_ADD_PLUGIN(
  tethys_comm_plugin::TethysCommPlugin,
  ignition::gazebo::System,
  tethys_comm_plugin::TethysCommPlugin::ISystemConfigure,
  tethys_comm_plugin::TethysCommPlugin::ISystemPostUpdate)
