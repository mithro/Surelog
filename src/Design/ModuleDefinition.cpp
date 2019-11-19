/*
 Copyright 2019 Alain Dargelas

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/*
 * File:   ModuleDefinition.cpp
 * Author: alain
 *
 * Created on October 20, 2017, 10:29 PM
 */

#include "SourceCompile/SymbolTable.h"
#include "Library/Library.h"
#include "FileContent.h"
#include "ModuleDefinition.h"

using namespace SURELOG;

ModuleDefinition::ModuleDefinition(FileContent* fileContent, NodeId nodeId,
                                   std::string& name)
    : DesignComponent(fileContent), m_name(name) {
  if (fileContent) {
    addFileContent(fileContent, nodeId);
  }
}

bool ModuleDefinition::isInstance() {
  VObjectType type = getType();
  if ((type == VObjectType::slN_input_gate_instance) ||
      (type == VObjectType::slModule_declaration) ||
      (type == VObjectType::slUdp_declaration))
    return true;
  return false;
}

ModuleDefinition::~ModuleDefinition() {}

ModuleDefinition* ModuleDefinitionFactory::newModuleDefinition(
    FileContent* fileContent, NodeId nodeId, std::string name) {
  return new ModuleDefinition(fileContent, nodeId, name);
}

unsigned int ModuleDefinition::getSize() {
  if (m_fileContents.size()) {
    return 0;
  }
  unsigned int size = 0;
  for (unsigned int i = 0; i < m_fileContents.size(); i++) {
    NodeId end = m_nodeIds[i];
    NodeId begin = m_fileContents[i]->Child(end);
    size += (end - begin);
  }
  return size;
}

void ModuleDefinition::insertModPort(SymbolId modport, Signal& signal) {
  ModPortSignalMap::iterator itr = m_modportSignalMap.find(modport);
  if (itr == m_modportSignalMap.end()) {
    std::vector<Signal> signals;
    signals.push_back(signal);
    m_modportSignalMap.insert(std::make_pair(modport, signals));
  } else {
    (*itr).second.push_back(signal);
  }
}

Signal* ModuleDefinition::getModPortSignal(SymbolId modport, NodeId port) {
  ModPortSignalMap::iterator itr = m_modportSignalMap.find(modport);
  if (itr == m_modportSignalMap.end()) {
    return NULL;
  } else {
    for (auto& sig : (*itr).second) {
      if (sig.getNodeId() == port) {
        return &sig;
      }
    }
  }
  return NULL;
}

void ModuleDefinition::insertModPort(SymbolId modport, ClockingBlock& cb) {
  ModPortClockingBlockMap::iterator itr =
      m_modportClockingBlockMap.find(modport);
  if (itr == m_modportClockingBlockMap.end()) {
    std::vector<ClockingBlock> cbs;
    cbs.push_back(cb);
    m_modportClockingBlockMap.insert(std::make_pair(modport, cbs));
  } else {
    (*itr).second.push_back(cb);
  }
}

ClockingBlock* ModuleDefinition::getModPortClockingBlock(SymbolId modport,
                                                         NodeId port) {
  ModPortClockingBlockMap::iterator itr =
      m_modportClockingBlockMap.find(modport);
  if (itr == m_modportClockingBlockMap.end()) {
    return NULL;
  } else {
    for (auto& cb : (*itr).second) {
      if (cb.getNodeId() == port) {
        return &cb;
      }
    }
  }
  return NULL;
}

ClassDefinition* ModuleDefinition::getClassDefinition(const std::string& name) {
  ClassNameClassDefinitionMultiMap::iterator itr =
      m_classDefinitions.find(name);
  if (itr == m_classDefinitions.end()) {
    return NULL;
  } else {
    return (*itr).second;
  }
}