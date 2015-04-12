#include "SymEngine/NDRangePoint.h"

#include "SymEngine/NDRange.h"
#include "SymEngine/NDRangeSpace.h"

using namespace SymEngine;

NDRangePoint::NDRangePoint() {
  local.assign(3, 0);
  group.assign(3, 0);
  global.assign(3, 0);
}

NDRangePoint::NDRangePoint(int localX, int localY, int localZ, int groupX,
                           int groupY, int groupZ, const NDRangeSpace *ndRangeSpace) {

  int tmpLocal[] = { localX, localY, localZ };
  int tmpGroup[] = { groupX, groupY, groupZ };
  int tmpGlobal[] = { localX + groupX * ndRangeSpace->getLocalSizeX(),
                      localY + groupY * ndRangeSpace->getLocalSizeY(),
                      localZ + groupZ * ndRangeSpace->getLocalSizeZ() };

  local.assign(tmpLocal, tmpLocal + NDRange::DIRECTION_NUMBER);
  group.assign(tmpGroup, tmpGroup + NDRange::DIRECTION_NUMBER);
  global.assign(tmpGlobal, tmpGlobal + NDRange::DIRECTION_NUMBER);
}

int NDRangePoint::getLocalX() const { return local[0]; }
int NDRangePoint::getLocalY() const { return local[1]; }
int NDRangePoint::getLocalZ() const { return local[2]; }

int NDRangePoint::getGlobalX() const { return global[0]; }
int NDRangePoint::getGlobalY() const { return global[1]; }
int NDRangePoint::getGlobalZ() const { return global[2]; }

int NDRangePoint::getGroupX() const { return group[0]; }
int NDRangePoint::getGroupY() const { return group[1]; }
int NDRangePoint::getGroupZ() const { return group[2]; }

int NDRangePoint::getLocal(int direction) const { return local[direction]; }

int NDRangePoint::getGlobal(int direction) const { return global[direction]; }

int NDRangePoint::getGroup(int direction) const { return group[direction]; }

int NDRangePoint::getCoordinate(const std::string &name, int direction) const {
  if (name == NDRange::GET_LOCAL_ID)
    return getLocal(direction);
  if (name == NDRange::GET_GLOBAL_ID)
    return getGlobal(direction);
  if (name == NDRange::GET_GROUP_ID)
    return getGroup(direction);

  return -1;
}

std::string NDRangePoint::toString() const {
  std::string result = "Local: (" + std::to_string(local[0]) + ", " +
                                    std::to_string(local[1]) + ", " + 
                                    std::to_string(local[2]) + ") " +
                       "Global: (" + std::to_string(global[0]) + ", " + 
                                     std::to_string(global[1]) + ", " + 
                                     std::to_string(global[2]) +  ")\n"; 
  return result;
}
