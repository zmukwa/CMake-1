/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2001 Insight Consortium
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * The name of the Insight Consortium, nor the names of any consortium members,
   nor of any contributors, may be used to endorse or promote products derived
   from this software without specific prior written permission.

  * Modified source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "cmNMakeMakefileGenerator.h"
#include "cmMakefile.h"
#include "cmStandardIncludes.h"
#include "cmSystemTools.h"
#include "cmSourceFile.h"
#include "cmMakeDepend.h"
#include "cmCacheManager.h"
#include "cmGeneratedFileStream.h"
#include "windows.h"


cmNMakeMakefileGenerator::cmNMakeMakefileGenerator()
{
  this->SetLibraryPathOption("@CMAKE_C_LIBPATH_FLAG@"); // Use @ here
  this->SetObjectFileExtension("$(CMAKE_OBJECT_FILE_SUFFIX)");
  this->SetExecutableExtension("$(CMAKE_EXECUTABLE_SUFFIX)");
  this->SetLibraryPrefix("");
  this->SetStaticLibraryExtension("$(CMAKE_STATICLIB_SUFFIX)");
  this->SetSharedLibraryExtension("$(CMAKE_SHLIB_SUFFIX)");
}

cmNMakeMakefileGenerator::~cmNMakeMakefileGenerator()
{
}

// convert to windows short paths if there are spaces
// in path
std::string cmNMakeMakefileGenerator::ShortPath(const char* path)
{
  std::string ret = path;
  cmSystemTools::ConvertToWindowsSlashes(ret);
  // if there are no spaces in path, then just return path
  if(ret.find(' ') == std::string::npos)
    {
    return ret;
    }
    
  // if there are spaces then call GetShortPathName to get rid of them
  char *buffer = new char[strlen(path)+1];
  if(GetShortPathName(path, buffer, 
                      strlen(path)+1) != 0)
    {
    ret = buffer;
    }
  else
    {
    // if GetShortPathName failed for some reason use
    // EscapeSpaces instead
    ret = cmSystemTools::EscapeSpaces(path);
    }
  delete [] buffer;
  return ret;
}


// convert a command to a short path if it has spaces
// this separates the arguments from the command and puts
// them back together
std::string cmNMakeMakefileGenerator::ShortPathCommand(const char* command)
{
  if(!strchr(command, ' '))
    {
    return command;
    }
  cmRegularExpression reg("^\"([^\"]*)\"(.*)");
  if(reg.find(command))
    {
    std::string c = reg.match(1);
    cmRegularExpression removeIntDir("(.*)/\\$\\(IntDir\\)(.*)");
    if(removeIntDir.find(c))
      {
	c = removeIntDir.match(1) + removeIntDir.match(2);
      }
    c = ShortPath(c.c_str());
    std::string ret = c;
    std::string args = reg.match(2);
    ret += args;
    return ret;
    }
  return command;
}


void cmNMakeMakefileGenerator::ComputeSystemInfo()
{
  // now load the settings
  if(!m_Makefile->GetDefinition("CMAKE_ROOT"))
    {
    cmSystemTools::Error(
      "CMAKE_ROOT has not been defined, bad GUI or driver program");
    return;
    }
  std::string fpath = 
    m_Makefile->GetDefinition("CMAKE_ROOT");
  fpath += "/Templates/CMakeNMakeWindowsSystemConfig.cmake";
  m_Makefile->ReadListFile(NULL,fpath.c_str());
}


  
void cmNMakeMakefileGenerator::OutputMakeVariables(std::ostream& fout)
{
  fout << "# NMake Makefile generated by cmake\n";
  const char* variables = 
    "# general variables used in the makefile\n"
    "\n"
    "# Path to cmake\n"
    "MAKESILENT                             = /nologo\n"
    "CMAKE_STANDARD_WINDOWS_LIBRARIES       = @CMAKE_STANDARD_WINDOWS_LIBRARIES@\n"
    "CMAKE_C_FLAGS                          = @CMAKE_C_FLAGS@ @BUILD_FLAGS@\n"
    "CMAKE_C_LINK_EXECUTABLE_FLAG           = @CMAKE_C_LINK_EXECUTABLE_FLAG@\n"
    "CMAKE_CXX_FLAGS                        = @CMAKE_CXX_FLAGS@ @BUILD_FLAGS@\n"
    "CMAKE_LINKER_FLAGS                     = @CMAKE_LINKER_FLAGS@ @LINKER_BUILD_FLAGS@\n"
    "CMAKE_LINKER_SHARED_LIBRARY_FLAG       = @CMAKE_LINKER_SHARED_LIBRARY_FLAG@\n"
    "CMAKE_LIBRARY_MANAGER_FLAGS            = @CMAKE_LIBRARY_MANAGER_FLAGS@\n"
    "CMAKE_OBJECT_FILE_SUFFIX               = @CMAKE_OBJECT_FILE_SUFFIX@\n"
    "CMAKE_EXECUTABLE_SUFFIX                = @CMAKE_EXECUTABLE_SUFFIX@\n"
    "CMAKE_STATICLIB_SUFFIX                 = @CMAKE_STATICLIB_SUFFIX@\n"
    "CMAKE_SHLIB_SUFFIX                     = @CMAKE_SHLIB_SUFFIX@\n"
    "RM = del\n";

  std::string buildType = "CMAKE_CXX_FLAGS_";
  buildType +=  m_Makefile->GetDefinition("CMAKE_BUILD_TYPE");
  buildType = cmSystemTools::UpperCase(buildType);
  m_Makefile->AddDefinition("BUILD_FLAGS",
                            m_Makefile->GetDefinition(
                              buildType.c_str()));

  buildType = "CMAKE_LINKER_FLAGS_";
  buildType +=  m_Makefile->GetDefinition("CMAKE_BUILD_TYPE");
  buildType = cmSystemTools::UpperCase(buildType);
  m_Makefile->AddDefinition("LINKER_BUILD_FLAGS",
                            m_Makefile->GetDefinition(
                              buildType.c_str()));

  std::string replaceVars = variables;
  m_Makefile->ExpandVariablesInString(replaceVars);
  fout << replaceVars.c_str();

  std::string ccompiler = m_Makefile->GetDefinition("CMAKE_C_COMPILER");
  cmSystemTools::ConvertToWindowsSlashes(ccompiler);
  fout << "CMAKE_C_COMPILER                       = " << cmSystemTools::EscapeSpaces(ccompiler.c_str()) << "\n";

  std::string cxxcompiler = m_Makefile->GetDefinition("CMAKE_CXX_COMPILER");
  cmSystemTools::ConvertToWindowsSlashes(cxxcompiler);
  fout << "CMAKE_CXX_COMPILER                     = " << cmSystemTools::EscapeSpaces(cxxcompiler.c_str()) << "\n";

  std::string linker = m_Makefile->GetDefinition("CMAKE_LINKER");
  cmSystemTools::ConvertToWindowsSlashes(linker);
  fout << "CMAKE_LINKER                           = " << cmSystemTools::EscapeSpaces(linker.c_str()) << "\n";

  std::string lib_manager = m_Makefile->GetDefinition("CMAKE_LIBRARY_MANAGER");
  cmSystemTools::ConvertToWindowsSlashes(lib_manager);
  fout << "CMAKE_LIBRARY_MANAGER                  = " << cmSystemTools::EscapeSpaces(lib_manager.c_str()) << "\n";

  std::string cmakecommand = m_Makefile->GetDefinition("CMAKE_COMMAND");
  cmSystemTools::ConvertToWindowsSlashes(cmakecommand);
  fout << "CMAKE_COMMAND                          = " << cmSystemTools::EscapeSpaces(cmakecommand.c_str()) << "\n";

  fout << "CMAKE_CURRENT_SOURCE                   = " 
       << ShortPath(m_Makefile->GetStartDirectory() )
       << "\n";
  fout << "CMAKE_CURRENT_BINARY                   = " 
       << ShortPath(m_Makefile->GetStartOutputDirectory())
       << "\n";
  fout << "CMAKE_SOURCE_DIR                       = " 
       << ShortPath(m_Makefile->GetHomeDirectory()) << "\n";
  fout << "CMAKE_BINARY_DIR                       = " 
       << ShortPath(m_Makefile->GetHomeOutputDirectory() )
       << "\n";

  // Output Include paths
  fout << "INCLUDE_FLAGS                          = ";
  std::vector<std::string>& includes = m_Makefile->GetIncludeDirectories();
  std::vector<std::string>::iterator i;
  fout << "-I" << cmSystemTools::EscapeSpaces(m_Makefile->GetStartDirectory()) << " ";
  for(i = includes.begin(); i != includes.end(); ++i)
    {
    std::string include = *i;
    // Don't output a -I for the standard include path "/usr/include".
    // This can cause problems with certain standard library
    // implementations because the wrong headers may be found first.
    if(include != "/usr/include")
      {
      fout << "-I" << cmSystemTools::EscapeSpaces(i->c_str()).c_str() << " ";
      }
    } 

  fout << m_Makefile->GetDefineFlags();
  fout << "\n\n";
}


void cmNMakeMakefileGenerator::BuildInSubDirectory(std::ostream& fout,
                                                  const char* directory,
                                                  const char* target1,
                                                  const char* target2)
{
  if(target1)
    {
    std::string dir = directory;
    cmSystemTools::ConvertToWindowsSlashes(dir);
    dir = cmSystemTools::EscapeSpaces(dir.c_str());
    fout << "\tif not exist " << dir
         << " " 
         << "$(MAKE) $(MAKESILENT) rebuild_cache\n"
         << "\techo Building " << target1 << " in directory " << directory << "\n"
         << "\tcd " << dir << "\n"
         << "\t$(MAKE) -$(MAKEFLAGS) $(MAKESILENT) " << target1 << "\n";
    }
  if(target2)
    {
    fout << "\techo Building " << target2 << " in directory " << directory << "\n";
    fout << "\t$(MAKE) -$(MAKEFLAGS) $(MAKESILENT) " << target2 << "\n";
    }
  std::string currentDir = m_Makefile->GetCurrentOutputDirectory();
  cmSystemTools::ConvertToWindowsSlashes(currentDir);
  fout << "\tcd " << cmSystemTools::EscapeSpaces(currentDir.c_str()) << "\n";
}




// This needs to be overriden because nmake requires commands to be quoted
// if the are full paths to the executable????

void cmNMakeMakefileGenerator::OutputMakeRule(std::ostream& fout, 
                                              const char* comment,
                                              const char* target,
                                              const char* depends, 
                                              const char* command,
                                              const char* command2,
                                              const char* command3,
                                              const char* command4)
{
  if(!target)
    {
    cmSystemTools::Error("no target for OutputMakeRule");
    return;
    }
  
  std::string replace;
  if(comment)
    {
    replace = comment;
    m_Makefile->ExpandVariablesInString(replace);
    fout << "#---------------------------------------------------------\n";
    fout << "# " << comment;
    fout << "\n#\n";
    }
  fout << "\n";
  replace = target;
  m_Makefile->ExpandVariablesInString(replace);
  replace = cmSystemTools::EscapeSpaces(replace.c_str());
  fout << replace.c_str() << ": ";
  if(depends)
    {
    replace = depends;
    m_Makefile->ExpandVariablesInString(replace);
    fout << replace.c_str();
    }
  fout << "\n";
  if(command)
    {
    replace = ShortPathCommand(command);
    m_Makefile->ExpandVariablesInString(replace);
    fout << "\t" << "echo " << replace.c_str() << "\n";
    fout << "\t" << replace.c_str() << "\n";
    }
  if(command2)
    {
    replace = ShortPathCommand(command2);
    m_Makefile->ExpandVariablesInString(replace);
    fout << "\t" << "echo " << replace.c_str() << "\n";
    fout << "\t" << replace.c_str() << "\n";
    }
  if(command3)
    {
    replace = ShortPathCommand(command3);
    m_Makefile->ExpandVariablesInString(replace);
    fout << "\t" << "echo " << replace.c_str() << "\n";
    fout << "\t" << replace.c_str() << "\n";
    }
  if(command4)
    {
    replace = ShortPathCommand(command4);
    m_Makefile->ExpandVariablesInString(replace);
    fout << "\t" << "echo " << replace.c_str() << "\n";
    fout << "\t" << replace.c_str() << "\n";
    }
  fout << "\n";
}

void 
cmNMakeMakefileGenerator::
OutputBuildObjectFromSource(std::ostream& fout,
                            const char* shortName,
                            const cmSourceFile& source,
                            const char* extraCompileFlags,
                            bool shared)
{ 
  std::string comment = "Build ";
  std::string objectFile = std::string(shortName) + 
    this->GetOutputExtension(source.GetSourceExtension().c_str());
  
  comment += objectFile + "  From ";
  comment += source.GetFullPath();
  std::string compileCommand;
  std::string ext = source.GetSourceExtension();
  if(ext == "c" )
    {
    compileCommand = "$(CMAKE_C_COMPILER) $(CMAKE_C_FLAGS) ";
    compileCommand += extraCompileFlags;
    if(shared)
      {
      compileCommand += "$(CMAKE_SHLIB_CFLAGS) ";
      }
    compileCommand += "$(INCLUDE_FLAGS) -c ";
    compileCommand += 
      cmSystemTools::EscapeSpaces(source.GetFullPath().c_str());

    // Need to get the definition here because this value might have
    // trailing space (since it is directly prepended to the filename)
    std::string output_object_file_flag = 
      m_Makefile->GetDefinition("CMAKE_C_OUTPUT_OBJECT_FILE_FLAG");
    m_Makefile->ExpandVariablesInString(output_object_file_flag);

    compileCommand += " " + output_object_file_flag;
    compileCommand += objectFile;
    }
  else if (ext == "rc")
    {
    compileCommand = "$(RC) /fo\"";
    compileCommand += objectFile;
    compileCommand += "\" ";
    compileCommand += 
      cmSystemTools::EscapeSpaces(source.GetFullPath().c_str());
    }
  else if (ext == "def")
    {
    // no rule to output for this one
    return;
    }
  // assume c++ if not c rc or def
  else
    {
    
    compileCommand = "$(CMAKE_CXX_COMPILER) $(CMAKE_CXX_FLAGS) ";
    compileCommand += extraCompileFlags;
    if(shared)
      {
      compileCommand += "$(CMAKE_SHLIB_CFLAGS) ";
      }
    compileCommand += "$(INCLUDE_FLAGS) -c ";
    compileCommand += 
      cmSystemTools::EscapeSpaces(source.GetFullPath().c_str());

    // Need to get the definition here because this value might have
    // trailing space (since it is directly prepended to the filename)
    std::string output_object_file_flag = 
      m_Makefile->GetDefinition("CMAKE_C_OUTPUT_OBJECT_FILE_FLAG");
    m_Makefile->ExpandVariablesInString(output_object_file_flag);

    compileCommand += " " + output_object_file_flag;
    compileCommand += objectFile;
    }

  this->OutputMakeRule(fout,
                       comment.c_str(),
                       objectFile.c_str(),
                       cmSystemTools::EscapeSpaces(
                         source.GetFullPath().c_str()).c_str(),
                       compileCommand.c_str());
}

void cmNMakeMakefileGenerator::OutputSharedLibraryRule(std::ostream& fout, 
                                                       const char* name,
                                                       const cmTarget &t)
{
  std::string target = m_LibraryOutputPath + name + m_SharedLibraryExtension;
  std::string depend = "$(";
  depend += name;
  depend += "_SRC_OBJS) $(" + std::string(name) + "_DEPEND_LIBS)";

  // Need to get the definition here because this value might have
  // trailing space (since it is directly prepended to the filename)
  std::string linker_output_file_flag = 
    m_Makefile->GetDefinition("CMAKE_LINKER_OUTPUT_FILE_FLAG");
  m_Makefile->ExpandVariablesInString(linker_output_file_flag);

  std::string command = "$(CMAKE_LINKER) $(CMAKE_LINKER_SHARED_LIBRARY_FLAG)";

  bool hide_param = m_Makefile->IsOn("CMAKE_LINKER_HIDE_PARAMETERS");
  if (hide_param)
    {
    command += " @<<\n\t";
    }

  command += " $(CMAKE_LINKER_FLAGS) " + linker_output_file_flag;

  std::string dllpath = m_LibraryOutputPath +  std::string(name) + m_SharedLibraryExtension;
  command += cmSystemTools::EscapeSpaces(dllpath.c_str());

  command += " $(" + std::string(name) + "_SRC_OBJS) ";

  std::strstream linklibs;
  this->OutputLinkLibraries(linklibs, name, t);
  linklibs << std::ends;
  command += linklibs.str();
  delete [] linklibs.str();

  const std::vector<cmSourceFile>& sources = t.GetSourceFiles();
  for(std::vector<cmSourceFile>::const_iterator i = sources.begin();
      i != sources.end(); ++i)
    {
    if(i->GetSourceExtension() == "def")
      {
      command += "/DEF:";
      command += i->GetFullPath();
      }
    }

  command += "\n";
  if (hide_param)
    {
    command += "<<\n";
    }

  std::string customCommands = this->CreateTargetRules(t, name);
  const char* cc = 0;
  if(customCommands.size() > 0)
    {
    cc = customCommands.c_str();
    }
  this->OutputMakeRule(fout, "rules for a shared library",
                       target.c_str(),
                       depend.c_str(),
                       command.c_str(), cc);
}

void cmNMakeMakefileGenerator::OutputModuleLibraryRule(std::ostream& fout, 
                                                       const char* name, 
                                                       const cmTarget &target)
{
  this->OutputSharedLibraryRule(fout, name, target);
}

void cmNMakeMakefileGenerator::OutputStaticLibraryRule(std::ostream& fout, 
                                                       const char* name,
                                                       const cmTarget &t)
{
  std::string target = m_LibraryOutputPath + std::string(name) + m_StaticLibraryExtension;
  std::string depend = "$(";
  depend += std::string(name) + "_SRC_OBJS)";

  // Need to get the definition here because this value might have
  // trailing space (since it is directly prepended to the filename)
  std::string library_manager_output_file_flag = 
    m_Makefile->GetDefinition("CMAKE_LIBRARY_MANAGER_OUTPUT_FILE_FLAG");
  m_Makefile->ExpandVariablesInString(library_manager_output_file_flag);

  std::string command = "$(CMAKE_LIBRARY_MANAGER) $(CMAKE_LIBRARY_MANAGER_FLAGS) @<<\n\t " + library_manager_output_file_flag;

  std::string libpath = m_LibraryOutputPath + std::string(name) + m_StaticLibraryExtension;
  command += cmSystemTools::EscapeSpaces(libpath.c_str());

  command += " $(";
  command += std::string(name) + "_SRC_OBJS)";
  command += "\n<<\n";

  std::string comment = "rule to build static library: ";
  comment += name;

  std::string customCommands = this->CreateTargetRules(t, name);
  const char* cc = 0;
  if(customCommands.size() > 0)
    {
    cc = customCommands.c_str();
    }
  this->OutputMakeRule(fout,
                       comment.c_str(),
                       target.c_str(),
                       depend.c_str(),
                       command.c_str(), cc);
}

void cmNMakeMakefileGenerator::OutputExecutableRule(std::ostream& fout,
                                                    const char* name,
                                                    const cmTarget &t)
{
  std::string target = m_ExecutableOutputPath + name;
  target += m_ExecutableExtension;
  std::string depend = "$(";
  depend += std::string(name) + "_SRC_OBJS) $(" + std::string(name) + "_DEPEND_LIBS)";
  std::string command = 
    "$(CMAKE_CXX_COMPILER) $(CMAKE_CXX_FLAGS) ";
  command += "$(" + std::string(name) + "_SRC_OBJS) ";
  std::string path = m_ExecutableOutputPath + name + m_ExecutableExtension;

  // Need to get the definition here because this value might have
  // trailing space (since it is directly prepended to the filename)
  std::string output_executable_file_flag = 
    m_Makefile->GetDefinition("CMAKE_C_OUTPUT_EXECUTABLE_FILE_FLAG");
  m_Makefile->ExpandVariablesInString(output_executable_file_flag);

  command += " " + output_executable_file_flag + 
    cmSystemTools::EscapeSpaces(path.c_str());

  command += " $(CMAKE_C_LINK_EXECUTABLE_FLAG) ";
  if(t.GetType() == cmTarget::WIN32_EXECUTABLE)
    {
    command +=  " /subsystem:windows ";
    }
  
  std::strstream linklibs;
  this->OutputLinkLibraries(linklibs, 0, t);
  linklibs << std::ends;
  command += linklibs.str();

  std::string comment = "rule to build executable: ";
  comment += name;

  std::string customCommands = this->CreateTargetRules(t, name);
  const char* cc = 0;
  if(customCommands.size() > 0)
    {
    cc = customCommands.c_str();
    }
  this->OutputMakeRule(fout, 
                       comment.c_str(),
                       target.c_str(),
                       depend.c_str(),
                       command.c_str(), cc);
}

  
void cmNMakeMakefileGenerator::OutputLinkLibraries(std::ostream& fout,
                                                   const char* targetLibrary,
                                                   const cmTarget &tgt)
{
  // Try to emit each search path once
  std::set<std::string> emitted;

  // Embed runtime search paths if possible and if required.
  // collect all the flags needed for linking libraries
  // Do not try if there is no library path option (it is set to -L or
  // -LIBPATH for some linker, but some others do not even support link
  // search path).
  std::string linkLibs;

  // Expand content because this value might have
  // trailing space (since it is directly prepended to the filename)
  std::string lib_path_opt = m_LibraryPathOption;
  m_Makefile->ExpandVariablesInString(lib_path_opt);
        
  if (lib_path_opt.size())
    {
    std::vector<std::string>& libdirs = m_Makefile->GetLinkDirectories();
    for(std::vector<std::string>::iterator libDir = libdirs.begin();
        libDir != libdirs.end(); ++libDir)
      { 
      std::string libpath = ShortPath(libDir->c_str());
      if(emitted.insert(libpath).second)
        {
        linkLibs += lib_path_opt;
        cmSystemTools::ConvertToWindowsSlashes(libpath);
        linkLibs += libpath;
        linkLibs += " ";
        }
      }
    }

  std::string librariesLinked;
  const cmTarget::LinkLibraries& libs = tgt.GetLinkLibraries();
  for(cmTarget::LinkLibraries::const_iterator lib = libs.begin();
      lib != libs.end(); ++lib)
    {
    // Don't link the library against itself!
    if(targetLibrary && (lib->first == targetLibrary)) continue;

// ** should fix this later, it should check to see if this is 
// a debug build and add the library
// don't look at debug libraries
//    if (lib->second == cmTarget::DEBUG) continue;
    // skip zero size library entries, this may happen
    // if a variable expands to nothing.
    if (lib->first.size() == 0) continue;
    if(emitted.insert(lib->first).second)
      {
      std::string regexp = ".*\\";
      regexp += m_Makefile->GetDefinition("CMAKE_STATICLIB_SUFFIX");
      regexp += "$";
      cmRegularExpression reg(regexp.c_str());
      // if it ends in .lib, then it is a full path and should
      // be escaped, and does not need .lib added
      if(reg.find(lib->first))
	{
	librariesLinked +=  ShortPath(lib->first.c_str());
	librariesLinked += " ";
	}
      else
	{
        librariesLinked += m_LibraryLinkOption;
	librariesLinked += lib->first;
	librariesLinked += m_StaticLibraryExtension + " ";
	}
      }
    }
  linkLibs += librariesLinked;
  fout << linkLibs;
  fout << "$(CMAKE_STANDARD_WINDOWS_LIBRARIES) ";
}


std::string cmNMakeMakefileGenerator::GetOutputExtension(const char* s)
{
  std::string sourceExtension = s;
  if(sourceExtension == "def")
    {
    return "";
    }
  if(sourceExtension == "ico" || sourceExtension == "rc2")
    {
    return "";
    }
  if(sourceExtension == "rc")
    {
    return ".res";
    }
  return m_ObjectFileExtension;
}


void cmNMakeMakefileGenerator::OutputIncludeMakefile(std::ostream& fout,
                                                     const char* file)
{
  fout << "!include " << file << "\n";
}

bool cmNMakeMakefileGenerator::SamePath(const char* path1, const char* path2)
{
  // first check to see if they are the same anyway
  if (strcmp(path1, path2) == 0)
    {
    return true;
    }
  // next short path and lower case both of them for the compare
  return 
    cmSystemTools::LowerCase(ShortPath(path1)) ==
    cmSystemTools::LowerCase(ShortPath(path2));
}

void cmNMakeMakefileGenerator::OutputBuildLibraryInDir(std::ostream& fout,
						       const char* path,
						       const char* ,
						       const char* fullpath)
{

  std::string currentDir = m_Makefile->GetCurrentOutputDirectory();
  cmSystemTools::ConvertToWindowsSlashes(currentDir);
  fout << cmSystemTools::EscapeSpaces(fullpath)
       << ":\n\tcd " << cmSystemTools::EscapeSpaces(path)
       << "\n\t$(MAKE) $(MAKESILENT) " << cmSystemTools::EscapeSpaces(fullpath)
       << "\n\tcd " <<
    cmSystemTools::EscapeSpaces(currentDir.c_str()) << "\n";
}


std::string cmNMakeMakefileGenerator::ConvertToNativePath(const char* s)
{
  std::string ret = s;
  cmSystemTools::ConvertToWindowsSlashes(ret);
  return ret;
}
