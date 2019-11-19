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
 * File:   CompileDesign.cpp
 * Author: alain
 *
 * Created on July 1, 2017, 1:11 PM
 */
#include "SourceCompile/SymbolTable.h"
#include "Library/Library.h"
#include "Design/FileContent.h"
#include "ErrorReporting/Error.h"
#include "ErrorReporting/Location.h"
#include "ErrorReporting/Error.h"
#include "ErrorReporting/ErrorDefinition.h"
#include "ErrorReporting/ErrorContainer.h"
#include "SourceCompile/CompilationUnit.h"
#include "SourceCompile/PreprocessFile.h"
#include "SourceCompile/CompileSourceFile.h"
#include "CommandLine/CommandLineParser.hpp"
#include "SourceCompile/ParseFile.h"
#include "Testbench/ClassDefinition.h"
#include "SourceCompile/Compiler.h"
#include "CompileDesign.h"
#include "ResolveSymbols.h"
#include "DesignElaboration.h"
#include "UVMElaboration.h"
#include "CompilePackage.h"
#include "CompileModule.h"
#include "CompileFileContent.h"
#include "CompileProgram.h"
#include "CompileClass.h"
#include "Builtin.h"
#include "PackageAndRootElaboration.h"

#ifdef USETBB
#include <tbb/task.h>
#include <tbb/task_group.h>
#include "tbb/task_scheduler_init.h"
#endif

#include <vector>
#include <thread>

using namespace SURELOG;

CompileDesign::CompileDesign(Compiler* compiler) : m_compiler(compiler) {}

CompileDesign::CompileDesign(const CompileDesign& orig) {}

CompileDesign::~CompileDesign() {}

bool CompileDesign::compile() {
  Location loc(0);
  Error err1(ErrorDefinition::COMP_COMPILE, loc);
  ErrorContainer* errors = new ErrorContainer(getCompiler()->getSymbolTable());
  errors->regiterCmdLine(getCompiler()->getCommandLineParser());
  errors->addError(err1);
  errors->printMessage(err1,
                       getCompiler()->getCommandLineParser()->muteStdout());
  delete errors;
  if (!preCompile_()) {
    return false;
  }

  return true;
}

template <class ObjectType, class ObjectMapType, typename FunctorType>
void CompileDesign::compileMT_(ObjectMapType& objects, int maxThreadCount) {
  if (maxThreadCount == 0) {
    for (auto itr : objects) {
      FunctorType funct(this, itr.second, m_compiler->getDesign(),
                        m_symbolTables[0], m_errorContainers[0]);
      funct.operator()();
    }
  } else {
    // Optimize the load balance, try to even out the work in each thread by the
    // number of VObjects
    std::vector<unsigned long> jobSize(maxThreadCount);
    for (unsigned short i = 0; i < maxThreadCount; i++) {
      jobSize[i] = 0;
    }
    std::vector<std::vector<ObjectType*>> jobArray(maxThreadCount);
    for (auto mod : objects) {
      unsigned int size = mod.second->getSize();
      if (size == 0) size = 100;
      unsigned int newJobIndex = 0;
      unsigned long minJobQueue = ULLONG_MAX;
      for (unsigned short ii = 0; ii < maxThreadCount; ii++) {
        if (jobSize[ii] < minJobQueue) {
          newJobIndex = ii;
          minJobQueue = jobSize[ii];
        }
      }
      jobSize[newJobIndex] += size;
      jobArray[newJobIndex].push_back(mod.second);
    }

    if (getCompiler()->getCommandLineParser()->profile()) {
      std::cout << "Compilation Task\n";
      for (unsigned short i = 0; i < maxThreadCount; i++) {
        std::cout << "Thread " << i << " : \n";
        for (unsigned int j = 0; j < jobArray[i].size(); j++) {
          std::cout << jobArray[i][j]->getName() << "\n";
        }
      }
    }

    // Create the threads with their respective workloads
    std::vector<std::thread*> threads;
    for (unsigned short i = 0; i < maxThreadCount; i++) {
      std::thread* th = new std::thread([=] {
        for (unsigned int j = 0; j < jobArray[i].size(); j++) {
          FunctorType funct(this, jobArray[i][j], m_compiler->getDesign(),
                            m_symbolTables[i], m_errorContainers[i]);
          funct.operator()();
        }
      });
      threads.push_back(th);
    }
    // Sync the threads
    for (unsigned int th = 0; th < threads.size(); th++) {
      threads[th]->join();
    }
    // Delete the threads
    for (unsigned int th = 0; th < threads.size(); th++) {
      delete threads[th];
    }
  }
}

void CompileDesign::collectObjects_(Design::FileIdDesignContentMap& all_files,
                                    Design* design, bool finalCollection) {
  typedef std::map<std::string, std::vector<Package*>> FileNamePackageMap;
  FileNamePackageMap fileNamePackageMap;
  SymbolTable* symbols = m_compiler->getSymbolTable();
  ErrorContainer* errors = m_compiler->getErrorContainer();
  // Collect all packages and module definitions
  for (Design::FileIdDesignContentMap::iterator itr = all_files.begin();
       itr != all_files.end(); itr++) {
    FileContent* fC = (*itr).second;
    std::string fileName = fC->getChunkFileName();
    Library* lib = fC->getLibrary();
    for (auto mod : fC->getModuleDefinitions()) {
      ModuleDefinition* existing = design->getModuleDefinition(mod.first);
      if (existing) {
        FileContent* oldFC = existing->getFileContents()[0];
        FileContent* oldParentFile = oldFC->getParent();

        ModuleDefinition* newM = mod.second;
        FileContent* newFC = newM->getFileContents()[0];
        FileContent* newParentFile = newFC->getParent();

        if (oldParentFile && (oldParentFile == newParentFile)) {
          // Recombine splitted module
          existing->addFileContent(mod.second->getFileContents()[0],
                                   mod.second->getNodeIds()[0]);
          for (auto classdef : mod.second->getClassDefinitions()) {
            existing->addClassDefinition(classdef.first, classdef.second);
            classdef.second->setContainer(existing);
          }
        } else {
          design->addModuleDefinition(mod.first, mod.second);
          if (finalCollection) lib->addModuleDefinition(mod.second);
        }
      } else {
        design->addModuleDefinition(mod.first, mod.second);
        if (finalCollection) lib->addModuleDefinition(mod.second);
      }
    }
    for (auto prog : fC->getProgramDefinitions()) {
      design->addProgramDefinition(prog.first, prog.second);
    }
    for (auto pack : fC->getPackageDefinitions()) {
      Package* existing = design->getPackage(pack.first);
      if (existing) {
        FileContent* oldFC = existing->getFileContents()[0];
        FileContent* oldParentFile = oldFC->getParent();
        NodeId oldNodeId = existing->getNodeIds()[0];
        std::string oldFileName = oldFC->getFileName();
        unsigned int oldLine = oldFC->Line(oldNodeId);
        Package* newP = pack.second;
        FileContent* newFC = newP->getFileContents()[0];
        FileContent* newParentFile = newFC->getParent();
        NodeId newNodeId = newP->getNodeIds()[0];
        std::string newFileName = newFC->getFileName();
        unsigned int newLine = newFC->Line(newNodeId);
        if (!finalCollection) {
          if (((oldParentFile != newParentFile) ||
               (oldParentFile == NULL && newParentFile == NULL)) &&
              ((oldFileName != newFileName) || (oldLine != newLine))) {
            Location loc1(symbols->registerSymbol(oldFileName), oldLine, 0,
                          symbols->registerSymbol(pack.first));
            Location loc2(symbols->registerSymbol(newFileName), newLine, 0,
                          symbols->registerSymbol(pack.first));
            Error err(ErrorDefinition::COMP_MULTIPLY_DEFINED_PACKAGE, loc1,
                      loc2);
            errors->addError(err);
          }
        }
        if (oldParentFile && (oldParentFile == newParentFile)) {
          // Recombine splitted package
          existing->addFileContent(newFC, newNodeId);
          for (auto classdef : pack.second->getClassDefinitions()) {
            existing->addClassDefinition(classdef.first, classdef.second);
            classdef.second->setContainer(existing);
          }
        } else {
          design->addPackageDefinition(pack.first, pack.second);
        }
      } else {
        design->addPackageDefinition(pack.first, pack.second);
      }
    }
    for (auto def : fC->getClassDefinitions()) {
      design->addClassDefinition(def.first, def.second);
      for (auto def1 : def.second->getClassMap()) {
        design->addClassDefinition(def1.first, def1.second);
      }
    }
  }
}

bool CompileDesign::elaborate() {
  Location loc(0);
  Error err2(ErrorDefinition::ELAB_ELABORATING_DESIGN, loc);
  ErrorContainer* errors = new ErrorContainer(getCompiler()->getSymbolTable());
  errors->regiterCmdLine(getCompiler()->getCommandLineParser());
  errors->addError(err2);
  errors->printMessage(err2,
                       getCompiler()->getCommandLineParser()->muteStdout());
  delete errors;
  if (!elaboration_()) {
    return false;
  }
  return true;
}

bool CompileDesign::preCompile_() {
  Design* design = m_compiler->getDesign();

  auto& all_files = design->getAllFileContents();

  int maxThreadCount = m_compiler->getCommandLineParser()->getNbMaxTreads();
  if (maxThreadCount == 0) {
    SymbolTable* symbols =
        new SymbolTable(*m_compiler->getCommandLineParser()->getSymbolTable());
    m_symbolTables.push_back(symbols);

    ErrorContainer* errors = new ErrorContainer(symbols);
    errors->regiterCmdLine(m_compiler->getCommandLineParser());
    m_errorContainers.push_back(errors);

  } else if (m_compiler->getCommandLineParser()->useTbb()) {
#ifdef USETBB
    // Use TBB Thread management

    tbb::task_group& group = m_compiler->getTaskGroup();
    for (Design::FileIdDesignContentMap::iterator itr = all_files.begin();
         itr != all_files.end(); itr++) {
      group.run(FunctorCreateLookup(this, (*itr).second));
    }
    group.wait();

#endif
  } else {
    for (unsigned short i = 0; i < maxThreadCount; i++) {
      SymbolTable* symbols = new SymbolTable(
          *m_compiler->getCommandLineParser()->getSymbolTable());
      m_symbolTables.push_back(symbols);
      ErrorContainer* errors = new ErrorContainer(symbols);
      errors->regiterCmdLine(m_compiler->getCommandLineParser());
      m_errorContainers.push_back(errors);
    }
  }

  compileMT_<FileContent, Design::FileIdDesignContentMap, FunctorCreateLookup>(
      all_files, maxThreadCount);

  compileMT_<FileContent, Design::FileIdDesignContentMap, FunctorResolve>(
      all_files, maxThreadCount);

  compileMT_<FileContent, Design::FileIdDesignContentMap,
             FunctorCompileFileContent>(all_files, maxThreadCount);
  collectObjects_(all_files, design, false);
  m_compiler->getDesign()->orderPackages();

  // Compile packages in strict order
  // compileMT_ <Package, PackageNamePackageDefinitionMap,
  // FunctorCompilePackage> (
  //                                                                             m_compiler->getDesign ()->getPackageDefinitions (), 0);
  for (auto itr : m_compiler->getDesign()->getOrderedPackageDefinitions()) {
    FunctorCompilePackage funct(this, itr, m_compiler->getDesign(),
                                m_symbolTables[0], m_errorContainers[0]);
    funct.operator()();
  }

  // Compile modules
  compileMT_<ModuleDefinition, ModuleNameModuleDefinitionMap,
             FunctorCompileModule>(
      m_compiler->getDesign()->getModuleDefinitions(), maxThreadCount);

  // Compile programs
  compileMT_<Program, ProgramNameProgramDefinitionMap, FunctorCompileProgram>(
      m_compiler->getDesign()->getProgramDefinitions(), maxThreadCount);

  // Compile classes
  compileMT_<ClassDefinition, ClassNameClassDefinitionMultiMap,
             FunctorCompileClass>(
      m_compiler->getDesign()->getClassDefinitions(), maxThreadCount);
  design->clearContainers();
  collectObjects_(all_files, design, true);

  Builtin* builtin = new Builtin(design);
  builtin->addBuiltins();

  m_compiler->getDesign()->orderPackages();

  unsigned int size = m_symbolTables.size();
  for (unsigned int i = 0; i < size; i++) {
    m_compiler->getErrorContainer()->appendErrors(*m_errorContainers[i]);
    delete m_symbolTables[i];
    delete m_errorContainers[i];
  }

  return true;
}

bool CompileDesign::checkPrecompilation_() { return true; }

bool CompileDesign::elaboration_() {
  int maxThreadCount = m_compiler->getCommandLineParser()->getNbMaxTreads();
  maxThreadCount = 0;
  if (maxThreadCount == 0) {
    PackageAndRootElaboration* packEl = new PackageAndRootElaboration(this);
    packEl->elaborate();
    delete packEl;
    DesignElaboration* designEl = new DesignElaboration(this);
    designEl->elaborate();
    delete designEl;
    UVMElaboration* uvmEl = new UVMElaboration(this);
    uvmEl->elaborate();
    delete uvmEl;
  } else {
    PackageAndRootElaboration* packEl = new PackageAndRootElaboration(this);
    packEl->elaborate();
    delete packEl;

    std::vector<std::thread*> threads;

    std::thread* th = new std::thread([=] {
      DesignElaboration* designEl = new DesignElaboration(this);
      designEl->elaborate();
      delete designEl;
    });
    threads.push_back(th);

    th = new std::thread([=] {
      UVMElaboration* uvmEl = new UVMElaboration(this);
      uvmEl->elaborate();
      delete uvmEl;
    });
    threads.push_back(th);

    // Sync the threads
    for (unsigned int th = 0; th < threads.size(); th++) {
      threads[th]->join();
    }

    // Delete the threads
    for (unsigned int th = 0; th < threads.size(); th++) {
      delete threads[th];
    }
  }
  return true;
}