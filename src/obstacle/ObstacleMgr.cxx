/* bzflag
 * Copyright (c) 1993 - 2004 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#include "common.h"

// implementation header
#include "ObstacleMgr.h"

// system headers
#include <string.h>
#include <string>
#include <vector>
#include <iostream>

// common headers
#include "Pack.h"
#include "MeshTransform.h"
#include "ObstacleModifier.h"
#include "StateDatabase.h"
#include "PhysicsDriver.h"
#include "BzMaterial.h"

// obstacle headers
#include "Obstacle.h"
#include "WallObstacle.h"
#include "BoxBuilding.h"
#include "PyramidBuilding.h"
#include "BaseBuilding.h"
#include "Teleporter.h"
#include "MeshObstacle.h"
#include "ArcObstacle.h"
#include "ConeObstacle.h"
#include "SphereObstacle.h"
#include "TetraBuilding.h"


//////////////////////////////////////////////////////////////////////////////
//
// Group Instance
// - uses a group definition and a transform to produce obstacles
//


void GroupInstance::init()
{
  modifyTeam = false;
  team = 0;
  modifyColor = false;
  tint[0] = tint[1] = tint[2] = tint[3] = 1.0f;
  modifyPhysicsDriver = false;
  phydrv = -1;
  modifyMaterial = false;
  material = NULL;
  driveThrough = false;
  shootThrough = false;
  
  return;
}


GroupInstance::GroupInstance(const std::string& _groupdef)
{
  groupdef = _groupdef;
  init();
  return;
}


GroupInstance::GroupInstance()
{
  init();
  return;
}


GroupInstance::~GroupInstance()
{
  return;
}


void GroupInstance::setTransform(const MeshTransform& _transform)
{
  transform = _transform;
  return;
}


void GroupInstance::setName(const std::string& _name)
{
  name = _name;
  return;
}


const std::string& GroupInstance::getName() const
{
  return name;
}


void GroupInstance::setTeam(int _team)
{
  team = _team;
  modifyTeam = true;
  return;
}


void GroupInstance::setTint(const float _tint[4])
{
  memcpy(tint, _tint, sizeof(float[4]));
  modifyColor = true;
  return;
}


void GroupInstance::setPhysicsDriver(int _phydrv)
{
  phydrv = _phydrv;
  modifyPhysicsDriver = true;
  return;
}


void GroupInstance::setMaterial(const BzMaterial* _material)
{
  material = _material;
  modifyMaterial = true;
  return;
}


void GroupInstance::setDriveThrough()
{
  driveThrough = true;
  return;
}


void GroupInstance::setShootThrough()
{
  shootThrough = true;
  return;
}
                

const std::string& GroupInstance::getGroupDef() const
{
  return groupdef;
}


const MeshTransform& GroupInstance::getTransform() const
{
  return transform;
}


void* GroupInstance::pack(void* buf)
{
  buf = nboPackStdString(buf, groupdef);
  buf = nboPackStdString(buf, name);
  buf = transform.pack(buf);

  uint8_t bits = 0;
  if (modifyTeam)	   bits |= (1 << 0);
  if (modifyColor)	   bits |= (1 << 1);
  if (modifyPhysicsDriver) bits |= (1 << 2);
  if (modifyMaterial)	   bits |= (1 << 3);
  if (driveThrough)	   bits |= (1 << 4);
  if (shootThrough)	   bits |= (1 << 5);
  buf = nboPackUByte(buf, bits);

  if (modifyTeam) {
    buf = nboPackUShort(buf, team);
  }
  if (modifyColor) {
    buf = nboPackVector(buf, tint);
    buf = nboPackFloat(buf, tint[3]);
  }
  if (modifyPhysicsDriver) {
    buf = nboPackInt(buf, phydrv);
  }
  if (modifyMaterial) {
    int matindex = MATERIALMGR.getIndex(material);
    buf = nboPackInt(buf, (int32_t) matindex);
  }

  return buf;
}


void* GroupInstance::unpack(void* buf)
{
  buf = nboUnpackStdString(buf, groupdef);
  buf = nboUnpackStdString(buf, name);
  buf = transform.unpack(buf);

  uint8_t bits;
  buf = nboUnpackUByte(buf, bits);
  modifyTeam =		((bits & (1 << 0)) == 0) ? false : true;
  modifyColor =		((bits & (1 << 1)) == 0) ? false : true;
  modifyPhysicsDriver = ((bits & (1 << 2)) == 0) ? false : true;
  modifyMaterial =	((bits & (1 << 3)) == 0) ? false : true;
  driveThrough =	((bits & (1 << 4)) == 0) ? false : true;
  shootThrough =	((bits & (1 << 5)) == 0) ? false : true;

  if (modifyTeam) {
    uint16_t tmpTeam;
    buf = nboUnpackUShort(buf, tmpTeam);
    team = (int)tmpTeam;
  }
  if (modifyColor) {
    buf = nboUnpackVector(buf, tint);
    buf = nboUnpackFloat(buf, tint[3]);
  }
  if (modifyPhysicsDriver) {
    int32_t inPhyDrv;
    buf = nboUnpackInt(buf, inPhyDrv);
    phydrv = int(inPhyDrv);
  }
  if (modifyMaterial) {
    int32_t matindex;
    buf = nboUnpackInt(buf, matindex);
    material = MATERIALMGR.getMaterial(matindex);
  }
  
  return buf;
}


int GroupInstance::packSize()
{
  int fullSize = 0;
  fullSize += nboStdStringPackSize(groupdef);
  fullSize += nboStdStringPackSize(name);
  fullSize += transform.packSize();
  fullSize += sizeof(uint8_t);
  if (modifyTeam) {
    fullSize += sizeof(uint16_t);
  }
  if (modifyColor) {
    fullSize += sizeof(float[4]);
  }
  if (modifyPhysicsDriver) {
    fullSize += sizeof(int32_t);
  }
  if (modifyMaterial) {
    fullSize += sizeof(int32_t);
  }
  return fullSize;
}


void GroupInstance::print(std::ostream& out, const std::string& indent) const
{
  out << indent << "group " << groupdef << std::endl;
  
  if (name.size() > 0) {
    out << indent << "  name " << name << std::endl;
  }

  transform.printTransforms(out, indent);
  if (modifyTeam) {
    out << indent << "  team " << team << std::endl;
  }
  if (modifyColor) {
    out << indent << "  tint " << tint[0] << " " << tint[1] << " "
			       << tint[2] << " " << tint[3] << " "
			       << std::endl;
  }
  if (modifyPhysicsDriver) {
    const PhysicsDriver* driver = PHYDRVMGR.getDriver(phydrv);
    if (driver != NULL) {
      out << indent << "  phydrv ";
      if (driver->getName().size() > 0) {
        out << driver->getName();
      } else {
        out << phydrv;
      }
      out << std::endl;
    }
  }
  if (modifyMaterial) {
    out << indent << "  matref ";
    MATERIALMGR.printReference(out, material);
    out << std::endl;
  }
  if (driveThrough) {
    out << indent << "  driveThrough " << phydrv << std::endl;
  }
  if (shootThrough) {
    out << indent << "  shootTHrough " << phydrv << std::endl;
  }

  out << indent << "end" << std::endl;
  
  return;
}


//////////////////////////////////////////////////////////////////////////////
//
// Group Definition
// - defines an obstacle group
//

std::string GroupDefinition::depthName;


GroupDefinition::GroupDefinition(const std::string& _name)
{
  name = _name;
  active = false;
  return;
}


GroupDefinition::~GroupDefinition()
{
  return;
}


Obstacle* GroupDefinition::newObstacle(int type)
{
  Obstacle* obs = NULL;

  if (type == wallType) {
    obs = new WallObstacle();
  } else if (type == boxType) {
    obs = new BoxBuilding();
  } else if (type == pyrType) {
    obs = new PyramidBuilding();
  } else if (type == baseType) {
    obs = new BaseBuilding();
  } else if (type == teleType) {
    obs = new Teleporter();
  } else if (type == meshType) {
    obs = new MeshObstacle();
  } else if (type == arcType) {
    obs = new ArcObstacle();
  } else if (type == coneType) {
    obs = new ConeObstacle();
  } else if (type == sphereType) {
    obs = new SphereObstacle();
  } else if (type == tetraType) {
    obs = new TetraBuilding();
  }

  return obs;
}


void GroupDefinition::addObstacle(Obstacle* obstacle)
{
  const char* type = obstacle->getType();

  if (WallObstacle::getClassName() == type) {
    lists[wallType].push_back(obstacle);
  } else if (BoxBuilding::getClassName() == type) {
    lists[boxType].push_back(obstacle);
  } else if (BaseBuilding::getClassName() == type) {
    lists[baseType].push_back(obstacle);
  } else if (PyramidBuilding::getClassName() == type) {
    lists[pyrType].push_back(obstacle);
  } else if (Teleporter::getClassName() == type) {
    lists[teleType].push_back(obstacle);
  } else if (MeshObstacle::getClassName() == type) {
    lists[meshType].push_back(obstacle);
  } else if (ArcObstacle::getClassName() == type) {
    lists[arcType].push_back(obstacle);
  } else if (ConeObstacle::getClassName() == type) {
    lists[coneType].push_back(obstacle);
  } else if (SphereObstacle::getClassName() == type) {
    lists[sphereType].push_back(obstacle);
  } else if (TetraBuilding::getClassName() == type) {
    lists[tetraType].push_back(obstacle);
  } else {
    printf ("GroupDefinition::addObstacle() ERROR: type = %s\n", type);
    exit(1);
  }

  return;
}


void GroupDefinition::addGroupInstance(GroupInstance* group)
{
  groups.push_back(group);
  return;
}


static bool isContainer(int type)
{
  switch (type) {
    case GroupDefinition::arcType:
    case GroupDefinition::coneType:
    case GroupDefinition::sphereType:
    case GroupDefinition::tetraType:
      return true;
    default:
      return false;
  }
}


static MeshObstacle* makeContainedMesh(int type, const Obstacle* obs)
{
  MeshObstacle* mesh = NULL;
  switch (type) {
    case GroupDefinition::arcType: {
      mesh = ((ArcObstacle*)obs)->makeMesh();
      break;
    }
    case GroupDefinition::coneType: {
      mesh = ((ConeObstacle*)obs)->makeMesh();
      break;
    }
    case GroupDefinition::sphereType: {
      mesh = ((SphereObstacle*)obs)->makeMesh();
      break;
    }
    case GroupDefinition::tetraType: {
      mesh = ((TetraBuilding*)obs)->makeMesh();
      break;
    }
  }
  return mesh;
}


void GroupDefinition::makeGroups(const MeshTransform& xform,
				 const ObstacleModifier& obsMod) const
{
  if (active) {
    DEBUG1("warning: avoided recursion, groupdef \"%s\"\n", name.c_str());
    return; // avoid recursion
  }

  active = true;

  const bool isWorld = (this == OBSTACLEMGR.getWorld());
  char groupDefBit = isWorld ? 0 : Obstacle::GroupDefSource;

  for (int type = 0; type < ObstacleTypeCount; type++) {
    const ObstacleList& list = lists[type];
    for (unsigned int i = 0; i < list.size(); i++) {
      Obstacle* obs;
      if (isWorld) {
        obs = list[i]; // no need to copy
      } else {
        obs = list[i]->copyWithTransform(xform);
      }
      if (obs->isValid()) {
        if (!isWorld) {
          // add it to the world
          obs->setSource(Obstacle::GroupDefSource);
          obsMod.execute(obs);
          OBSTACLEMGR.addWorldObstacle(obs);
        }
        // generate contained meshes
        MeshObstacle* mesh = makeContainedMesh(type, obs);
        if ((mesh != NULL) && mesh->isValid()) {
          mesh->setSource(Obstacle::ContainerSource | groupDefBit);
          obsMod.execute(mesh);
          OBSTACLEMGR.addWorldObstacle(mesh);
        }
      }
    }
  }

  for (unsigned int i = 0; i < groups.size(); i++) {
    const GroupInstance* group = groups[i];
    const GroupDefinition* groupDef =
      OBSTACLEMGR.findGroupDef(group->getGroupDef());
    if (groupDef != NULL) {
      ObstacleModifier newObsMod(obsMod, *group);
      MeshTransform tmpXform = xform;
      tmpXform.prepend(group->getTransform());
      // recurse
      groupDef->makeGroups(tmpXform, newObsMod);
    } else {
      DEBUG1("warning: group definition \"%s\" is missing\n",
             group->getGroupDef().c_str());
    }
  }

  active = false;

  return;
}


void GroupDefinition::replaceBasesWithBoxes()
{
  ObstacleList& list = lists[baseType];
  for (unsigned int i = 0; i < list.size(); i++) {
    BaseBuilding* base = (BaseBuilding*) list[i];
    const float* baseSize = base->getSize();
    BoxBuilding* box =
      new BoxBuilding(base->getPosition(), base->getRotation(),
		      baseSize[0], baseSize[1], baseSize[2],
		      base->isDriveThrough(), base->isShootThrough(),
		      false);
    delete base;
    list.erase(i);
    i--;
    addObstacle(box);
  }
  return;
}


void GroupDefinition::clear()
{
  for (int type = 0; type < ObstacleTypeCount; type++) {
    ObstacleList& list = lists[type];
    for (unsigned int i = 0; i < list.size(); i++) {
      delete list[i];
    }
    list.clear();
  }

  for (unsigned int i = 0; i < groups.size(); i++) {
    delete groups[i];
  }
  groups.clear();

  return;
}


void GroupDefinition::tighten()
{
  for (int type = 0; type < ObstacleTypeCount; type++) {
    lists[type].tighten();
  }
  return;
}


void* GroupDefinition::pack(void* buf) const
{
  buf = nboPackStdString(buf, name);

  unsigned int i;
  for (int type = 0; type < ObstacleTypeCount; type++) {
    const ObstacleList& list = getList(type);
    int count = 0;
    for (i = 0; i < list.size(); i++) {
      if (list[i]->isFromWorldFile()) {
	count++;
      }
    }
    buf = nboPackUInt(buf, count);
    for (i = 0; i < list.size(); i++) {
      if (list[i]->isFromWorldFile()) {
	buf = list[i]->pack(buf);
      }
    }
  }

  buf = nboPackUInt(buf, groups.size());
  for (i = 0; i < groups.size(); i++) {
    buf = groups[i]->pack(buf);
  }

  return buf;
}


void* GroupDefinition::unpack(void* buf)
{
  buf = nboUnpackStdString(buf, name);

  uint32_t i, count;

  for (int type = 0; type < ObstacleTypeCount; type++) {
    buf = nboUnpackUInt(buf, count);
    for (i = 0; i < count; i++) {
      Obstacle* obs = newObstacle(type);
      if (obs != NULL) {
	buf = obs->unpack(buf);
	if (obs->isValid()) {
	  lists[type].push_back(obs);
	}
      }
    }
  }

  buf = nboUnpackUInt(buf, count);
  for (i = 0; i < count; i++) {
    GroupInstance* group = new GroupInstance;
    buf = group->unpack(buf);
    addGroupInstance(group);
  }

  return buf;
}


int GroupDefinition::packSize() const
{
  int fullSize = 0;

  fullSize += nboStdStringPackSize(name);

  for (int type = 0; type < ObstacleTypeCount; type++) {
    fullSize += sizeof(uint32_t);
    const ObstacleList& list = getList(type);
    for (unsigned int i = 0; i < list.size(); i++) {
      if (list[i]->isFromWorldFile()) {
	fullSize += list[i]->packSize();
      }
    }
  }
  fullSize += sizeof(uint32_t);
  for (unsigned int i = 0; i < groups.size(); i++) {
    fullSize += groups[i]->packSize();
  }

  return fullSize;
}


void GroupDefinition::printGrouped(std::ostream& out,
                                   const std::string& indent) const
{
  const bool isWorld = (this == OBSTACLEMGR.getWorld());
  const bool saveAsMeshes = BZDB.isTrue("saveAsMeshes");
  std::string myIndent = indent;
  
  // deal with indenting
  if (!isWorld) {
    myIndent += "  ";
    out << indent << "define " << name << std::endl;
  }

  // print the obstacles
  for (int type = 0; type < ObstacleTypeCount; type++) {
    const ObstacleList& list = getList(type);
    for (unsigned int i = 0; i < list.size(); i++) {
      const Obstacle* obs = list[i];

      if (!obs->isFromGroupDef() && !obs->isFromContainer()) {
        if (!saveAsMeshes) {
          if (!obs->isFromContainer()) {
            obs->print(out, myIndent);
          }
        }
        else {
          if (!isContainer(type)) {
            obs->print(out, myIndent);
          } else {
            // rebuild the mesh  (ya, just to print it)
            MeshObstacle* mesh = makeContainedMesh(type, obs);
            if ((mesh != NULL) && (mesh->isValid())) {
              mesh->print(out, myIndent);
            }
            delete mesh;
          }
        }
      }
    }
  }
  
  // print the groups
  for (unsigned int i = 0; i < groups.size(); i++) {
    groups[i]->print(out, myIndent);
  }

  // deal with indenting
  if (!isWorld) {
    out << indent << "enddef" << std::endl << std::endl;
  }

  return;
}


void GroupDefinition::printFlatFile(std::ostream& out,
                                    const std::string& indent) const
{
  const bool saveAsMeshes = BZDB.isTrue("saveAsMeshes");

  // print the obstacles
  for (int type = 0; type < ObstacleTypeCount; type++) {
    const ObstacleList& list = getList(type);
    for (unsigned int i = 0; i < list.size(); i++) {
      const Obstacle* obs = list[i];

      if (!saveAsMeshes) {
        if (!obs->isFromContainer()) {
          obs->print(out, indent);
        }
      } else {
        if (!isContainer(type)) {
          obs->print(out, indent);
        }
      }
    }
  }

  return;
}


void GroupDefinition::clearDepthName()
{
  depthName = "";
  return;
}


//////////////////////////////////////////////////////////////////////////////
//
// Group Definition Manager
// - utility class to keep track of group definitions
//

GroupDefinitionMgr OBSTACLEMGR;


GroupDefinitionMgr::GroupDefinitionMgr() : world("")
{
  return;
}


GroupDefinitionMgr::~GroupDefinitionMgr()
{
  clear();
  return;
}


void GroupDefinitionMgr::clear()
{
  world.clear();
  for (unsigned int i = 0; i < list.size(); i++) {
    list[i]->clear();
  }
  list.clear();
  return;
}


void GroupDefinitionMgr::tighten()
{
  for (unsigned int i = 0; i < list.size(); i++) {
    list[i]->tighten();
  }
  return;
}


void GroupDefinitionMgr::makeWorld()
{
  GroupDefinition::clearDepthName();

  MeshTransform noXform;
  ObstacleModifier noMods;
  
  world.makeGroups(noXform, noMods);
  
  tighten();
  
  return;
}


void GroupDefinitionMgr::replaceBasesWithBoxes()
{
  world.replaceBasesWithBoxes();
}


void GroupDefinitionMgr::addWorldObstacle(Obstacle* obstacle)
{
  world.addObstacle(obstacle);
  return;
}


void GroupDefinitionMgr::addGroupDef(GroupDefinition* groupdef)
{
  if (groupdef->getName().size() > 0) {
    list.push_back(groupdef);
  } else {
    delete groupdef;
  }
  return;
}


GroupDefinition* GroupDefinitionMgr::findGroupDef(const std::string& name)
{
  if (name.size() <= 0) {
    return NULL;
  }
  for (unsigned int i = 0; i < list.size(); i++) {
    if (name == list[i]->getName()) {
      return list[i];
    }
  }
  return NULL;
}


void* GroupDefinitionMgr::pack(void* buf) const
{
  buf = world.pack(buf);
  buf = nboPackUInt(buf, list.size());
  for (unsigned int i = 0; i < list.size(); i++) {
    buf = list[i]->pack(buf);
  }
  return buf;
}


void* GroupDefinitionMgr::unpack(void* buf)
{
  buf = world.unpack(buf);
  uint32_t i, count;
  buf = nboUnpackUInt(buf, count);
  for (i = 0; i < count; i++) {
    GroupDefinition* groupdef = new GroupDefinition("");
    buf = groupdef->unpack(buf);
    addGroupDef(groupdef);
  }
  return buf;
}


int GroupDefinitionMgr::packSize() const
{
  int fullSize = 0;

  fullSize += world.packSize();
  fullSize += sizeof(uint32_t);
  for (unsigned int i = 0; i < list.size(); i++) {
    fullSize += list[i]->packSize();
  }
  return fullSize;
}


void GroupDefinitionMgr::print(std::ostream& out,
			       const std::string& indent) const
{
  const bool saveFlatFile = BZDB.isTrue("saveFlatFile");

  if (!saveFlatFile) {  
    // print the group definitions
    for (unsigned int i = 0; i < list.size(); i++) {
      list[i]->printGrouped(out, indent);
    }
    // print the world
    world.printGrouped(out, indent);
  }
  else {
    // print the world
    world.printFlatFile(out, indent);
  }

  return;
}


// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

