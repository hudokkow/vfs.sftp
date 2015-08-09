/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "libXBMC_addon.h"
#include "platform/threads/mutex.h"
#include "SFTPSession.h"

#include <map>
#include <sstream>

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

extern "C" {

#include "kodi_vfs_dll.h"
#include "IFileTypes.h"

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct SFTPContext
{
  CSFTPSessionPtr session;
  sftp_file sftp_handle;
  std::string file;
};

void* Open(VFSURL* url)
{
  SFTPContext* result = new SFTPContext;

  result->session = CSFTPSessionManager::Get().CreateSession(url);

  if (result->session)
  {
    result->file = url->filename;
    result->sftp_handle = result->session->CreateFileHande(result->file);
    if (result->sftp_handle)
      return result;
  }
  else
    XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Failed to allocate session");

  delete result;
  return NULL;
}

ssize_t Read(void* context, void* lpBuf, size_t uiBufSize)
{
  SFTPContext* ctx = (SFTPContext*)context;
  if (ctx && ctx->session && ctx->sftp_handle)
  {
    int rc = ctx->session->Read(ctx->sftp_handle, lpBuf, (size_t)uiBufSize);

    if (rc >= 0)
      return rc;
    else
      XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Failed to read %i", rc);
  }
  else
    XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Can't read without a filehandle");

  return -1;
}

bool Close(void* context)
{
  SFTPContext* ctx = (SFTPContext*)context;
  if (ctx->session && ctx->sftp_handle)
    ctx->session->CloseFileHandle(ctx->sftp_handle);
  delete ctx;

  return true;
}

int64_t GetLength(void* context)
{
  SFTPContext* ctx = (SFTPContext*)context;
  struct __stat64 buffer;
  if (ctx->session->Stat(ctx->file.c_str(), &buffer) != 0)
    return 0;
  else
    return buffer.st_size;
}

int64_t GetPosition(void* context)
{
  SFTPContext* ctx = (SFTPContext*)context;
  if (ctx->session && ctx->sftp_handle)
    return ctx->session->GetPosition(ctx->sftp_handle);

  XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Can't get position without a filehandle for '%s'", ctx->file.c_str());
  return 0;
}


int64_t Seek(void* context, int64_t iFilePosition, int iWhence)
{
  SFTPContext* ctx = (SFTPContext*)context;
  if (ctx && ctx->session && ctx->sftp_handle)
  {
    uint64_t position = 0;
    if (iWhence == SEEK_SET)
      position = iFilePosition;
    else if (iWhence == SEEK_CUR)
      position = GetPosition(context) + iFilePosition;
    else if (iWhence == SEEK_END)
      position = GetLength(context) + iFilePosition;

    if (ctx->session->Seek(ctx->sftp_handle, position) == 0)
      return GetPosition(context);
    else
      return -1;
  }
  else
  {
    XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Can't seek without a filehandle");
    return -1;
  }
}

bool Exists(VFSURL* url)
{
  CSFTPSessionPtr session = CSFTPSessionManager::Get().CreateSession(url);
  if (session)
    return session->FileExists(url->filename);
  else
  {
    XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Failed to create session to check exists for '%s'", url->filename);
    return false;
  }
}

int Stat(VFSURL* url, struct __stat64* buffer)
{
  CSFTPSessionPtr session = CSFTPSessionManager::Get().CreateSession(url);
  if (session)
    return session->Stat(url->filename, buffer);
  else
  {
    XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Failed to create session to stat for '%s'", url->filename);
    return -1;
  }
}

int IoControl(void* context, XFILE::EIoControl request, void* param)
{
  if(request == XFILE::IOCTRL_SEEK_POSSIBLE)
    return 1;

  return -1;
}

void ClearOutIdle()
{
  CSFTPSessionManager::Get().ClearOutIdleSessions();
}

void DisconnectAll()
{
  CSFTPSessionManager::Get().DisconnectAllSessions();
}

bool DirectoryExists(VFSURL* url)
{
  CSFTPSessionPtr session = CSFTPSessionManager::Get().CreateSession(url);
  if (session)
    return session->DirectoryExists(url->filename);
  else
  {
    XBMC->Log(ADDON::LOG_ERROR, "SFTPFile: Failed to create session to check exists");
    return false;
  }
}

void* GetDirectory(VFSURL* url, VFSDirEntry** items,
                   int* num_items, VFSCallbacks* callbacks)
{
  std::vector<VFSDirEntry>* result = new std::vector<VFSDirEntry>;
  CSFTPSessionPtr session = CSFTPSessionManager::Get().CreateSession(url);
  std::stringstream str;
  str << "sftp://" << url->username << ":" << url->password << "@" << url->hostname << ":" << url->port << "/";
  if (!session->GetDirectory(str.str(), url->filename, *result))
  {
    delete result;
    return NULL;
  }

  if (result->size())
    *items = &(*result)[0];
  *num_items = result->size();

  return result;
}

void FreeDirectory(void* items)
{
  std::vector<VFSDirEntry>& ctx = *(std::vector<VFSDirEntry>*)items;
  for (size_t i=0;i<ctx.size();++i)
  {
    free(ctx[i].label);
    for (size_t j=0;j<ctx[i].num_props;++j)
    {
      free(ctx[i].properties[j].name);
      free(ctx[i].properties[j].val);
    }
    delete ctx[i].properties;
    free(ctx[i].path);
  }
  delete &ctx;
}

void* OpenForWrite(VFSURL* url, bool bOverWrite)
{
  return NULL;
}

bool Rename(VFSURL* url, VFSURL* url2)
{
  return false;
}

bool Delete(VFSURL* url)
{
  return false;
}

ssize_t Write(void* context, const void* lpBuf, size_t uiBufSize)
{
  return -1;
}

int Truncate(void* context, int64_t size)
{
  return -1;
}

bool RemoveDirectory(VFSURL* url)
{
  return false;
}

bool CreateDirectory(VFSURL* url)
{
  return false;
}

void* ContainsFiles(VFSURL* url, VFSDirEntry** items, int* num_items, char* rootpath)
{
  return NULL;
}

int GetStartTime(void* ctx)
{
  return 0;
}

int GetTotalTime(void* ctx)
{
  return 0;
}

bool NextChannel(void* context, bool preview)
{
  return false;
}

bool PrevChannel(void* context, bool preview)
{
  return false;
}

bool SelectChannel(void* context, unsigned int uiChannel)
{
  return false;
}

bool UpdateItem(void* context)
{
  return false;
}

int GetChunkSize(void* context)
{
  return 1;
}

}
