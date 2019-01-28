#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>
#include <pthread.h>

#include "Application.h"
#include "MultiServer.h"
#include "MultiServerHandler.h"

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

#include "ping.hpp"

WORK_CLASS_TYPE(PingMsg)

using namespace gxy;

extern "C" void
init()
{
  PingMsg::Register();
}

void
Ping()
{
  pthread_mutex_lock(&lck);
  PingMsg p("ping");
  p.Send(1);
  pthread_cond_wait(&w8, &lck);
  pthread_mutex_unlock(&lck);
  cerr << "ping done\n";
}

extern "C" bool
server(MultiServerHandler *handler)
{
  bool client_done = false;
  while (! client_done)
  {
    char *buf; int sz;
    if (! handler->CRecv(buf, sz))
    {
      cerr << "receive failed\n";
      break;
    }

    cerr << "received " << buf << "\n";

    if (!strcmp(buf, "ping"))
    {
      Ping();
    }
    else if (!strcmp(buf, "quit"))
    {
      client_done = true;
    }
    else
      cerr << "huh?\n";

    std::string ok("ping ok");
    handler->CSend(ok.c_str(), ok.length()+1);

    free(buf);
  }

  return true;
}