// admin.hh: the admin port, through which patella-admin (admin/) gives the
// game commands while it runs (config.toml's [admin]). admin.cc has the port:
// a thread that listens on 127.0.0.1, takes one request at a time and hands
// its line to the main loop; scripting_admin.cc has the commands (admin.toml,
// their scripts and the Admin API; scripting.hh, section "The admin port").
//
// The protocol, one request per connection: the client sends the password
// and the command line, each ended by a newline ('\n'; a '\r' before it is
// dropped), at most MAX_ADMIN_LINE bytes each, within ADMIN_TIMEOUT seconds;
// the game answers with "OK" or "ERROR" on a line of its own, then the reply's
// lines, and closes the connection. The command line and the reply are UTF-8
// (the game works in Latin-1 and converts them). A wrong password is answered
// with ERROR after a second. Each request is logged in the log "admin".

#ifndef PATELLA_ADMIN_HH
#define PATELLA_ADMIN_HH

#include <string>

// The longest line a client may send (the password, the command line).
const int MAX_ADMIN_LINE = 4096;

// The seconds a client has to send its two lines.
const int ADMIN_TIMEOUT = 10;

// Opens the admin port and starts its thread when [admin] enabled is true;
// does nothing otherwise. Throws a const char * when it cannot open the port
// or start the thread. Called by InitAll after the commands are read.
void InitAdmin();

// Answers a request that is still waiting ("The game server is shutting
// down."), stops the thread and closes the port. Safe without InitAdmin.
void ExitAdmin();

// Main thread: the line of the next request, false when none waits. Each
// request taken must be answered with AnswerAdminRequest.
bool TakeAdminRequest(std::string &Line);

// Main thread: answers the request taken last: Ok (OK or ERROR) and the
// reply's text (lines ended by '\n'), both Latin-1. Does nothing when no
// request is taken (ExitAdmin answered it).
void AnswerAdminRequest(bool Ok, const std::string &Reply);

// The commands (scripting_admin.cc). InitAdminCommands reads the commands of
// FileName (admin.toml) and loads their scripts; throws a const char * as the
// other loaders do. ExitAdminCommands forgets them and ends a run. Both on
// the main thread, after InitScripting and before ExitScripting.
void InitAdminCommands(const char *FileName);
void ExitAdminCommands();

// Main thread, called by AdvanceGame at every beat: does nothing when [admin]
// is not enabled; else goes on with a run over the characters (some each
// beat), or carries out the next request waiting.
void ProcessAdminRequests();

// Registers the API table Admin (scripting_admin.cc); InitAll calls it after
// RegisterEventAPI.
void RegisterAdminAPI();

#endif
