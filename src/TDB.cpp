////////////////////////////////////////////////////////////////////////////////
// Copyright 2007, 2008, Paul Beckingham.  All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <sys/file.h>
#include <unistd.h>

#include "task.h"
#include "TDB.h"

////////////////////////////////////////////////////////////////////////////////
TDB::TDB ()
: mPendingFile ("")
, mCompletedFile ("")
, mLogFile ("")
{
}

////////////////////////////////////////////////////////////////////////////////
TDB::~TDB ()
{
}

////////////////////////////////////////////////////////////////////////////////
void TDB::dataDirectory (const std::string& directory)
{
  if (! access (directory.c_str (), F_OK))
  {
    mPendingFile   = directory + "/pending.data";
    mCompletedFile = directory + "/completed.data";
    mLogFile       = directory + "/command.log";
  }
  else
  {
    std::string error = "Directory '";
    error += directory;
    error += "' does not exist, or is not readable and writable.";
    throw error;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Combine allPendingT with allCompletedT.
// Note: this method is O(N1) + O(N2), where N2 is not bounded.
bool TDB::allT (std::vector <T>& all) const
{
  all.clear ();

  // Retrieve all the pending records.
  std::vector <T> allp;
  if (allPendingT (allp))
  {
    std::vector <T>::iterator i;
    for (i = allp.begin (); i != allp.end (); ++i)
      all.push_back (*i);

    // Retrieve all the completed records.
    std::vector <T> allc;
    if (allCompletedT (allc))
    {
      for (i = allc.begin (); i != allc.end (); ++i)
        all.push_back (*i);

      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Only accesses to the pending file result in Tasks that have assigned ids.
bool TDB::pendingT (std::vector <T>& all) const
{
  all.clear ();

  std::vector <std::string> lines;
  if (readLockedFile (mPendingFile, lines))
  {
    int id = 1;

    std::vector <std::string>::iterator it;
    for (it = lines.begin (); it != lines.end (); ++it)
    {
      T t (*it);
      t.setId (id++);
      if (t.getStatus () == T::pending)
        all.push_back (t);
    }

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Only accesses to the pending file result in Tasks that have assigned ids.
bool TDB::allPendingT (std::vector <T>& all) const
{
  all.clear ();

  std::vector <std::string> lines;
  if (readLockedFile (mPendingFile, lines))
  {
    int id = 1;

    std::vector <std::string>::iterator it;
    for (it = lines.begin (); it != lines.end (); ++it)
    {
      T t (*it);
      t.setId (id++);
      all.push_back (t);
    }

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::completedT (std::vector <T>& all) const
{
  all.clear ();

  std::vector <std::string> lines;
  if (readLockedFile (mCompletedFile, lines))
  {
    std::vector <std::string>::iterator it;
    for (it = lines.begin (); it != lines.end (); ++it)
    {
      T t (*it);
      if (t.getStatus () != T::deleted)
        all.push_back (t);
    }

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::allCompletedT (std::vector <T>& all) const
{
  all.clear ();

  std::vector <std::string> lines;
  if (readLockedFile (mCompletedFile, lines))
  {
    std::vector <std::string>::iterator it;
    for (it = lines.begin (); it != lines.end (); ++it)
    {
      T t (*it);
      all.push_back (t);
    }

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::deleteT (const T& t) const
{
  T task (t);

  std::vector <T> all;
  allPendingT (all);

  std::vector <T>::iterator it;
  for (it = all.begin (); it != all.end (); ++it)
    if (task.getId () == it->getId ())
    {
      it->setStatus (T::deleted);

      char endTime[16];
      sprintf (endTime, "%u", (unsigned int) time (NULL));
      it->setAttribute ("end", endTime);

      return overwritePending (all);
    }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::completeT (const T& t) const
{
  T task (t);

  std::vector <T> all;
  allPendingT (all);

  std::vector <T>::iterator it;
  for (it = all.begin (); it != all.end (); ++it)
    if (task.getId () == it->getId ())
    {
      it->setStatus (T::completed);

      char endTime[16];
      sprintf (endTime, "%u", (unsigned int) time (NULL));
      it->setAttribute ("end", endTime);

      return overwritePending (all);
    }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::addT (const T& t) const
{
  T task (t);

  std::vector <std::string> tags;
  task.getTags (tags);

  // TODO This logic smells funny.
  // +tag or -tag are both considered valid tags to add to a new pending task.
  // Generating an error here would not be friendly.
  for (unsigned int i = 0; i < tags.size (); ++i)
  {
    if (tags[i][0] == '-' || tags[i][0] == '+')
    {
      task.removeTag (tags[i]);
      task.addTag (tags[i].substr (1, std::string::npos));
    }
  }

  if (task.getStatus () == T::pending)
    return writePending (task);

  return writeCompleted (task);
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::modifyT (const T& t) const
{
  T modified (t);

  std::vector <T> all;
  allPendingT (all);

  std::vector <T> pending;

  std::vector <T>::iterator it;
  for (it = all.begin (); it != all.end (); ++it)
  {
    if (it->getId () == t.getId ())
    {
      modified.setUUID (it->getUUID ());
      pending.push_back (modified);
    }
    else
      pending.push_back (*it);
  }

  return overwritePending (pending);
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::logRead (std::vector <std::string>& entries) const
{
  entries.clear ();
  return readLockedFile (mLogFile, entries);
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::logCommand (int argc, char** argv) const
{
  // Get time info.
  time_t now;
  time (&now);
  struct tm* t = localtime (&now);

  // Generate timestamp.
  char timestamp[20];
  sprintf (timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
                      t->tm_year + 1900,
                      t->tm_mon + 1,
                      t->tm_mday,
                      t->tm_hour,
                      t->tm_min,
                      t->tm_sec);

  std::string command = timestamp;
  command += " \"";
  for (int i = 0; i < argc; ++i)
    command += std::string (i ? " " : "") + argv[i];
  command += "\"\n";

  if (! access (mLogFile.c_str (), F_OK | W_OK))
  {
    FILE* out;
    if ((out = fopen (mLogFile.c_str (), "a")))
    {
#ifdef HAVE_FLOCK
      int retry = 0;
      while (flock (fileno (out), LOCK_EX) && ++retry <= 3)
        delay (0.25);
#endif

      fprintf (out, command.c_str ());

      fclose (out);
      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::lock (FILE* file) const
{
#ifdef HAVE_FLOCK
  return flock (fileno (file), LOCK_EX) ? false : true;
#else
  return true;
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::overwritePending (std::vector <T>& all) const
{
  // Write a single task to the pending file
  FILE* out;
  if ((out = fopen (mPendingFile.c_str (), "w")))
  {
#ifdef HAVE_FLOCK
    int retry = 0;
    while (flock (fileno (out), LOCK_EX) && ++retry <= 3)
      delay (0.25);
#endif

    std::vector <T>::iterator it;
    for (it = all.begin (); it != all.end (); ++it)
      fprintf (out, it->compose ().c_str ());

    fclose (out);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::writePending (const T& t) const
{
  // Write a single task to the pending file
  FILE* out;
  if ((out = fopen (mPendingFile.c_str (), "a")))
  {
#ifdef HAVE_FLOCK
    int retry = 0;
    while (flock (fileno (out), LOCK_EX) && ++retry <= 3)
      delay (0.25);
#endif

    fprintf (out, t.compose ().c_str ());

    fclose (out);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::writeCompleted (const T& t) const
{
  // Write a single task to the pending file
  FILE* out;
  if ((out = fopen (mCompletedFile.c_str (), "a")))
  {
#ifdef HAVE_FLOCK
    int retry = 0;
    while (flock (fileno (out), LOCK_EX) && ++retry <= 3)
      delay (0.25);
#endif

    fprintf (out, t.compose ().c_str ());

    fclose (out);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TDB::readLockedFile (
  const std::string& file,
  std::vector <std::string>& contents) const
{
  contents.clear ();

  if (! access (file.c_str (), F_OK | R_OK))
  {
    FILE* in;
    if ((in = fopen (file.c_str (), "r")))
    {
#ifdef HAVE_FLOCK
      int retry = 0;
      while (flock (fileno (in), LOCK_EX) && ++retry <= 3)
        delay (0.25);
#endif

      char line[T_LINE_MAX];
      while (fgets (line, T_LINE_MAX, in))
      {
        int length = ::strlen (line);
        if (length > 1)
        {
          line[length - 1] = '\0'; // Kill \n
          contents.push_back (line);
        }
      }

      fclose (in);
      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
int TDB::gc () const
{
  int count = 0;

  // Read everything from the pending file.
  std::vector <T> all;
  allPendingT (all);

  // A list of the truly pending tasks.
  std::vector <T> pending;

  std::vector<T>::iterator it;
  for (it = all.begin (); it != all.end (); ++it)
  {
    // Some tasks stay in the pending file.
    if (it->getStatus () == T::pending)
      pending.push_back (*it);

    // Others are transferred to the completed file.
    else
    {
      writeCompleted (*it);
      ++count;
    }
  }

  // Dump all clean tasks into pending.
  overwritePending (pending);
  return count;
}

////////////////////////////////////////////////////////////////////////////////

