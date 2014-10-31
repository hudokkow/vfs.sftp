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

#include <kodi/threads/mutex.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <boost/shared_ptr.hpp>
#include <kodi/xbmc_addon_dll.h>
#include <kodi/kodi_vfs_types.h>
#include <map>
#include <string>
#include <vector>

class CSFTPSession
{
public:
  CSFTPSession(VFSURL* url);
  virtual ~CSFTPSession();

  sftp_file CreateFileHande(const std::string& file);
  void CloseFileHandle(sftp_file handle);
  bool GetDirectory(const std::string& base, const std::string& folder, std::vector<VFSDirEntry>& items);
  bool DirectoryExists(const char *path);
  bool FileExists(const char *path);
  int Stat(const char *path, struct __stat64* buffer);
  int Seek(sftp_file handle, uint64_t position);
  int Read(sftp_file handle, void *buffer, size_t length);
  int64_t GetPosition(sftp_file handle);
  bool IsIdle();
private:
  bool VerifyKnownHost(ssh_session session);
  bool Connect(VFSURL* url);
  void Disconnect();
  bool GetItemPermissions(const char *path, uint32_t &permissions);
  PLATFORM::CMutex m_lock;

  bool m_connected;
  ssh_session  m_session;
  sftp_session m_sftp_session;
  int m_LastActive;
};

typedef boost::shared_ptr<CSFTPSession> CSFTPSessionPtr;

class CSFTPSessionManager
{
public:
  static CSFTPSessionManager& Get();
  CSFTPSessionPtr CreateSession(VFSURL* url);
  void ClearOutIdleSessions();
  void DisconnectAllSessions();
private:
  CSFTPSessionManager() {}
  CSFTPSessionManager& operator=(const CSFTPSessionManager&);
  PLATFORM::CMutex m_lock;
  std::map<std::string, CSFTPSessionPtr> sessions;
};
