// CODYlib		-*- mode:c++ -*-
// Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
// License: LGPL v3.0 or later

// Cody
#include "internal.hh"

#if __windows__
inline bool IsDirSep (char c)
{
  return c == '/' || c == '\\';
}
inline bool IsAbsPath (char const *str)
{
  // IIRC windows has the concept of per-drive current directories,
  // which make drive-using paths confusing.  Let's not get into that.
  return IsDirSep (str)
    || (((str[0] >= 'A' && str[1] <= 'Z')
	 || (str[0] >= 'a' && str[1] <= 'z'))&& str[1] == ':');
}
#else
inline bool IsDirSep (char c)
{
  return c == '/';
}
inline bool IsAbsPath (char const *str)
{
  return IsDirSep (str[0]);
}
#endif

#define DOT_REPLACE ','
#define COLON_REPLACE '_'

namespace Cody {

Resolver::~Resolver ()
{
}

bool Resolver::ConnectRequest (Server *s, unsigned version,
			       std::string &, std::string &)
{
  if (version > Version)
    s->ErrorResponse ("version mismatch");
  else
    s->ConnectResponse ("default");
  return false;
}

bool Resolver::ModuleRepoRequest (Server *s)
{
  s->ModuleRepoResponse ("gcm.cache");
  return false;
}

static std::string DefaultCMIMapping (std::string &module)
{
  std::string result;

  result.reserve (module.size () + 8);
  bool is_header = false;
  bool is_abs = false;

  if (IsAbsPath (module.c_str ()))
    is_header = is_abs = true;
  else if (module.front () == '.' && IsDirSep (module.c_str ()[1]))
    is_header = true;

  if (is_abs)
    {
      result.push_back ('.');
      result.append (module);
    }
  else
    result = std::move (module);

  if (is_header)
    {
      if (!is_abs)
	result[0] = DOT_REPLACE;
      
      /* Map .. to DOT_REPLACE, DOT_REPLACE.  */
      for (size_t ix = 1; ; ix++)
	{
	  ix = result.find ('.', ix);
	  if (ix == result.npos)
	    break;
	  if (ix + 2 > result.size ())
	    break;
	  if (result[ix + 1] != '.')
	    continue;
	  if (!IsDirSep (result[ix - 1]))
	    continue;
	  if (!IsDirSep (result[ix + 2]))
	    continue;
	  result[ix] = DOT_REPLACE;
	  result[ix + 1] = DOT_REPLACE;
	}
    }
  else if (COLON_REPLACE != ':')
    {
      // There can only be one colon in a module name
      auto colon = result.find (':');
      if (colon != result.npos)
	result[colon] = COLON_REPLACE;
    }

  result.append (".gcm");

  return result;
}

bool Resolver::ModuleExportRequest (Server *s, std::string &module)
{
  auto cmi = DefaultCMIMapping (module);
  s->ModuleCMIResponse (cmi);
  return false;
}

bool Resolver::ModuleImportRequest (Server *s, std::string &module)
{
  auto cmi = DefaultCMIMapping (module);
  s->ModuleCMIResponse (cmi);
  return false;
}

bool Resolver::ModuleCompiledRequest (Server *s, std::string &)
{
  s->OKResponse ();
  return false;
}

bool Resolver::IncludeTranslateRequest (Server *s, std::string &)
{
  s->IncludeTranslateResponse (0);
  return false;
}

void Resolver::ErrorResponse (Server *server, std::string &&msg)
{
  server->ErrorResponse (msg);
}

}