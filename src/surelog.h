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

// Header file for Surelog library

// Compilation control classes
#include "CommandLine/CommandLineParser.h"
#include "SourceCompile/SymbolTable.h"
#include "SourceCompile/CompilationUnit.h"
#include "SourceCompile/PreprocessFile.h"
#include "SourceCompile/CompileSourceFile.h"
#include "SourceCompile/Compiler.h"
#include "ErrorReporting/ErrorContainer.h"
#include "ErrorReporting/Report.h"
#include "ErrorReporting/Waiver.h"

// Antlr headers
#include "antlr4-runtime.h"
using namespace antlr4;

// Python API (optional - needed to be enabled early in the main with PythonAPI::init if needed)
#include "API/PythonAPI.h"

// Full C++ DataModel API
#include "Common/ClockingBlockHolder.h"
#include "Design/ClockingBlock.h"
#include "Design/Design.h"
#include "Design/Instance.h"
#include "Design/Signal.h"
#include "Design/ValuedComponentI.h"
#include "Design/DataType.h"
#include "Design/Enum.h"
#include "Design/ModuleDefinition.h"
#include "Design/Statement.h"
#include "Design/VObject.h"
#include "Design/DefParam.h"
#include "Design/FileCNodeId.h"
#include "Design/ModuleInstance.h"
#include "Design/Task.h"
#include "Design/DesignComponent.h"
#include "Design/FileContent.h"
#include "Design/Parameter.h"
#include "Design/TfPortItem.h"
#include "Design/DesignElement.h"
#include "Design/Function.h"
#include "Design/Scope.h"
#include "Design/TimeInfo.h"
#include "Testbench/ClassDefinition.h"
#include "Testbench/CoverGroupDefinition.h"
#include "Testbench/Property.h"
#include "Testbench/Variable.h"
#include "Testbench/ClassObject.h"
#include "Testbench/FunctionMethod.h"
#include "Testbench/TaskMethod.h"
#include "Testbench/Constraint.h"
#include "Testbench/Program.h"
#include "Testbench/TypeDef.h"
#include "Package/Package.h"
#include "Library/Library.h"
#include "Library/LibrarySet.h"
#include "Config/Config.h"
#include "Config/ConfigSet.h"
#include "Expression/ExprBuilder.h"
#include "Expression/Expr.h"
#include "Expression/Value.h"

// Limited C-like API
#include "API/SLAPI.h"




