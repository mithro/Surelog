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
 * File:   CommonListenerHelper.cpp
 * Author: alain
 * 
 * Created on December 5, 2019, 9:13 PM
 */
#include "SourceCompile/SymbolTable.h"

#include <cstdlib>
#include <iostream>
#include "antlr4-runtime.h"
using namespace std;
using namespace antlr4;
#include "Utils/ParseUtils.h"
#include "CommonListenerHelper.h"
using namespace SURELOG;

CommonListenerHelper::~CommonListenerHelper()
{
}

int CommonListenerHelper::registerObject(VObject& object) {
  m_fileContent->getVObjects().push_back(object);
  return LastObjIndex();
}

int CommonListenerHelper::LastObjIndex() {
  return m_fileContent->getVObjects().size() - 1;
}

int CommonListenerHelper::ObjectIndexFromContext(tree::ParseTree* ctx) {
  ContextToObjectMap::iterator itr = m_contextToObjectMap.find(ctx);
  if (itr == m_contextToObjectMap.end()) {
    return -1;
  } else {
    return (*itr).second;
  }
}

VObject& CommonListenerHelper::Object(NodeId index) {
  return m_fileContent->getVObjects()[index];
}

NodeId CommonListenerHelper::UniqueId(NodeId index) {
  return index;
}

SymbolId& CommonListenerHelper::Name(NodeId index) {
  return m_fileContent->getVObjects()[index].m_name;
}

NodeId& CommonListenerHelper::Child(NodeId index) {
  return m_fileContent->getVObjects()[index].m_child;
}

NodeId& CommonListenerHelper::Sibling(NodeId index) {
  return m_fileContent->getVObjects()[index].m_sibling;
}

NodeId& CommonListenerHelper::Definition(NodeId index) {
  return m_fileContent->getVObjects()[index].m_definition;
}

NodeId& CommonListenerHelper::Parent(NodeId index) {
  return m_fileContent->getVObjects()[index].m_parent;
}

unsigned short& CommonListenerHelper::Type(NodeId index) {
  return m_fileContent->getVObjects()[index].m_type;
}

unsigned int& CommonListenerHelper::Line(NodeId index) {
  return m_fileContent->getVObjects()[index].m_line;
}

int CommonListenerHelper::addVObject(ParserRuleContext* ctx, std::string name,
                                      VObjectType objtype) {
  SymbolId fileId;
  unsigned int line = getFileLine(ctx, fileId);

  VObject object(registerSymbol(name), fileId, objtype, line, 0);
  m_fileContent->getVObjects().push_back(object);
  int objectIndex = m_fileContent->getVObjects().size() - 1;
  m_contextToObjectMap.insert(std::make_pair(ctx, objectIndex));
  addParentChildRelations(objectIndex, ctx);
  if (m_fileContent->getDesignElements().size()) {
    for (unsigned int i = 0; i <= m_fileContent->getDesignElements().size() - 1;
         i++) {
      DesignElement& elem =
          m_fileContent
              ->getDesignElements()[m_fileContent->getDesignElements().size() -
                                    1 - i];
      if (elem.m_context == ctx) {
        // Use the file and line number of the design object (package, module),
        // true file/line when splitting
        m_fileContent->getVObjects().back().m_fileId = elem.m_fileId;
        m_fileContent->getVObjects().back().m_line = elem.m_line;
        elem.m_node = objectIndex;
        break;
      }
    }
  }
  return objectIndex;
}

int CommonListenerHelper::addVObject(ParserRuleContext* ctx,
                                      VObjectType objtype) {
  SymbolId fileId;
  unsigned int line = getFileLine(ctx, fileId);

  VObject object(0, fileId, objtype, line, 0);
  m_fileContent->getVObjects().push_back(object);
  int objectIndex = m_fileContent->getVObjects().size() - 1;
  m_contextToObjectMap.insert(std::make_pair(ctx, objectIndex));
  addParentChildRelations(objectIndex, ctx);
  if (m_fileContent->getDesignElements().size()) {
    for (unsigned int i = 0; i <= m_fileContent->getDesignElements().size() - 1;
         i++) {
      DesignElement& elem =
          m_fileContent
              ->getDesignElements()[m_fileContent->getDesignElements().size() -
                                    1 - i];
      if (elem.m_context == ctx) {
        // Use the file and line number of the design object (package, module),
        // true file/line when splitting
        m_fileContent->getVObjects().back().m_fileId = elem.m_fileId;
        m_fileContent->getVObjects().back().m_line = elem.m_line;
        elem.m_node = objectIndex;
        break;
      }
    }
  }
  return objectIndex;
}

void CommonListenerHelper::addParentChildRelations(int indexParent,
                                                    ParserRuleContext* ctx) {
  int currentIndex = indexParent;
  for (tree::ParseTree* child : ctx->children) {
    int childIndex = ObjectIndexFromContext(child);
    if (childIndex != -1) {
      Parent(childIndex) = UniqueId(indexParent);
      if (currentIndex == indexParent) {
        Child(indexParent) = UniqueId(childIndex);
      } else {
        Sibling(currentIndex) = UniqueId(childIndex);
      }
      currentIndex = childIndex;
    }
  }
}

NodeId CommonListenerHelper::getObjectId(ParserRuleContext* ctx) {
  ContextToObjectMap::iterator itr = m_contextToObjectMap.find(ctx);
  if (itr == m_contextToObjectMap.end()) {
    return 0;
  } else {
    return (*itr).second;
  }
}




