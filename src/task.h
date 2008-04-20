////////////////////////////////////////////////////////////////////////////////
// Copyright 2006 - 2008, Paul Beckingham.  All rights reserved.
//
//
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include "Config.h"
#include "Table.h"
#include "color.h"
#include "TDB.h"
#include "T.h"
#include "stlmacros.h"
#include "../auto.h"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

// parse.cpp
void parse (std::vector <std::string>&, std::string&, T&);
bool validDate (std::string&);

// task.cpp
void handleAdd (const TDB&, T&, Config&);
void handleProjects (const TDB&, T&, Config&);
void handleTags (const TDB&, T&, Config&);
void handleList (const TDB&, T&, Config&);
void handleInfo (const TDB&, T&, Config&);
void handleLongList (const TDB&, T&, Config&);
void handleSmallList (const TDB&, T&, Config&);
void handleCompleted (const TDB&, T&, Config&);
void handleReportSummary (const TDB&, T&, Config&);
void handleReportNext (const TDB&, T&, Config&);
void handleReportHistory (const TDB&, T&, Config&);
void handleReportUsage (const TDB&, T&, Config&);
void handleReportCalendar (const TDB&, T&, Config&);
void handleReportActive (const TDB&, T&, Config&);
void handleReportOverdue (const TDB&, T&, Config&);
void handleReportStats (const TDB&, T&, Config&);
void handleVersion (Config&);
void handleExport (const TDB&, T&, Config&);
void handleDelete (const TDB&, T&, Config&);
void handleStart (const TDB&, T&, Config&);
void handleDone (const TDB&, T&, Config&);
void handleModify (const TDB&, T&, Config&);
void gatherNextTasks (const TDB&, T&, Config&, std::vector <T>&, std::vector <int>&);
void nag (const TDB&, T&, Config&);

// util.cpp
bool confirm (const std::string&);
void wrapText (std::vector <std::string>&, const std::string&, const int);
std::string trimLeft (const std::string& in, const std::string& t = " ");
std::string trimRight (const std::string& in, const std::string& t = " ");
std::string trim (const std::string& in, const std::string& t = " ");
std::wstring trimLeft (const std::wstring& in, const std::wstring& t = L" ");  // UNICODE safe
std::wstring trimRight (const std::wstring& in, const std::wstring& t = L" "); // UNICODE safe
std::wstring trim (const std::wstring& in, const std::wstring& t = L" ");      // UNICODE safe
void extractParagraphs (const std::string&, std::vector<std::string>&);
void extractLine (std::string&, std::string&, int);
void split (std::vector<std::string>&, const std::string&, const char);
void split (std::vector<std::string>&, const std::string&, const std::string&);
void join (std::string&, const std::string&, const std::vector<std::string>&);
std::string commify (const std::string&);
std::string lowerCase (const std::string&);
void delay (float);
int autoComplete (const std::string&, const std::vector<std::string>&, std::vector<std::string>&);
void formatTimeDeltaDays (std::string&, time_t);
std::string formatSeconds (time_t);
const std::string uuid ();

// rules.cpp
void initializeColorRules (Config&);
void autoColorize (T&, Text::color&, Text::color&);

////////////////////////////////////////////////////////////////////////////////
